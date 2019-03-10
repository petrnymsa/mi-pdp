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

    ArrayMap(const int rows, const int columns)
            : rows(rows), columns(columns), matrix(rows * columns) {

    }

    ArrayMap(const ArrayMap &copy) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->matrix = copy.matrix;
    }

    bool freeBlock(const int &x, const int &y) const {
        return getValue(x, y) == 0;
    }

    int getValue(const int &x, const int &y) const {
        assert(x < columns && x >= 0);
        assert(y < rows && y >= 0);
        return matrix[x + y * columns];
    }

    void setValue(const int &x, const int &y, const int value) {
     //   assert(x < columns);
     //   assert(y < rows);
        matrix[x + y * columns] = value;
    }


    pair<int, int> nextCoordinates(const int &x, const int &y) const {
        if (x + 1 < columns)
            return make_pair(x + 1, y);
        else if (y + 1 < rows)
            return make_pair(0, y + 1);

        return make_pair(-1, -1);
    }

    pair<int, int> prevCoordinates(const int &x, const int &y) const {
        if (x - 1 >= 0)
            return make_pair(x - 1, y);
        else
            return make_pair(columns - 1, y - 1);
    }

    bool isOnRightBottomCorner(const int &x, const int &y) {
        return x == columns - 1 && y == rows - 1;
    }


    friend ostream &operator<<(ostream &os, const ArrayMap &map);

private:
    vector<int> matrix;
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

    ArrayMap map;

    const static int FREE = 0;
    const static int BLOCK = -1;
    const static int START = -2;

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

    int computePrice(int i1_cnt, int i2_cnt, int not_covered) {
        return c1 * i1_cnt + c2 * i2_cnt + cn * not_covered;
    }

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

        solveInternal(info.map, start.first, start.second, 0, 0, info.startUncovered, 0);

        //TODO tohle asi nechci
        for (int x = 0; x < best->map.columns; x++) {
            for (int y = 0; y < best->map.rows; y++) {
                if (best->map.getValue(x, y) == MapInfo::FREE)
                    best->empty.insert(make_pair(x, y));
            }
        }

        return *best;
    }

    ~Solver() {
        delete best;
    }

private:
    MapInfo &info;
    SolverResult *best;
    const int TILE_NULL = 0;
    int nextId = 0;

//    int jumpToNextAvailableXAfterTile(const int & x, const int & tile){
//        if(x + tile < info.columns)
//            return x+ tile;
//        else x  + 1;
//    }

    void solveInternal(ArrayMap & map, const int & x, const int & y, int i1_cnt, int i2_cnt, int uncovered, int skipped) {

        int upperPrice = info.computeUpperPrice(uncovered - skipped);
        int currentPrice = info.computePrice(i1_cnt, i2_cnt, uncovered);

      //  printMap(map,x,y,currentPrice, uncovered,skipped);

        if(currentPrice - info.cn*uncovered + info.cn*skipped + upperPrice <= best->price) {
            return;
        }

        if (map.isOnRightBottomCorner(x, y)) {
            if (currentPrice > best->price) {
                best->map = map;
                best->price = currentPrice;
            }
            return;
        }

        pair<int, int> next = map.nextCoordinates(x, y);
//        if(next.first != 0 && placed && horizontal){ //TODO check it later
//            next.first = next.first + tileSize < info.columns ? next.first + tileSize : next.first;
//        }

        while(!map.freeBlock(next.first, next.second) && !map.isOnRightBottomCorner(next.first, next.second))
            next = map.nextCoordinates(next.first, next.second);

        

        //place H I1
        bool placed = tryPlaceHorizontal(map,info.i1, x,y);
        solveInternal(map, next.first, next.second, placed ? i1_cnt + 1 : i1_cnt, i2_cnt, placed ? uncovered-info.i1 : uncovered, !placed && map.freeBlock(x,y) ? skipped+1 : skipped);
        if(placed)
            removeHorizontal(map,info.i1,x,y);

        //place V I1
        placed = tryPlaceVertical(map,info.i1, x,y);
        solveInternal(map, next.first, next.second, placed ? i1_cnt + 1 : i1_cnt, i2_cnt, placed ? uncovered-info.i1 : uncovered, !placed && map.freeBlock(x,y) ? skipped+1 : skipped);
        if(placed)
            removeVertical(map,info.i1,x,y);

        //place H I2
        placed = tryPlaceHorizontal(map,info.i2, x,y);
        solveInternal(map, next.first, next.second, i1_cnt, placed ? i2_cnt + 1 : i2_cnt, placed ? uncovered-info.i2 : uncovered, !placed && map.freeBlock(x,y) ? skipped+1 : skipped);
        if(placed)
            removeHorizontal(map,info.i2,x,y);

        //place V I2
        placed = tryPlaceVertical(map,info.i2, x,y);
        solveInternal(map, next.first, next.second, i1_cnt, placed ? i2_cnt + 1 : i2_cnt, placed ? uncovered-info.i2 : uncovered, !placed && map.freeBlock(x,y) ? skipped+1 : skipped);
        if(placed)
            removeVertical(map,info.i2,x,y);

        //SKIP
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered, map.freeBlock(x,y) ? skipped + 1 : skipped);


        //TODO kontrola nejlepsi ceny taky tady?
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

//    void placeHorizontal(ArrayMap &map, int tile, const int &x, const int &y, const int &id) {
//        tile--;
//        for (int i = x; i <= x + tile; i++)
//            map.setValue(i, y, id);
//    }
//
//    void placeVertical(ArrayMap &map, int tile, const int &x, const int &y, const int &id) {
//        tile--;
//        for (int i = y; i <= y + tile; i++)
//            map.setValue(x, i, id);
//    }

    void removeHorizontal(ArrayMap & map, int tile, const int & x, const int & y){
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

    void printMap(const ArrayMap &matrix, const int &cx, const int &cy, const int &price, const int &uncovered,
                  const int &skipped) const {
        cout << "X: " << cx << " Y: " << cy << " P: " << price << " B: " << best->price <<
             " U: " << uncovered << " S: " << skipped << endl;
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