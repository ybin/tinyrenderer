#pragma once

#include "geometry.h"

Matrix viewport(int x, int y, int w, int h);

// simple projection, no clip, coeff is the position norm of camera
Matrix projection(float coeff = 0.f);

Matrix frustum(float l, float t, float r, float b, float n, float f);

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up);