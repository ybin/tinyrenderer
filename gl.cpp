#include <cmath>
#include <limits>
#include <cstdlib>
#include "gl.h"

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
        (this->*interpolator)(screen_coords);
    }
}

void GL::triangle(const std::vector<Vec3f> &screen_coords, IShader *shader, TGAImage *image,
                  float *zbuffer, bool colored, GL::DepthTestFunc depthTest) {
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
    TGAColor color;
    const TGAColor white = {255, 255, 255, 255};
    for (P.x = static_cast<int>(l); P.x <= r; P.x++) {
        for (P.y = static_cast<int>(b); P.y <= t; P.y++) {
            Vec3f c = barycentric2(proj<2>(screen_coords[0]),
                                   proj<2>(screen_coords[1]),
                                   proj<2>(screen_coords[2]),
                                   P);
            float z = perspective_interpolate_z(screen_coords, c);
            if (c.x < 0 || c.y < 0 || c.z < 0 || !depthTest(zbuffer[P.x + P.y * image->get_width()], z)) continue;
            bool discard = shader->fragment(c, color);
            if (!discard) {
                zbuffer[P.x + P.y * image->get_width()] = z;
                image->set(P.x, P.y, colored ? color : white);
            }
        }
    }
}

void GL::line_interpolator(const std::vector<Vec3f> &screen_coords) {
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
            framebuffer->set(static_cast<int>(v0.x + (v1.x - v0.x) * t),
                             static_cast<int>(v0.y + (v1.y - v0.y) * t),
                             white);
            t += step;
        }
    }
}

void
GL::points_interpolator(const std::vector<Vec3f> &screen_coords) {
    const TGAColor white(255, 255, 255);
    for (auto &pt : screen_coords) {
        framebuffer->set(static_cast<int>(pt[0]), static_cast<int>(pt[1]), white);
    }
}

void
GL::triangle_interpolator(const std::vector<Vec3f> &screen_coords) {
    switch (depthFunc) {
        case LESS:
            triangle(screen_coords, shader, framebuffer, zbuffer.data(), false, depth_less);
            break;
        case GREATER:
            triangle(screen_coords, shader, framebuffer, zbuffer.data(), false, depth_more);
            break;
    }
}

void GL::default_interpolator(const std::vector<Vec3f> &screen_coords) {
    switch (depthFunc) {
        case LESS:
            triangle(screen_coords, shader, framebuffer, zbuffer.data(), true, depth_less);
            break;
        case GREATER:
            triangle(screen_coords, shader, framebuffer, zbuffer.data(), true, depth_more);
            break;
    }
}
