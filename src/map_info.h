#include <set>
#include <iostream>

using namespace std;

#ifndef MI_PDP_MAP_INFO_H
#define MI_PDP_MAP_INFO_H

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

#endif //MI_PDP_MAP_INFO_H
