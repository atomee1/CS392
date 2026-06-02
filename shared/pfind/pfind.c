/*
*   Name: William Ee
*   Course: CS 392
*   Date: Due 3/22/2026
*   Pledge: I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int valid_pstring(const char* pstring) {
    const char expected[9] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};

    if (strlen(pstring) != 9) {
        return 0;
    }

    for (int i = 0; i < 9; i++) {
        if (pstring[i] != '-' && pstring[i] != expected[i]) {
            return 0;
        }
    }

    return 1;
}

static int perms_match(mode_t mode, const char* pstring) {
    if (((mode & S_IRUSR) != 0) != (pstring[0] == 'r')) {
        return 0;
    }
    if (((mode & S_IWUSR) != 0) != (pstring[1] == 'w')) {
        return 0;
    }
    if (((mode & S_IXUSR) != 0) != (pstring[2] == 'x')) {
        return 0;
    }
    if (((mode & S_IRGRP) != 0) != (pstring[3] == 'r')) {
        return 0;
    }
    if (((mode & S_IWGRP) != 0) != (pstring[4] == 'w')) {
        return 0;
    }
    if (((mode & S_IXGRP) != 0) != (pstring[5] == 'x')) {
        return 0;
    }
    if (((mode & S_IROTH) != 0) != (pstring[6] == 'r')) {
        return 0;
    }
    if (((mode & S_IWOTH) != 0) != (pstring[7] == 'w')) {
        return 0;
    }
    if (((mode & S_IXOTH) != 0) != (pstring[8] == 'x')) {
        return 0;
    }

    return 1;
}

static char* join_path(const char* parent, const char* child) {
    size_t parent_len;
    size_t child_len;
    size_t total_len;
    char* path;

    int need_slash;

    parent_len = strlen(parent);
    child_len = strlen(child);

    if (parent_len > 0 && parent[parent_len - 1] != '/') {
        need_slash = 1;
    } else {
        need_slash = 0;
    }

    total_len = parent_len + need_slash + child_len + 1;
    path = malloc(total_len);

    if (path == NULL) {
        perror("malloc");

        exit(EXIT_FAILURE);
    }

    strcpy(path, parent);
    if (need_slash) {
        strcat(path, "/");
    }
    strcat(path, child);

    return path;
}

static char* make_abspath(const char* dir) {
    char cwd[4096];
    char* path;

    if (dir[0] == '/') {
        path = malloc(strlen(dir) + 1);
        if (path == NULL) {
            perror("malloc");

            exit(EXIT_FAILURE);
        }

        strcpy(path, dir);

        return path;
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");

        exit(EXIT_FAILURE);
    }

    return join_path(cwd, dir);
}

static void search_dir(const char* dirpath, const char* pstring) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir(dirpath);

    if (dir == NULL) {
        if (errno == EACCES) {
            fprintf(stderr, "Error: Cannot open directory '%s'. Permission denied.\n", dirpath);

            return;
        }

        perror("opendir");

        exit(EXIT_FAILURE);
    }

    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        char* full_path;
        struct stat sb;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        full_path = join_path(dirpath, entry->d_name);

        if (stat(full_path, &sb) == -1) {
            perror("stat");
            free(full_path);
            closedir(dir);

            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(sb.st_mode)) {
            search_dir(full_path, pstring);
        } else if (S_ISREG(sb.st_mode) && perms_match(sb.st_mode, pstring)) {
            printf("%s\n", full_path);
        }

        free(full_path);
    }

    if (errno != 0) {
        perror("readdir");
        closedir(dir);

        exit(EXIT_FAILURE);
    }

    if (closedir(dir) == -1) {
        perror("closedir");

        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    char* root_path;

    if (!valid_pstring(argv[2])) {
        fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", argv[2]);

        return EXIT_FAILURE;
    }

    root_path = make_abspath(argv[1]);
    search_dir(root_path, argv[2]);
    free(root_path);

    return EXIT_SUCCESS;
}
