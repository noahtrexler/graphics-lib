#include "include/GCanvas.h"
#include "include/GBitmap.h"
#include "include/GPixel.h"
#include "include/GRect.h"
#include "include/GPoint.h"
#include "include/GColor.h"
#include "include/GPaint.h"
#include "include/GMatrix.h"
#include "include/GShader.h"

#include <iostream>
#include <vector>
#include <stack>

#include "my_utils.h"
#include "my_edge.h"
#include "my_shader.h"

class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device), width(device.width()), height(device.height()) {
        ctm = GMatrix();
        save();
    }

    /**
     *  Save off a copy of the canvas state (CTM), to be later used if the balancing call to
     *  restore() is made. Calls to save/restore can be nested:
     *  save();
     *      save();
     *          concat(...);    // this modifies the CTM
     *          .. draw         // these are drawn with the modified CTM
     *      restore();          // now the CTM is as it was when the 2nd save() call was made
     *      ..
     *  restore();              // now the CTM is as it was when the 1st save() call was made
     */
    void save() override {
        saves.push(ctm);
    }

    /**
     *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
     *  the canvas. It is an error to call restore() if there has been no previous call to save().
     */
    void restore() override {
        ctm = saves.top();
        saves.pop();
    }

    /**
     *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
     *  is constructed with an identity CTM.
     *
     *  CTM' = CTM * matrix
     */
    void concat(const GMatrix& m) override {
        ctm = ctm * m;
    }

    /**
     *  Fill the entire canvas with the specified color, using SRC porter-duff mode.
     */
    void drawPaint(const GPaint& paint) override {

        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!(shader->setContext(ctm))) return;
        }

        GPixel new_pixel = MUcolorToPixel(paint.getColor());
        for (int y = 0; y < fDevice.height(); ++y) {
            for (int x = 0; x < fDevice.width(); ++x) {
                GPixel* p = fDevice.getAddr(x, y); // p stores ADDRESS in memory
                *p = new_pixel;
            }
        }
    }
    
    /**
     *  Fill the rectangle with the color, using SRC_OVER porter-duff mode.
     *
     *  The affected pixels are those whose centers are "contained" inside the rectangle:
     *      e.g. contained == center > min_edge && center <= max_edge
     *
     *  Any area in the rectangle that is outside of the bounds of the canvas is ignored.
     */
    void drawRect(const GRect& rect, const GPaint& paint) override {
        GPoint pts[4];
        pts[0] = {rect.fLeft, rect.fTop};
        pts[1] = {rect.fRight, rect.fTop};
        pts[2] = {rect.fRight, rect.fBottom};
        pts[3] = {rect.fLeft, rect.fBottom};
        drawConvexPolygon(pts, 4, paint);
    }

    void drawConvexPolygon(const GPoint* points, int count, const GPaint& paint) override {

        // build edges. sort. ray cast/draw.

        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!(shader->setContext(ctm))) return;
        }

        GPoint matrix_pts[count];
        ctm.mapPoints(matrix_pts, points, count); // map points

        std::vector<my_edge> edges;

        for (int i = 0; i < count - 1; i++) { // clip points for each point pair except last
            MUclipPoints(matrix_pts[i], matrix_pts[i+1], width, height, edges);
        }
        MUclipPoints(matrix_pts[count-1], matrix_pts[0], width, height, edges);

        if (edges.size() == 0) return;

        MUsortEdges(edges);

        // ray cast here (blit)
        int min_y = edges.front().top;
        int max_y = edges.back().bottom;
        
        int L = 0;
        int R = 1;
        int next_edge = 2;

        GPixel src = MUcolorToPixel(paint.getColor());

        for (int y = min_y; y < max_y; ++y) {

            my_edge e_L = edges.at(L);
            my_edge e_R = edges.at(R);

            // blit

            if (shader != nullptr) {

                int L = e_L.get_X(y); 
                int R = e_R.get_X(y);
                int width = R-L;
                assert(L >= 0 && width >= 0); // ensure row initialized correctly
                GPixel row[width];
                shader->shadeRow(L, y, width, row);
                
                for (int x = L; x < R; x++) {
                    GPixel *p = fDevice.getAddr(x, y);
                    GPixel s = row[x - L];
                    *p = MUblend(s, *p, paint.getBlendMode());
                }

            } else {
                for (int x = e_L.get_X(y); x < e_R.get_X(y); ++x) {
                    GPixel *p = fDevice.getAddr(x, y);
                    *p = MUblend(src, *p, paint.getBlendMode());
                }
            }         

            // validate next edge is valid

            if (e_L.valid(y+1) == false) {
                L = next_edge;
                next_edge++;
                if (L >= edges.size()) return;
            }

            if (e_R.valid(y+1) == false) {
                R = next_edge;
                next_edge++;
                if (R >= edges.size()) return;
            }
        }     
        
    }

private:
    const GBitmap fDevice;
    const int width;
    const int height;
    GMatrix ctm;
    std::stack<GMatrix> saves;
};

/**
 *  If the bitmap is valid for drawing into, this returns a subclass that can perform the
 *  drawing. If bitmap is invalid, this returns NULL.
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& bitmap) {
    return std::unique_ptr<GCanvas>(new my_canvas(bitmap));
}

/**
 *  Implement this, drawing into the provided canvas, and returning the title of your artwork.
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim) {

    GBitmap bitmap;
    bitmap.readFromFile("apps/spock.png");

    //float cx = canvas->width();
    //float cy = canvas->height();
    float dx = bitmap.width();
    float dy = bitmap.height();
    GPoint pts[] = {
        { 0, 0 }, { dx, 0 }, { dx, dy }, { 0, dy },
    };

    auto shader = GCreateBitmapShader(bitmap, GMatrix());
    GPaint paint(shader.get());

    for (int i = 0; i < 10; ++i) {
        canvas->save();
        canvas->translate(i*30, i*30);
        canvas->scale(0.25, 0.25);
        canvas->rotate(i * M_PI/12);
        canvas->drawConvexPolygon(pts, 4, paint);
        canvas->restore();
    }

    return "live long and prosperrrrrrrrrrr";
}
 
