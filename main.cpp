#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "gl.h"

Model *model = NULL;

const int width = 800;
const int height = 800;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const TGAColor white = TGAColor(255, 255, 255, 255);

struct Shader : public IShader {
    mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<4, 3, float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
    mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates

    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() *
                                             embed<4>(model->normal(iface, nthvert), 0.f)));
        Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        return gl_Vertex;
    }

    bool fragment(Vec3f bar, TGAColor &color) override {
        Vec3f bn = (varying_nrm * bar).normalize();
        Vec2f uv = varying_uv * bar;

        mat<3, 3, float> A;
        A[0] = ndc_tri.col(1) - ndc_tri.col(0);
        A[1] = ndc_tri.col(2) - ndc_tri.col(0);
        A[2] = bn;

        mat<3, 3, float> AI = A.invert();

        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);

        mat<3, 3, float> B;
        B.set_col(0, i.normalize());
        B.set_col(1, j.normalize());
        B.set_col(2, bn);

        Vec3f n = (B * model->normal(uv)).normalize();

        color = model->diffuse(uv) * std::max(0.f, n * light_dir);

        return false;
    }
};

void render_vertex(const std::vector<std::string> &objs, const std::string &out) {
    TGAImage frame(width, height, TGAImage::RGB);
    for (auto &obj : objs) {
        model = new Model(obj.data());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                Vec3f v = proj<3>(shader.vertex(i, j));
                Vec2i p = {int((v.x + 1.) * width / 2.), int((v.y + 1.) * height / 2.)};
                frame.set(p.x, p.y, white);
            }
        }
        delete model;
    }
    frame.flip_vertically();
    frame.write_tga_file(out.data());
}

void render_line(const std::vector<std::string> &objs, const std::string &out) {
    TGAImage frame(width, height, TGAImage::RGB);
    auto line = [](int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
        for (float t = 0.0f; t < 1.0; t += 0.01f) {
            int x = x0 + (x1 - x0) * t;
            int y = y0 + (y1 - y0) * t;
            image.set(x, y, color);
        }
    };
    for (auto &obj : objs) {
        model = new Model(obj.data());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                Vec4f v;
                v = shader.vertex(i, j);
                Vec3f v0 = proj<3>(v);
                v = shader.vertex(i, (j + 1) % 3);
                Vec3f v1 = proj<3>(v);

                int x0 = (v0.x + 1.) * width / 2.;
                int y0 = (v0.y + 1.) * height / 2.;
                int x1 = (v1.x + 1.) * width / 2.;
                int y1 = (v1.y + 1.) * height / 2.;
                line(x0, y0, x1, y1, frame, white);
            }
        }
        delete model;
    }
    frame.flip_vertically();
    frame.write_tga_file(out.data());
}

void render_triangle(const std::vector<std::string> &objs, const std::string &out) {
    auto zbuffer = std::vector<float>(width * height);
    for (int i = width * height; i--;) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    TGAImage frame(width, height, TGAImage::RGB);
    for (auto &obj : objs) {
        model = new Model(obj.c_str());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                shader.vertex(i, j);
            }
            triangle(shader.varying_tri, shader, frame, zbuffer.data());
        }
        delete model;
    }
    frame.flip_vertically(); // to place the origin in the bottom left corner of the image
    frame.write_tga_file(out.data());
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

    lookat(eye, center, up);
    viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    projection(-1.f / (eye - center).norm());
    light_dir = proj<3>(Projection * ModelView * embed<4>(light_dir, 0.f)).normalize();

    render_triangle(objs, "triangle.tga");
    render_line(objs, "line.tga");
    render_vertex(objs, "vertex.tga");

    return 0;
}

