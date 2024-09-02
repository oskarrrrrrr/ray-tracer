#include <cstdlib>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>  // sleep
#include "vec3.h"
#include "common.h"
#include "obj.h"

struct RGB {
    i32 mem;

    inline u8 get_red() { return (u8) (mem / (1 << 16)); }
    inline u8 get_green() { return (u8) (mem / (1 << 8)) ; }
    inline u8 get_blue() { return (u8) mem; }

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
    return vec3_cross(A, B);
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

f64 hit_triangle(const Triangle& triangle, const Vec3& n, const Ray& ray) {
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
        vec3_dot(n, vec3_cross(e0, p - triangle.a)) > 0 &&
        vec3_dot(n, vec3_cross(e1, p - triangle.b)) > 0 &&
        vec3_dot(n, vec3_cross(e2, p - triangle.c)) > 0
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
        new_RGB(255, 255, 0),
        new_RGB(0, 255, 255),
        new_RGB(255, 0, 255),
        new_RGB(125, 125, 125),
        new_RGB(255, 255, 255)
    };
    return colors[i%8];
}

struct Task {
    int row;
    int col;
};

void shuffle_tasks(Task* array, size_t n) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int usec = tv.tv_usec;
    srand48(usec);

    if (n > 1) {
        size_t i;
        for (i = n - 1; i > 0; i--) {
            size_t j = (unsigned int) (drand48()*(i+1));
            Task t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

struct Counter {
    int val;
    pthread_mutex_t mutex;

    Counter() {
        mutex = PTHREAD_MUTEX_INITIALIZER;
    }

    void inc(int v) {
        pthread_mutex_lock(&mutex);
        val += v;
        pthread_mutex_unlock(&mutex);
    }
};

struct BatchArgs {
    int width;
    int height;
    Point3 top_left_pixel;
    Point3 camera_origin;
    Vec3 viewport_width_d;
    Vec3 viewport_height_d;
    Triangle* triangles;
    Vec3* normals;
    Obj::Mesh* mesh;
    FrameBuffer frame_buffer;
    int tasks_count;
    Task* tasks;
    Counter* counter;
};

void* process_batch(void* arg) {
    BatchArgs* args = (BatchArgs*) arg;
    int width = args->width;
    int height = args->height;
    Point3 top_left_pixel = args->top_left_pixel;
    Point3 camera_origin = args->camera_origin;
    Vec3 viewport_width_d = args->viewport_width_d;
    Vec3 viewport_height_d = args->viewport_height_d;
    Triangle* triangles = args->triangles;
    Vec3* normals = args->normals;
    Obj::Mesh* mesh = args->mesh;
    FrameBuffer frame_buffer = args->frame_buffer;
    int tasks_count = args->tasks_count;
    Task* tasks = args->tasks;
    Counter* counter = args->counter;

    for (int i = 0; i < tasks_count; i++) {
        int row = tasks[i].row;
        int col = tasks[i].col;
        Point3 curr = top_left_pixel - 0.5 * viewport_width_d - row * viewport_height_d - col * viewport_width_d;
        f64 min_t = F64_INF;
        int min_i = -1;
        for (int i = 0; i < mesh->faces_count; i++) {
            Ray ray = { .origin = camera_origin, .direction = curr - camera_origin };
            f64 new_t = hit_triangle(triangles[i], normals[i], ray);
            if (new_t > 0 && new_t < min_t) {
                min_t = new_t;
                min_i = i;
            }
        }
        if (min_t != F64_INF) {
            frame_buffer.set(row, col, get_rand_color(min_i));
        }
        if (i % 1000 == 0) {
            counter->inc(1000);
        }
    }
    return NULL;
}

struct StatusPrinterArgs {
    Counter* counter;
    int tasks_count;
    bool* done;
};

void _print_status(StatusPrinterArgs* args) {
    int val = args->counter->val;
    fprintf(
        stderr,
        "\r%dk/%dk (%d%%)                    ",
        val/1000,
        args->tasks_count/1000,
        (int)((f64) val * 100 / args->tasks_count)
    );
}

void* status_printer(void* args) {
    auto _args = (StatusPrinterArgs*) args;
    while (!(*_args->done)) {
        _print_status(_args);
        sleep_ms(200);
    }
    _print_status(_args);
    printf("\nDone!                      \n");
    return NULL;
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

    Triangle* triangles = (Triangle*) malloc(sizeof(Triangle) * mesh->faces_count);
    Vec3* normals = (Vec3*) malloc(sizeof(Vec3) * mesh->faces_count);

    for (int i = 0; i < mesh->faces_count; i++) {
        Obj::Face face = mesh->faces[i];
        triangles[i] = Triangle {
            .a = *face.gv[0],
            .b = *face.gv[1],
            .c = *face.gv[2]
        };
        // TODO: use precalculated normals when available
        normals[i] = triangle_normal(triangles[i]);
    }

    int height = 800;
    int width = 1200;
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

    int tasks_count = width * height;
    Task* tasks = (Task*) malloc(sizeof(Task) * tasks_count);
    int i = 0;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            tasks[i++] = Task { .row = row, .col = col };
        }
    }
    shuffle_tasks(tasks, tasks_count);

    const int THREADS = 6;
    pthread_t threads[THREADS];
    BatchArgs args[THREADS];

    Counter progress_counter = Counter();

    Task* tasks_ptr = tasks;
    for (int i = 0; i < THREADS; i++) {
        int tasks_count = width * height / THREADS;
        args[i] = {
            .width = width,
            .height = height,
            .top_left_pixel = top_left_pixel,
            .camera_origin = camera_origin,
            .viewport_width_d = viewport_width_d,
            .viewport_height_d = viewport_height_d,
            .triangles = triangles,
            .normals = normals,
            .mesh = mesh,
            .frame_buffer = frame_buffer,
            .tasks_count = tasks_count, // TODO: fix if not divisible?
            .tasks = tasks_ptr,
            .counter = &progress_counter
        };
        pthread_create(&threads[i], NULL, process_batch, (void*)(&args[i]));
        tasks_ptr += tasks_count;
    }

    bool done = false;
    auto status_printer_args = StatusPrinterArgs {
        .counter = &progress_counter,
        .tasks_count = tasks_count,
        .done = &done
    };

    pthread_t status_printer_thread;
    pthread_create(&status_printer_thread, NULL, status_printer, (void*)(&status_printer_args));

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    done = true;
    pthread_join(status_printer_thread, NULL);

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
