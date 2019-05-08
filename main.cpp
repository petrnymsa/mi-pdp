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
#include <chrono> // for high_resolution_clock
#include "src/map_info.h"
#include "src/array_map.h"

using namespace std;

#define TAG_WORK 1
#define TAG_END 2
#define TAG_UPDATE 3
#define TAG_DONE 4


// ------------------------------------------------------------------------------------------------------------------
class SolverResult {
public:
    set <pair<int, int>> empty;
    int price;
    ArrayMap map;

    SolverResult(const ArrayMap &map)
            : price(INT32_MIN), map(map) {
    }

    SolverResult(const int &bestPrice, const ArrayMap &map)
            : price(bestPrice), map(map) {
    }

    friend ostream &operator<<(ostream &out, const SolverResult &result);
};

ostream &operator<<(ostream &os, const SolverResult &result) {
    // os << result.map;
    os << result.price << endl;
//    os << result.empty.size() << endl;
//    for (const auto &p : result.empty)
//        os << p.first << " " << p.second << endl;

    return os;
}

ostream &operator<<(ostream &os, const ArrayMap &map) {
    for (int y = 0; y < map.rows; y++) {
        for (int x = 0; x < map.columns; x++) {
            int p = map.getValue(x, y);
            switch (p) {
                case BLOCK_FREE:
                    os << ".\t";
                    break;
                case BLOCK_BAN:
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
    int num_procs;
    int num_threads;
    // int num_procs;

    Solver(MapInfo *mapInfo, const int &threads)
            : best(nullptr), num_threads(threads), info(mapInfo) {
    }

    void solve() {

        int proc_num;
        MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
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
    deque <QueueItem> dataQueue;

    void master(const int &num_procs) {
        // prepare map
        ArrayMap map(info->rows, info->columns, info->banned);
        map.setStart();
        best = new SolverResult(map);

        // prepare data with BFS
        const unsigned int max = num_procs * 1.5f;
        cout << "PREPARE " << max << endl;
        prepare_tasks(map, max, 0, info->startUncovered);

        int workers = num_procs - 1;
        int initialWorkers = workers;
        set<int> aliveWorkers = set<int>();
        // initial send of work
#ifdef DEBUG
        cout << "MASTER - initial-send-to-work, workers" << workers << ", works to do: " << dataQueue.size() << endl;
#endif
        for (int workerId = 1; workerId <= workers && !dataQueue.empty(); workerId++) {
            QueueItem task = dataQueue.front();
            dataQueue.pop_front();

            initialWorkers--;
            pair<int, int *> dataInfo = task.serialize(best->price);
            MPI_Send(dataInfo.second, dataInfo.first, MPI_INT, workerId, TAG_WORK, MPI_COMM_WORLD);
#ifdef DEBUG
            cout << "MASTER - initial work sended to " << workerId << endl;
#endif
            delete[] dataInfo.second;

            aliveWorkers.insert(workerId);
        }

        // repair workers -- that can happen only when initial work was << than number of workers
        workers -= initialWorkers;
        int bufferSize = map.serialize_size() + 1; // result from worker -- this can have always same size
        while (workers > 0) {
            vector<int> buffer(bufferSize);

            MPI_Status status; // wait for result from some slave
            MPI_Recv(buffer.data(), bufferSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if(status.MPI_TAG == TAG_DONE) {

                int n = info->columns * info->rows;
                int nextId = buffer[n];
                int x = buffer[n + 1];
                int y = buffer[n + 2];
                int bestPriceUpdate = buffer[n + 3];

                //update best price
                // update best map
                if (bestPriceUpdate > best->price) {
#ifdef DEBUG
                    cout << "MASTER - update PRICE to " << bestPriceUpdate << " from slave: " << status.MPI_SOURCE << endl;
#endif
                    best->map = ArrayMap(buffer.data(), info->rows, info->columns, nextId, x, y);
                    best->price = bestPriceUpdate;

                    //send non-blocking message that price got updated to others
                    for (const auto &wid : aliveWorkers) {
                        if (wid != status.MPI_SOURCE) {
                            MPI_Request request;
                            int copy = bestPriceUpdate;
                            MPI_Isend(&copy, 1, MPI_INT, wid, TAG_UPDATE, MPI_COMM_WORLD, &request);
#ifdef DEBUG
                            cout << "MASTER - sended updated price to slave: " << wid << endl;
#endif
                        }
                    }
                }

                if (!dataQueue.empty()) { //more work
                    QueueItem task = dataQueue.front();
                    dataQueue.pop_front();
                    // prepare task
                    pair<int, int *> dataInfo = task.serialize(best->price);
                    MPI_Send(dataInfo.second, dataInfo.first, MPI_INT, status.MPI_SOURCE, TAG_WORK, MPI_COMM_WORLD);

                    delete[] dataInfo.second;
                } else {                  // no more work -- finish
                    int dummy = 1;
#ifdef DEBUG
                    cout << "MASTER - no more work for " << status.MPI_SOURCE << endl;
#endif
                    MPI_Send(&dummy, 1, MPI_INT, status.MPI_SOURCE, TAG_END, MPI_COMM_WORLD);
                    workers--;

                    aliveWorkers.erase(status.MPI_SOURCE);
                }
            }
            else if(status.MPI_TAG == TAG_UPDATE){
//                int updatedPrice = buffer[0];
//                if (updatedPrice > best->price) {
//                    best->price = updatedPrice; //todo note that map is not updated, but map is not printed
//                    //send non-blocking message that price got updated to others
//                    for (const auto &wid : aliveWorkers) {
//                        if (wid != status.MPI_SOURCE) {
//                            MPI_Request request;
//                            int copy = updatedPrice;
//                            MPI_Isend(&copy, 1, MPI_INT, wid, TAG_UPDATE, MPI_COMM_WORLD, &request);
//#ifdef DEBUG
//                            cout << "MASTER - sended updated price from: " << status.MPI_SOURCE << " to: " << wid << endl;
//#endif
//                        }
//                    }
//                }

            }
            else cout << "MASTER - UNKNOWN tag"<< endl;
        }
#ifdef DEBUG
        cout << "MASTER -- quit " << workers << endl;
#endif
    }

    void slave(const int &id) {
        int bufferSize = info->columns * info->rows + 6;

        while (true) {
            std::vector<int> buffer(bufferSize);

            MPI_Status status;
            MPI_Recv(buffer.data(), bufferSize, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // signal to quit. Can go home.
            if (status.MPI_TAG == TAG_END)
                break;

            unsigned int n = info->columns * info->rows;
            int nextId = buffer[n];
            int x = buffer[n + 1];
            int y = buffer[n + 2];
            int price = buffer[n + 3];
            int uncovered = buffer[n + 4];
            int bestPrice = buffer[n + 5];

            //setup data
            ArrayMap map(buffer.data(), info->rows, info->columns, nextId, x, y);
            best = new SolverResult(bestPrice, map);

            // Prepare BFS
            const unsigned int max = num_threads * 2;
            prepare_tasks(map, max, price, uncovered);

            // Paralell FOR
            unsigned long i = 0;
            unsigned long queueSize = dataQueue.size();
#pragma omp parallel for private(i) schedule(static) shared(info, best)
            for (i = 0; i < queueSize; i++) {
                QueueItem item;
#pragma omp critical
                {
                    item = dataQueue.front();
                    dataQueue.pop_front();
                }
                solve_dfs(id, &item.map, item.price, item.uncovered);
            }

            //send UPDATED solution
            int size = best->map.serialize_size() + 1;
            int *data = new int[size];

            pair<int, int *> mapSerialize = best->map.serialize();
            for (int i = 0; i < mapSerialize.first; i++)
                data[i] = mapSerialize.second[i];

            data[mapSerialize.first] = best->price;
            MPI_Send(data, size, MPI_INT, 0, TAG_DONE, MPI_COMM_WORLD);
//#ifdef DEBUG
//            cout << "SLAVE:= " << id << " - sending DONE ok" << endl;
//#endif
            delete[] mapSerialize.second;
            delete[] data;
        }
#ifdef DEBUG
        cout << "SLAVE:= " << id << " ends" << endl;
#endif
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

    void solve_dfs(const int &id, ArrayMap *map, int price, int uncovered) {
        int upperPrice = info->computeUpperPrice(uncovered);

        //todo check for better price -- direction MASTER => Slave
        MPI_Status status;
        int flag;
        MPI_Iprobe(0, TAG_UPDATE, MPI_COMM_WORLD, &flag, &status);

        if (flag) {
            //got update
            int update;
#ifdef  DEBUG

#endif
            MPI_Recv(&update, 1, MPI_INT, 0, TAG_UPDATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            best->price = update;
            cout << "Slave:" << id << " got updated price=" << update << endl;
        }

        if (price + upperPrice <= best->price)
            return;
        if (best->price == info->optimPrice)
            return;

        if (price + info->cn * uncovered > best->price) {
#pragma omp critical
            {
                if (price + info->cn * uncovered > best->price) {
                    best->map = *map;
                    best->price = price + info->cn * uncovered;

                    //TODO send updated price to master
                   // cout << "Slave: " << id << "found better price=" << best->price << endl;
                   //slave_sendUpdate(best->price);
                }
            }
        }

        if (map->isOnRightBottomCorner())
            return;

        if (map->freeBlock()) {
            //place H I2
            if (map->canPlaceHorizontal(info->i2)) {
                ArrayMap modifiedMap = map->placeHorizontal(info->i2);
                solve_dfs(id, &modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place V I2
            if (map->canPlaceVertical(info->i2)) {
                ArrayMap modifiedMap = map->placeVertical(info->i2);
                solve_dfs(id, &modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place H I1
            if (map->canPlaceHorizontal(info->i1)) {
                ArrayMap modifiedMap = map->placeHorizontal(info->i1);
                solve_dfs(id, &modifiedMap, price + info->c1, uncovered - info->i1);
            }

            //place V I1
            if (map->canPlaceVertical(info->i1)) {
                ArrayMap modifiedMap = map->placeVertical(info->i1);
                solve_dfs(id, &modifiedMap, price + info->c1, uncovered - info->i1);
            }
            //SKIP on purpose
            map->nextFree();
            solve_dfs(id, map, price + info->cn, uncovered - 1);
        } else { // standing on forbiden or placed tile
            map->nextFree();
            solve_dfs(id, map, price, uncovered);
        }
    }

    void slave_sendUpdate(int price) {
        MPI_Request request;
        MPI_Isend(&price, 1, MPI_INT, 0, TAG_UPDATE, MPI_COMM_WORLD, &request);
    }

    void FindLeftEmptyTiles() const { //Find left empty tiles
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == BLOCK_FREE) {
                    best->empty.insert(make_pair(x, y));
                }
            }
        }
    }

    void prepare_tasks(ArrayMap &map, unsigned int max, int actualPrice, int uncovered) {
        this->dataQueue = deque<QueueItem>();

        solve_bfs(&map, actualPrice, uncovered);

        while (dataQueue.size() < max) {
            QueueItem item = dataQueue.front();
            dataQueue.pop_front();
            solve_bfs(&item.map, item.price, item.uncovered);
        }
    }

    void printMap(const ArrayMap &matrix, const int &cx, const int &cy, const int &price, const int &uncovered) const {
        cout << "X: " << cx << " Y: " << cy << " P: " << price << " B: " << best->price << " U: " << uncovered << endl;
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
    // MPI_Init(&argc, &argv); // inicializace MPI knihovny
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);


    if (provided != MPI_THREAD_MULTIPLE) {
        cout << "cant go on. Provided only: " << provided << endl;
        MPI_Finalize();
        return -1;
    }

    int proc_num, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_num);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    MapInfo *mapInfo;

    if (argc >= 2) { //load from file
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

    int threads = 1;
    const int maxNumThreads = omp_get_max_threads();
    if (argc > 2) {
        std::istringstream iss(argv[2]);

        if (iss >> threads) {
            if (threads > maxNumThreads) {
                threads = maxNumThreads;
            }
            omp_set_num_threads(threads);

        } else {
            cout << "Provided number of threads is invalid. EXIT" << endl;
            return -2;
        }
    } else {
        threads = maxNumThreads;
        omp_set_num_threads(threads);
    }

    Solver solver(mapInfo, threads);

    if (proc_num == 0) {
        // auto t1 = MPI_Wtime();
        // auto t2 = MPI_Wtime();
        auto start = std::chrono::high_resolution_clock::now();
        solver.solve();

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;

        cout << "input:\t" << argv[1] << endl;
        cout << "final price:\t" << *solver.best;
        cout << "proc num:\t" << solver.num_procs << endl;
        cout << "threads:\t" << threads << "/" << maxNumThreads << endl;
        cout << "time (ms):\t" << elapsed.count() * 1000 << endl;
        cout << "time (sec):\t" << elapsed.count() << endl;
        cout << "time (minute):\t" << elapsed.count() / 60 << endl;

//        cout << "input:\t" << argv[1] << endl;
//
//        cout << *solver.best;
//        cout << "time (: " << t2 - t1 << endl;
    } else {
        solver.solve();
    }

    delete mapInfo;

    MPI_Finalize();
    return 0;
}