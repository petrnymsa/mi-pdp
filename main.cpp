#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <cassert>

using namespace std;

// ------------------------------------------------------------------------------------------------------------------
class ArrayMap {
public:
    int rows, columns;
    int n;

    ArrayMap(const int rows, const int columns)
            : rows(rows), columns(columns), n(rows * columns) {
        matrix = new int[n];
    }

    void writeCopy(const ArrayMap &copy) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->n = copy.n;

        this->matrix = new int[n];
        for (int i = 0; i < n; i++)
            this->matrix[i] = copy.matrix[i];
    }

    ArrayMap(const ArrayMap &copy) {
        writeCopy(copy);
    }

    ArrayMap &operator=(const ArrayMap &copy) {
        if (this == &copy)
            return *this;
        delete[] matrix;
        writeCopy(copy);
        return *this;
    }

    ~ArrayMap() {
        delete[] matrix;
    }

    bool freeBlock(const int &x, const int &y) const {
        return getValue(x, y) == 0;
    }

    int getValue(const int &x, const int &y) const {
//        assert(x < columns && x >= 0);
//        assert(y < rows && y >= 0);
        return matrix[x + y * columns];
        //  return matrix[x][y];
    }

    void setValue(const int &x, const int &y, const int value) {
        //   assert(x < columns);
        //   assert(y < rows);
        matrix[x + y * columns] = value;
        // matrix[x][y] = value;
    }


    pair<int, int> nextCoordinates(const int &x, const int &y) const {
        if (x + 1 < columns)
            return make_pair(x + 1, y);
        else if (y + 1 < rows)
            return make_pair(0, y + 1);

        return make_pair(-1, -1);
    }

//    pair<int, int> prevCoordinates(const int &x, const int &y) const {
//        if (x - 1 >= 0)
//            return make_pair(x - 1, y);
//        else
//            return make_pair(columns - 1, y - 1);
//    }

    bool isOnRightBottomCorner(const int &x, const int &y) {
        return x == columns - 1 && y == rows - 1;
    }

    friend ostream &operator<<(ostream &os, const ArrayMap &map);

private:
    //vector<int> matrix;
    int *matrix;
};

// ------------------------------------------------------------------------------------------------------------------
class MapInfo {
public:
    int rows;
    int columns;
    int i1;
    int i2;
    int c1;
    int c2;
    int cn;
    int startUncovered;
    int optimPrice;

    ArrayMap map;

    const static int FREE = 0;
    const static int BLOCK = -1;
//    const static int START = -2;

    MapInfo(int rows, int columns, int i1, int i2, int c1, int c2, int cn)
            : rows(rows), columns(columns), i1(i1), i2(i2), c1(c1), c2(c2), cn(cn), map(rows, columns) {

    }

    pair<int, int> findStart() {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < columns; x++) {
                if (map.getValue(x, y) == FREE)
                    return pair<int, int>(x, y);
            }
        }

        return pair<int, int>(-1, -1);
    }

//    int computePrice(int i1_cnt, int i2_cnt, int not_covered) {
//        return c1 * i1_cnt + c2 * i2_cnt + cn * not_covered;
//    }

    int computeUpperPrice(int number) {
        // vraci maximalni cenu pro "number" nevyresenych policek
        // returns the maximal price for the "number" of unsolved squares
        int i, x;
        int max = c2 * (number / i2);
        int zb = number % i2;
        max += c1 * (zb / i1);
        zb = zb % i1;
        max += zb * cn;
        for (i = 0; i < (number / i2); i++) {
            x = c2 * i;
            zb = number - i * i2;
            x += c1 * (zb / i1);
            zb = zb % i1;
            x += zb * cn;
            if (x > max) max = x;
        }
        return max;
    }

    static MapInfo *load(istream &is) {
        int rows, columns, i1, i2, c1, c2, cn, k;

        is >> rows >> columns;
        is >> i1 >> i2 >> c1 >> c2 >> cn;
        is >> k;

        MapInfo *mapInfo = new MapInfo(rows, columns, i1, i2, c1, c2, cn);

        int x, y;
        for (int i = 0; i < k; i++) {
            is >> x >> y;
            mapInfo->map.setValue(x, y, BLOCK);
        }

        mapInfo->startUncovered = rows * columns - k;
        mapInfo->optimPrice = mapInfo->computeUpperPrice(mapInfo->startUncovered);

        return mapInfo;
    }
};

// ------------------------------------------------------------------------------------------------------------------
class SolverResult {
public:
    set<pair<int, int>> empty;
    int price;
    ArrayMap map;
    int emptyCount;

    SolverResult(ArrayMap &map)
            : price(INT32_MIN), map(map), emptyCount(0) {
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
                case MapInfo::FREE :
                    os << '.';
                    break;
                case MapInfo::BLOCK :
                    os << 'X';
                    break;
                default:
                    os << p;
                    break;
            }
        }
        os << endl;
    }

    return os;
}

// ------------------------------------------------------------------------------------------------------------------
class Solver {
public:
    Solver(MapInfo &mapInfo)
            : info(mapInfo) {
    }

    SolverResult &solve() {
        best = new SolverResult(info.map);

        pair<int, int> start = info.findStart();
        //    printMap(info.map, 0,0,0,0,0);
        cout << "Max optim price: " << info.optimPrice << endl;
        solveInternal(info.map, start.first, start.second, 0, info.startUncovered);

        //TODO tohle asi nechci
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == MapInfo::FREE) {
                    best->empty.insert(make_pair(x, y));
                }
            }
        }

        return *best;
    }

    ~Solver() {
        delete best;
    }

private:
    MapInfo info;
    SolverResult *best;
    const int TILE_NULL = 0;
    int nextId = 0;

    void solveInternal(ArrayMap &map, const int &x, const int &y, int price, int uncovered) {

        if (best->price == info.optimPrice)
            return;

        int upperPrice = info.computeUpperPrice(uncovered);

        //  printMap(map, x, y, price, uncovered);

        if (price + upperPrice < best->price) {
            return;
        }

        if (price > best->price) {
            //  printMap(map, x, y, price, uncovered);
            best->map = map;
            best->price = price + info.cn * uncovered;
        }

        if (map.isOnRightBottomCorner(x, y)) {
            return;
        }

//        //find next free X space
        pair<int, int> next = map.nextCoordinates(x, y);
        // look for the next really free tile or until you are on the right corner
        while (!map.freeBlock(next.first, next.second) && !map.isOnRightBottomCorner(next.first, next.second))
            next = map.nextCoordinates(next.first, next.second);

        if (map.freeBlock(x, y)) {
            //place H I2
            if (tryPlaceHorizontal(map, info.i2, x, y)) {
                solveInternal(map, next.first, next.second, price + info.c2, uncovered - info.i2);
                removeHorizontal(map, info.i2, x, y);
            }

            //place H I1
            if (tryPlaceHorizontal(map, info.i1, x, y)) {
                solveInternal(map, next.first, next.second, price + info.c1, uncovered - info.i1);
                removeHorizontal(map, info.i1, x, y);
            }

            //place V I2
            if (tryPlaceVertical(map, info.i2, x, y)) {
                solveInternal(map, next.first, next.second, price + info.c2, uncovered - info.i2);
                removeVertical(map, info.i2, x, y);
            }

            //place V I1
            if (tryPlaceVertical(map, info.i1, x, y)) {
                solveInternal(map, next.first, next.second, price + info.c1,uncovered - info.i1);
                removeVertical(map, info.i1, x, y);
            }
            //SKIP on purpose
            solveInternal(map, next.first, next.second, price + info.cn, uncovered - 1);

        } else { // standing on forbiden or placed tile
            solveInternal(map, next.first, next.second, price, uncovered);
        }

    }

    bool tryPlaceHorizontal(ArrayMap &map, int tile, const int &x, const int &y) {
        tile--;
        if (x + tile >= map.columns)
            return false;

        for (int i = x; i <= x + tile; i++) {
            if (map.getValue(i, y) != MapInfo::FREE)
                return false;
        }
        nextId++;
        for (int i = x; i <= x + tile; i++)
            map.setValue(i, y, nextId);

        return true;
    }

    bool tryPlaceVertical(ArrayMap &map, int tile, const int &x, const int &y) {
        tile--;
        if (y + tile >= map.rows)
            return false;

        for (int i = y; i <= y + tile; i++) {
            if (map.getValue(x, i) != MapInfo::FREE)
                return false;
        }
        nextId++;
        for (int i = y; i <= y + tile; i++)
            map.setValue(x, i, nextId);

        return true;
    }

    void removeHorizontal(ArrayMap &map, int tile, const int &x, const int &y) {
        tile--;
        for (int i = x; i <= x + tile; i++)
            map.setValue(i, y, MapInfo::FREE);
        nextId--;
    }

    void removeVertical(ArrayMap &map, int tile, const int &x, const int &y) {
        tile--;
        for (int i = y; i <= y + tile; i++)
            map.setValue(x, i, MapInfo::FREE);
        nextId--;
    }

    void printMap(const ArrayMap &matrix, const int &cx, const int &cy, const int &price, const int &uncovered) const {
        cout << "X: " << cx << " Y: " << cy << " P: " << price << " B: " << best->price <<
             " U: " << uncovered << endl;
        cout << matrix << endl;
    }
};

// -----------------------------------------------------------------------------------------------------------------
int main(int argc, char **argv) {
    MapInfo *mapInfo;

    if (argc == 2) {    //load from file
        ifstream ifile(argv[1], ios::in);
        if (ifile) {
            mapInfo = MapInfo::load(ifile);
        } else {
            cout << "Problem with opening file. Exit." << endl;
            return -1;
        }
    } else {
        mapInfo = MapInfo::load(cin);
    }

    //cout << info->map << endl;

    Solver solver(*mapInfo);

    SolverResult &result = solver.solve();
    cout << result;
    delete mapInfo;
    return 0;
}