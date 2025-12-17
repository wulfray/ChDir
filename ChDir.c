/* ChDir
*  Author: Ray Wulf.
*  Language: The mighty and powerful C.
*/

/* Initial usage requires FULL PATH, I designed this
* to keep a history of directories you have used before
* and allow you to use a short name to return to them.
*/

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct history_entry {
    char *key;
    char *path;
};

static void free_history(struct history_entry *entries, size_t count) {
    if (entries == NULL) {
        return;
    }

    for (size_t i = 0; i < count; ++i) {
        free(entries[i].key);
        free(entries[i].path);
    }

    free(entries);
}

static char *build_history_path(void) {
    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        return NULL;
    }

    const char suffix[] = "/.chdir_history";
    size_t len = strlen(home) + sizeof(suffix);
    char *path = malloc(len);
    if (path == NULL) {
        return NULL;
    }

    snprintf(path, len, "%s%s", home, suffix);
    return path;
}

static int load_history(const char *history_path, struct history_entry **entries_out, size_t *count_out) {
    *entries_out = NULL;
    *count_out = 0;

    if (history_path == NULL) {
        return 0;
    }

    FILE *file = fopen(history_path, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            return 0;
        }
        fprintf(stderr, "Warning: unable to open history file '%s': %s\n", history_path, strerror(errno));
        return -1;
    }

    struct history_entry *entries = NULL;
    size_t count = 0;
    size_t capacity = 0;
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, file)) != -1) {
        if (linelen == 0) {
            continue;
        }

        char *tab = strchr(line, '\t');
        if (tab == NULL) {
            continue;
        }

        *tab = '\0';
        char *path = tab + 1;
        char *newline = strchr(path, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        if (path[0] == '\0') {
            continue;
        }

        char *key_copy = strdup(line);
        char *path_copy = strdup(path);
        if (key_copy == NULL || path_copy == NULL) {
            free(key_copy);
            free(path_copy);
            continue;
        }

        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 8 : capacity * 2;
            struct history_entry *tmp = realloc(entries, new_capacity * sizeof(*entries));
            if (tmp == NULL) {
                free(key_copy);
                free(path_copy);
                break;
            }
            entries = tmp;
            capacity = new_capacity;
        }

        entries[count].key = key_copy;
        entries[count].path = path_copy;
        count++;
    }

    free(line);
    fclose(file);

    *entries_out = entries;
    *count_out = count;
    return 0;
}

static const char *lookup_history(const struct history_entry *entries, size_t count, const char *key) {
    for (size_t i = count; i > 0; --i) {
        const struct history_entry *entry = &entries[i - 1];
        if (strcmp(entry->key, key) == 0) {
            return entry->path;
        }
    }

    return NULL;
}

static void remember_directory(const char *history_path, const char *key, const char *resolved_path) {
    if (history_path == NULL || key == NULL || key[0] == '\0') {
        return;
    }

    FILE *file = fopen(history_path, "a");
    if (file == NULL) {
        fprintf(stderr, "Warning: unable to update history file '%s': %s\n", history_path, strerror(errno));
        return;
    }

    if (fprintf(file, "%s\t%s\n", key, resolved_path) < 0) {
        fprintf(stderr, "Warning: failed to write to history file '%s': %s\n", history_path, strerror(errno));
    }

    fclose(file);
}

static void remember_aliases(const char *history_path, const char *key, const char *resolved_path) {
    remember_directory(history_path, key, resolved_path);

    if (resolved_path == NULL) {
        return;
    }

    const char *slash = strrchr(resolved_path, '/');
    const char *alias = slash != NULL ? slash + 1 : resolved_path;
    if (alias == NULL || alias[0] == '\0') {
        return;
    }

    if (key != NULL && strcmp(alias, key) == 0) {
        return;
    }

    remember_directory(history_path, alias, resolved_path);
}

static const char *resolve_target_directory(int argc, char *argv[],
                                            const struct history_entry *entries,
                                            size_t entry_count,
                                            char **allocated_result,
                                            const char *history_path) {
    if (argc > 1 && argv[1][0] != '\0') {
        const char *saved = lookup_history(entries, entry_count, argv[1]);
        if (saved != NULL) {
            return saved;
        }

        char *resolved = realpath(argv[1], NULL);
        if (resolved == NULL) {
            fprintf(stderr, "Failed to resolve directory '%s': %s\n", argv[1], strerror(errno));
            return NULL;
        }

        remember_aliases(history_path, argv[1], resolved);
        *allocated_result = resolved;
        return resolved;
    }

    const char *home = getenv("HOME");
    if (home == NULL || home[0] == '\0') {
        return NULL;
    }

    return home;
}

static const char *resolve_shell(void) {
    const char *shell = getenv("SHELL");
    if (shell == NULL || shell[0] == '\0') {
        return "/bin/bash";
    }

    return shell;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s <directory>\n", prog);
}

int main(int argc, char *argv[]) {
    char *history_path = build_history_path();
    struct history_entry *history = NULL;
    size_t history_count = 0;

    if (load_history(history_path, &history, &history_count) != 0) {
        free(history_path);
        return EXIT_FAILURE;
    }

    char *resolved = NULL;
    int has_argument = (argc > 1 && argv[1][0] != '\0') ? 1 : 0;
    const char *target = resolve_target_directory(argc, argv, history, history_count, &resolved, history_path);
    if (target == NULL) {
        if (!has_argument) {
            print_usage(argv[0]);
            fprintf(stderr, "Error: target directory not provided and HOME is unset.\n");
        }
        free(resolved);
        free_history(history, history_count);
        free(history_path);
        return EXIT_FAILURE;
    }

    if (chdir(target) != 0) {
        fprintf(stderr, "Failed to change directory to '%s': %s\n", target, strerror(errno));
        free(resolved);
        free_history(history, history_count);
        free(history_path);
        return EXIT_FAILURE;
    }

    printf("Changed directory to %s\n", target);
    fflush(stdout);

    const char *shell = resolve_shell();
    execl(shell, shell, (char *)NULL);

    fprintf(stderr, "Failed to start shell '%s': %s\n", shell, strerror(errno));
    free(resolved);
    free_history(history, history_count);
    free(history_path);
    return EXIT_FAILURE;
}
