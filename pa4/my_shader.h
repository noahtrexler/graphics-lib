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

// include my_matrix?

class my_linear_gradient;

class my_shader : public GShader {
public:
    my_shader(const GBitmap& device, const GMatrix& matrix) : fDevice(device), fMatrix(matrix) {
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
            int x_ = std::max(GFloorToInt(inv_pt.fX), 0);
            x_ = std::min(GFloorToInt(x_), fDevice.width() - 1);
            int y_ = std::max(GFloorToInt(inv_pt.fY), 0);
            y_ = std::min(GFloorToInt(y_), fDevice.height() - 1);

            row[i] = *fDevice.getAddr(x_, y_);
        }
        
    }

private:
    const GBitmap fDevice;
    const GMatrix fMatrix;
    GMatrix fInverse;
};

class my_linear_gradient : public GShader {
public:

    my_linear_gradient(GPoint _p0, GPoint _p1, const GColor _c[], int _count) : colors_count(_count) {
        
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
            float p_x = clamp(p);
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

private:
    std::vector<GColor> colors;
    int colors_count;
    GMatrix fInverse;
    GMatrix fMatrix;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localM) {
    //if (bitmap == NULL || localM == NULL) return NULL;
    return std::unique_ptr<GShader>(new my_shader(bitmap, localM));
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
std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor c[], int count) {
    if (count < 1) return nullptr;
    return std::unique_ptr<GShader>(new my_linear_gradient(p0, p1, c, count));
}

#endif