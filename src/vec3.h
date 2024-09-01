#pragma once

#include "common.h"

typedef struct __sFILE FILE;

struct Vec3 {
    f64 x, y, z;

    void print() const;
    void fprint(FILE *f) const;
};

Vec3 operator+(const Vec3& v, const Vec3& u);
Vec3 operator-(const Vec3& v);
Vec3 operator-(const Vec3& v, const Vec3& u);
Vec3 operator*(const Vec3& v, f64 t);
Vec3 operator*(f64 t, const Vec3& v);
Vec3 operator/(const Vec3& v, f64 t);
bool operator==(const Vec3& v, const Vec3& u);

f64  vec3_dot(const Vec3& v, const Vec3& u);
Vec3 vec3_cross(const Vec3& v, const Vec3& u);
