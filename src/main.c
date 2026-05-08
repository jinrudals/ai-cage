#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "backend.h"

#ifndef CONFIG_DIR
#define CONFIG_DIR "/etc/ai"
#endif
#define CONFIG_PATH CONFIG_DIR "/config.json"

#define MAX_NFS 256

static int g_debug = 0;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static int is_debug_val(const char *v) {
    if (!v) return 0;
    return strcmp(v, "1") == 0 || strcasecmp(v, "true") == 0 ||
           strcasecmp(v, "yes") == 0 || strcasecmp(v, "y") == 0;
}

static int path_exists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0;
}

static int starts_with(const char *str, const char *prefix) {
    size_t plen = strlen(prefix);
    if (strncmp(str, prefix, plen) != 0) return 0;
    return str[plen] == '\0' || str[plen] == '/';
}

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

static void mount_add(MountList *ml, const char *path, int readonly) {
    if (!path || !path[0] || !path_exists(path)) return;
    /* dedup */
    for (int i = 0; i < ml->count; i++)
        if (strcmp(ml->entries[i].path, path) == 0) return;
    if (ml->count >= MAX_BINDS) return;
    ml->entries[ml->count].path = strdup(path);
    ml->entries[ml->count].readonly = readonly;
    ml->count++;
}

/* Expand $USER in path string. Returns static buffer. */
static const char *expand_user_variable(const char *path, const char *username) {
    static char buf[PATH_MAX_LEN];
    const char *pos = strstr(path, "$USER");
    if (!pos) {
        snprintf(buf, sizeof(buf), "%s", path);
        return buf;
    }
    size_t prefix_len = pos - path;
    snprintf(buf, sizeof(buf), "%.*s%s%s",
             (int)prefix_len, path, username, pos + 5);
    return buf;
}

/* Check if workspace (cwd) is allowed by home or configured workspaces. */
static int is_workspace_allowed(const char *cwd, const char *home,
                                cJSON *ws_array, const char *username) {
    if (starts_with(cwd, home)) return 1;
    if (!ws_array) return 0;
    cJSON *item;
    cJSON_ArrayForEach(item, ws_array) {
        if (!cJSON_IsString(item)) continue;
        const char *expanded = expand_user_variable(item->valuestring, username);
        if (starts_with(cwd, expanded)) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
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

    /* Workspace */
    struct passwd *pw = getpwuid(getuid());
    if (!pw) fail("cannot determine home directory");
    const char *user_home = pw->pw_dir;

    char workspace[PATH_MAX_LEN];
    if (!getcwd(workspace, sizeof(workspace)))
        fail("cannot get current directory");

    /* Parse config */
    if (!path_exists(CONFIG_PATH))
        fail("config not found: " CONFIG_PATH);

    char *json_str = read_file(CONFIG_PATH);
    if (!json_str) fail("cannot read config");
    cJSON *cfg = cJSON_Parse(json_str);
    if (!cfg) fail("invalid JSON in config");

    /* Parse workspaces */
    cJSON *ws_array = cJSON_GetObjectItem(cfg, "workspaces");

    /* Validate workspace */
    if (!is_workspace_allowed(workspace, user_home, ws_array, pw->pw_name))
        fail("workspace must be under your home directory or a configured workspace path");

    /* NFS targets */
    static char nfs_targets[MAX_NFS][PATH_MAX_LEN];
    int nfs_count = get_nfs_targets(nfs_targets, MAX_NFS);

    /* Build mount list */
    MountList mounts = { .count = 0 };
    cJSON *mounts_json = cJSON_GetObjectItem(cfg, "mounts");
    if (mounts_json) {
        const char *sections[] = {"read-only", "read-write"};
        const int ro_flags[] = {1, 0};
        for (int s = 0; s < 2; s++) {
            cJSON *arr = cJSON_GetObjectItem(mounts_json, sections[s]);
            if (!arr) continue;
            cJSON *item;
            cJSON_ArrayForEach(item, arr) {
                cJSON *exact = cJSON_GetObjectItem(item, "exact-matched");
                cJSON *nfs_pre = cJSON_GetObjectItem(item, "NFS-mounts-starts-with");
                if (exact) {
                    mount_add(&mounts, exact->valuestring, ro_flags[s]);
                } else if (nfs_pre) {
                    for (int n = 0; n < nfs_count; n++) {
                        if (starts_with(nfs_targets[n], nfs_pre->valuestring))
                            mount_add(&mounts, nfs_targets[n], ro_flags[s]);
                    }
                }
            }
        }
    }

    if (g_debug) {
        fprintf(stderr, "WORKSPACE=%s\n", workspace);
        if (ws_array && cJSON_IsArray(ws_array)) {
            int ws_count = cJSON_GetArraySize(ws_array);
            fprintf(stderr, "Workspaces (%d):\n", ws_count);
            cJSON *ws_item;
            cJSON_ArrayForEach(ws_item, ws_array) {
                if (cJSON_IsString(ws_item))
                    fprintf(stderr, "  %s -> %s\n", ws_item->valuestring,
                            expand_user_variable(ws_item->valuestring, pw->pw_name));
            }
        }
        fprintf(stderr, "Mounts (%d):\n", mounts.count);
        for (int i = 0; i < mounts.count; i++)
            fprintf(stderr, "  %s [%s]\n", mounts.entries[i].path,
                    mounts.entries[i].readonly ? "ro" : "rw");
    }

    /* Delegate to backend */
    return backend_run(workspace, &mounts, argc - arg_start, argv + arg_start,
                       wrap_cmd, image_name, shell_mode);
}
