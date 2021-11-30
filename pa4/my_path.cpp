#include "include/GPath.h"
#include "include/GPoint.h"
#include "include/GRect.h"
#include "include/GMatrix.h"
#include "include/GShader.h"

#include <iostream>

/**
 *  Append a new contour, made up of the 4 points of the specified rect, in the specified
 *  direction. The contour will begin at the top-left corner of the rect.
 */
GPath& GPath::addRect(const GRect& r, Direction dir) {

    float left = r.left();
    float right = r.right();
    float top = r.top();
    float bottom = r.bottom();

    if (dir == Direction::kCW_Direction) {
        return moveTo(left, top).lineTo(right, top).lineTo(right, bottom).lineTo(left, bottom);
    } else {
        return moveTo(left, top).lineTo(left, bottom).lineTo(right, bottom).lineTo(right, top);
    }
}

/**
 *  Append a new contour with the specified polygon. Calling this is equivalent to calling
 *  moveTo(pts[0]), lineTo(pts[1..count-1]).
 */
GPath& GPath::addPolygon(const GPoint pts[], int count) {
    GPath& p = moveTo(pts[0]);
    for (int i = 1; i < count; i++) {
        p.lineTo(pts[i]);
    }
    return p;
}

/**
 *  Return the bounds of all of the control-points in the path.
 *
 *  If there are no points, return {0, 0, 0, 0}
 */
GRect GPath::bounds() const {

    if (countPoints() == 0) return GRect::MakeLTRB(0,0,0,0);
    if (countPoints() == 1) return GRect::MakeXYWH(fPts.at(0).x(), fPts.at(0).y(),0,0);
    if (countPoints() == 2) {
        float x1 = fPts.at(0).x(); float x2 = fPts.at(1).x();
        float y1 = fPts.at(0).y(); float y2 = fPts.at(1).y();
        float l = std::min(x1,x2);
        float t = std::min(y1,y2);
        float r = std::max(x1,x2);
        float b = std::max(y1,y2);
        return GRect::MakeLTRB(l,t,r,b);
    }
    
    float l = 999999.0;
    float t = 999999.0;
    float r = 0.0;
    float b = 0.0;

    for (int i = 0; i < countPoints(); i++) {
        float x = fPts.at(i).x();
        float y = fPts.at(i).y();
        if (x < l) {
            l = x;
        }
        else if (x > r) {
            r = x;
        }

        if (y < t) {
            t = y;
        }
        else if (y > b) {
            b = y;
        }        
    }

    return GRect::MakeLTRB(l,t,r,b);
}

/**
 *  Transform the path in-place by the specified matrix.
 */
void GPath::transform(const GMatrix& m) {
    for (int i = 0; i < countPoints(); i++) {
        fPts.at(i) = m*fPts.at(i);
    }
}