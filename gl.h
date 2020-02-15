#pragma once

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "mat.h"

class GL;

inline bool depth_less(float o, float n) {
    return n > 0.f && n < 1.f && n < o;
}

inline bool depth_more(float o, float n) {
    return n > 0.f && n < 1.f && n > o;
}

void points_interpolator(GL &context, const std::vector<Vec3f> &screen_coords);

void line_interpolator(GL &context, const std::vector<Vec3f> &screen_coords);

void triangle_interpolator(GL &context, const std::vector<Vec3f> &screen_coords);

void default_interpolator(GL &context, const std::vector<Vec3f> &screen_coords);

struct IShader {
    virtual ~IShader() = default;

    virtual void set_mvp(Matrix mvp) {}

    virtual void set_model(Model *model) {}

    virtual Model *get_model() {
        return nullptr;
    }

    virtual Vec4f vertex(int iface, int nthvert) = 0;

    virtual bool fragment(Vec3f bar, TGAColor &color) = 0;
};

class GL {
public:
    enum RendererType {
        VERTEX, LINE, TRIANGLE, TRIANGLE_COLORED,
    };

    enum DepthTestType {
        LESS, GREATER,
    };

    explicit GL(TGAImage *target) : framebuffer(target) {
        zbuffer = std::vector<float>(
                static_cast<unsigned long>(framebuffer->get_width() * framebuffer->get_height()));
        glRenderer(TRIANGLE_COLORED);
        glDepthFunc(GREATER);
    }

    ~GL() = default;

    void glDraw();

    void glShader(IShader *shader) {
        this->shader = shader;
    }

    void glDepthFunc(DepthTestType func) {
        switch (func) {
            case LESS:
                depthTestFunc = depth_less;
                break;
            case GREATER:
                depthTestFunc = depth_more;
                break;
        }
        for (auto &p: zbuffer) {
            switch (func) {
                case LESS:
                    p = MAXFLOAT;
                    break;
                case GREATER:
                    p = -MAXFLOAT;
                    break;
            }
        }
    }

    void glViewport(int x, int y, int width, int height) {
        viewportMat = viewport(x, y, width, height);
    }

    void glRenderer(RendererType rendererType) {
        switch (rendererType) {
            case VERTEX:
                interpolator = points_interpolator;
                break;
            case LINE:
                interpolator = line_interpolator;
                break;
            case TRIANGLE:
                interpolator = triangle_interpolator;
                break;
            case TRIANGLE_COLORED:
                interpolator = default_interpolator;
                break;
        }
    }

    using DepthTestFunc = bool (*)(float, float);
    using Interpolator = void (*)(GL &, const std::vector<Vec3f> &);

    TGAImage *framebuffer;
    IShader *shader;
    std::vector<float> zbuffer;
    Matrix viewportMat;
    Interpolator interpolator;
    DepthTestFunc depthTestFunc;
};
