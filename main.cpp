#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "gl.h"
#include "shader.h"

void render_vertex(const std::vector<std::string> &objs, TGAImage &frame) {
    const TGAColor white = TGAColor(255, 255, 255, 255);
    for (auto &obj : objs) {
        Model model(obj.data());
        Shader shader;
        shader.model = &model;
        for (int i = 0; i < model.nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                Vec4f v = shader.vertex(i, j);
                Vec2i p = proj<2>(v / v[3]);
                frame.set(p.x, p.y, white);
            }
        }
    }
}

void render_line(const std::vector<std::string> &objs, TGAImage &frame) {
    const TGAColor white = TGAColor(255, 255, 255, 255);
    auto line = [](int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
        for (float t = 0.0f; t < 1.0; t += 0.01f) {
            int x = x0 + (x1 - x0) * t;
            int y = y0 + (y1 - y0) * t;
            image.set(x, y, color);
        }
    };
    for (auto &obj : objs) {
        Model model(obj.data());
        Shader shader;
        shader.model = &model;
        for (int i = 0; i < model.nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                Vec4f v;
                v = shader.vertex(i, j);
                Vec2i v0 = proj<2>(v / v[3]);
                v = shader.vertex(i, (j + 1) % 3);
                Vec2i v1 = proj<2>(v / v[3]);

                line(v0.x, v0.y, v1.x, v1.y, frame, white);
            }
        }
    }
}

void render_triangle(const std::vector<std::string> &objs, TGAImage &frame) {
    TGAImage zbuffer(frame.get_width(), frame.get_height(), TGAImage::GRAYSCALE);
    for (auto &obj : objs) {
        Model model(obj.c_str());
        Shader shader;
        shader.model = &model;
        for (int i = 0; i < model.nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, frame, zbuffer, false);
        }
    }
}

void render_triangle_colored(const std::vector<std::string> &objs, TGAImage &frame) {
    TGAImage zbuffer(frame.get_width(), frame.get_height(), TGAImage::GRAYSCALE);
    for (auto &obj : objs) {
        Model model(obj.c_str());
        Shader shader;
        shader.model = &model;
        for (int i = 0; i < model.nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, frame, zbuffer);
        }
    }
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

    lookat(eye, center, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection(-1.f / (eye - center).norm());
    TGAImage image(width, height, TGAImage::RGB);

    render_vertex(objs, image);
    image.flip_vertically();
    image.write_tga_file("vertex.tga");

    image.clear();
    render_line(objs, image);
    image.flip_vertically();
    image.write_tga_file("line.tga");

    image.clear();
    render_triangle(objs, image);
    image.flip_vertically();
    image.write_tga_file("triangle.tga");

    image.clear();
    render_triangle_colored(objs, image);
    image.flip_vertically();
    image.write_tga_file("triangle_colored.tga");

    return 0;
}

