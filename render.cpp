#include <vector>
#include "geometry.h"
#include "gl.h"
#include "tgaimage.h"
#include "mat.h"
#include "model.h"
#include "shader.h"
#include "render.h"

void points_interpolator(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
    const TGAColor white(255, 255, 255);
    for (auto &pt : pts) {
        image.set(static_cast<int>(pt[0] / pt[3]), static_cast<int>(pt[1] / pt[3]), white);
    }
}

void line_interpolator(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
    const TGAColor white(255, 255, 255);
    const int len = 100;
    const float step = 1.f / len;
    int k;
    float t;
    Vec2i v0, v1;
    for (size_t i = 0, j = 0; i < pts.size(); ++i) {
        j = (i + 1) % pts.size();
        v0 = proj<2>(pts[i] / pts[i][3]);
        v1 = proj<2>(pts[j] / pts[j][3]);
        k = 0;
        t = 0.f;
        while (k++ < len) {
            image.set(static_cast<int>(v0.x + (v1.x - v0.x) * t),
                      static_cast<int>(v0.y + (v1.y - v0.y) * t),
                      white);
            t += step;
        }
    }
}

void render(const std::vector<std::string> objs,
            const int width, const int height,
            const Vec3f &eye, const Vec3f &center, const Vec3f &up,
            const std::string &output,
            Interpolator interpolator) {
    auto MV = lookat(eye, center, up);
    auto VP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    auto P = projection(-1.f / (eye - center).norm());
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

    for (auto &obj : objs) {
        Model model(obj.data());
        Shader shader;
        shader.set_model(&model);
        shader.set_mvp(VP * P * MV);
        std::vector<Vec4f> screen_coords(3);
        for (int i = 0; i < model.nfaces(); i++) {
            screen_coords.clear();
            for (int j = 0; j < 3; j++) {
                screen_coords.emplace_back(shader.vertex(i, j));
            }
            interpolator(screen_coords, shader, framebuffer, zbuffer);
        }
    }

    framebuffer.flip_vertically();
    framebuffer.write_tga_file(output.data());
}