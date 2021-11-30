#ifndef my_shader_DEFINED
#define my_shader_DEFINED

#include "include/GShader.h"
#include "include/GPixel.h"
#include "include/GMatrix.h"
#include "include/GBitmap.h"

#include <iostream>

// include my_matrix?

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

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bitmap, const GMatrix& localM) {
    //if (bitmap == NULL || localM == NULL) return NULL;
    return std::unique_ptr<GShader>(new my_shader(bitmap, localM));
}

#endif