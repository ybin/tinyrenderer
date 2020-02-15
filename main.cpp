#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "gl.h"
#include "shader.h"

void glRender(const std::vector<std::string> objs,
              const int width, const int height,
              const Vec3f &eye, const Vec3f &center, const Vec3f &up,
              const GL::Renderer renderer, const std::string &output) {
    auto V = lookat(eye, center, up);
//    auto P = projection((eye - center).norm());
    auto len = 1 - 1 / (eye - center).norm();
    auto P = frustum(-len, len, -len, len, (eye - center).norm() - 1, (eye - center).norm() + 1);
    TGAImage framebuffer(width, height, TGAImage::RGB);
    GL gl(&framebuffer);
    gl.glViewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    gl.glRenderer(renderer);

    for (auto &obj : objs) {
        Model model(obj.data());
        BumpShader shader;
        shader.set_mvp(P * V);
        shader.set_model(&model);

        gl.glShader(&shader);
        gl.glDraw();
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

    glRender(objs, width, height, eye, center, up, GL::VERTEX, "vertex.tga");

    glRender(objs, width, height, eye, center, up, GL::LINE, "line.tga");

    glRender(objs, width, height, eye, center, up, GL::TRIANGLE, "triangle.tga");

    glRender(objs, width, height, eye, center, up, GL::TRIANGLE_COLORED, "framebuffer.tga");

    return 0;
}

