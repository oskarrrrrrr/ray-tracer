#include <cstring>
#include <stdio.h>

#include "obj.h"
#include "vec3.h"

using namespace Obj;

void Mesh::print() const {
    fprint(stdout);
}

static int mesh_vertex_index(const Mesh& mesh, const Vec3& v) {
    for (int i = 0; i < mesh.vertices_count; i++) {
        if (mesh.vertices[i] == v) {
            return i + 1;
        }
    }
    return -1;
}

static int mesh_normal_index(const Mesh& mesh, const Vec3& v) {
    for (int i = 0; i < mesh.normals_count; i++) {
        if (mesh.normals[i] == v) {
            return i + 1;
        }
    }
    return -1;
}

void Mesh::fprint(FILE* f) const {
    for (int i = 0; i < vertices_count; i++) {
        fprintf(f, "v %f %f %f\n", vertices[i].x, vertices[i].y, vertices[i].z);
    }

    if (normals_count > 0) fprintf(f, "\n");
    for (int i = 0; i < normals_count; i++) {
        fprintf(f, "vn %f %f %f\n", normals[i].x, normals[i].y, normals[i].z);
    }

    if (faces_count > 0) fprintf(f, "\n");
    for (int i = 0; i < faces_count; i++) {
        fprintf(
            f,
            "f %d//%d %d//%d %d//%d\n",
            mesh_vertex_index(*this, *faces[i].gv[0]),
            mesh_normal_index(*this, *faces[i].vn[0]),
            mesh_vertex_index(*this, *faces[i].gv[1]),
            mesh_normal_index(*this, *faces[i].vn[1]),
            mesh_vertex_index(*this, *faces[i].gv[2]),
            mesh_normal_index(*this, *faces[i].vn[2])
        );
    }
}

static char read_word(FILE* f, char* buffer, int buffer_size) {
    int word_len = 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            break;
        }
        buffer[word_len++] = c;
        if (word_len == buffer_size - 1) {
            break;
        }
    }
    buffer[word_len] = '\0';
    return c;
}

 bool Obj::calc_memory(const char* file_name, MeshInfo& info) {
    FILE* f = fopen(file_name, "r");
    if (f == NULL) return false;

    int vertices_count = 0;
    int normals_count = 0;
    int faces_count = 0;

    const int word_buffer_len = 100;
    char word_buffer[word_buffer_len];
    int c;
    bool new_line = true;
    do {
        c = read_word(f, word_buffer, word_buffer_len);

        if (word_buffer[0] == '#') {}
        else if (strcmp(word_buffer, "v") == 0) vertices_count++;
        else if (strcmp(word_buffer, "vn") == 0) normals_count++;
        else if (strcmp(word_buffer, "f") == 0) faces_count++;

        while (c != EOF && c != '\n') c = fgetc(f);
    } while (c != EOF);

    fclose(f);

    info = MeshInfo {
        .vertices = vertices_count,
        .vertices_mem = vertices_count * sizeof(Vec3),
        .normals = normals_count,
        .normals_mem = normals_count * sizeof(Vec3),
        .faces = faces_count,
        .faces_mem = faces_count * (sizeof(Obj::Face) + 2 * 3 * sizeof(Vec3*))
    };
    return true;
}

static Mesh* mesh_alloc_in_buffer(void* buffer, const MeshInfo& info) {
    Mesh* mesh = (Mesh*) buffer;
    mesh->vertices_count = info.vertices;
    mesh->vertices = (Vec3*)(mesh + sizeof(Mesh));
    mesh->normals_count = info.normals;
    mesh->normals = mesh->vertices + sizeof(Vec3) * mesh->vertices_count;
    mesh->faces_count = info.faces;
    mesh->faces = (Face*)(mesh->normals + sizeof(Vec3) * mesh->normals_count);
    Vec3** gv = (Vec3**)(mesh->faces + sizeof(Face) * mesh->faces_count);
    Vec3** vn = gv + 3 * sizeof(Vec3*) * mesh->faces_count;
    for (int i = 0; i < info.faces; i++) {
        mesh->faces[i].gv = gv;
        gv += 3 * sizeof(Vec3*);
        mesh->faces[i].vn = vn;
        vn += 3 * sizeof(Vec3*);
    }
    return mesh;
}

Mesh* Obj::parse(const char* file_name, void* buffer, const MeshInfo& info) {
    Mesh* mesh = mesh_alloc_in_buffer(buffer, info);

    FILE* f = fopen(file_name, "r");
    if (f == NULL) return NULL;

    int vertices_count = 0;
    int normals_count = 0;
    int faces_count = 0;

    const int word_buffer_len = 100;
    char word_buffer[word_buffer_len];
    int c;
    bool new_line = true;
    do {
        c = read_word(f, word_buffer, word_buffer_len);

        if (word_buffer[0] == '#') {}
        else if (strcmp(word_buffer, "v") == 0) {
            Vec3* v = &mesh->vertices[vertices_count++];
            fscanf(f, "%lf %lf %lf", &v->x, &v->y, &v->z);
        } else if (strcmp(word_buffer, "vn") == 0) {
            Vec3* n = &mesh->normals[normals_count++];
            fscanf(f, "%lf %lf %lf", &n->x, &n->y, &n->z);
        } else if (strcmp(word_buffer, "f") == 0) {
            Face* face = &mesh->faces[faces_count++];
            face->len = 3;
            for (int i = 0; i < face->len; i++) {
                int gv_i, vn_i;
                fscanf(f, "%d//%d", &gv_i, &vn_i);
                face->gv[i] = &mesh->vertices[gv_i-1];
                face->vn[i] = &mesh->normals[vn_i-1];
            }
        }

        while (c != EOF && c != '\n') c = fgetc(f);
    } while (c != EOF);

    fclose(f);
    return mesh;
}
