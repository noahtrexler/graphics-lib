#include "include/GCanvas.h"
#include "include/GBitmap.h"
#include "include/GPixel.h"
#include "include/GRect.h"
#include "include/GPoint.h"
#include "include/GColor.h"
#include "include/GPaint.h"
#include "include/GMatrix.h"
#include "include/GShader.h"
#include "include/GPath.h"

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
        GRect r = GRect::MakeXYWH(0, 0, fDevice.width(), fDevice.height());
        drawRect(r, paint);
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

    void blit(int x0, int x1, int y, const GPaint& paint) {
        GShader* shader = paint.getShader();

        if (shader != nullptr) {
            int width = x1 - x0;
            assert(x0 >= 0 && width >= 0); // ensure row initialized correctly
            GPixel row[width];
            shader->shadeRow(x0, y, width, row);
            
            for (int x = x0; x < x1; x++) {
                GPixel *p = fDevice.getAddr(x, y);
                GPixel s = row[x - x0];
                *p = MUblend(s, *p, paint.getBlendMode());
            }

        } else {
            GPixel src = MUcolorToPixel(paint.getColor());
            for (int x = x0; x < x1; ++x) {
                GPixel *p = fDevice.getAddr(x, y);
                *p = MUblend(src, *p, paint.getBlendMode());
            }
        }

    }

    /**
     *  Fill the convex polygon with the color and blendmode,
     *  following the same "containment" rule as rectangles.
     */
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

        for (int y = min_y; y < max_y; ++y) {

            my_edge e_L = edges.at(L);
            my_edge e_R = edges.at(R);

            // blit
            blit(e_L.get_X(y), e_R.get_X(y), y, paint);     

            // is next edge valid

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

    void complex_scan(std::vector<my_edge> edges, const GPaint& paint) {
        assert(edges.size() > 0);

        int x0, x1;
        // loop through all y’s containing edges
        int y = edges.at(0).top;
        while (edges.size() > 0) {
            int index = 0;
            int w = 0;

            // loop through active edges
            while (index < edges.size() && edges.at(index).top <= y) {                
                // check w (for left) → did we go from 0 to non-0
                if (w == 0) {
                    x0 = edges.at(index).get_X(y);
                }

                // update w → w += edge[index].winding
                w += edges.at(index).winding;

                // check w (for right and blit) → did we go from non-0 to 0
                if (w == 0) {
                    x1 = edges.at(index).get_X(y);
                    blit(x0, x1, y, paint);
                }

                // if the edge is done, remove from array
                // else update currX by slope (and ++index)
                if (edges.at(index).valid(y+1) == false) {
                    edges.erase(edges.begin() + index);
                } else {
                    edges.at(index).curr_x += edges.at(index).m;
                    ++index;
                }
            }

            y++;
            
            // move index to include edges that will be valid
            while (index < edges.size() && y == edges.at(index).top) {
                index += 1;
            }

            // sort edge[] from [0...index) based solely on currX
            MUsortInX(edges, index);
        }
    }

    /**
     *  Fill the path with the paint, interpreting the path using winding-fill (non-zero winding).
     */
    void drawPath(const GPath& path, const GPaint& paint) override {

        GPath p = path;

        GShader* shader = paint.getShader();
        if (shader != nullptr) {
            if (!(shader->setContext(ctm))) return;
        }

        p.transform(ctm);

        std::vector<my_edge> edges;
        GPoint next_pts[2];
        GPath::Edger edger = GPath::Edger(p);

        while (edger.next(next_pts) != GPath::Verb::kDone) {
            MUclipPoints(next_pts[0], next_pts[1], width, height, edges);
        }

        if (edges.size() == 0) return;

        MUsortEdges(edges);

        complex_scan(edges, paint);     
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

    canvas->clear({.5, .8, .9, 1});
    
    GMatrix scale = GMatrix::Scale(100, 100);
    
    GPath path;

    float da = M_PI / 8;
    float angle = M_PI / 4;
    path.moveTo({ cosf(angle), sinf(angle) });
    for (int i = 1; i < 100; ++i) {
        GPoint p = { cosf(angle), sinf(angle) };
        path.lineTo(p);
        angle += da;
    }

    path.transform(scale);
    canvas->translate(60, 60);

    GRect r = GRect::MakeXYWH(0, 0, 100, 100);
    const GColor colors[] = {
        {1,0,0,1}, {.5,0,.5,1},
    };
    auto sh = GCreateLinearGradient({r.fLeft, r.fTop}, {r.fRight, r.fBottom},
                                    colors, GARRAY_COUNT(colors));
    GPaint paint(sh.get());

    canvas->drawPath(path, paint);

    return "red sun in the corner";
}