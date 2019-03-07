#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

/*
 * m >= n >= 10 = rozměry obdélníkové mřížky R[1..m,1..n], skládající se z m x n políček.
3 < i1 < n = počet políček tvořících dlaždice tvaru písmene I a délky i1 (= tvaru I1).
3 < i2 < n = počet políček tvořících dlaždice tvaru písmene I a délky i2 (= tvaru I2), i1 < i2 .
3 < c1 < n = cena bezkolizního umístění dlaždice tvaru I1.
3 < c2 < n = cena bezkolizního umístění dlaždice tvaru I2, c1 < c2.
cn < 0 = cena (penalizace) políčka nepokrytého žádnou dlaždicí.
k < 0.3*m*n = počet zakázaných políček v mřížce R.
D[1..k] = pole souřadnic k zakázaných políček rozmístěných v mřížce R.
 *
 *
 * 1. řádka:celá čísla m a n = rozměry obdélníkové mřížky (počet řádků a sloupců)
další řádka: (pouze pro zadání POI) celá čísla i1, i2, c1, c2, cn= viz zadání úlohy POI
další řádka: číslo k = počet zakázaných políček v mřížce
následujících k řádek obsahuje x,y souřadnice zakázaných políček
 */
using namespace std;

class ArrayMap {
public:
    int rows, columns;

    ArrayMap(const int rows, const int columns)
            : rows(rows), columns(columns), matrix(rows * columns) {

    }

    int getValue(const int &x, const int &y) const {
        return matrix[x + y * rows];
    }

    void setValue(const int &x, const int &y, const int value) {
        matrix[x + y * rows] = value;
    }

    friend ostream & operator << (ostream & os, const ArrayMap & map);

private:
    vector<int> matrix;
};

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

class SolverResult {
public:
    vector<pair<int, int>> empty;
    int price;
    ArrayMap map;

    SolverResult(ArrayMap &map)
            : price(INT32_MIN), map(map) {

    }
    friend ostream & operator << (ostream &out, const SolverResult &result);

};

ostream & operator << (ostream & os, const SolverResult & result){
    os << result.map;
    os << result.price << endl;
    os << result.empty.size() << endl;
    for(const auto & p : result.empty)
        os << p.first << " " << p.second << endl;

    return os;
}

ostream & operator << (ostream & os, const ArrayMap & map){
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

class Solver {
public:
    Solver(MapInfo &mapInfo)
            : mapInfo(mapInfo) {

    }

    SolverResult & solve() {
        solverResult = new SolverResult(mapInfo.map);
        pair<int, int> start = mapInfo.findStart();
        solveInternal(mapInfo.map, start.first, start.second, 0, 0, mapInfo.startUncovered, 1);

        return *solverResult;
    }

    ~Solver() {
        delete solverResult;
    }



private:
    MapInfo &mapInfo;


    SolverResult *solverResult;

    void solveInternal(ArrayMap map, int x, int y, int i1_cnt, int i2_cnt, int uncovered, int nextTileId) {
        //TODO staci zde pocitat nejlepsi ?
        //TODO pokladat v ramci volani rekurze nebo pred? viz priklad
        int currentPrice = mapInfo.computePrice(i1_cnt, i2_cnt, uncovered);
        if (currentPrice > solverResult->price) {
            solverResult->map = map;
            solverResult->price = currentPrice;
        }

        // place H I1
        if (tryPlaceHorizontal(map, mapInfo.i1, x, y,nextTileId)) {
            i1_cnt++;
            uncovered -= mapInfo.i1;
        }
        pair<int, int> next = nextCoordinates(x, y);
        if (next.first == -1) // on the right down corner
            return;

        printMap(map, x, y);

        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered, nextTileId);

        // place V I1
        if (tryPlaceVertical(map, mapInfo.i1, x, y,nextTileId)) {
            i1_cnt++;
            uncovered -= mapInfo.i1;
        }
        next = nextCoordinates(x, y);

        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,nextTileId);
        printMap(map, x, y);

        // place H I2
        if (tryPlaceHorizontal(map, mapInfo.i2, x, y, nextTileId)) {
            i2_cnt++;
            uncovered -= mapInfo.i2;
        }
        next = nextCoordinates(x, y);
        printMap(map, x, y);

        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,nextTileId);

        // place V I2
        if (tryPlaceVertical(map, mapInfo.i2, x, y,nextTileId)) {
            i2_cnt++;
            uncovered -= mapInfo.i2;
        } else { //add current x,y to blacklist if it is empty
            if (map.getValue(x, y) == MapInfo::FREE)
                solverResult->empty.emplace_back(x, y);
        }
        next = nextCoordinates(x, y);
        printMap(map, x, y);

        solveInternal(map, next.first, next.second, i1_cnt, i2_cnt, uncovered,nextTileId);
    }

    pair<int, int> nextCoordinates(const int &x, const int &y) const {
        if (x + 1 < mapInfo.columns)
            return make_pair(x + 1, y);
        else if (y + 1 < mapInfo.rows)
            return make_pair(0, y + 1);

        return make_pair(-1, -1);
    }

    bool tryPlaceHorizontal(ArrayMap &map, int tile, const int &x, const int &y, int & nextTileId) {
        tile--;
        if (x + tile >= map.columns)
            return false;

        for (int i = x; i <= x + tile; i++) {
            if (map.getValue(i, y) != MapInfo::FREE)
                return false;
        }

        for (int i = x; i <= x + tile; i++)
            map.setValue(i, y, nextTileId);

        nextTileId++;

        return true;
    }

    bool tryPlaceVertical(ArrayMap &map, int tile, const int &x, const int &y, int & nextTileId) {
        tile--;
        if (y + tile >= map.rows)
            return false;

        for (int i = y; i <= y + tile; i++) {
            if (map.getValue(x, i) != MapInfo::FREE)
                return false;
        }

        for (int i = y; i <= y + tile; i++)
            map.setValue(x, i, nextTileId);

        nextTileId++;

        return true;
    }

    void printMap(ArrayMap &matrix, int cx, int cy) {
        cout << "CX: " << cx << " CY: " << cy << endl;
        cout << matrix << endl;
//        for (int y = 0; y < matrix.rows; y++) {
//            for (int x = 0; x < matrix.columns; x++) {
////                if (cx == x && cy == y) {
////                    cout << '*';
////                    continue;
////                }
//
//                int p = matrix.getValue(x, y);
//
//                switch (p) {
//                    case MapInfo::FREE :
//                        cout << '.';
//                        break;
//                    case MapInfo::BLOCK :
//                        cout << 'X';
//                        break;
//                    case MapInfo::START :
//                        cout << 'S';
//                        break;
//                    default:
//                        cout << p;
//                        break;
//                }
//            }
//            cout << endl;
//        }

      //  cout << endl;
    }

};



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

    Solver solver(*mapInfo);

    SolverResult & result = solver.solve();
    cout << result;
    delete mapInfo;
    return 0;
}