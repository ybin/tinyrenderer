#include "mat.h"

Matrix viewport(int x, int y, int w, int h) {
    // x, y is in the NDC space, that means x, y is in [-1, 1]
    // [-1, 1] => [0, 2]
    Matrix translate = Matrix::identity();
    translate[0][3] = translate[1][3] = 1;
    translate[2][3] = 1; // make sure z > 0, because of the interpolation correction in triangle interpolation

    // [0, 2] => [0, 1]
    Matrix scale = Matrix::identity();
    scale[0][0] = scale[1][1] = scale[2][2] = 0.5f;

    // [0, 1] => [0, w], [0, h]
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

Matrix frustum(float l,float r, float b, float t, float n, float f) {
    Matrix p;
    p[0][0] = 2 * n / (r - l);
    p[0][2] = (r + l) / (r - l);
    p[1][1] = 2 * n / (t - b);
    p[1][2] = (t + b) / (t - b);
    p[2][2] = -(f + n) / (f - n);
    p[2][3] = -2 * f * n / (f - n);
    p[3][2] = -1;

    // ugly correction, otherwise triangle interpolation must change the zbuffer order,
    // but that make the result worse, WHY?!
    auto q = Matrix::identity();
    q[2][2] = -1;
    return q * p;
}

Matrix lookat2(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye-center).normalize();
    Vec3f x = cross(up,z).normalize();
    Vec3f y = cross(z,x).normalize();
    Matrix Minv = Matrix::identity();
    Matrix Tr   = Matrix::identity();
    for (int i=0; i<3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = z[i];
        Tr[i][3] = -(eye[i] - center[i]);
    }
    return Minv*Tr;
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
    translate.set_col(3, embed<4>(eye - center));

    return (translate * linearTransform).invert();
}