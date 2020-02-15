#pragma once

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"

struct IShader {
    virtual ~IShader() = default;

    virtual void set_mvp(Matrix mvp) {}

    virtual void set_model(Model *model) {}

    virtual Vec4f vertex(int iface, int nthvert) = 0;

    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

typedef bool (*DepthTestFunc)(float o, float n);

inline bool depth_less(float o, float n) {
    return n > 0.f && n < 1.f && n < o;
}

inline bool depth_more(float o, float n) {
    return n > 0.f && n < 1.f && n > o;
}

void triangle(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer,
        bool colored = true, DepthTestFunc depthTest = depth_more);

/*
void triangle(mat<4, 3, float> &pts, IShader &shader, TGAImage &image, float *zbuffer, bool colored = true);
*/
