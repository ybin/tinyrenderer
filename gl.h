#pragma once

#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include "mat.h"

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
    enum Renderer {
        VERTEX, LINE, TRIANGLE, TRIANGLE_COLORED,
    };

    enum DepthFunc {
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

    void glDepthFunc(DepthFunc func) {
        this->depthFunc = func;
        for (auto &p: zbuffer) {
            switch (depthFunc) {
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

    void glRenderer(Renderer renderer) {
        this->renderer = renderer;
        switch (this->renderer) {
            case VERTEX:
                interpolator = &GL::points_interpolator;
                break;
            case LINE:
                interpolator = &GL::line_interpolator;
                break;
            case TRIANGLE:
                interpolator = &GL::triangle_interpolator;
                break;
            case TRIANGLE_COLORED:
                interpolator = &GL::default_interpolator;
                break;
        }
    }

private:
    using DepthTestFunc = bool(float, float);
    using Interpolator = void (GL::*)(const std::vector<Vec3f> &);

    static bool depth_less(float o, float n) {
        return n > 0.f && n < 1.f && n < o;
    }

    static bool depth_more(float o, float n) {
        return n > 0.f && n < 1.f && n > o;
    }

    static Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
        Vec3f s[2];
        for (int i = 2; i--;) {
            s[i][0] = C[i] - A[i];
            s[i][1] = B[i] - A[i];
            s[i][2] = A[i] - P[i];
        }
        Vec3f u = cross(s[0], s[1]);
        if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
            return {1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z};
        return {-1, 1, 1}; // in this case generate negative coordinates, it will be thrown away by the rasterizator
    }

    static float linear_interpolate_z(const std::vector<Vec3f> &screen_coords, const Vec3f &c) {
        float z = 0.f;
        for (int i = 0; i < 3; ++i) {
            z += c[i] * screen_coords[i].z;
        }
        return z;
    }

    // https://zhuanlan.zhihu.com/p/50141767
    static float perspective_interpolate_z(const std::vector<Vec3f> &screen_coords, Vec3f &c) {
        float z = 0.f;
        for (int i = 0; i < 3; ++i) {
            z += c[i] / screen_coords[i].z;
        }
        z = 1 / z;

        for (int i = 0; i < 3; ++i) {
            c[i] *= z / screen_coords[i].z;
        }

        return z;
    }

    // https://codeplea.com/triangular-interpolation
    static Vec3f barycentric2(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
        float u, v;
        u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) /
            ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) /
            ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        return {u, v, 1 - u - v};
    }

    static void triangle(const std::vector<Vec3f> &screen_coords, IShader *shader, TGAImage *image, float *zbuffer,
                         bool colored, DepthTestFunc depthTest);

    void points_interpolator(const std::vector<Vec3f> &screen_coords);

    void line_interpolator(const std::vector<Vec3f> &screen_coords);

    void triangle_interpolator(const std::vector<Vec3f> &screen_coords);

    void default_interpolator(const std::vector<Vec3f> &screen_coords);

    TGAImage *framebuffer;
    IShader *shader;
    DepthFunc depthFunc;
    std::vector<float> zbuffer;
    Matrix viewportMat;
    Renderer renderer = TRIANGLE_COLORED;
    Interpolator interpolator;
};
