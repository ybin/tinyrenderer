#pragma once

void points_interpolator(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, float *zbuffer);

void line_interpolator(const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, float *zbuffer);

void render(std::vector<std::string> objs,
            int width, int height,
            const Vec3f &eye, const Vec3f &center, const Vec3f &up,
            const std::string &output = "framebuffer.tga",
            Interpolator interpolator =
            [](const std::vector<Vec4f> &pts, IShader &shader, TGAImage &image, float *zbuffer) {
                triangle(pts, shader, image, zbuffer);
            });