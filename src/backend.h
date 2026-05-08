#ifndef BACKEND_H
#define BACKEND_H

#define MAX_BINDS 256
#define PATH_MAX_LEN 4096

typedef struct {
    const char *path;
    int readonly; /* 1 = ro, 0 = rw */
} MountEntry;

typedef struct {
    MountEntry entries[MAX_BINDS];
    int count;
} MountList;

/* Each backend implements this function.
 * workspace: current working directory (always rw)
 * mounts: parsed mount policy from config.json
 * argc/argv: remaining args to pass to the AI tool
 * command: WRAPPED_AI_COMMAND (NULL if not set)
 * image_name: WRAPPED_AI_IMAGE_NAME (may be NULL for landlock)
 * shell_mode: 1 if --shell was passed
 */
int backend_run(const char *workspace, const MountList *mounts,
                int argc, char **argv, const char *command,
                const char *image_name, int shell_mode);

#endif /* BACKEND_H */
