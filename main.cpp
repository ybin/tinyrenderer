#include <vector>
#include <limits>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "gl.h"

#define CLAMP(t) ((t > 1.f) ? 1.f : ((t < 0.f) ? 0.f : t))

Model *model = NULL;

const int width = 800;
const int height = 800;

Vec3f light_dir(1, 1, 1);
Vec3f eye(1, 1, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

const TGAColor white = TGAColor(255, 255, 255, 255);

struct Shader0 : public IShader {
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

struct Shader : public IShader {
    Vec3f varying_intensity; // write by vertex shader, read by fragment shader
    mat<2, 3, float> varying_uv; // write by vertex shader, read by fragment shader

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir); // diffuse light intensity
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from obj file
        return Viewport * Projection * ModelView * gl_Vertex;
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        float intensity = varying_intensity * bar; //interpolate intensity for current Pixel
        Vec2f uv = varying_uv * bar; //interpolate uv for current Pixel
        color = model->diffuse(uv) * intensity;
        return false; // do not discard pixel
    }
};

struct Shader1 : public IShader {
    mat<2, 3, float> varying_uv; // write by vertex shader, read by fragment shader
    mat<4, 4, float> uniform_M = Projection * ModelView;; //Projection*ModelView
    mat<4, 4, float> uniform_MIT = (Projection *
                                    ModelView).invert_transpose();; // (Projection*ModelView).invert_transpose()

    virtual Vec4f vertex(int iface, int nthvert) {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from obj file
        return Viewport * Projection * ModelView * gl_Vertex; // transform to screen coords
    }

    virtual bool fragment(Vec3f bar, TGAColor &color) {
        Vec2f uv = varying_uv * bar; //interpolate uv for current Pixel
        Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize(); // transform normal vector
        Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize(); // transfrom light direction
        float intensity = std::max(0.f, n * l);
        color = model->diffuse(uv) * intensity; //uv
        return false; // do not discard pixel
    }
};

void render_vertex(const std::vector<std::string> &objs, const std::string &out) {
    TGAImage frame(width, height, TGAImage::RGB);
    for (auto &obj : objs) {
        model = new Model(obj.data());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            for (int j = 0; j < 3; j++) {
                Vec4f v = shader.vertex(i, j);
                Vec2i p = proj<2>(v / v[3]);
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
                Vec2i v0 = proj<2>(v / v[3]);
                v = shader.vertex(i, (j + 1) % 3);
                Vec2i v1 = proj<2>(v / v[3]);

                line(v0.x, v0.y, v1.x, v1.y, frame, white);
            }
        }
        delete model;
    }
    frame.flip_vertically();
    frame.write_tga_file(out.data());
}

void render_triangle(const std::vector<std::string> &objs, const std::string &out) {
    TGAImage frame(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
    for (auto &obj : objs) {
        model = new Model(obj.c_str());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, frame, zbuffer, false);
        }
        delete model;
    }
    frame.flip_vertically(); // to place the origin in the bottom left corner of the image
    frame.write_tga_file(out.data());
}

void render_triangle_colored(const std::vector<std::string> &objs, const std::string &out) {
    TGAImage frame(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);
    for (auto &obj : objs) {
        model = new Model(obj.c_str());
        Shader shader;
        for (int i = 0; i < model->nfaces(); i++) {
            Vec4f screen_coords[3];
            for (int j = 0; j < 3; j++) {
                screen_coords[j] = shader.vertex(i, j);
            }
            triangle(screen_coords, shader, frame, zbuffer);
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
//    light_dir = proj<3>(Projection * ModelView * embed<4>(light_dir, 0.f)).normalize();
    light_dir.normalize();

    render_vertex(objs, "vertex.tga");
    render_line(objs, "line.tga");
    render_triangle(objs, "triangle.tga");
    render_triangle_colored(objs, "triangle_colored.tga");

    return 0;
}

