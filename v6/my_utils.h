#ifndef my_utils_defined
#define my_utils_defined

#include "include/GColor.h"
#include "include/GPixel.h"
#include "include/GMath.h"
#include "include/GBlendMode.h"

#include <iostream>
#include <math.h>

#include "my_edge.h"

// PA1

static inline char MUfloatToByte(float f) {
    int c = floor(f * 255 + 0.5);
    return (char) c;
}

static inline int MUfloatTo255(float f) {
    int i = floor(f * 255 + 0.5);
    return i;
}

static inline unsigned MUquickDivide255(unsigned value) {
    return (value + 128) * 257 >> 16;
}

static inline GPixel MUcolorToPixel(GColor c) {
    GColor nc = c.pinToUnit();
    nc.r *= nc.a;
    nc.g *= nc.a;
    nc.b *= nc.a;
    return GPixel_PackARGB(MUfloatTo255(nc.a), MUfloatTo255(nc.r), MUfloatTo255(nc.g), MUfloatTo255(nc.b));
}

static inline GPixel MUsrcOver(GPixel src, GPixel dest) {
    //!<     S + (1 - Sa)*D
    unsigned r, g, b, a;
    a = GPixel_GetA(src) + MUquickDivide255(GPixel_GetA(dest) * (255 - GPixel_GetA(src)));
    r = GPixel_GetR(src) + MUquickDivide255(GPixel_GetR(dest) * (255 - GPixel_GetA(src)));
    g = GPixel_GetG(src) + MUquickDivide255(GPixel_GetG(dest) * (255 - GPixel_GetA(src)));
    b = GPixel_GetB(src) + MUquickDivide255(GPixel_GetB(dest) * (255 - GPixel_GetA(src)));
    
    return GPixel_PackARGB(a, r, g, b);

}

static inline GRect MUclip (int w, int h, const GRect& r2) {
    
    GRect r1 = GRect::MakeXYWH(0, 0, w, h); // pass in the canvas dimensions

    if (!r1.intersect(r2)) {
        return GRect::MakeXYWH(0,0,0,0);
    } else {
        return r1; // intersect() should set to intersection
    }

}

static inline void MUprintPixel(GPixel p) {
    std::cout << "R:" << GPixel_GetR(p);
    std::cout << " G:" << GPixel_GetG(p);
    std::cout << " B:" << GPixel_GetB(p);
    std::cout << " A:" << GPixel_GetA(p) << std::endl;
}

static inline void MUprintPixelFromAddr(GPixel* p) {
    std::cout << "Pixel memory address: " << p << std::endl;
    std::cout << *p << std::endl;
}

// PA2

static inline void MUprintPoint(GPoint p) {
    std::cout << "(" << p.fX << ", " << p.fY << ")" << std::endl;
}

static inline void MUprintEdge(my_edge e) {
    std::cout << "edge x = " << e.m << " * y + " << e.b << ". top = " << e.top << ". bottom = " << e.bottom << std::endl;
}

static inline void MUprintEdges(std::vector<my_edge> edges) {
    for (my_edge e : edges) {
        MUprintEdge(e); 
    }
}

static inline GPixel MUsrcIn(GPixel src, GPixel dest) {
    //!<     Da * S
    unsigned r, g, b, a;
    a = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetA(src));
    r = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetR(src));
    g = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetG(src));
    b = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetB(src));
    
    return GPixel_PackARGB(a, r, g, b);

}

static inline GPixel MUsrcOut(GPixel src, GPixel dest) {
    //!<     (1 - Da)*S
    unsigned r, g, b, a;
    a = MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetA(src));
    r = MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetR(src));
    g = MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetG(src));
    b = MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetB(src));
    
    return GPixel_PackARGB(a, r, g, b);

}

static inline GPixel MUsrcATop(GPixel src, GPixel dest) {
    //!<     Da*S + (1 - Sa)*D
    unsigned r, g, b, a;
    a = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetA(src)) + MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetA(dest));
    r = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetR(src)) + MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetR(dest));
    g = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetG(src)) + MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetG(dest));
    b = MUquickDivide255(GPixel_GetA(dest) * GPixel_GetB(src)) + MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetB(dest));
    
    return GPixel_PackARGB(a, r, g, b);

}

static inline GPixel MUxor(GPixel src, GPixel dest) {
    //!<     (1 - Sa)*D + (1 - Da)*S
    // if doesn't work try adding two MUsrcOut
    unsigned r, g, b, a;
    a = MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetA(dest)) + MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetA(src));
    r = MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetR(dest)) + MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetR(src));
    g = MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetG(dest)) + MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetG(src));
    b = MUquickDivide255((255 - GPixel_GetA(src)) * GPixel_GetB(dest)) + MUquickDivide255((255 - GPixel_GetA(dest)) * GPixel_GetB(src));
    
    return GPixel_PackARGB(a, r, g, b);

}

static inline GPixel MUblend(GPixel src, GPixel dest, GBlendMode mode) {
    switch (mode) {
        case GBlendMode::kClear: return GPixel_PackARGB(0,0,0,0);       //!<     0
        case GBlendMode::kSrc: return src;                              //!<     S
        case GBlendMode::kDst: return dest;                             //!<     D
        case GBlendMode::kSrcOver: return MUsrcOver(src, dest);         //!<     S + (1 - Sa)*D
        case GBlendMode::kDstOver: return MUsrcOver(dest, src);         //!<     D + (1 - Da)*S
        case GBlendMode::kSrcIn: return MUsrcIn(src, dest);             //!<     Da * S
        case GBlendMode::kDstIn: return MUsrcIn(dest, src);             //!<     Sa * D
        case GBlendMode::kSrcOut: return MUsrcOut(src, dest);           //!<     (1 - Da)*S
        case GBlendMode::kDstOut: return MUsrcOut(dest, src);           //!<     (1 - Sa)*D
        case GBlendMode::kSrcATop: return MUsrcATop(src, dest);         //!<     Da*S + (1 - Sa)*D
        case GBlendMode::kDstATop: return MUsrcATop(dest, src);         //!<     Sa*D + (1 - Da)*S
        case GBlendMode::kXor: return MUxor(src, dest);                 //!<     (1 - Sa)*D + (1 - Da)*S
        default: return GPixel_PackARGB(0,0,0,0);
    }
}

static inline float MUhorizontalIntersect(float y, GPoint p0, GPoint p1) {
    // x = my + b

    float m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
    float b = p0.fX - (m * p0.fY);
    std::cout << "x = " << m << "*" << y << "+" << b << std::endl;
    std::cout << "= " << m * y + b << std::endl;
    return m * y + b;
}

static inline float MUverticalIntersect(float x, GPoint p0, GPoint p1) {
    // y = (x-b)/m

    float m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
    float b = p0.fX - (m * p0.fY);
    std::cout << "y = " << x << "-" << b << "/" << m << std::endl;
    std::cout << "= " << (x - b) / m << std::endl;
    return (x - b) / m;
}

static inline void MUmakeEdge(GPoint p0, GPoint p1, std::vector<my_edge>& edges) {
    my_edge e; 
    if (e.set(p0, p1)) {
        edges.push_back(e);
    }
}

static inline bool MULT (my_edge a, my_edge b) {
    if (a.top < b.top) return true;
    if (b.top < a.top) return false;
    if (a.curr_x < b.curr_x) return true;
    if (b.curr_x < a.curr_x) return false;
    return a.m < b.m;
}

static inline void MUsortEdges(std::vector<my_edge>& edges) {
    std::sort(edges.begin(), edges.end(), MULT);
}

// PA3

static inline void MUprintMatrix(GMatrix matrix) {
    std::cout << " [ " << matrix[0] << " " << matrix[1] << " " << matrix[2] << " ] " << std::endl;
    std::cout << " [ " << matrix[3] << " " << matrix[4] << " " << matrix[5] << " ] " << std::endl;
    std::cout << " [ 0 0 1 ] " << std::endl;
}

static inline void MUprintMatrix(float* matrix) {
    std::cout << " [ ";
    for (int i = 0; i < 9; i++) {
        std::cout << matrix[i] << " ";
        if (i == 2 || i == 5) std::cout << " ]\n [ ";
    }
    std::cout << " ] " << std::endl;
}

static inline void MUprintMatrix6(const float* matrix) {
    std::cout << " [ ";
    for (int i = 0; i < 6; i++) {
        std::cout << matrix[i] << " ";
        if (i == 2) std::cout << " ]\n [ ";
    }
    std::cout << " ] " << std::endl;
    std::cout << " [ 0 0 1 ] " << std::endl;
}

static inline float* MUtranspose(float* matrix) {
    // 3x3 matrix
    float *t_matrix = new float[9];

    t_matrix[0] = matrix[0];
    t_matrix[1] = matrix[3];
    t_matrix[2] = matrix[6];

    t_matrix[3] = matrix[1];
    t_matrix[4] = matrix[4];
    t_matrix[5] = matrix[7];

    t_matrix[6] = matrix[2];
    t_matrix[7] = matrix[5];
    t_matrix[8] = matrix[8];

    return t_matrix;
}

static inline void MUprintBlendMode(GBlendMode mode) {

    switch (mode) {
        case GBlendMode::kClear: std::cout << "kClear" << std::endl; break;             //!<     0
        case GBlendMode::kSrc: std::cout << "kSrc" << std::endl; break;                 //!<     S
        case GBlendMode::kDst: std::cout << "kDst" << std::endl; break;                 //!<     D
        case GBlendMode::kSrcOver: std::cout << "kSrcOver" << std::endl; break;         //!<     S + (1 - Sa)*D
        case GBlendMode::kDstOver: std::cout << "kDstOver" << std::endl; break;         //!<     D + (1 - Da)*S
        case GBlendMode::kSrcIn: std::cout << "kSrcIn" << std::endl; break;             //!<     Da * S
        case GBlendMode::kDstIn: std::cout << "kDstIn" << std::endl; break;             //!<     Sa * D
        case GBlendMode::kSrcOut: std::cout << "kSrcOut" << std::endl; break;           //!<     (1 - Da)*S
        case GBlendMode::kDstOut: std::cout << "kDstOut" << std::endl; break;           //!<     (1 - Sa)*D
        case GBlendMode::kSrcATop: std::cout << "kSrcATop" << std::endl; break;         //!<     Da*S + (1 - Sa)*D
        case GBlendMode::kDstATop: std::cout << "kDstATop" << std::endl; break;         //!<     Sa*D + (1 - Da)*S
        case GBlendMode::kXor: std::cout << "kXor" << std::endl; break;                 //!<     (1 - Sa)*D + (1 - Da)*S
        default: std::cout << "none" << std::endl; break;
    }

}

// PA4

static inline void MUmakeEdgeWinding(GPoint p0, GPoint p1, std::vector<my_edge>& edges, int w) {
    my_edge e; 
    if (e.set(p0, p1, w)) {
        edges.push_back(e);
    }
}

static inline void MUclipPoints(GPoint p0, GPoint p1, int w, int h, std::vector<my_edge>& edges) {

    int winding = 1;

    if (p0.fY > p1.fY) {
        std::swap(p0, p1); // reassign so that p0 is always on top
        winding *= -1;
    }
    
    GPoint *pTop = &p0;
    GPoint *pBottom = &p1;

    GPoint *pLeft;
    GPoint *pRight;

    if (GRoundToInt(p0.fY) == GRoundToInt(p1.fY)) return;

    if (p0.fX >= p1.fX) {
        pRight = &p0;
        pLeft = &p1;
        winding *= -1;
    } else if (p0.fX < p1.fX) {
        pLeft = &p0;
        pRight = &p1;
        winding *= -1;
    }

    if (pBottom->fY <= 0) return; // p1 is above implies p0 is above
    if (pTop->fY >= h) return; // p0 is below implies p1 is below

    float m = (p1.fX - p0.fX) / (p1.fY - p0.fY);
    float b = p0.fX - (m * p0.fY);

    // TOP
    if (pTop->fY < 0) {
        pTop->set(b, 0);
    }

    // BOTTOM
    if (pBottom->fY > h) {
        pBottom->set(((m * h) + b), h);
    }

    // LEFT
    if (pLeft->fX < 0) {
        if (pRight->fX < 0) {
            pLeft->set(0, pLeft->fY);
            pRight->set(0, pRight->fY);
        }
        else {
            GPoint pProjection;
        
            pProjection.set(0, pLeft->fY);
            pLeft->set(0, ((0 - b)/m));
            MUmakeEdgeWinding(*pLeft, pProjection, edges, winding);
        }
    }

    // RIGHT
    if (pRight->fX > w) {
        if (pLeft->fX > w) {
            pLeft->set(w, pLeft->fY);
            pRight->set(w, pRight->fY);
        } else {
            GPoint pProjection;
        
            pProjection.set(w, pRight->fY);
            pRight->set(w, ((w - b)/m));
            MUmakeEdgeWinding(*pRight, pProjection, edges, winding);
        }
    }

    MUmakeEdgeWinding(*pLeft, *pRight, edges, winding);
}

static inline bool MUSinX (my_edge a, my_edge b) {
    if (a.curr_x < b.curr_x) return true;
    if (b.curr_x < a.curr_x) return false;
    return a.m < b.m;
}

static inline void MUsortInX(std::vector<my_edge>& edges, int x) {
    std::sort(edges.begin(), edges.begin() + x, MUSinX);
}

// PA5 

static inline int MUcomputeQuadSegments(GPoint pts[3]) {
    GPoint e = (-1 * pts[0] + 2 * pts[1] - pts[2]) * .25;
    float magnitude = sqrt(e.fX * e.fX + e.fY * e.fY);
    return ceil(sqrt(4 * magnitude));
}

static inline int MUcomputeCubicSegments(GPoint pts[4]) {
    GPoint p = -1 * pts[0] + 2 * pts[1] - pts[2];
    GPoint q = -1 * pts[1] + 2 * pts[2] - pts[3];
    GPoint e = {std::max(std::abs(p.fX), std::abs(q.fX)), std::max(std::abs(p.fY), std::abs(q.fY))};
    float magnitude = sqrt(e.fX * e.fX + e.fY * e.fY);
    return ceil(sqrt(3 * magnitude));
}

static inline GPoint MUevalQuad(GPoint src[3], float t) {
    return (1 - t) * (1 - t) * src[0] + 2 * t * (1 - t) * src[1] + t * t * src[2];
}

static inline GPoint MUevalCubic(GPoint src[4], float t) {
    return (1 - t) * (1 - t) * (1 - t) * src[0] + 3 * t * (1 - t) * (1 - t) * src[1] + 3 * t * t * (1 - t) * src[2] + t * t * t * src[3];
}

// PA6

static inline GPixel MUmultiplyPixels(GPixel p0, GPixel p1) {
    // 1/255(255 * 255)
    unsigned r,g,b,a;
    r = MUquickDivide255(GPixel_GetR(p0) * GPixel_GetR(p1));
    g = MUquickDivide255(GPixel_GetG(p0) * GPixel_GetG(p1));
    b = MUquickDivide255(GPixel_GetB(p0) * GPixel_GetB(p1));
    a = MUquickDivide255(GPixel_GetA(p0) * GPixel_GetA(p1));
    return GPixel_PackARGB(a,r,g,b);
}

static inline GColor MUbilerpColor(const GColor c[4], float u, float v) {
    return (1.f - u) * (1.f - v) * c[0]
         + u         * (1.f - v) * c[1]
         + u         * v         * c[2]
         + v         * (1.f - u) * c[3];
}

static inline GPoint MUbilerpPoint(const GPoint p[4], float u, float v) {
    return (1.f - u) * (1.f - v) * p[0]
         + u         * (1.f - v) * p[1]
         + u         * v         * p[2]
         + v         * (1.f - u) * p[3];    
}

#endif
 
