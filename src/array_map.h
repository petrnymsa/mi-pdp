#include <iostream>
#include <set>

#ifndef MI_PDP_ARRAY_MAP_H
#define MI_PDP_ARRAY_MAP_H

#define BLOCK_FREE 0
#define BLOCK_BAN -1

using namespace std;

class ArrayMap {
public:
    int rows, columns;
    int nextId;
    int x, y;

    ArrayMap() : matrix(nullptr) {
        // cout << "DEFAULT CONSTRUCTOR ARRAY_MAP " << endl;
    }

    ArrayMap(const int rows, const int columns, const set<pair<int,int>> & banned)
            : rows(rows), columns(columns), nextId(1), x(0), y(0) {
        int n = rows * columns;
        this->matrix = new int[n];
        for (int i = 0; i < n; i++)
            matrix[i] = BLOCK_FREE;

        for(auto & ban : banned)
            setValue(ban.first, ban.second, BLOCK_BAN);
    }

    void writeCopy(const ArrayMap &copy) {
        this->rows = copy.rows;
        this->columns = copy.columns;
        this->nextId = copy.nextId;
        this->x = copy.x;
        this->y = copy.y;

        unsigned int n = rows * columns;
        this->matrix = new int[n];
        for (unsigned int i = 0; i < n; i++)
            this->matrix[i] = copy.matrix[i];
    }

    ArrayMap(const ArrayMap &copy) {
        writeCopy(copy);
    }

    //move constructor
//    ArrayMap(ArrayMap &&copy)
//            : rows(copy.rows), columns(copy.columns), nextId(copy.nextId), x(copy.x), y(copy.y), matrix(copy.matrix) {
//
//        copy.matrix = nullptr;
//        cout << "MOVE constructor" << endl;
//    }

    ArrayMap &operator=(const ArrayMap &copy) {
        if (this == &copy)
            return *this;
        delete[] matrix;

        writeCopy(copy);
        return *this;
    }

//    //move operator
//    ArrayMap &operator=(ArrayMap &&copy) {
//        if (this == &copy)
//            return *this;
//        delete[] matrix;
//
//        cout << "MOVE =" << endl;
//
//        this->rows = copy.rows;
//        this->columns = copy.columns;
//        this->nextId = copy.nextId;
//        this->x = copy.x;
//        this->y = copy.y;
//        this->matrix = copy.matrix;
//
//        copy.matrix = nullptr;
//        return *this;
//    }

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

    bool canPlaceHorizontal(const int &tile) {
        if (x + tile - 1 >= columns)
            return false;

        for (int i = x; i < x + tile; i++) {
            if (getValue(i, y) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    bool canPlaceVertical(const int &tile) {
        if (y + tile - 1 >= rows)
            return false;

        for (int i = y; i < y + tile; i++) {
            if (getValue(x, i) != BLOCK_FREE)
                return false;
        }
        return true;
    }

    ArrayMap placeHorizontal(const int &tile) {
        ArrayMap map = *this;
        for (int i = x; i < x + tile; i++)
            map.setValue(i, y, nextId);
        map.nextId++;
        map.nextFree();
        return map;
    }

    ArrayMap placeVertical(const int &tile) {
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



#endif //MI_PDP_ARRAY_MAP_H
