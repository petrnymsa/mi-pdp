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

    ArrayMap(const ArrayMap &copy, int fromX, int toX, int fromY, int toY, int id) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->matrix = copy.matrix;

        for (int x = fromX; x <= toX; x++) {
            for (int y = fromY; y <= toY; y++) {
                setValue(x, y, id);
            }
        }
    }

    bool freeBlock(const int &x, const int &y) const{
        return getValue(x,y) == 0;
    }

    int getValue(const int &x, const int &y) const {
        assert(x < columns);
        assert(y < rows);
        return matrix[x + y * columns];
    }

    void setValue(const int &x, const int &y, const int value) {
        assert(x < columns);
        assert(y < rows);
        matrix[x + y * columns] = value;
    }



    pair<int, int> nextCoordinates(const int &x, const int &y) const {
        if (x + 1 < columns)
            return make_pair(x + 1, y);
        else if (y + 1 < rows)
            return make_pair(0, y + 1);

        return make_pair(-1, -1);
    }

    pair<int,int> prevCoordinates(const int &x, const int &y) const{
        if(x - 1 >= 0)
            return make_pair(x-1,y);
        else
            return make_pair(columns-1,y-1);
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
        solveInternal(info.map, start.first, start.second, 0, 0, info.startUncovered,0, 1, TILE_SIZE_NULL, true);

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
    const int TILE_SIZE_NULL = 0;

    void solveInternal(ArrayMap map, int x, int y, int i1_cnt, int i2_cnt, int uncovered, int skipped, int nextTileId, int tileSize,bool horizontal) {

        if (tileSize != TILE_SIZE_NULL) {  // place new
            uncovered-= tileSize;
            if(tileSize == info.i1)
                i1_cnt++;
            else
                i2_cnt++;

            pair<int,int> tilePosition = map.prevCoordinates(x,y);
            if(horizontal)
                placeHorizontal(map, tileSize, tilePosition.first, tilePosition.second, nextTileId);
            else
                placeVertical(map, tileSize, tilePosition.first, tilePosition.second, nextTileId);

            nextTileId++;
        }

        int upperPrice = info.computeUpperPrice(uncovered);
        int currentPrice = info.computePrice(i1_cnt, i2_cnt, skipped);

        printMap(map, x,y, currentPrice, uncovered, skipped, tileSize, horizontal);
        //TODO this shit
//        if(currentPrice - info.cn*uncovered + info.cn*skipped + upperPrice <= best->price) {
//            cout << "CUT" << endl << endl;
//            return;
//        }
        //TODO kontrola na zacatku pred pokladanim?
        if (map.isOnRightBottomCorner(x, y)) {
            if (currentPrice > best->price) {
                best->map = map;
                best->price = currentPrice;
            }
            return;
        }

        pair<int, int> next = map.nextCoordinates(x, y);

        //place H I1
        bool canPlace = tryPlaceHorizontal(map, info.i1, x, y);
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered, !canPlace && map.freeBlock(x,y) ? skipped + 1 : skipped, nextTileId, canPlace ? info.i1 : TILE_SIZE_NULL, true);

        //place V I1
        canPlace = tryPlaceVertical(map, info.i1, x, y);
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,!canPlace && map.freeBlock(x,y) ? skipped + 1 : skipped, nextTileId, canPlace ? info.i1 : TILE_SIZE_NULL, false);

        //place H I2
        canPlace = tryPlaceHorizontal(map, info.i2, x, y);
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,!canPlace && map.freeBlock(x,y) ? skipped + 1 : skipped, nextTileId, canPlace ? info.i2 : TILE_SIZE_NULL, true);

        //place V I2
        canPlace = tryPlaceVertical(map, info.i2, x, y);
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,!canPlace && map.freeBlock(x,y) ? skipped + 1 : skipped, nextTileId, canPlace ? info.i2 : TILE_SIZE_NULL, false);
        //TODO pokud se zvysi skipped, nesnizi se uncovered (to je ok?, je skipped podmnozina?)
        //TODO navic skipped se pocita pravd. spatne
       //skip current
        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered, map.freeBlock(x,y) ? skipped + 1 : skipped, nextTileId, TILE_SIZE_NULL, true);

        //TODO kontrola nejlepsi ceny taky tady?
        if (currentPrice > best->price) {
            best->map = map;
            best->price = currentPrice;
        }
    }

    bool tryPlaceHorizontal(const ArrayMap &map, int tile, const int &x, const int &y) const {
        tile--;
        if (x + tile >= map.columns)
            return false;

        for (int i = x; i <= x + tile; i++) {
            if (map.getValue(i, y) != MapInfo::FREE)
                return false;
        }
        return true;
    }

    bool tryPlaceVertical(const ArrayMap &map, int tile, const int &x, const int &y) const {
        tile--;
        if (y + tile >= map.rows)
            return false;

        for (int i = y; i <= y + tile; i++) {
            if (map.getValue(x, i) != MapInfo::FREE)
                return false;
        }
        return true;
    }

    void placeHorizontal(ArrayMap &map, int tile, const int &x, const int &y, const int &id) {
        tile--;
        for (int i = x; i <= x + tile; i++)
            map.setValue(i, y, id);
    }

    void placeVertical(ArrayMap &map, int tile, const int &x, const int &y, const int &id) {
        tile--;
        for (int i = y; i <= y + tile; i++)
            map.setValue(x, i, id);
    }

    void printMap(const ArrayMap &matrix, const int & cx, const int & cy, const int & price,const int & uncovered,
            const int & skipped, const int & tileSize, const bool & horizontal) const {
        cout << "X: " << cx << " Y: " << cy << " P: " << price << " B: " << best->price <<
                " U: " << uncovered << " S: " << skipped << " T: " << tileSize << " H: " << horizontal << endl;
        cout << matrix;
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