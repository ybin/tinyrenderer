#include <vector>
#include "geometry.h"
#include "gl.h"
#include "tgaimage.h"
#include "mat.h"
#include "model.h"
#include "shader.h"
#include "render.h"

void points_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer) {
    const TGAColor white(255, 255, 255);
    for (auto &pt : screen_coords) {
        image.set(static_cast<int>(pt[0]), static_cast<int>(pt[1]), white);
    }
}

void line_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer) {
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
            image.set(static_cast<int>(v0.x + (v1.x - v0.x) * t),
                      static_cast<int>(v0.y + (v1.y - v0.y) * t),
                      white);
            t += step;
        }
    }
}

void triangle_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer) {
    triangle(screen_coords, shader, image, zbuffer, false);
}

void default_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer) {
    triangle(screen_coords, shader, image, zbuffer);
}

void render(const std::vector<std::string> objs,
            const int width, const int height,
            const Vec3f &eye, const Vec3f &center, const Vec3f &up,
            const std::string &output, Interpolator interpolator) {
    auto V = lookat(eye, center, up);
//    auto P = projection((eye - center).norm());
    auto P = frustum(-1, 1, -1, 1, (eye - center).norm(), (eye - center).norm() + 2);
    auto VP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<float> zbuffer(static_cast<unsigned long>(width * height));
    for (float &k : zbuffer) {
        k = -MAXFLOAT;
    }

    for (auto &obj : objs) {
        Model model(obj.data());
        BumpShader shader;
        shader.set_model(&model);
        shader.set_mvp(P * V);
        std::vector<Vec3f> screen_coords(3);
        Vec4f v;
        for (int i = 0; i < model.nfaces(); i++) {
            screen_coords.clear();
            for (int j = 0; j < 3; j++) {
                v = shader.vertex(i, j);
                v = v / v[3];
                v = VP * v;
                screen_coords.emplace_back(proj<3>(v));
            }
            interpolator(screen_coords, shader, framebuffer, zbuffer.data());
        }
    }

    framebuffer.flip_vertically();
    framebuffer.write_tga_file(output.data());
}