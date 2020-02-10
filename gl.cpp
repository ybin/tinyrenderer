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

void triangle(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer, bool colored) {
    auto MAX = std::numeric_limits<float>::max();
    Vec2f bboxmin(MAX, MAX);
    Vec2f bboxmax(-MAX, -MAX);
    for (auto &pt : pts) {
        for (int j = 0; j < 2; j++) {
            // x/w y/w
            bboxmin[j] = std::min(bboxmin[j], pt[j] / pt[3]);
            bboxmax[j] = std::max(bboxmax[j], pt[j] / pt[3]);
        }
    }

    Vec2i P;
    TGAColor color;
    TGAColor white = {255, 255, 255, 255};
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f c = barycentric(proj<2>(pts[0] / pts[0][3]),
                                  proj<2>(pts[1] / pts[1][3]),
                                  proj<2>(pts[2] / pts[2][3]),
                                  P);
            float z = pts[0][2] * c.x + pts[1][2] * c.y + pts[2][2] * c.z;
            float w = pts[0][3] * c.x + pts[1][3] * c.y + pts[2][3] * c.z;
            int frag_depth = std::max(0, std::min(255, int(z / w + .5)));
            if (c.x < 0 || c.y < 0 || c.z < 0 || zbuffer.get(P.x, P.y)[0] > frag_depth) continue;
            bool discard = shader.fragment(c, color);
            if (!discard) {
                zbuffer.set(P.x, P.y, TGAColor(static_cast<unsigned char>(frag_depth)));
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

