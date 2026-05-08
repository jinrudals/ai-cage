#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <linux/landlock.h>
#include "backend.h"

#ifndef LANDLOCK_ACCESS_FS_REFER
#define LANDLOCK_ACCESS_FS_REFER (1ULL << 13)
#endif
#ifndef LANDLOCK_ACCESS_FS_TRUNCATE
#define LANDLOCK_ACCESS_FS_TRUNCATE (1ULL << 14)
#endif

#define ACCESS_FS_ROUGHLY_READ ( \
    LANDLOCK_ACCESS_FS_EXECUTE | \
    LANDLOCK_ACCESS_FS_READ_FILE | \
    LANDLOCK_ACCESS_FS_READ_DIR)

#define ACCESS_FS_ROUGHLY_WRITE ( \
    LANDLOCK_ACCESS_FS_WRITE_FILE | \
    LANDLOCK_ACCESS_FS_REMOVE_DIR | \
    LANDLOCK_ACCESS_FS_REMOVE_FILE | \
    LANDLOCK_ACCESS_FS_MAKE_CHAR | \
    LANDLOCK_ACCESS_FS_MAKE_DIR | \
    LANDLOCK_ACCESS_FS_MAKE_REG | \
    LANDLOCK_ACCESS_FS_MAKE_SOCK | \
    LANDLOCK_ACCESS_FS_MAKE_FIFO | \
    LANDLOCK_ACCESS_FS_MAKE_BLOCK | \
    LANDLOCK_ACCESS_FS_MAKE_SYM | \
    LANDLOCK_ACCESS_FS_REFER | \
    LANDLOCK_ACCESS_FS_TRUNCATE)

static int landlock_create_ruleset(const struct landlock_ruleset_attr *attr,
                                   size_t size, __u32 flags) {
    return syscall(__NR_landlock_create_ruleset, attr, size, flags);
}

static int landlock_add_rule(int fd, enum landlock_rule_type type,
                             const void *attr, __u32 flags) {
    return syscall(__NR_landlock_add_rule, fd, type, attr, flags);
}

static int landlock_restrict_self(int fd, __u32 flags) {
    return syscall(__NR_landlock_restrict_self, fd, flags);
}

static int add_path_rule(int ruleset_fd, const char *path, __u64 allowed) {
    int fd = open(path, O_PATH | O_CLOEXEC);
    if (fd < 0) return -1; /* skip non-existent paths */

    struct landlock_path_beneath_attr attr = {
        .allowed_access = allowed,
        .parent_fd = fd,
    };
    int ret = landlock_add_rule(ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &attr, 0);
    close(fd);
    return ret;
}

int backend_run(const char *workspace, const MountList *mounts,
                int argc, char **argv, const char *command,
                const char *image_name, int shell_mode) {
    (void)image_name;
    (void)shell_mode;

    /* Check ABI */
    int abi = syscall(__NR_landlock_create_ruleset, NULL, 0,
                      LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 0) {
        fprintf(stderr, "FAIL: Landlock not supported on this kernel\n");
        return 1;
    }

    /* Determine handled access based on ABI version */
    __u64 fs_access = ACCESS_FS_ROUGHLY_READ | ACCESS_FS_ROUGHLY_WRITE;
    if (abi < 2) fs_access &= ~LANDLOCK_ACCESS_FS_REFER;
    if (abi < 3) fs_access &= ~LANDLOCK_ACCESS_FS_TRUNCATE;

    struct landlock_ruleset_attr ruleset_attr = {
        .handled_access_fs = fs_access,
    };

    int ruleset_fd = landlock_create_ruleset(&ruleset_attr, sizeof(ruleset_attr), 0);
    if (ruleset_fd < 0) {
        perror("landlock_create_ruleset");
        return 1;
    }

    /* Workspace: full access */
    if (add_path_rule(ruleset_fd, workspace, fs_access) < 0) {
        perror("landlock: add workspace rule");
        close(ruleset_fd);
        return 1;
    }

    /* Config mounts */
    for (int i = 0; i < mounts->count; i++) {
        __u64 access = ACCESS_FS_ROUGHLY_READ;
        if (!mounts->entries[i].readonly)
            access |= ACCESS_FS_ROUGHLY_WRITE;
        /* Adjust for ABI */
        if (!mounts->entries[i].readonly) {
            if (abi < 2) access &= ~LANDLOCK_ACCESS_FS_REFER;
            if (abi < 3) access &= ~LANDLOCK_ACCESS_FS_TRUNCATE;
        }
        add_path_rule(ruleset_fd, mounts->entries[i].path, access);
    }

    /* Home directory: full access for shell config, dotfiles, etc. */
    const char *home = getenv("HOME");
    if (home && add_path_rule(ruleset_fd, home, fs_access) < 0) {
        perror("landlock: add home rule");
    }

    /* Always allow read on common system paths */
    const char *sys_ro[] = {"/usr", "/lib", "/lib64", "/bin", "/sbin",
                            "/etc", "/proc", "/sys", "/var", "/mnt", NULL};
    for (int i = 0; sys_ro[i]; i++)
        add_path_rule(ruleset_fd, sys_ro[i], ACCESS_FS_ROUGHLY_READ);

    /* /dev needs write access for /dev/null, /dev/tty, etc. */
    add_path_rule(ruleset_fd, "/dev", ACCESS_FS_ROUGHLY_READ | LANDLOCK_ACCESS_FS_WRITE_FILE);

    /* /run needs write for socket connections (DNS, D-Bus, etc.) */
    add_path_rule(ruleset_fd, "/run", ACCESS_FS_ROUGHLY_READ | LANDLOCK_ACCESS_FS_WRITE_FILE);

    /* /tmp full access */
    add_path_rule(ruleset_fd, "/tmp", fs_access);

    /* Enforce */
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
        perror("prctl(NO_NEW_PRIVS)");
        close(ruleset_fd);
        return 1;
    }
    if (landlock_restrict_self(ruleset_fd, 0)) {
        perror("landlock_restrict_self");
        close(ruleset_fd);
        return 1;
    }
    close(ruleset_fd);

    /* Exec the command */
    if (command && command[0]) {
        execlp("/bin/sh", "sh", "-lc", command, (char *)NULL);
    } else if (argc > 0) {
        execvp(argv[0], argv);
    } else if (image_name && image_name[0]) {
        execlp(image_name, image_name, (char *)NULL);
    } else {
        const char *shell = getenv("SHELL");
        if (!shell) shell = "/bin/sh";
        execlp(shell, shell, "-l", (char *)NULL);
    }

    perror("exec");
    return 1;
}
