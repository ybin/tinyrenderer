#include <cmath>
#include <limits>
#include <cstdlib>
#include "gl.h"

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

void triangle(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer, bool colored) {
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
            if (c.x < 0 || c.y < 0 || c.z < 0 || zbuffer[P.x + P.y * image.get_width()] > z) continue;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = z;
                image.set(P.x, P.y, colored ? color : white);
            }
        }
    }
}

/*
void triangle(mat<4, 3, float> &clipc, IShader &shader, TGAImage &image, float *zbuffer, bool colored) {
    mat<3, 4, float> pts = (Viewport * clipc).transpose(); // transposed to ease access to each of the points
    mat<3, 2, float> pts2;
    for (int i = 0; i < 3; i++) pts2[i] = proj<2>(pts[i] / pts[i][3]);

    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    TGAColor color;
    TGAColor white = {255, 255, 255, 255};
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip = Vec3f(bc_screen.x / pts[0][3], bc_screen.y / pts[1][3], bc_screen.z / pts[2][3]);
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            float frag_depth = clipc[2] * bc_clip;
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0 ||
                zbuffer[P.x + P.y * image.get_width()] > frag_depth)
                continue;
            bool discard = shader.fragment(bc_clip, color);
            if (!discard) {
                zbuffer[P.x + P.y * image.get_width()] = frag_depth;
                image.set(P.x, P.y, colored ? color : white);
            }
        }
    }
}
*/

