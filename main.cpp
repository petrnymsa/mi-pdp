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

    ArrayMap(int rows, int columns)
            : rows(rows), columns(columns), matrix(rows * columns) {

    }

    int getValue(int x, int y) {
        return matrix[x + y * columns];
    }

    void setValue(int x, int y, int value) {
        matrix[x + y * columns] = value;

    }

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


    static MapInfo *load(istream &is) {
        int rows, columns, i1, i2, c1, c2, cn, k;

        is >> rows >> columns;
        is >> i1 >> i2 >> c1 >> c2 >> cn;
        is >> k;

        MapInfo *mapInfo = new MapInfo(rows, columns, i1, i2, c1, c2, cn);

        int x, y;
        for (int i = 0; i < k; i++) {
            is >> x >> y;
            mapInfo->map.setValue(x, y, MapInfo::BLOCK);
        }

        return mapInfo;
    }

//    static void setMapValue(vector<int> &matrix, int columns, int x, int y, int value) {
//        matrix[x + y * columns] = value;
//    }
//
//    static int getMapValue(vector<int> & matrix, int columns, int x, int y){
//        return matrix[x + y * columns];
//    }
};

void printMap(ArrayMap &matrix) {
    for (int y = 0; y < matrix.rows; y++) {
        for (int x = 0; x < matrix.columns; x++) {
            int p = matrix.getValue(x, y);
            switch (p) {
                case MapInfo::FREE :
                    cout << '.';
                    break;
                case MapInfo::BLOCK :
                    cout << 'X';
                    break;
                case MapInfo::START :
                    cout << 'S';
                    break;
                default:
                    cout << p;
                    break;
            }
        }
        cout << endl;
    }
}


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

    printMap(mapInfo->map);
    cout << mapInfo->map.getValue(2,0) << endl;
    delete mapInfo;
    return 0;
}