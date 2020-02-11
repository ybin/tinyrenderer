#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "gl.h"
#include "render.h"

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

    render(objs, width, height, eye, center, up, "vertex.tga", points_interpolator);

    render(objs, width, height, eye, center, up, "line.tga", line_interpolator);

    render(objs, width, height, eye, center, up, "triangle.tga", triangle_interpolator);

    render(objs, width, height, eye, center, up);

    return 0;
}

