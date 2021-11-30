#ifndef my_shader_DEFINED
#define my_shader_DEFINED

#include "include/GShader.h"
#include "include/GPixel.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"
#include "include/GPoint.h"
#include "include/GPath.h"

#include "my_utils.h"

#include <iostream>
#include <vector>
#include <math.h>

// include my_matrix?

class my_linear_gradient;

class my_shader : public GShader {
public:
    my_shader(const GBitmap& device, const GMatrix& matrix, GShader::TileMode _tm) : fDevice(device), fMatrix(matrix), tm(_tm) {
        fInverse = GMatrix();
    }
        
    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return fDevice.isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        return (ctm * fMatrix).invert(&fInverse);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {

        for (int i = 0; i < count; i++) {
            GPoint canvas_pt; 
            canvas_pt.set(x + 0.5 + i, y + 0.5);
            GPoint inv_pt = fInverse * canvas_pt;

            // clamp
            int x_, y_;
            switch (tm) {
                case TileMode::kRepeat : {
                    x_ = repeat(inv_pt.fX, fDevice.width());
                    y_ = repeat(inv_pt.fY, fDevice.height());
                    break;
                }
                case TileMode::kMirror : {
                    x_ = clamp((float) mirror(inv_pt.fX, fDevice.width()), fDevice.width());
                    y_ = clamp((float) mirror(inv_pt.fY, fDevice.height()), fDevice.height());
                    break;
                }
                default : {
                    x_ = clamp(inv_pt.fX, fDevice.width());
                    y_ = clamp(inv_pt.fY, fDevice.height());
                    break;
                }
            }

            row[i] = *fDevice.getAddr(x_, y_);
        }
        
    }

    int clamp(float x, int bounds) {
        int x_ = std::max(GFloorToInt(x), 0);
        x_ = std::min(GFloorToInt(x_), bounds - 1);
        return x_;
    }

    int repeat(float x, int bounds) {
        while (x < 0) {
            x += bounds;
        }
        while (x >= bounds) {
            x -= bounds;
        }
        return GFloorToInt(x);
    }

    int mirror(float x, int bounds) {
        if (x < 0) x *= -1;
        float r = x / bounds;
        int floor = GFloorToInt(r);
        int x_ = GFloorToInt(x);
        if (floor % 2 == 0) {
            return x_ % bounds;
        } else {
            return bounds - (x_ % bounds);
        }
    }

private:
    const GBitmap fDevice;
    const GMatrix fMatrix;
    GMatrix fInverse;
    GShader::TileMode tm;
};

class my_linear_gradient : public GShader {
public:

    my_linear_gradient(GPoint _p0, GPoint _p1, const GColor _c[], int _count, GShader::TileMode _tm) : colors_count(_count), tm(_tm) {
        
        for (int i = 0; i < _count; i++) {
            colors.push_back(_c[i]);
        }

        fInverse = GMatrix();

        float dx = _p1.x() - _p0.x();
        float dy = _p1.y() - _p0.y();
        fMatrix = GMatrix(dx, -dy, _p0.x(), dy, dx, _p0.y());
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        for (int i = 0; i < colors_count; i++) {
            if (colors.at(i).a != 1.0) return false;
        }
        return true;
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& ctm) override {
        return (ctm * fMatrix).invert(&fInverse);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {

        for (int i = 0; i < count; i++) {
            GPoint pt; 
            pt.set(x + 0.5 + i, y + 0.5);
            GPoint p = fInverse * pt;
            GColor c;

            // clamp
            float p_x;
            switch (tm) {
                case TileMode::kMirror : {
                    p_x = mirror(p);
                    break;
                }
                case TileMode::kRepeat : {
                    p_x = repeat(p);
                    break;
                }
                default : {
                    p_x = clamp(p);
                    break;
                }
            }
            // scale
            float x_ = p_x * (colors_count-1);
            // declare index
            int index = GFloorToInt(x_);
            // declare w
            float w = x_ - index;
            // calculate c
            if (w == 0) {
                assert(index <= colors_count - 1);
                c = colors.at(index);
            } else {
                c = (1 - w) * colors.at(index) + w * colors.at(index + 1);
            }
            
            row[i] = MUcolorToPixel(c);
        }
        
    }

    float clamp(GPoint p) {
        float x = p.x();
        if (p.x() < 0.0) x = 0;
        if (p.x() > 1.0) x = 1;
        return x;
    }

    float repeat(GPoint p) {
        float x = p.x();
        while (x < 0) {
            x += 1.0;
        }
        while (x > 1.0) {
            x -= 1.0;
        }
        return x;
    }

    float mirror(GPoint p) { 
        float x = p.x(); // 4.5
        float r = x - GFloorToInt(x); // .5
        if (GFloorToInt(x) % 2 == 0) {
            return 1 - r;
        } else {
            return r;
        }
    }

private:
    std::vector<GColor> colors;
    int colors_count;
    GMatrix fInverse;
    GMatrix fMatrix;
    GShader::TileMode tm;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localM, GShader::TileMode tm) {
    //if (bitmap == NULL || localM == NULL) return NULL;
    return std::unique_ptr<GShader>(new my_shader(bitmap, localM, tm));
}


/**
 *  Return a subclass of GShader that draws the specified gradient of [count] colors between
 *  the two points. Color[0] corresponds to p0, and Color[count-1] corresponds to p1, and all
 *  intermediate colors are evenly spaced between.
 *
 *  The gradient colors are GColors, and therefore unpremul. The output colors (in shadeRow)
 *  are GPixel, and therefore premul. The gradient has to interpolate between pairs of GColors
 *  before "pre" multiplying them into GPixels.
 *
 *  If count < 1, this should return nullptr.
 */
std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor c[], int count, GShader::TileMode tm) {
    if (count < 1) return nullptr;
    return std::unique_ptr<GShader>(new my_linear_gradient(p0, p1, c, count, tm));
}

#endif