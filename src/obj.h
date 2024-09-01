// .obj file format documentation:
//     https://people.sc.fsu.edu/~jburkardt/txt/obj_format.txt

#pragma once

#include <cstddef>

struct Vec3;
typedef struct __sFILE FILE;

namespace Obj {
    struct Face {
        int    len;
        Vec3** gv;  // geometric vertex
        Vec3** vn;  // vertex normal
    };

    struct Mesh {
        int    vertices_count;
        Vec3*  vertices;

        int    normals_count;
        Vec3*  normals;

        int    faces_count;
        Face*  faces;

        void print() const;
        void fprint(FILE* file) const;
    };

    struct MeshInfo {
        int vertices;
        size_t vertices_mem;
        int normals;
        size_t normals_mem;
        int faces;
        size_t faces_mem;
    };

    bool calc_memory(const char* file_name, MeshInfo& info);
    Mesh* parse(const char* file_name, void* buffer, const MeshInfo& info);
}
