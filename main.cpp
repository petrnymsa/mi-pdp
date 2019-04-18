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

#include "src/map_info.h"
#include "src/array_map.h"

using namespace std;

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

class QueueItem {
public:
    ArrayMap map;
    int price;
    int uncovered;

    QueueItem() {

    }

    QueueItem(const ArrayMap & map, int price, int uncovered) : map(map), price(price), uncovered(uncovered) {

    }

    QueueItem &operator=(const QueueItem &copy) {
        if (this == &copy)
            return *this;

        map = copy.map;
        price = copy.price;
        uncovered = copy.uncovered;
        return *this;
    }
};

// ------------------------------------------------------------------------------------------------------------------
class Solver {
public:
    Solver(MapInfo *mapInfo)
            : info(mapInfo), best(nullptr) {
    }

    SolverResult &solve() {
        ArrayMap map(info->rows, info->columns, info->banned);
//        for (auto &ban : info->banned) {
//            map.setValue(ban.first, ban.second, BLOCK_BAN);
//        }
        map.setStart();
        best = new SolverResult(map);

        unsigned int max = 8;
        solve_bfs(&map, 0, info->startUncovered);

        while (dataQueue.size() < max)
        {
            QueueItem item = dataQueue.front();
            dataQueue.pop_front();
            solve_bfs(&item.map,item.price, item.uncovered);
        }

        unsigned long i = 0;
        unsigned long n = dataQueue.size();
        #pragma omp parallel for private(i) schedule(static,1) shared(info, best)
        for (i = 0; i < n; i++) {
            QueueItem item;
            #pragma omp critical
            {
                item = dataQueue.front();
                dataQueue.pop_front();
            }
            solve_dfs(&item.map, item.price, item.uncovered);
        }

        FindLeftEmptyTiles();
        return *best;
    }

    ~Solver() {
        delete best;
    }

private:
    MapInfo *info;
    SolverResult *best;
    deque<QueueItem> dataQueue;

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

    void solve_dfs(ArrayMap * map, int price, int uncovered) {
        //    bool canContinue = true;
        int upperPrice = info->computeUpperPrice(uncovered);
        //  printMap(map, x, y, price, uncovered);
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
                }
            }
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

    void FindLeftEmptyTiles() const {//Find left empty tiles
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == BLOCK_FREE) {
                    best->empty.insert(make_pair(x, y));
                }
            }
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
        mapInfo = load(cin);
    }
    Solver solver(mapInfo);
    SolverResult &result = solver.solve();
    cout << result;
    delete mapInfo;
    return 0;
}