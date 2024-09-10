#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"

static void set_default_cmd_args(CmdArgs& args) {
    args.threads = 1;
    args.height = 480;
    args.width = 640;
}

enum PresetType {
    Preset_TeddyBear,
    Preset_Teapot,
    Preset_Cube,

    Preset_Count
};

static const size_t PRESET_MAX_NAME_LEN = 100;

struct Preset {
    PresetType type;
    char name[PRESET_MAX_NAME_LEN+1];
    char file_name[CMD_MAX_IN_FILE_NAME_LEN+1];
    Vec3 focal_offset;
    Vec3 camera_origin;  // minus focal offset
};

static const Preset Presets[Preset_Count] {
    Preset {
        .type = Preset_TeddyBear,
        .name = "teddy-bear",
        .file_name = "assets/teddy_bear.obj",
        .focal_offset = Vec3 { .z = 1 },
        .camera_origin = Vec3{ .z = 40 }
    },
    Preset {
        .type = Preset_Teapot,
        .name = "teapot",
        .file_name = "assets/teapot.obj",
        .focal_offset = Vec3 { .z = -1 },
        .camera_origin = Vec3{ .y = 1.5, .z = -4 }
    },
    Preset {
        .type = Preset_Cube,
        .name = "cube",
        .file_name = "assets/cube.obj",
        .focal_offset = Vec3 { .z = -1 },
        .camera_origin = Vec3{ .x = -1, .y = -1, .z = -3 }
    }
};

static bool parse_preset(CmdArgs& args, char* preset_arg) {
    for (int i = 0; i < Preset_Count; i++) {
        auto preset = Presets[i];
        if (strcmp(preset.name, preset_arg) == 0) {
            strcpy(args.in_file_name, preset.file_name);
            args.focal_offset = preset.focal_offset;
            args.camera_origin = preset.camera_origin + preset.focal_offset;
            return true;
        }
    }
    return false;
}

bool parse_cmd_args(int argc, char* argv[], CmdArgs& args) {
    set_default_cmd_args(args);

    int c;
    int errors = 0;
    bool preset_set = false;
    bool out_file_set = false;

    while ((c = getopt(argc, argv, "h:w:n:o:p:")) != -1) {
        switch (c) {
        case 'h': {
            char* end;
            long num = strtol(optarg, &end, 10);
            if (num <= 0) {
                errors++;
                fprintf(stderr, "Invalid height: %ld\n", num);
            } else {
                args.height = (int) num;
            }
            break;
        }
        case 'w': {
            char* end;
            long num = strtol(optarg, &end, 10);
            if (num <= 0) {
                errors++;
                fprintf(stderr, "Invalid width: %ld\n", num);
            } else {
                args.width = (int) num;
            }
            break;
        }
        case 'n': {
            char* end;
            long num = strtol(optarg, &end, 10);
            if (num <= 0) {
                errors++;
                fprintf(stderr, "Invalid number of threads: %ld\n", num);
            } else {
                args.threads = (int) num;
            }
            break;
        }
        case 'o': {
            if (strlen(optarg) > CMD_MAX_OUT_FILE_NAME_LEN) {
                errors++;
            } else {
                strcpy(args.out_file_name, optarg);
                out_file_set = true;
            }
            break;
        }
        case 'p':
            if (!parse_preset(args, optarg)) {
                errors++;
            } else {
                preset_set = true;
            }
            break;
        case ':':
            errors++;
            break;
        case '?':
            fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
            break;
        }
    }

    if (!out_file_set) {
        fprintf(stderr, "Out file not specified.\n");
    }

    if (!preset_set) {
        fprintf(stderr, "Preset not chosen.\n");
    }

    bool any_not_set = !preset_set || !out_file_set;
    if (errors > 0 || any_not_set) {
        if (any_not_set) fprintf(stderr, "\n");
        fprintf(
            stderr,
            (
                "Usage:\n"
                "   TODO :)\n"
            )
        );
    }

    return errors == 0 && !any_not_set;
}
