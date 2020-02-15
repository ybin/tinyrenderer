#include <cmath>
#include <limits>
#include <cstdlib>
#include "gl.h"

namespace {
    Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
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

    float linear_interpolate_z(const std::vector<Vec3f> &screen_coords, const Vec3f &c) {
        float z = 0.f;
        for (int i = 0; i < 3; ++i) {
            z += c[i] * screen_coords[i].z;
        }
        return z;
    }

    // https://zhuanlan.zhihu.com/p/50141767
    float perspective_interpolate_z(const std::vector<Vec3f> &screen_coords, Vec3f &c) {
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
    Vec3f barycentric2(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
        float u, v;
        u = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) /
            ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        v = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) /
            ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        return {u, v, 1 - u - v};
    }

    void triangle(GL &ctx, const std::vector<Vec3f> &screen_coords, bool colored) {
        auto MAX = std::numeric_limits<float>::max();
        float l, t, r, b;
        l = b = MAX;
        t = r = -MAX;
        for (auto &pt : screen_coords) {
            l = std::min(l, pt.x);
            r = std::max(r, pt.x);
            t = std::max(t, pt.y);
            b = std::min(b, pt.y);
        }

        Vec2i P;
        float z;
        int index;
        TGAColor color;
        const TGAColor white = {255, 255, 255, 255};
        for (P.x = static_cast<int>(l); P.x <= r; P.x++) {
            for (P.y = static_cast<int>(b); P.y <= t; P.y++) {
                Vec3f c = barycentric2(proj<2>(screen_coords[0]),
                                       proj<2>(screen_coords[1]),
                                       proj<2>(screen_coords[2]),
                                       P);
                z = perspective_interpolate_z(screen_coords, c);
                index = P.x + P.y * ctx.framebuffer->get_width();
                if (c.x < 0 || c.y < 0 || c.z < 0 || !ctx.depthTestFunc(ctx.zbuffer[index], z)) continue;
                bool discard = ctx.shader->fragment(c, color);
                if (!discard) {
                    ctx.zbuffer[P.x + P.y * ctx.framebuffer->get_width()] = z;
                    ctx.framebuffer->set(P.x, P.y, colored ? color : white);
                }
            }
        }
    }
}

void GL::glDraw() {
    std::vector<Vec3f> screen_coords(3);
    Vec4f v;
    Model *model = shader->get_model();
    for (int i = 0; i < model->nfaces(); i++) {
        screen_coords.clear();
        for (int j = 0; j < 3; j++) {
            v = shader->vertex(i, j);
            v = v / v[3];
            v = viewportMat * v;
            screen_coords.emplace_back(proj<3>(v));
        }
        interpolator(*this, screen_coords);
    }
}

void line_interpolator(GL &context, const std::vector<Vec3f> &screen_coords) {
    const TGAColor white(255, 255, 255);
    const int len = 100;
    const float step = 1.f / len;
    int k;
    float t;
    Vec2i v0, v1;
    for (size_t i = 0, j = 0; i < screen_coords.size(); ++i) {
        j = (i + 1) % screen_coords.size();
        v0 = proj<2>(screen_coords[i]);
        v1 = proj<2>(screen_coords[j]);
        k = 0;
        t = 0.f;
        while (k++ < len) {
            context.framebuffer->set(static_cast<int>(v0.x + (v1.x - v0.x) * t),
                                     static_cast<int>(v0.y + (v1.y - v0.y) * t),
                                     white);
            t += step;
        }
    }
}

void points_interpolator(GL &context, const std::vector<Vec3f> &screen_coords) {
    const TGAColor white(255, 255, 255);
    for (auto &pt : screen_coords) {
        context.framebuffer->set(static_cast<int>(pt[0]), static_cast<int>(pt[1]), white);
    }
}

void triangle_interpolator(GL &context, const std::vector<Vec3f> &screen_coords) {
    triangle(context, screen_coords, false);
}

void default_interpolator(GL &context, const std::vector<Vec3f> &screen_coords) {
    triangle(context, screen_coords, true);
}
