#include <utility>

#include <iostream>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include <cassert>
#include <omp.h>

using namespace std;

#define BLOCK_FREE 0
#define BLOCK_BAN -1

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
class ArrayMap {
public:
    int rows, columns;
    int nextId;
    int x, y;

    ArrayMap(const int rows, const int columns)
            : rows(rows), columns(columns), nextId(1), x(0), y(0) {
        int n = rows * columns;
        this->matrix = new int[n];
        for (int i = 0; i < n; i++)
            matrix[i] = BLOCK_FREE;
    }

    void writeCopy(const ArrayMap &copy) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->nextId = copy.nextId;
        this->x = copy.x;
        this->y = copy.y;

        int n = rows * columns;
        this->matrix = new int[n];
        for (int i = 0; i < n; i++)
            this->matrix[i] = copy.matrix[i];
    }

    ArrayMap(const ArrayMap &copy) {
        writeCopy(copy);
    }

    //move constructor
    ArrayMap(ArrayMap &&copy)
            : rows(copy.rows), columns(copy.columns), nextId(copy.nextId), x(copy.x), y(copy.y), matrix(copy.matrix) {

        copy.matrix = nullptr;
        cout << "MOVE constructor" << endl;
    }

    ArrayMap &operator=(const ArrayMap &copy) {
        if (this == &copy)
            return *this;
        delete[] matrix;

        writeCopy(copy);
        return *this;
    }

    //move operator
    ArrayMap &operator=(ArrayMap &&copy) {
        if (this == &copy)
            return *this;
        delete[] matrix;

        cout << "MOVE =" << endl;

        this->rows = copy.rows;
        this->columns = copy.columns;
        this->nextId = copy.nextId;
        this->x = copy.x;
        this->y = copy.y;
        this->matrix = copy.matrix;

        copy.matrix = nullptr;
        return *this;
    }

    ~ArrayMap() {
        delete[] matrix;
    }

    bool freeBlock() const {
        return getValue(x, y) == BLOCK_FREE;
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

    void nextFree() {
        pair<int, int> next = nextCoordinates(x, y);

        while (!freeBlock(next.first, next.second) && !isOnRightBottomCorner(next.first, next.second)) {
            next = nextCoordinates(next.first, next.second);
        }
        this->x = next.first;
        this->y = next.second;
    }

    bool isOnRightBottomCorner() const {
        return isOnRightBottomCorner(x, y);
    }

    void setStart() {
        for (int iy = 0; iy < rows; iy++) {
            for (int ix = 0; ix < columns; ix++) {
                if (getValue(ix, iy) == BLOCK_FREE) {
                    x = ix;
                    y = iy;
                    return;
                }
            }
        }
        x = columns - 1;
        y = rows - 1;

    }

    bool canPlaceHorizontal(const int & tile) {
        if (x + tile - 1 >= columns)
            return false;

        for (int i = x; i < x + tile; i++) {
            if (getValue(i, y) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    bool canPlaceVertical(const int & tile) {
        if (y + tile -1 >= rows)
            return false;

        for (int i = y; i < y + tile; i++) {
            if (getValue(x, i) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    ArrayMap placeHorizontal(const int & tile) {
        ArrayMap map = *this;
        for (int i = x; i < x + tile; i++)
            map.setValue(i, y, nextId);
        map.nextId++;
        map.nextFree();
        return map;
    }

    ArrayMap placeVertical(const int & tile) {
        ArrayMap map = *this;
        for (int i = y; i < y + tile; i++)
            map.setValue(x, i, nextId);
        map.nextId++;
        map.nextFree();
        return map;
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

    bool isOnRightBottomCorner(const int &x, const int &y) const {
        return x == columns - 1 && y == rows - 1;
    }

    bool freeBlock(const int &x, const int &y) const {
        return getValue(x, y) == BLOCK_FREE;
    }
};


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
class Solver {
public:
    Solver(MapInfo *mapInfo)
            : info(mapInfo), best(nullptr) {
    }

    SolverResult &solve() {
        ArrayMap map(info->rows, info->columns);
        for (auto &ban : info->banned) {
            map.setValue(ban.first, ban.second, BLOCK_BAN);
        }
        map.setStart();

        best = new SolverResult(map);
        #pragma omp parallel shared(info) shared(best) num_threads(4)
        {
            #pragma omp single
            startSolve(&map, 0, info->startUncovered);
        }

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
        delete best;
    }

private:
    MapInfo *info;
    SolverResult *best;

    void startSolve(ArrayMap *map, int price, int uncovered) {

        //place H I2
        if (map->canPlaceHorizontal(info->i2)) {
            ArrayMap modifiedMap = map->placeHorizontal(info->i2);
            #pragma omp task firstprivate(price, uncovered)
            solve(&modifiedMap, price + info->c2, uncovered - info->i2);
        }

        //place V I2
        if (map->canPlaceVertical(info->i2)) {
            ArrayMap modifiedMap = map->placeVertical(info->i2);
            #pragma omp task firstprivate(price, uncovered)
            solve(&modifiedMap, price + info->c2, uncovered - info->i2);
        }

        //place H I1
        if (map->canPlaceHorizontal(info->i1)) {
            ArrayMap modifiedMap = map->placeHorizontal(info->i1);
            #pragma omp task firstprivate(price, uncovered)
            solve(&modifiedMap, price + info->c1, uncovered - info->i1);
        }

        //place V I1
        if (map->canPlaceVertical(info->i1)) {
            ArrayMap modifiedMap = map->placeVertical(info->i1);
            #pragma omp task firstprivate(price, uncovered)
            solve(&modifiedMap, price + info->c1, uncovered - info->i1);
        }

        //SKIP on purpose
        map->nextFree();
        #pragma omp task firstprivate(price, uncovered)
        solve(map, price + info->cn, uncovered - 1);
    }

    void solve(ArrayMap *map, int price, int uncovered) {
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
                solve(&modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place V I2
            if (map->canPlaceVertical(info->i2)) {
                ArrayMap modifiedMap = map->placeVertical(info->i2);
                solve(&modifiedMap, price + info->c2, uncovered - info->i2);
            }

            //place H I1
            if (map->canPlaceHorizontal(info->i1)) {
                ArrayMap modifiedMap = map->placeHorizontal(info->i1);
                solve(&modifiedMap, price + info->c1, uncovered - info->i1);
            }

            //place V I1
            if (map->canPlaceVertical(info->i1)) {
                ArrayMap modifiedMap = map->placeVertical(info->i1);
                solve(&modifiedMap, price + info->c1, uncovered - info->i1);
            }
            //SKIP on purpose
            map->nextFree();
            solve(map, price + info->cn, uncovered - 1);
        } else { // standing on forbiden or placed tile
            map->nextFree();
            solve(map, price, uncovered);
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