#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "gl.h"
#include "shader.h"
#include "mat.h"

void render(const std::vector<std::string> objs,
            const int width, const int height,
            const Vec3f &eye, const Vec3f &center, const Vec3f &up,
            const std::string &output = "framebuffer.tga",
            Interpolator interpolator =
            [](const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
                triangle(pts, shader, image, zbuffer);
            }) {
    auto MV = lookat(eye, center, up);
    auto VP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    auto P = projection(-1.f / (eye - center).norm());
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

    for (auto &obj : objs) {
        Model model(obj.data());
        Shader shader;
        shader.model = &model;
        shader.ModelView = MV;
        shader.Projection = P;
        shader.Viewport = VP;
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

int main(int argc, char **argv) {
    std::vector<std::string> objs;

    if (argc < 2) {
        objs.emplace_back("../obj/african_head/african_head.obj");
        objs.emplace_back("../obj/african_head/african_head_eye_inner.obj");

        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        std::cerr << "Use default model now!" << std::endl;
    } else {
        for (int m = 1; m < argc; m++) {
            objs.emplace_back(argv[m]);
        }
    }

    const int width = 800;
    const int height = 800;
    const Vec3f eye(1, 1, 3);
    const Vec3f center(0, 0, 0);
    const Vec3f up(0, 1, 0);
    const auto points_interpolator =
            [](const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
                const TGAColor white(255, 255, 255);
                for (auto &pt : pts) {
                    image.set(static_cast<int>(pt[0] / pt[3]), static_cast<int>(pt[1] / pt[3]), white);
                }
            };
    const auto line_interpolator =
            [](const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
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
            };

    render(objs, width, height, eye, center, up, "vertex.tga", points_interpolator);

    render(objs, width, height, eye, center, up, "line.tga", line_interpolator);

    render(objs, width, height, eye, center, up, "triangle.tga",
           [](const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, TGAImage &zbuffer) {
               triangle(pts, shader, image, zbuffer, false);
           });

    render(objs, width, height, eye, center, up);

    return 0;
}

