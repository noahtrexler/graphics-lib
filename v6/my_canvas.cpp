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

    // PA5

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
        GPoint next_pts[4];
        GPath::Edger edger = GPath::Edger(p);
        GPath::Verb v = edger.next(next_pts);

        while (v != GPath::Verb::kDone) {
            switch (v) {
                case GPath::Verb::kLine : {
                    MUclipPoints(next_pts[0], next_pts[1], width, height, edges);
                    break;
                }
                case GPath::Verb::kCubic : {
                    int k = MUcomputeCubicSegments(next_pts);
                    GPoint p0 = next_pts[0];
                    for (int i = 1; i < k; ++i) {
                        float t = (float) i / k;
                        GPoint p1 = MUevalCubic(next_pts, t);
                        MUclipPoints(p0, p1, width, height, edges);
                        p0 = p1;
                    }
                    MUclipPoints(p0, next_pts[3], width, height, edges);
                    break;
                }
                case GPath::Verb::kQuad : {
                    int k = MUcomputeQuadSegments(next_pts);
                    GPoint p0 = next_pts[0];
                    for (int i = 1; i < k; ++i) {
                        float t = (float) i / k;
                        GPoint p1 = MUevalQuad(next_pts, t);
                        MUclipPoints(p0, p1, width, height, edges);
                        p0 = p1;
                    }
                    MUclipPoints(p0, next_pts[2], width, height, edges);
                    break;
                }
                default : break;
            }
            v = edger.next(next_pts);
        }

        if (edges.size() == 0) return;

        MUsortEdges(edges);

        complex_scan(edges, paint);     
    }

    // PA6

    void drawTriangle(const GPoint points[3], const GColor colors[], const GPoint texs[], const GPaint& paint) {
        if (colors != nullptr && texs != nullptr) {
            GPaint _paint(new my_composite_shader(new my_tri_color_shader(points, colors), paint.getShader()));
            drawConvexPolygon(points, 3, _paint);
        } else if (colors != nullptr) {
            GPaint _paint(new my_tri_color_shader(points, colors));
            drawConvexPolygon(points, 3, _paint);
        } else {
            drawConvexPolygon(points, 3, paint);
        }
    }

    void drawTriangleWithTex(const GPoint points[3], const GColor colors[3], const GPoint texs[3], GShader* originalShader) {
        // https://docs.google.com/document/d/1prUc-SAs7FFw7MlH78UkLN6EJegwcESa-3Bcf4nXGs0/edit#
        GMatrix P, T, invT;
        P = GMatrix(points[1].x() - points[0].x(), points[2].x() - points[0].x(), points[0].x(),
                    points[1].y() - points[0].y(), points[2].y() - points[0].y(), points[0].y());
        T = GMatrix(texs[1].x() - texs[0].x(), texs[2].x() - texs[0].x(), texs[0].x(),
                    texs[1].y() - texs[0].y(), texs[2].y() - texs[0].y(), texs[0].y());

        if (T.invert(&invT) == false) {
            return;
        }

        my_proxy_shader proxy = my_proxy_shader(originalShader, P * invT);
        GPaint p(&proxy);

        drawTriangle(points, colors, texs, p);
    }

    /**
     *  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex.
     *
     *  The triangles are specified by successive triples of indices.
     *      int n = 0;
     *      for (i = 0; i < count; ++i) {
     *          point0 = vertx[indices[n+0]]
     *          point1 = verts[indices[n+1]]
     *          point2 = verts[indices[n+2]]
     *          ...
     *          n += 3
     *      }
     *
     *  If colors is not null, then each vertex has an associated color, to be interpolated
     *  across the triangle. The colors are referenced in the same way as the verts.
     *          color0 = colors[indices[n+0]]
     *          color1 = colors[indices[n+1]]
     *          color2 = colors[indices[n+2]]
     *
     *  If texs is not null, then each vertex has an associated texture coordinate, to be used
     *  to specify a coordinate in the paint's shader's space. If there is no shader on the
     *  paint, then texs[] should be ignored. It is referenced in the same way as verts and colors.
     *          texs0 = texs[indices[n+0]]
     *          texs1 = texs[indices[n+1]]
     *          texs2 = texs[indices[n+2]]
     *
     *  If both colors and texs[] are specified, then at each pixel their values are multiplied
     *  together, component by component.
     */
    // GPoint verts[], GColor colors[], GPoint texs[], int count, int indices[], GPaint& paint
    void drawMesh(const GPoint verts[], const GColor _colors[], const GPoint _texs[], int count, const int indices[], const GPaint& paint) {
        
        int n = 0;

        for (int i = 0; i < count; ++i) {

            const GPoint points[3] = {verts[indices[n+0]], verts[indices[n+1]], verts[indices[n+2]]};

            if (_colors != nullptr && _texs != nullptr) {

                const GColor colors[3] = {_colors[indices[n+0]], _colors[indices[n+1]], _colors[indices[n+2]]};
                const GPoint texs[3] = {_texs[indices[n+0]], _texs[indices[n+1]], _texs[indices[n+2]]};   
                drawTriangleWithTex(points, colors, texs, paint.getShader());

            } else if (_colors != nullptr) {

                const GColor colors[3] = {_colors[indices[n+0]], _colors[indices[n+1]], _colors[indices[n+2]]};
                drawTriangle(points, colors, nullptr, paint);

            } else if (_texs != nullptr) {
            
                const GPoint texs[3] = {_texs[indices[n+0]], _texs[indices[n+1]], _texs[indices[n+2]]};  
                drawTriangleWithTex(points, nullptr, texs, paint.getShader());
            
            } else {
                drawTriangle(points, nullptr, nullptr, paint);
            }
            n += 3;
        }
    }

    /**
     *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
     *  the quad based on "level":
     *      level == 0 --> 1 quad  -->  2 triangles
     *      level == 1 --> 4 quads -->  8 triangles
     *      level == 2 --> 9 quads --> 18 triangles
     *      ...
     *  The 4 corners of the quad are specified in this order:
     *      top-left --> top-right --> bottom-right --> bottom-left
     *  Each quad is triangulated on the diagonal top-right --> bottom-left
     *      0---1
     *      |  /|
     *      | / |
     *      |/  |
     *      3---2
     *
     *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
     */
    // GPoint verts[4], GColor colors[4], GPoint texs[4], int level, const GPaint&
    void drawQuad(const GPoint verts[4], const GColor _colors[4], const GPoint _texs[4], int level, const GPaint& paint) {
        
        int nQuads = (int) pow(level+1, 2);
        int nTris = nQuads * 2;
        int nCorners = (int) pow(level+2, 2);
        GPoint corner[nCorners];
        GColor colors[nCorners];
        GPoint texs[nCorners];
        
        // point assignment
        int i = 0;
        for (int y = 0; y <= level + 1; y++) {
            float v = (float) y / (level + 1);
            for (int x = 0; x <= level + 1; x++) {
                float u = (float) x / (level + 1);
                corner[i] = MUbilerpPoint(verts, u, v);
                if (_colors != nullptr) {
                    colors[i] = MUbilerpColor(_colors, u, v);
                }
                if (_texs != nullptr) {
                    texs[i] = MUbilerpPoint(_texs, u, v);
                }
                i++;
            }
        }

        // level 0 {0,1,2, 1,2,3}
        // level 1 {0,1,3, 1,3,4, 1,2,4, 2,4,5, 3,4,6, 4,6,7, 4,5,7, 5,7,8}
        // level 2 {0,1,4, 1,4,5, 1,2,5, 2,5,6, 2,3,6, 3,6,7 ...}

        int indices[6 * nQuads];
        int j = 0, k = 0, l = 0;

        // iterate through each row of indices
        for (; j <= level; j++) {

            // assign top left triangle of row
            indices[k] = l;
            indices[k+1] = l + 1;
            indices[k+2] = l + level + 2;

            l += 1; k += 3;

            // assign inner triangles of row
            for (int m = 0; m < level; m++) {
                indices[k] = l;
                indices[k+1] = l + level + 2;
                indices[k+2] = l + level + 1;

                indices[k+3] = l;
                indices[k+4] = l + 1;
                indices[k+5] = l + level + 2;

                l += 1; k += 6;
            }

            // assign bottom right triangle of row
            indices[k] = l;
            indices[k+1] = l + level + 2;
            indices[k+2] = l + level + 1;
            
            l += 1; k += 3;
        }
        
        if (_texs == nullptr && _colors == nullptr) {
            drawMesh(corner, nullptr, nullptr, nTris, indices, paint);
        } else if (_texs == nullptr) {
            drawMesh(corner, colors, nullptr, nTris, indices, paint);
        } else if (_colors == nullptr) {
            drawMesh(corner, nullptr, texs, nTris, indices, paint);
        } else {
            drawMesh(corner, colors, texs, nTris, indices, paint);
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

    const GPoint pts[] = {
        { 20, 20 }, { 240, 20 }, { 20, 140 }, { 140, 260 }
    };
    const GColor clr[] = {
        { 1, .75, .8, 1 }, { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 }
    };

    canvas->drawQuad(pts, clr, nullptr, 3, GPaint());

    return "folded sheet gradient";
}