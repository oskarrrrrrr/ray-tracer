#include <cstdlib>
#include <stdio.h>
#include "vec3.h"
#include "common.h"
#include "obj.h"

struct RGB {
    i32 mem;

    inline u8 get_red() { return (u8) (mem / (1 << 16)); }
    inline u8 get_green() { return (u8) (mem / (1 << 8)) ; }
    inline u8 get_blue() { return (u8) mem; }

    inline void set_red(u8 v) { }
    inline void set_green(u8 v) { }
    inline void set_blue(u8 v) { }

    void print() {
        fprint(stdout);
    }

    void fprint(FILE* f) {
        fprintf(f, "RGB(%hhu, %hhu, %hhu)", get_red(), get_green(), get_blue());
    }
};

RGB new_RGB(u8 red, u8 green, u8 blue) {
    return RGB{
        .mem = red * (1 << 16) + green * (1 << 8) + blue
    };
}

struct FrameBuffer {
    RGB* buffer;
    int width;
    int height;

    void set(int row, int col, RGB color) {
        buffer[(row * width) + col] = color;
    }

    RGB get(int row, int col) {
        return buffer[(row * width) + col];
    }

    static inline size_t mem(int width, int height) {
        return width * height * sizeof(RGB);
    }

    void to_ppm(FILE* f) {
        fprintf(f, "P3\n%d\n%d\n255\n", width, height);
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                RGB color = get(row, col);
                fprintf(f, "%hhu %hhu %hhu\n", color.get_red(), color.get_green(), color.get_blue());
            }
        }
    }
};

using Point3 = Vec3;

struct Triangle {
    Point3 a, b, c;
};

Vec3 triangle_normal(Triangle triangle) {
    Vec3 A = triangle.b - triangle.a;
    Vec3 B = triangle.c - triangle.a;
    return vec3_cross(B, A);
}

struct Ray {
    Point3 origin;
    Vec3 direction;

    void print() {
        fprint(stdout);
    }

    void fprint(FILE *f) {
        fprintf(f, "Ray<origin: ");
        origin.fprint(f);
        fprintf(f, ", direction: ");
        direction.fprint(f);
        fprintf(f, ">");
    }
};

f64 hit_triangle(const Triangle& triangle, const Ray& ray) {
    Vec3 n = triangle_normal(triangle);

    f64 n_dot_d = vec3_dot(ray.direction, n);
    if (n_dot_d == 0) return -1;

    f64 d = -vec3_dot(n, triangle.a);
    f64 t = -(vec3_dot(n, ray.origin) + d) / n_dot_d;
    if (t < 0) return -1;

    Point3 p = ray.origin + t * ray.direction;

    Vec3 e0 = triangle.b - triangle.a;
    Vec3 e1 = triangle.c - triangle.b;
    Vec3 e2 = triangle.a - triangle.c;
    if (
        vec3_dot(n, vec3_cross(e0, p - triangle.a)) < 0 &&
        vec3_dot(n, vec3_cross(e1, p - triangle.b)) < 0 &&
        vec3_dot(n, vec3_cross(e2, p - triangle.c)) < 0
    ) {
        return t;
    } else {
        return -1;
    }
}

Obj::Mesh* parse_obj(const char* file_name) {
    Obj::MeshInfo info;
    bool ok = Obj::calc_memory(file_name, info);
    if (!ok) {
        printf("Failed to open file: \"%s\"\n", file_name);
        exit(1);
    }
    size_t mem = sizeof(Obj::Mesh) + info.vertices_mem + info.normals_mem + info.faces_mem;
    void* buffer = malloc(mem);
    Obj::Mesh* mesh = Obj::parse(file_name, buffer, info);
    return mesh;
}

RGB get_rand_color(int i) {
    static RGB colors[] = {
        new_RGB(255, 0, 0),
        new_RGB(0, 255, 0),
        new_RGB(0, 0, 255),
        // new_RGB(255, 255, 0),
        // new_RGB(0, 255, 255),
        // new_RGB(255, 0, 255),
        // new_RGB(125, 125, 125),
        // new_RGB(255, 255, 255)
    };
    return colors[i%3];
}

int main() {
    // const char* in_file_name = "assets/teddy_bear.obj";
    // Vec3 focal_offset = Vec3 { .z = 1 };
    // Vec3 camera_origin = Vec3{ .z = 40 } + focal_offset;

    // const char* in_file_name = "assets/cube.obj";
    // Vec3 focal_offset = Vec3 { .z = -1 };
    // Vec3 camera_origin = Vec3{ .x = -1, .y = -1, .z = -3 } + focal_offset;


    const char* in_file_name = "assets/teapot.obj";
    Vec3 focal_offset = Vec3 { .x = 0, .z = -1 };
    Vec3 camera_origin = Vec3{ .x = 0, .y = 1.5, .z = -4 } + focal_offset;

    fprintf(stderr, "Parsing obj file: \"%s\"\n", in_file_name);
    Obj::Mesh* mesh = parse_obj(in_file_name);
    fprintf(stderr, "Triangle Count: %d\n", mesh->faces_count);

    Triangle* triangles = (Triangle *) malloc(sizeof(Triangle) * mesh->faces_count);

    for (int i = 0; i < mesh->faces_count; i++) {
        Obj::Face face = mesh->faces[i];
        triangles[i] = Triangle {
            .a = *face.gv[0],
            .b = *face.gv[1],
            .c = *face.gv[2]
        };
    }

    int height = 400;
    int width = 600;
    RGB* buffer = (RGB*) calloc(width * height, sizeof(RGB));
    FrameBuffer frame_buffer = { .buffer = buffer, .width = width, .height = height };

    f64 aspect_ratio = ((f64)width) / height;

    f64 viewport_width = 2.0;
    Vec3 viewport_width_v = Vec3 { .x = viewport_width };
    f64 viewport_height = viewport_width / aspect_ratio;
    Vec3 viewport_height_v = Vec3 { .y = viewport_height };

    Vec3 top_left_pixel = (
        camera_origin - focal_offset + viewport_width_v/2 + viewport_height_v/2
    );

    Vec3 viewport_width_d = Vec3 { .x = viewport_width / width };
    Vec3 viewport_height_d = Vec3 { .y = viewport_height / height };

    for (int row = 0; row < height; row++) {
        Point3 curr = top_left_pixel - 0.5 * viewport_width_d - row * viewport_height_d;
        for (int col = 0; col < width; col++) {
            bool any_hit = false;
            f64 min_t = 1000000000;
            int min_i = -1;
            for (int i = 0; i < mesh->faces_count; i++) {
                Ray ray = { .origin = camera_origin, .direction = curr - camera_origin };
                f64 new_t = hit_triangle(triangles[i], ray);
                if (new_t != -1) {
                    any_hit = true;
                    if (new_t < min_t) {
                        min_t = new_t;
                        min_i = i;
                    }
                }
            }
            if (any_hit) {
                frame_buffer.set(row, col, get_rand_color(min_i));
            }
            curr = curr - viewport_width_d;
        }
        fprintf(stderr, "\r%d/%d                        ", row+1, height);
    }
    fprintf(stderr, "\n");


    const char* file_name = "img.ppm";
    FILE* f = fopen(file_name, "w");
    if (f == NULL) {
        fprintf(stderr, "Failed to open: \"%s\"\n", file_name);
        exit(1);
    }
    fprintf(stderr, "Saving result to: \"%s\"\n", file_name);
    frame_buffer.to_ppm(f);
    fclose(f);
}
