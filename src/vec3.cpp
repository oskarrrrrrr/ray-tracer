#include "vec3.h"
#include <stdio.h>

void Vec3::print() const {
    fprint(stdout);
}

void Vec3::fprint(FILE *f) const {
    fprintf(f, "(%f, %f, %f)", x, y, z);
}

Vec3 operator+(const Vec3& v, const Vec3& u) {
    return Vec3 {
        .x = v.x + u.x,
        .y = v.y + u.y,
        .z = v.z + u.z,
    };
}

Vec3 operator-(const Vec3& v) {
    return Vec3 { .x = -v.x, .y = -v.y, .z = -v.z };
}

Vec3 operator-(const Vec3& v, const Vec3& u) {
    return Vec3 {
        .x = v.x - u.x,
        .y = v.y - u.y,
        .z = v.z - u.z,
    };
}

Vec3 operator*(const Vec3& v, f64 t) {
    return Vec3 {
        .x = v.x * t,
        .y = v.y * t,
        .z = v.z * t,
    };
}

Vec3 operator*(f64 t, const Vec3& v) {
    return v * t;
}

Vec3 operator/(const Vec3& v, f64 t) {
    return v * (1/t);
}

bool operator==(const Vec3& v, const Vec3& u) {
    return v.x == u.x && v.y == u.y && v.z == u.z;
}

f64 vec3_dot(const Vec3& v, const Vec3& u) {
    return v.x * u.x + v.y * u.y + v.z * u.z;
}

Vec3 vec3_cross(const Vec3& v, const Vec3& u) {
    return Vec3 {
        .x = v.y * u.z - v.z * u.y,
        .y = v.z * u.x - v.x * u.z,
        .z = v.x * u.y - v.y * u.x,
    };
}
