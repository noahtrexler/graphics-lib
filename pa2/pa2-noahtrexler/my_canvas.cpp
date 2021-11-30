#include "include/GCanvas.h"
#include "include/GBitmap.h"
#include "include/GPixel.h"
#include "include/GRect.h"
#include "include/GPoint.h"

#include <iostream>
#include <vector>

#include "my_utils.h"
#include "my_edge.h"

class my_canvas : public GCanvas {
public:
    my_canvas(const GBitmap& device) : fDevice(device), width(device.width()), height(device.height()) {}

    /**
     *  Fill the entire canvas with the specified color, using SRC porter-duff mode.
     */
    void drawPaint(const GPaint& paint) override {
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
        GPixel src = MUcolorToPixel(paint.getColor());
        GRect clip = MUclip(width, height, rect);
        GIRect new_rect = clip.round();
        int L = new_rect.left();
        int R = new_rect.right();
        int T = new_rect.top();
        int B = new_rect.bottom();
        for (int y = T; y < B; ++y) {
            for (int x = L; x < R; ++x) {
                GPixel *p = fDevice.getAddr(x, y);
                *p = MUblend(src, *p, paint.getBlendMode());
            }
        }
    }

    void drawConvexPolygon(const GPoint* points, int count, const GPaint& paint) override {

        // build edges. sort. ray cast/draw.

        std::vector<my_edge> edges;

        for (int i = 0; i < count - 1; i++) { // clip points for each point pair except last
            MUclipPoints(points[i], points[i+1], width, height, edges);
        }
        MUclipPoints(points[count-1], points[0], width, height, edges);

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

            for (int x = e_L.get_X(y); x < e_R.get_X(y); ++x) {
                GPixel *p = fDevice.getAddr(x, y);
                *p = MUblend(src, *p, paint.getBlendMode());
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

    };

private:
    const GBitmap fDevice;
    const int width;
    const int height;
};

/**
 *  If the bitmap is valid for drawing into, this returns a subclass that can perform the
 *  drawing. If bitmap is invalid, this returns NULL.
 */
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& bitmap) {

    // need to check if valid?
    return std::unique_ptr<GCanvas>(new my_canvas(bitmap));
}

/**
 *  Implement this, drawing into the provided canvas, and returning the title of your artwork.
 */
std::string GDrawSomething(GCanvas* canvas, GISize dim) {

    GColor red; red.r = 255; red.b = 0; red.g = 0; red.a = 255;
    GPaint paint = GPaint(red);

    /* TEST 1: OUT OF BOUNDS TRIANGLE
    GPoint p0; p0.set(-10, 10);
    GPoint p1; p1.set(20, 20);
    GPoint p2; p2.set(0, 20);
    GPoint points[] = {p0, p1, p2};

    canvas->drawConvexPolygon(points, 3, paint);
    */

    /* TEST 2: OUT OF BOUNDS RECTANGLE
    GPoint p0; p0.set(10, -10);
    GPoint p1; p1.set(246, -10);
    GPoint p2; p2.set(246, 266);
    GPoint p3; p3.set(10, 266);
    GPoint points[] = {p0, p1, p2, p3};
    */

    /* TEST 3: OUT OF BOUNDS TRIANGLE RIGHT
    GPoint p0; p0.set(266, 0);
    GPoint p1; p1.set(246, 246);
    GPoint p2; p2.set(10, 10);
    GPoint points[] = {p0, p1, p2};
    */

    /* TEST 4: PENTAGON IN BOUNDS
    */

    GPoint p0; p0.set(128, 10);
    GPoint p1; p1.set(246, 128);
    GPoint p2; p2.set(226, 246);
    GPoint p3; p3.set(30, 246);
    GPoint p4; p4.set(10, 128);
    GPoint points[] = {p0, p1, p2, p3, p4};

    canvas->drawConvexPolygon(points, 5, paint);
    
    return "!";
}
