/*
 *  Copyright 2016 Mike Reed
 */

#ifndef GPaint_DEFINED
#define GPaint_DEFINED

#include "GBlendMode.h"
#include "GColor.h"

class GPaint {
public:
    GPaint() : fColor(GColor::RGB(0, 0, 0)) {}
    GPaint(const GColor& c) : fColor(c) {}

    const GColor& getColor() const { return fColor; }
    GPaint& setColor(GColor c) { fColor = c; return *this; }
    GPaint& setRGBA(float r, float g, float b, float a) {
        return this->setColor(GColor::RGBA(r, g, b, a));
    }

    float   getAlpha() const { return fColor.a; }
    GPaint& setAlpha(float alpha) {
        fColor.a = alpha;
        return *this;
    }

    GBlendMode getBlendMode() const { return fMode; }
    GPaint&    setBlendMode(GBlendMode m) { fMode = m; return *this; }

private:
    GColor      fColor;
    GBlendMode  fMode = GBlendMode::kSrcOver;
};

#endif
