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

typedef void (*Interpolator)(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer);

void triangle(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer, bool colored = true);

/*
void triangle(mat<4, 3, float> &pts, IShader &shader, TGAImage &image, float *zbuffer, bool colored = true);
*/
