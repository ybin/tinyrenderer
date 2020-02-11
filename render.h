#pragma once

typedef void (*Interpolator)(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer);

void points_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer);

void line_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer);

void triangle_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer);

void default_interpolator(const std::vector<Vec3f> &screen_coords, IShader &shader, TGAImage &image, float *zbuffer);

void render(std::vector<std::string> objs, int width, int height,
            const Vec3f &eye, const Vec3f &center, const Vec3f &up,
            const std::string &output = "framebuffer.tga",
            Interpolator interpolator = default_interpolator);