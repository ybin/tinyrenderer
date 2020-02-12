#include "mat.h"

Matrix viewport(int x, int y, int w, int h) {
    // x, y is in the clip space, that means x, y is in [-1, 1]
    // [-1, 1] => [0, 2]
    Matrix translate = Matrix::identity();
    translate[0][3] = translate[1][3] = translate[2][3] = 1;

    // [0, 2] => [0, 1]
    Matrix scale = Matrix::identity();
    scale[0][0] = scale[1][1] = scale[2][2] = 0.5f;

    // [0, 1] => [0, w], [0, h], [0, depth]
    Matrix wh = Matrix::identity();
    wh[0][0] = w;
    wh[1][1] = h;

    // [0, w] => [x, x + w], [0, h] => [y, y + h]
    Matrix offset = Matrix::identity();
    offset[0][3] = x;
    offset[1][3] = y;

    return offset * wh * scale * translate;
}

Matrix projection(float coeff) {
    Matrix p = Matrix::identity();
    p[2][3] = coeff;
    p[3][2] = -1 / coeff;
    p[3][3] = 0;
    return p;
}

Matrix frustum(float l, float t, float r, float b, float n, float f) {
    Matrix p;
    p[0][0] = 2 * n / (r - l);
    p[0][2] = (r + l) / (r - l);
    p[1][1] = 2 * n / (t - b);
    p[1][2] = (t + b) / (t - b);
    p[2][2] = -(f + n) / (f - n);
    p[2][3] = -2 * f * n / (f - n);
    p[3][2] = -1;
    return p;
}

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = cross(up, z).normalize();
    Vec3f y = cross(z, x).normalize();
    Matrix linearTransform = Matrix::identity();
    Matrix translate = Matrix::identity();

    // 0: x, 1: y, 3: z. Keep Order! linearTransform is the linear transformation matrix
    linearTransform.set_col(0, embed<4>(x, 0.f));
    linearTransform.set_col(1, embed<4>(y, 0.f));
    linearTransform.set_col(2, embed<4>(z, 0.f));

    // translate to eye position
    translate.set_col(3, embed<4>(eye));

    return (translate * linearTransform).invert();
}