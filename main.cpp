#include <utility>
#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <cassert>
#include <omp.h>
#include <queue>
#include <deque>
#include <mpi.h>


#include "src/map_info.h"
#include "src/array_map.h"


using namespace std;

#define TAG_WORK 1
#define TAG_END 2
#define TAG_DONE_NO_UPDATE 3
#define TAG_DONE_UPDATE 4

// ------------------------------------------------------------------------------------------------------------------
class SolverResult {
public:
    set<pair<int, int>> empty;
    int price;
    ArrayMap map;

    SolverResult(ArrayMap map)
            : price(INT32_MIN), map(std::move(map)) {
    }

    friend ostream &operator<<(ostream &out, const SolverResult &result);
};

ostream &operator<<(ostream &os, const SolverResult &result) {
    os << result.map;
    os << result.price << endl;
    os << result.empty.size() << endl;
    for (const auto &p : result.empty)
        os << p.first << " " << p.second << endl;

    return os;
}

ostream &operator<<(ostream &os, const ArrayMap &map) {
    for (int y = 0; y < map.rows; y++) {
        for (int x = 0; x < map.columns; x++) {
            int p = map.getValue(x, y);
            switch (p) {
                case BLOCK_FREE :
                    os << ".\t";
                    break;
                case BLOCK_BAN :
                    os << "X\t";
                    break;
                default:
                    os << p << "\t";
                    break;
            }
        }
        os << endl;
    }

    return os;
}
// ------------------------------------------------------------------------------------------------------------------


class QueueItem {
public:
    ArrayMap map;
    int price;
    int uncovered;

    QueueItem() {

    }

    QueueItem(const ArrayMap &map, int price, int uncovered) : map(map), price(price), uncovered(uncovered) {

    }

    QueueItem &operator=(const QueueItem &copy) {
        if (this == &copy)
            return *this;

        map = copy.map;
        price = copy.price;
        uncovered = copy.uncovered;
        return *this;
    }

    pair<int, int *> serialize(const int &bestPrice) {
        pair<int, int *> sr_map = map.serialize();
        int map_size = sr_map.first;
        // map size + price + uncovered + bestPrice
        int size = map_size + 3;
        int *buffer = new int[size];

        for (int i = 0; i < map_size; i++) {
            buffer[i] = sr_map.second[i];
        }

        delete[] sr_map.second;

        buffer[map_size] = price;
        buffer[map_size + 1] = uncovered;
        buffer[map_size + 2] = bestPrice;

        return make_pair(size, buffer);
    }
};

// ------------------------------------------------------------------------------------------------------------------
class Solver {
public:
    SolverResult *best;

    Solver(MapInfo *mapInfo)
            : best(nullptr), info(mapInfo) {
    }

    void solve() {

        int proc_num, num_procs;
        MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
        MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

        // MASTER
        if (proc_num == 0) {
            master(num_procs);
            // as master, no more work, all slaves done. Finish result.
            FindLeftEmptyTiles();

        } else { //SLAVE
            slave(proc_num);
        }
    }

    ~Solver() {
        delete best;
    }

private:
    MapInfo *info;
    deque<QueueItem> dataQueue;

    void master(const int & num_procs) {
        // prepare map
        ArrayMap map(info->rows, info->columns, info->banned);
        map.setStart();
        best = new SolverResult(map);
        cout << "MASTER - prepare data bfs" << endl;
        // prepare data with BFS
        const unsigned int max = 8;
        prepare_tasks(map, max);

        int workers = num_procs - 1;
        int initialWorkers = workers;
        // initial send of work
        cout << "MASTER - initial-send-to-work, workers" << workers << ", works to do: " << dataQueue.size() << endl;
        for (int workerId = 1; workerId <= workers && !dataQueue.empty(); workerId++) {
            QueueItem task = dataQueue.front();
            dataQueue.pop_front();

            initialWorkers--;
            pair<int, int *> dataInfo = task.serialize(best->price);
            MPI_Send(dataInfo.second, dataInfo.first, MPI_INT, workerId, TAG_WORK, MPI_COMM_WORLD);
            cout << "MASTER - initial work sended to " << workerId << endl;
            delete[] dataInfo.second;
        }

        // repair workers -- that can happen only when initial work was << than number of workers
        workers -= initialWorkers;
        int bufferSize = map.serialize_size() + 1; // result from worker -- this can have always same size
        while (workers > 0) {
            vector<int> buffer(bufferSize);

            MPI_Status status; // wait for result from some slave
            MPI_Recv(buffer.data(), bufferSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // if best price was updated
            if (status.MPI_TAG == TAG_DONE_UPDATE) {
                //update best price
//                int received;
//                MPI_Get_count(&status, MPI_INT, &received);
                // update best map
                int n = info->columns * info->rows;
                int nextId = buffer[n];
                int x = buffer[n + 1];
                int y = buffer[n + 2];
                int bestPriceUpdate = buffer[n + 3];
                best->map = ArrayMap(buffer.data(), info->rows, info->columns, nextId, x, y);
                best->price = bestPriceUpdate;
            }

            if (!dataQueue.empty()) {   //more work
                QueueItem task = dataQueue.front();
                dataQueue.pop_front();
                // prepare task
                pair<int, int *> dataInfo = task.serialize(best->price);
                MPI_Send(dataInfo.second, dataInfo.first, MPI_INT, status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);

                delete[] dataInfo.second;
            } else {  // no more work -- finish
                int dummy = 1; // ???
                MPI_Send(&dummy, 1, MPI_INT, status.MPI_SOURCE, TAG_END, MPI_COMM_WORLD);
                workers--;
            }
        }

        cout << "MASTER -- routine quit " << workers << endl;
    }

    void slave(const int & id) {
        int bufferSize = info->columns * info->rows + 6;
        cout << "SLAVE:= " << id << " started" << endl;
        while (true) {
            std::vector<int> buffer(bufferSize);
            MPI_Status status;
            MPI_Recv(buffer.data(), bufferSize, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            cout << "SLAVE:= " << id << " - recieved: tag" << status.MPI_TAG << endl;

            // signal to quit. Can go home.
            if (status.MPI_TAG == TAG_END) break;

            //  int received;
            //   MPI_Get_count(&status, MPI_INT, &received);


            unsigned int n = info->columns * info->rows;
            int nextId = buffer[n];
            int x = buffer[n + 1];
            int y = buffer[n + 2];
            int price = buffer[n + 3];
            int uncovered = buffer[n + 4];
            int currentBestprice = buffer[n + 5];

            ArrayMap map(buffer.data(), info->rows, info->columns, nextId, x, y);
            best = new SolverResult(map);

            //  #pragma omp parallel shared(info) shared(best) num_threads(4)
            //     {
            //        #pragma omp single
            startSolve(&map, price, uncovered);
            //   }

            //send back result
            if (best->price > currentBestprice) {
                //send UPDATED solution
                int size = best->map.serialize_size() + 1;
                int * data = new int[size];

                pair<int,int*> mapSerialize = best->map.serialize();
                for(int i = 0; i< mapSerialize.first; i++)
                    data[i] = mapSerialize.second[i];

                data[mapSerialize.first] = best->price;
                cout << "SLAVE:= " << id << " - sending DONE_UPDATE" <<  endl;
                MPI_Send(data, size, MPI_INT, 0, TAG_DONE_UPDATE, MPI_COMM_WORLD);
                cout << "SLAVE:= " << id << " - sending DONE_UPDATE OK" << endl;

                delete[] mapSerialize.second;
                delete[] data;
            } else {
                //send no update
                int dummy = 1; // ???
                cout << "SLAVE:= " << id << " - sending DONE_NO_UPDATE" <<endl;

                MPI_Send(&dummy, 1, MPI_INT, 0, TAG_DONE_NO_UPDATE, MPI_COMM_WORLD);
                cout << "SLAVE:= " << id << " - sending DONE_NO_UPDATE OK" << endl;
            }
        }
        cout << "SLAVE:= " << id << " ends" << endl;

    }

    void solve_bfs(ArrayMap *map, int price, int uncovered) {

        //place H I2
        if (map->canPlaceHorizontal(info->i2)) {
            dataQueue.emplace_back(map->placeHorizontal(info->i2), price + info->c2, uncovered - info->i2);
        }

        //place V I2
        if (map->canPlaceVertical(info->i2)) {
            dataQueue.emplace_back(map->placeVertical(info->i2), price + info->c2, uncovered - info->i2);
        }

        //place H I1
        if (map->canPlaceHorizontal(info->i1)) {
            dataQueue.emplace_back(map->placeHorizontal(info->i1), price + info->c1, uncovered - info->i1);
        }

        //place V I1
        if (map->canPlaceVertical(info->i1)) {
            dataQueue.emplace_back(map->placeVertical(info->i1), price + info->c1, uncovered - info->i1);
        }

        //SKIP on purpose
        map->nextFree();
        dataQueue.emplace_back(*map, price + info->cn, uncovered - 1);
    }

    void solve_dfs(ArrayMap *map, int price, int uncovered) {
        int upperPrice = info->computeUpperPrice(uncovered);

        if (price + upperPrice <= best->price)
            return;
        if (best->price == info->optimPrice)
            return;

        if (price + info->cn * uncovered > best->price) {
            // #pragma omp critical
            //  {
            if (price + info->cn * uncovered > best->price) {
                best->map = *map;
                best->price = price + info->cn * uncovered;
            }
            //   }
        }

        if (map->isOnRightBottomCorner())
            return;

        if (map->freeBlock()) {
            //place H I2
            if (map->canPlaceHorizontal(info->i2)) {
                ArrayMap modifiedMap = map->placeHorizontal(info->i2);
                solve_dfs(&modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place V I2
            if (map->canPlaceVertical(info->i2)) {
                ArrayMap modifiedMap = map->placeVertical(info->i2);
                solve_dfs(&modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place H I1
            if (map->canPlaceHorizontal(info->i1)) {
                ArrayMap modifiedMap = map->placeHorizontal(info->i1);
                solve_dfs(&modifiedMap, price + info->c1, uncovered - info->i1);
            }

            //place V I1
            if (map->canPlaceVertical(info->i1)) {
                ArrayMap modifiedMap = map->placeVertical(info->i1);
                solve_dfs(&modifiedMap, price + info->c1, uncovered - info->i1);
            }
            //SKIP on purpose
            map->nextFree();
            solve_dfs(map, price + info->cn, uncovered - 1);
        } else { // standing on forbiden or placed tile
            map->nextFree();
            solve_dfs(map, price, uncovered);
        }
    }

    void startSolve(ArrayMap *map, int price, int uncovered) {
        //place H I2
        if (map->canPlaceHorizontal(info->i2)) {
            ArrayMap modifiedMap = map->placeHorizontal(info->i2);
            //  #pragma omp task firstprivate(price, uncovered)
            solve_dfs(&modifiedMap, price + info->c2, uncovered - info->i2);
        }

        //place V I2
        if (map->canPlaceVertical(info->i2)) {
            ArrayMap modifiedMap = map->placeVertical(info->i2);
            //  #pragma omp task firstprivate(price, uncovered)
            solve_dfs(&modifiedMap, price + info->c2, uncovered - info->i2);
        }

        //place H I1
        if (map->canPlaceHorizontal(info->i1)) {
            ArrayMap modifiedMap = map->placeHorizontal(info->i1);
            //  #pragma omp task firstprivate(price, uncovered)
            solve_dfs(&modifiedMap, price + info->c1, uncovered - info->i1);
        }

        //place V I1
        if (map->canPlaceVertical(info->i1)) {
            ArrayMap modifiedMap = map->placeVertical(info->i1);
            //  #pragma omp task firstprivate(price, uncovered)
            solve_dfs(&modifiedMap, price + info->c1, uncovered - info->i1);
        }

        //SKIP on purpose
        map->nextFree();
        //  #pragma omp task firstprivate(price, uncovered)
        solve_dfs(map, price + info->cn, uncovered - 1);
    }


    void FindLeftEmptyTiles() const {//Find left empty tiles
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == BLOCK_FREE) {
                    best->empty.insert(make_pair(x, y));
                }
            }
        }
    }

    void prepare_tasks(ArrayMap &map, unsigned int max) {
        solve_bfs(&map, 0, info->startUncovered);

        while (dataQueue.size() < max) {
            QueueItem item = dataQueue.front();
            dataQueue.pop_front();
            solve_bfs(&item.map, item.price, item.uncovered);
        }
    }

    void printMap(const ArrayMap &matrix, const int &cx, const int &cy, const int &price, const int &uncovered) const {
        cout << "X: " << cx << " Y: " << cy << " P: " << price << " B: " << best->price <<
             " U: " << uncovered << endl;
        cout << matrix << endl;
    }

};

// -----------------------------------------------------------------------------------------------------------------

MapInfo *load(istream &is) {
    int rows, columns, i1, i2, c1, c2, cn, k;

    is >> rows >> columns;
    is >> i1 >> i2 >> c1 >> c2 >> cn;
    is >> k;

    MapInfo *mapInfo = new MapInfo(rows, columns, i1, i2, c1, c2, cn, k);
    int x, y;
    for (int i = 0; i < k; i++) {
        is >> x >> y;
        mapInfo->addBanned(x, y);
    }

    return mapInfo;
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv); // inicializace MPI knihovny

    int proc_num, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    MapInfo *mapInfo;

    if (argc == 2) {    //load from file
        ifstream ifile(argv[1], ios::in);
        if (ifile) {
            mapInfo = load(ifile);
        } else {
            cout << "Problem with opening file. Exit." << endl;
            return -1;
        }
        ifile.close();
    } else {
        cout << "NO FILE PROVIDED" << endl;
        return -1;
       // mapInfo = load(cin);
    }
    Solver solver(mapInfo);
    solver.solve();

    if (proc_num == 0) {
        cout << *solver.best;
    }
    delete mapInfo;

    MPI_Finalize();
    return 0;
}