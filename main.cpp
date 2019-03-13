#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <cassert>

using namespace std;

#define BLOCK_FREE 0
#define BLOCK_BAN -1

// ------------------------------------------------------------------------------------------------------------------
class ArrayMap {
public:
    int rows, columns;
    int n;
    int nextId;

    ArrayMap(const int rows, const int columns)
            : rows(rows), columns(columns), n(rows * columns), nextId(1) {
        this->matrix = new int[n];
        for (int i = 0; i < n; i++)
            matrix[i] = BLOCK_FREE;
    }

    void writeCopy(const ArrayMap &copy) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->n = copy.n;
        this->nextId = copy.nextId;
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

    pair<int, int> nextFree(const int &x, const int &y) const {
        pair<int, int> next = nextCoordinates(x, y);

        while (!freeBlock(next.first, next.second) && !isOnRightBottomCorner(next.first, next.second)) {
            next = nextCoordinates(next.first, next.second);
        }
        return next;
    }

    bool isOnRightBottomCorner(const int &x, const int &y) const {
        return x == columns - 1 && y == rows - 1;
    }

    pair<int, int> findStart() {
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < columns; x++) {
                if (getValue(x, y) == BLOCK_FREE)
                    return pair<int, int>(x, y);
            }
        }

        return pair<int, int>(-1, -1);
    }

    bool canPlaceHorizontal(int tile, const int &x, const int &y) {
        tile--;
        if (x + tile >= columns)
            return false;

        for (int i = x; i <= x + tile; i++) {
            if (getValue(i, y) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    bool canPlaceVertical(int tile, const int &x, const int &y) {
        tile--;
        if (y + tile >= rows)
            return false;

        for (int i = y; i <= y + tile; i++) {
            if (getValue(x, i) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    ArrayMap placeHorizontal(int tile, const int &x, const int &y) {
        ArrayMap map = *this;
        tile--;
        for (int i = x; i <= x + tile; i++)
            map.setValue(i, y, nextId);
        map.nextId++;
        return map;
    }

    ArrayMap placeVertical(int tile, const int &x, const int &y) {
        ArrayMap map = *this;
        tile--;
        for (int i = y; i <= y + tile; i++)
            map.setValue(x, i, nextId);
        map.nextId++;

        return map;
    }

    bool tryPlaceHorizontal(int tile, const int &x, const int &y){
        if(!canPlaceHorizontal(tile, x,y))
            return false;
        tile--;
        for (int i = x; i <= x + tile; i++)
            setValue(i, y, nextId);
        nextId++;
        return true;
    }

    bool tryPlaceVertical(int tile, const int &x, const int &y){
        if(!canPlaceVertical(tile, x,y))
            return false;

        tile--;
        for (int i = y; i <= y + tile; i++)
            setValue(x, i, nextId);
        nextId++;
        return true;
    }

    friend ostream &operator<<(ostream &os, const ArrayMap &map);

private:
    //vector<int> matrix;
    int *matrix;

    pair<int, int> nextCoordinates(const int &x, const int &y) const {
        if (x + 1 < columns)
            return make_pair(x + 1, y);
        else if (y + 1 < rows)
            return make_pair(0, y + 1);

        return make_pair(-1, -1);
    }
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
    int k;
    int startUncovered;
    int optimPrice;
    set<pair<int, int>> banned;

    //ArrayMap map;

    //  const static int FREE = 0;
    // const static int BLOCK = -1;

    MapInfo(int rows, int columns, int i1, int i2, int c1, int c2, int cn, int k)
            : rows(rows), columns(columns), i1(i1), i2(i2), c1(c1), c2(c2), cn(cn), k(k),
              startUncovered(rows * columns - k)/* map(rows, columns)*/ {
        optimPrice = computeUpperPrice(startUncovered);
    }

    void addBanned(const int &x, const int &y) {
        banned.insert(make_pair(x, y));
    }

    int computeUpperPrice(int number) const {
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

};

// ------------------------------------------------------------------------------------------------------------------
class SolverResult {
public:
    set<pair<int, int>> empty;
    int price;
    ArrayMap map;

    SolverResult(const ArrayMap &map)
            : price(INT32_MIN), map(map) {
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
class Solver {
public:
    Solver(const MapInfo &mapInfo)
            : info(mapInfo), best(nullptr) {
    }

    SolverResult &solve() {

        ArrayMap map(info.rows, info.columns);
        for (auto &ban : info.banned) {
            map.setValue(ban.first, ban.second, BLOCK_BAN);
        }

        best = new SolverResult(map);

        pair<int, int> start = map.findStart();
        solveInternal(map, start.first, start.second, 0, info.startUncovered);

        //Find left empty tiles
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == BLOCK_FREE) {
                    best->empty.insert(make_pair(x, y));
                }
            }
        }

        return *best;
    }

    ~Solver() {
        if (best != nullptr)
            delete best;
    }

private:
    const MapInfo &info;
    SolverResult *best;
   // const int TILE_NULL = 0;
   // int nextId = 0;

    void solveInternal(ArrayMap & map, const int &x, const int &y, int price, int uncovered) {

        int upperPrice = info.computeUpperPrice(uncovered);
        //  printMap(map, x, y, price, uncovered);
        if (price + upperPrice <= best->price)
            return;

        if (best->price == info.optimPrice)
            return;

        if (price + info.cn * uncovered > best->price) {
            //  printMap(map, x, y, price, uncovered);
            best->map = map;
            best->price = price + info.cn * uncovered;
        }

        if (map.isOnRightBottomCorner(x, y))
            return;

//        //find next free X space
        pair<int, int> next = map.nextFree(x, y);

        if (map.freeBlock(x, y)) {
            //place H I2
            if (map.canPlaceHorizontal(info.i2, x, y)) {
                ArrayMap modifiedMap = map.placeHorizontal(info.i2, x,y);
                next = modifiedMap.nextFree(x, y);
                solveInternal(modifiedMap, next.first, next.second, price + info.c2, uncovered - info.i2);
               // removeHorizontal(map, info.i2, x, y);
            }

            //place V I2
            if (map.canPlaceVertical(info.i2, x, y)) {
                ArrayMap modifiedMap = map.placeVertical(info.i2, x,y);
                next = modifiedMap.nextFree(x, y);
                solveInternal(modifiedMap, next.first, next.second, price + info.c2, uncovered - info.i2);
              //  removeVertical(map, info.i2, x, y);
            }

            //place H I1
            if (map.canPlaceHorizontal(info.i1, x, y)) {
                ArrayMap modifiedMap = map.placeHorizontal(info.i1, x,y);
                next = modifiedMap.nextFree(x, y);
                solveInternal(modifiedMap, next.first, next.second, price + info.c1, uncovered - info.i1);
               // removeHorizontal(map, info.i1, x, y);
            }

            //place V I1
            if (map.canPlaceVertical(info.i1, x, y)) {
                ArrayMap modifiedMap = map.placeVertical(info.i1, x,y);
                next = modifiedMap.nextFree(x, y);
                solveInternal(modifiedMap, next.first, next.second, price + info.c1, uncovered - info.i1);
              //  removeVertical(map, info.i1, x, y);
            }
            //SKIP on purpose
            solveInternal(map, next.first, next.second, price + info.cn, uncovered - 1);

        } else { // standing on forbiden or placed tile
            solveInternal(map, next.first, next.second, price, uncovered);
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
    } else {
        mapInfo = load(cin);
    }
    Solver solver(*mapInfo);
    SolverResult &result = solver.solve();
    cout << result;
    delete mapInfo;
    return 0;
}