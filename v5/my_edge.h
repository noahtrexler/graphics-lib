
#ifndef my_edge_DEFINED
#define my_edge_DEFINED

#include "include/GPoint.h"
#include "include/GMath.h"

struct my_edge {

    // x = m*y + b
    // b = x0
    // m = (x1-x0 / y1-y0)
    // y = (x-b)/m

    float m, b;
    float curr_x;
    int top, bottom;
    int winding;

    void set(float _m, float _b, int _top, int _bottom) {
        m = _m;
        b = _b;
        top = _top;
        bottom = _bottom;
    }

    bool set(GPoint p0, GPoint p1) {
        if (p0.fY > p1.fY) {
            std::swap(p0, p1);
            winding *= -1;
        }

        top = GRoundToInt(p0.fY);
        bottom = GRoundToInt(p1.fY);
        if (top == bottom) return false;

        m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
        b = p0.fX - (m * p0.fY);

        curr_x = p0.fX + m * (top - p0.fY + 0.5);
        return true;
    }

    bool set(GPoint p0, GPoint p1, int w) {
        if (set(p0, p1)) {
            winding = w;
            return true;
        }
        return false;
    }

    int get_X(int y) {
        return GRoundToInt(m*(y+0.5) + b);
    }

    bool valid(int y) {
        if (y > top && y < bottom) {
            return true;
        }
        return false;
    }
    
};

#endif

 
