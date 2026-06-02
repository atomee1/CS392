/*
*   Name: William Ee
*   Course: CS 392
*   Date: Due 4/9/2026
*   Pledge: I pledge my honor that I have abided by the Stevens Honor System.
*/

// I used some basic standard library functions like atoi() and isdigit() to save time, hope that's fine

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

#define MAX_LINE 4096
#define MAX_ARGS 256

volatile sig_atomic_t interrupted = 0;

struct proc_info {
    int pid;
    uid_t uid;
    char* cmd;
};

static void handle_sigint(int signo) {
    (void)signo;
    interrupted = 1;
    write(STDOUT_FILENO, "\n", 1);
}

static int comp_procs(const void* a, const void* b) {
    const struct proc_info* pa = (const struct proc_info*)a;
    const struct proc_info* pb = (const struct proc_info*)b;

    return (pa->pid > pb->pid) - (pa->pid < pb->pid);
}

static int is_pid_name(const char* s) {
    int i;

    if (*s == '\0') {
        return 0;
    }

    for (i = 0; s[i] != '\0'; ++i) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }

    return 1;
}

static char* copy_string(const char* s) {
    char* copy = malloc(strlen(s) + 1);

    if (copy == NULL) {
        perror("malloc");

        return NULL;
    }

    strcpy(copy, s);

    return copy;
}

static const char* get_homedir(void) {
    struct passwd* pw = getpwuid(getuid());

    if (pw == NULL) {
        perror("getpwuid");

        return NULL;
    }

    return pw->pw_dir;
}

static char* expand_tilde(const char* path) {
    const char* home;
    char* result;

    if (path[0] != '~') {
        return copy_string(path);
    }

    home = get_homedir();

    if (home == NULL) {
        return NULL;
    }

    result = malloc(strlen(home) + strlen(path));
    if (result == NULL) {
        perror("malloc");

        return NULL;
    }

    strcpy(result, home);
    strcat(result, path + 1);

    return result;
}

static void print_prompt(void) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("%s[%s]%s> ", BLUE, cwd, DEFAULT);
    fflush(stdout);
}

static int read_line(char* line, char* argv[]) {
    int argc = 0;
    int i = 0;

    while (line[i] != '\0') {
        while (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r') {
            line[i] = '\0';
            ++i;
        }

        if (line[i] == '\0') {
            break;
        }

        if (argc >= MAX_ARGS - 1) {
            break;
        }

        argv[argc] = &line[i];
        ++argc;

        while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') {
            ++i;
        }
    }

    argv[argc] = NULL;

    return argc;
}

static void mini_pwd(void) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("%s\n", cwd);
}

static void mini_cd(char* argv[], int argc) {
    const char* home;
    char* expanded;

    if (argc == 1 || strcmp(argv[1], "~") == 0) {
        home = get_homedir();
        if (home != NULL && chdir(home) == -1) {
            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", home, strerror(errno));
        }
        return;
    }

    if (argc > 2) {
        fprintf(stderr, "Error: Too many arguments to cd.\n");
        return;
    }

    expanded = expand_tilde(argv[1]);
    if (expanded == NULL) {
        return;
    }

    if (chdir(expanded) == -1) {
        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", expanded, strerror(errno));
    }
    free(expanded);
}

static void mini_lf(void) {
    DIR* dir = opendir(".");
    struct dirent* entry;

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0) {
        perror("readdir");
    }
    if (closedir(dir) == -1) {
        perror("closedir");
    }
}

static size_t count_procs(void) {
    DIR* dir = opendir("/proc");
    struct dirent* entry;
    size_t count = 0;

    if (dir == NULL) {
        perror("opendir");
        return 0;
    }

    errno = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (is_pid_name(entry->d_name)) {
            ++count;
        }
    }

    if (errno != 0) {
        perror("readdir");
    }
    if (closedir(dir) == -1) {
        perror("closedir");
    }
    return count;
}

static char* read_cmdline(int pid) {
    char path[PATH_MAX], temp[PATH_MAX];
    int fd, i, num, path_pos = 0, digit_count = 0;
    ssize_t nread;
    const char prefix[] = "/proc/";
    char digits[32];

    for (i = 0; prefix[i] != '\0'; ++i) {
        path[path_pos] = prefix[i];
        ++path_pos;
    }

    num = pid;

    if (num == 0) {
        digits[digit_count++] = '0';
    } else {
        while (num > 0) {
            digits[digit_count++] = (char)('0' + (num % 10));
            num /= 10;
        }
    }

    while (digit_count > 0) {
        path[path_pos++] = digits[digit_count - 1];
        --digit_count;
    }

    path[path_pos++] = '/';
    path[path_pos++] = 'c';
    path[path_pos++] = 'm';
    path[path_pos++] = 'd';
    path[path_pos++] = 'l';
    path[path_pos++] = 'i';
    path[path_pos++] = 'n';
    path[path_pos++] = 'e';
    path[path_pos] = '\0';

    fd = open(path, O_RDONLY);

    if (fd == -1) {
        return copy_string("");
    }

    nread = read(fd, temp, sizeof(temp) - 1);

    if (nread == -1) {
        if (close(fd) == -1) {
            perror("close");
        }

        return copy_string("");
    }

    if (close(fd) == -1) {
        perror("close");
    }

    for (i = 0; i < nread; ++i) {
        if (temp[i] == '\0') {
            temp[i] = ' ';
        }
    }

    while (nread > 0 && temp[nread - 1] == ' ') {
        --nread;
    }

    temp[nread] = '\0';

    return copy_string(temp);
}

static void free_procs(struct proc_info* list, size_t count) {
    size_t i;

    if (list == NULL) {
        return;
    }

    for (i = 0; i < count; ++i) {
        free(list[i].cmd);
    }

    free(list);
}

static void mini_lp(void) {
    DIR* dir;
    struct dirent* entry;
    struct proc_info* list;
    size_t count = count_procs(), index = 0, i;

    list = malloc(count * sizeof(struct proc_info));

    if (count > 0 && list == NULL) {
        perror("malloc");

        return;
    }

    dir = opendir("/proc");

    if (dir == NULL) {
        perror("opendir");
        free(list);

        return;
    }

    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        char proc_path[PATH_MAX];
        char* cmd;
        int pid, p = 0, j;
        const char prefix[] = "/proc/";

        if (!is_pid_name(entry->d_name)) {
            continue;
        }

        for (j = 0; prefix[j] != '\0'; ++j) {
            proc_path[p++] = prefix[j];
        }

        for (j = 0; entry->d_name[j] != '\0'; ++j) {
            proc_path[p++] = entry->d_name[j];
        }

        proc_path[p] = '\0';

        if (stat(proc_path, &st) == -1) {
            continue;
        }

        pid = atoi(entry->d_name);
        cmd = read_cmdline(pid);

        if (cmd == NULL) {
            free_procs(list, index);

            if (closedir(dir) == -1) {
                perror("closedir");
            }

            return;
        }

        list[index].pid = pid;
        list[index].uid = st.st_uid;
        list[index].cmd = cmd;
        ++index;
    }

    if (errno != 0) {
        perror("readdir");
    }

    if (closedir(dir) == -1) {
        perror("closedir");
    }

    qsort(list, index, sizeof(struct proc_info), comp_procs);

    for (i = 0; i < index; ++i) {
        struct passwd* pw = getpwuid(list[i].uid);
        printf("%d %s %s\n", list[i].pid, pw ? pw->pw_name : "", list[i].cmd);
    }

    free_procs(list, index);
}

static void external_cmd(char* argv[]) {
    pid_t pid = fork();
    int status;
    struct sigaction sa;

    if (pid == -1) {
        perror("fork");

        return;
    }

    if (pid == 0) {
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;

        if (sigemptyset(&sa.sa_mask) == -1) {
            perror("sigemptyset");
            _exit(EXIT_FAILURE);
        }

        if (sigaction(SIGINT, &sa, NULL) == -1) {
            perror("sigaction");
            _exit(EXIT_FAILURE);
        }

        execvp(argv[0], argv);
        perror("execv");
        _exit(EXIT_FAILURE);
    }

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");

            break;
        }
    }
}

int main(void) {
    char line[MAX_LINE];
    char* argv[MAX_ARGS];
    int argc;
    struct sigaction sa;

    sa.sa_handler = handle_sigint;
    sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");

        return EXIT_FAILURE;
    }

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");

        return EXIT_FAILURE;
    }

    while (1) {
        interrupted = 0;
        print_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                break;
            }
            if (ferror(stdin)) {
                if (errno == EINTR || interrupted) {
                    clearerr(stdin);
                    continue;
                }
                perror("fgets");
                clearerr(stdin);
                continue;
            }
        }

        if (interrupted) {
            continue;
        }

        argc = read_line(line, argv);
        if (argc == 0) {
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            return EXIT_SUCCESS;
        } else if (strcmp(argv[0], "cd") == 0) {
            mini_cd(argv, argc);
        } else if (strcmp(argv[0], "pwd") == 0) {
            mini_pwd();
        } else if (strcmp(argv[0], "lf") == 0) {
            mini_lf();
        } else if (strcmp(argv[0], "lp") == 0) {
            mini_lp();
        } else {
            external_cmd(argv);
        }
    }

    return EXIT_SUCCESS;
}