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
            const std::string &framebuffer = "framebuffer.tga") {
    auto MV = lookat(eye, center, up);
    auto VP = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    auto P = projection(-1.f / (eye - center).norm());
    TGAImage image(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::RGB);

    for (auto &obj : objs) {
        Model model(obj.data());
        Shader shader;
        shader.model = &model;
        shader.ModelView = MV;
        shader.Projection = P;
        shader.Viewport = VP;
        for (int i = 0; i < model.nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, image, zbuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file(framebuffer.data());
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

    render(objs, width, height, eye, center, up);

    return 0;
}

