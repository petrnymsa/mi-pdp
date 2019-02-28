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

class Map{
public:
    int rows;
    int columns;
    int i1;
    int i2;
    int c1;
    int c2;
    int cn;
    vector<int> matrix;

    const static int FREE = 0;
    const static int BLOCK = 1;
    const static int TILE = 2;
    const static int START = 3;

    Map(int rows, int columns, int i1, int i2, int c1, int c2, int cn)
         :rows(rows), columns(columns), i1(i1), i2(i2), c1(c1), c2(c2), cn(cn), matrix(rows * columns)
    {

    }

    int value(int row, int column){
        return matrix[column + row * columns];
    }

    void setValue(int row, int column, int value){
        matrix[column + row * columns] = value;
    }
};

Map & loadInput(istream & is){
    int rows, columns, i1, i2, c1, c2, cn, k;

    is >> rows >> columns;
    is >> i1 >> i2 >> c1 >> c2 >> cn;
    is >> k;

    Map * map = new Map(rows, columns, i1,i2, c1,c2,cn);
    int x,y;
    for(int i = 0; i < k ; i++){
        is >> x >> y;
     //   map->matrix[y + x * columns] = Map::BLOCK;
     map->setValue(x,y,Map::BLOCK);
    }

    return *map;
}

void printMap(Map & map){
    int i = 0;
    for(const auto p : map.matrix){
        switch(p){
            case Map::FREE : cout << '.'; break;
            case Map::BLOCK : cout << 'X'; break;
            case Map::TILE : cout << 'O'; break;
            case Map::START : cout << 'S'; break;
            default:  cout << '.'; break;
        }

        i++;
        if(i == map.columns){
            i = 0;
            cout << endl;
        }

    }
}

pair<int,int> findStart(Map & map){
    for(int row = 0; row < map.rows; row++){
        for(int col = 0; col < map.columns; col++){
            if(map.value(row, col) == Map::FREE)
                return pair<int,int>(row, col);
        }
    }

    return pair<int,int>(-1,-1);

}

int main(int argc, char **argv) {
    Map * mapPtr;

    if(argc == 2){    //load from file
        ifstream ifile(argv[1], ios::in);
        if(ifile){
            mapPtr = &loadInput(ifile);
        }
        else{
            cout << "Problem with opening file. Exit." << endl;
            return -1;
        }
    }
    else{
        mapPtr = &loadInput(cin);
    }

    Map map = *mapPtr;

    pair<int,int> free = findStart(map);
    map.setValue(free.first, free.second, Map::START);

    printMap(map);

    delete mapPtr;
    return 0;
}