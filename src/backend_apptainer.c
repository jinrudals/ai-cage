#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "backend.h"

#ifndef APPTAINER_BIN
#define APPTAINER_BIN "/usr/bin/apptainer"
#endif

#ifndef APPTAINER_IMAGES
#define APPTAINER_IMAGES "/usr/local/share/ai-cage/images"
#endif

static int is_executable(const char *p) {
    return access(p, X_OK) == 0;
}

int backend_run(const char *workspace, const MountList *mounts,
                int argc, char **argv, const char *command,
                const char *image_name, int shell_mode) {
    (void)workspace;

    if (!image_name || !image_name[0]) {
        fprintf(stderr, "FAIL: WRAPPED_AI_IMAGE_NAME is not set\n");
        return 1;
    }

    if (!is_executable(APPTAINER_BIN)) {
        fprintf(stderr, "FAIL: apptainer not executable: %s\n", APPTAINER_BIN);
        return 1;
    }

    char image_path[PATH_MAX_LEN];
    snprintf(image_path, sizeof(image_path), "%s/%s.sif", APPTAINER_IMAGES, image_name);

    if (access(image_path, F_OK) != 0) {
        fprintf(stderr, "FAIL: image not found: %s\n", image_path);
        return 1;
    }

    /* Build argv: binary + subcmd + binds + image + extra args + NULL */
    int max_args = 4 + (mounts->count * 2) + argc + 4;
    char **exec_argv = calloc(max_args, sizeof(char *));
    int ai = 0;

    exec_argv[ai++] = (char *)APPTAINER_BIN;

    if (shell_mode) {
        exec_argv[ai++] = "shell";
    } else if (command && command[0]) {
        exec_argv[ai++] = "exec";
    } else {
        exec_argv[ai++] = "run";
    }

    /* Bind mounts */
    for (int i = 0; i < mounts->count; i++) {
        char *spec;
        if (asprintf(&spec, "%s:%s:%s", mounts->entries[i].path,
                     mounts->entries[i].path,
                     mounts->entries[i].readonly ? "ro" : "rw") < 0)
            continue;
        exec_argv[ai++] = "-B";
        exec_argv[ai++] = spec;
    }

    exec_argv[ai++] = image_path;

    if (!shell_mode && command && command[0]) {
        exec_argv[ai++] = "/bin/sh";
        exec_argv[ai++] = "-lc";
        exec_argv[ai++] = (char *)command;
    } else if (!shell_mode) {
        for (int i = 0; i < argc; i++)
            exec_argv[ai++] = argv[i];
    }

    exec_argv[ai] = NULL;

    execv(APPTAINER_BIN, exec_argv);
    perror("execv");
    return 1;
}
