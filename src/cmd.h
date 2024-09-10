#pragma once

#include "vec3.h"

const size_t CMD_MAX_IN_FILE_NAME_LEN = 512;
const size_t CMD_MAX_OUT_FILE_NAME_LEN = 2048;

struct CmdArgs {
    int threads;
    int height;
    int width;

    char in_file_name[CMD_MAX_IN_FILE_NAME_LEN+1];
    char out_file_name[CMD_MAX_OUT_FILE_NAME_LEN+1];
    Vec3 focal_offset;
    Vec3 camera_origin;
};

bool parse_cmd_args(int argc, char* argv[], CmdArgs& args);
