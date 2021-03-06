#pragma once

#include "geometry.h"

#define CLAMP(t) ((t > 1.f) ? 1.f : ((t < 0.f) ? 0.f : t))

struct Shader0 : public IShader {
    mat<2, 3, float> varying_uv;  // triangle uv coordinates, written by the vertex shader, read by the fragment shader
    mat<4, 3, float> varying_tri; // triangle coordinates (clip coordinates), written by VS, read by FS
    mat<3, 3, float> varying_nrm; // normal per vertex to be interpolated by FS
    mat<3, 3, float> ndc_tri;     // triangle in normalized device coordinates
    Model *model = nullptr;
    Vec3f light_dir = {1, 1, 1};
    Matrix ModelView;
    Matrix Viewport;
    Matrix Projection;

    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, proj<3>((Projection * ModelView).invert_transpose() *
                                             embed<4>(model->normal(iface, nthvert), 0.f)));
        Vec4f gl_Vertex = Projection * ModelView * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, gl_Vertex);
        ndc_tri.set_col(nthvert, proj<3>(gl_Vertex / gl_Vertex[3]));
        light_dir = proj<3>(Projection * ModelView * embed<4>(light_dir, 0.f)).normalize();
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

class GouraudShader : public IShader {
private:
    Vec3f varying_intensity;        // write by vertex shader, read by fragment shader
    mat<2, 3, float> varying_uv;    // write by vertex shader, read by fragment shader
    Vec3f light_dir = {1, 1, 1};
    Model *model = nullptr;
    Matrix mvp;

public:
    void set_mvp(Matrix mvp) override {
        this->mvp = mvp;
    }

    Model *get_model() override {
        return model;
    }

    void set_model(Model *model) override {
        this->model = model;
    }

    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_intensity[nthvert] = CLAMP(model->normal(iface, nthvert) * light_dir); // diffuse light intensity
        Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from obj file
        return mvp * gl_Vertex;
    }

    bool fragment(Vec3f bar, TGAColor &color) override {
        float intensity = varying_intensity * bar; //interpolate intensity for current Pixel
        Vec2f uv = varying_uv * bar; //interpolate uv for current Pixel
        color = model->diffuse(uv) * intensity;
        return false; // do not discard pixel
    }
};

class NoLightShader : public IShader {
private:
    mat<2, 3, float> varying_uv;
    Model *model = nullptr;
    Matrix mvp;

public:
    void set_mvp(Matrix mvp) override {
        this->mvp = mvp;
    }

    Model *get_model() override {
        return model;
    }

    void set_model(Model *model) override {
        this->model = model;
    }

    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        Vec4f gl_Vertex = mvp * embed<4>(model->vert(iface, nthvert));
        return gl_Vertex;
    }

    bool fragment(Vec3f bar, TGAColor &color) override {
        Vec2f uv = varying_uv * bar;
        color = model->diffuse(uv);
        return false;
    }
};

struct BumpShader : public IShader {
    mat<2, 3, float> varying_uv;
    mat<3, 3, float> varying_tri;
    mat<3, 3, float> varying_nrm;
    Model *model = nullptr;
    // let's do it in World Space
    Vec3f light_dir = {1, 1, 1};
    Matrix mvp;

public:
    void set_mvp(Matrix mvp) override {
        this->mvp = mvp;
    }

    void set_model(Model *model) override {
        this->model = model;
    }

    Model *get_model() override {
        return model;
    }

    Vec4f vertex(int iface, int nthvert) override {
        varying_uv.set_col(nthvert, model->uv(iface, nthvert));
        varying_nrm.set_col(nthvert, model->normal(iface, nthvert));
        Vec4f gl_Vertex = mvp * embed<4>(model->vert(iface, nthvert));
        varying_tri.set_col(nthvert, proj<3>(gl_Vertex));
        light_dir = light_dir.normalize();
        return gl_Vertex;
    }

#define BUMP_NORMAL 1
    mat<3, 3, float> compute_tbn_mat(const Vec3f &bar) {
        Vec3f vn = (varying_nrm * bar).normalize();
        mat<3, 3, float> TBN;
        TBN.set_col(2, vn);

#if (BUMP_NORMAL == 0)
        Vec3f deltaPos1 = varying_tri.col(1) - varying_tri.col(0);
        Vec3f deltaPos2 = varying_tri.col(2) - varying_tri.col(0);
        Vec2f deltaUV1 = varying_uv.col(1) - varying_uv.col(0);
        Vec2f deltaUV2 = varying_uv.col(2) - varying_uv.col(0);
        float r = 1 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        Vec3f tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        Vec3f bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
        TBN.set_col(0, tangent.normalize());
        TBN.set_col(1, bitangent.normalize());
#elif (BUMP_NORMAL == 1)
        mat<3, 3, float> Q; // make sure in World Space(or Tangent Space)
        Q[0] = varying_tri.col(1) - varying_tri.col(0);
        Q[1] = varying_tri.col(2) - varying_tri.col(0);
        Q[2] = vn;
        mat<3, 3, float> AI = Q.invert();
        Vec3f i = AI * Vec3f(varying_uv[0][1] - varying_uv[0][0], varying_uv[0][2] - varying_uv[0][0], 0);
        Vec3f j = AI * Vec3f(varying_uv[1][1] - varying_uv[1][0], varying_uv[1][2] - varying_uv[1][0], 0);
        TBN.set_col(0, i.normalize());
        TBN.set_col(1, j.normalize());
#elif (BUMP_NORMAL == 2)
        /*
         * World Space vectors:   e1 = AB, e2 = AC (vertex coordinate)
         * and the corresponding
         * Tangent Space vectors: v1 = AB, v2 = AC (uv coordinate)
         * The tangent space base vectors: t, b, n (the vertex normal vector of CURRENT pixel),
         * then
         * e1 = x * t + y * b + z * n, x, y, z are scalars, t, b, n are vectors. Then
         * e1 * t = x * t * t + 0 + 0 => x = e1 * t, y, z are same:
         * x = e1 * t
         * y = e1 * b
         * z = e1 * n
         * because the corresponding vector os e1 is v1 (in tangent space), so,
         * x = v1.x
         * y = v1.y
         * z = v1.z
         */
#endif
        return TBN;
    }

    bool fragment(Vec3f bar, TGAColor &color) override {
        Vec2f uv = varying_uv * bar;
        Vec3f n = (compute_tbn_mat(bar) * model->normal(uv)).normalize();
        color = model->diffuse(uv) * std::max(0.f, n * light_dir);

        return false;
    }
};