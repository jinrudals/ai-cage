#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include "cJSON.h"

#define CONFIG_PATH "/etc/ai/config.json"
#define MAX_BINDS 256
#define MAX_NFS 256
#define PATH_MAX_LEN 4096

static int g_debug = 0;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}



static int is_debug_val(const char *v) {
    if (!v) return 0;
    return strcmp(v,"1")==0 || strcasecmp(v,"true")==0 ||
           strcasecmp(v,"yes")==0 || strcasecmp(v,"y")==0;
}

static int path_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

static int is_executable(const char *p) {
    return access(p, X_OK) == 0;
}

static int starts_with(const char *str, const char *prefix) {
    size_t plen = strlen(prefix);
    if (strncmp(str, prefix, plen) != 0) return 0;
    return str[plen] == '\0' || str[plen] == '/';
}

/* Read NFS mount targets via findmnt */
static int get_nfs_targets(char targets[][PATH_MAX_LEN], int max) {
    FILE *fp = popen("findmnt -rn -t nfs,nfs4 -o TARGET", "r");
    if (!fp) return 0;
    int n = 0;
    while (n < max && fgets(targets[n], PATH_MAX_LEN, fp)) {
        targets[n][strcspn(targets[n], "\n")] = '\0';
        if (targets[n][0]) n++;
    }
    pclose(fp);
    return n;
}

/* Read entire file into malloc'd buffer */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, len, f) != (size_t)len) { free(buf); fclose(f); return NULL; }
    buf[len] = '\0';
    fclose(f);
    return buf;
}

typedef struct {
    char *args[MAX_BINDS * 2]; /* pairs of "-B" and "src:src:mode" */
    int count;
    char *seen[MAX_BINDS];
    int seen_count;
} BindList;

static void bind_add(BindList *bl, const char *src, const char *mode) {
    if (!src || !src[0] || !path_exists(src)) {
        if (g_debug && src && src[0])
            fprintf(stderr, "WARN: skip bind: %s\n", src);
        return;
    }
    for (int i = 0; i < bl->seen_count; i++)
        if (strcmp(bl->seen[i], src) == 0) return;

    char *bind_spec;
    if (asprintf(&bind_spec, "%s:%s:%s", src, src, mode) < 0) return;
    bl->args[bl->count++] = strdup("-B");
    bl->args[bl->count++] = bind_spec;
    bl->seen[bl->seen_count++] = strdup(src);
}

int main(int argc, char **argv) {
    /* Env vars */
    const char *image_name = getenv("WRAPPED_AI_IMAGE_NAME");
    const char *debug_env = getenv("WRAPPED_AI_DEBUG");
    const char *wrap_cmd = getenv("WRAPPED_AI_COMMAND");
    g_debug = is_debug_val(debug_env);

    /* Parse flags */
    int shell_mode = 0;
    int arg_start = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--shell") == 0) { shell_mode = 1; arg_start = i + 1; }
        else if (strcmp(argv[i], "--") == 0) { arg_start = i + 1; break; }
        else break;
    }

    /* Validate */
    if (!image_name || !image_name[0])
        fail("WRAPPED_AI_IMAGE_NAME is not set");
    if (!path_exists(CONFIG_PATH))
        fail("config not found: " CONFIG_PATH);

    /* Get user home & workspace */
    struct passwd *pw = getpwuid(getuid());
    if (!pw) fail("cannot determine home directory");
    const char *user_home = pw->pw_dir;

    char workspace[PATH_MAX_LEN];
    if (!getcwd(workspace, sizeof(workspace)))
        fail("cannot get current directory");

    if (!starts_with(workspace, user_home))
        fail("workspace must be under your home directory");

    /* Parse config */
    char *json_str = read_file(CONFIG_PATH);
    if (!json_str) fail("cannot read config");
    cJSON *cfg = cJSON_Parse(json_str);
    if (!cfg) fail("invalid JSON in config");

    cJSON *apt = cJSON_GetObjectItem(cfg, "apptainer");
    if (!apt) fail("config missing 'apptainer'");
    const char *binary = cJSON_GetObjectItem(apt, "binary")->valuestring;
    const char *images = cJSON_GetObjectItem(apt, "images")->valuestring;

    char image_path[PATH_MAX_LEN];
    snprintf(image_path, sizeof(image_path), "%s/%s.sif", images, image_name);

    if (g_debug) {
        fprintf(stderr, "WRAPPED_AI_IMAGE_NAME=%s\n", image_name);
        fprintf(stderr, "APPTAINER=%s\n", binary);
        fprintf(stderr, "IMAGE=%s\n", image_path);
        fprintf(stderr, "WORKSPACE=%s\n", workspace);
    }

    if (!is_executable(binary))
        fail("apptainer not executable");
    if (!path_exists(image_path))
        fail("image not found (is WRAPPED_AI_IMAGE_NAME correct?)");

    /* NFS targets */
    static char nfs_targets[MAX_NFS][PATH_MAX_LEN];
    int nfs_count = get_nfs_targets(nfs_targets, MAX_NFS);

    /* Build bind mounts */
    BindList bl = { .count = 0, .seen_count = 0 };
    cJSON *mounts = cJSON_GetObjectItem(cfg, "mounts");
    if (mounts) {
        const char *sections[] = {"read-only", "read-write"};
        const char *modes[] = {"ro", "rw"};
        for (int s = 0; s < 2; s++) {
            cJSON *arr = cJSON_GetObjectItem(mounts, sections[s]);
            if (!arr) continue;
            cJSON *item;
            cJSON_ArrayForEach(item, arr) {
                cJSON *exact = cJSON_GetObjectItem(item, "exact-matched");
                cJSON *nfs_pre = cJSON_GetObjectItem(item, "NFS-mounts-starts-with");
                if (exact) {
                    bind_add(&bl, exact->valuestring, modes[s]);
                } else if (nfs_pre) {
                    for (int n = 0; n < nfs_count; n++) {
                        if (starts_with(nfs_targets[n], nfs_pre->valuestring))
                            bind_add(&bl, nfs_targets[n], modes[s]);
                    }
                }
            }
        }
    }

    if (g_debug) {
        fprintf(stderr, "Bind args (%d):\n", bl.count / 2);
        for (int i = 0; i < bl.count; i += 2)
            fprintf(stderr, "  %s %s\n", bl.args[i], bl.args[i+1]);
    }

    /* Build exec argv */
    /* Max: binary + subcmd + binds + image + extra args + NULL */
    int max_args = 4 + bl.count + (argc - arg_start) + 4;
    char **exec_argv = calloc(max_args, sizeof(char *));
    int ai = 0;

    exec_argv[ai++] = (char *)binary;

    if (shell_mode) {
        exec_argv[ai++] = "shell";
        for (int i = 0; i < bl.count; i++)
            exec_argv[ai++] = bl.args[i];
        exec_argv[ai++] = image_path;
    } else if (wrap_cmd && wrap_cmd[0]) {
        exec_argv[ai++] = "exec";
        for (int i = 0; i < bl.count; i++)
            exec_argv[ai++] = bl.args[i];
        exec_argv[ai++] = image_path;
        exec_argv[ai++] = "/bin/sh";
        exec_argv[ai++] = "-lc";
        exec_argv[ai++] = (char *)wrap_cmd;
    } else {
        exec_argv[ai++] = "run";
        for (int i = 0; i < bl.count; i++)
            exec_argv[ai++] = bl.args[i];
        exec_argv[ai++] = image_path;
        for (int i = arg_start; i < argc; i++)
            exec_argv[ai++] = argv[i];
    }
    exec_argv[ai] = NULL;

    if (g_debug) {
        fprintf(stderr, "exec:");
        for (int i = 0; exec_argv[i]; i++)
            fprintf(stderr, " %s", exec_argv[i]);
        fprintf(stderr, "\n");
    }

    execv(binary, exec_argv);
    perror("execv");
    return 1;
}
