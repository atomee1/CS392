/*
*   Name: William Ee
*   Course: CS 392
*   Date: Due 4/22/2026
*   Pledge: I pledge my honor that I have abided by the Stevens Honor System.
*/

// My tester's case 8 did not print the "Total files" line in stderr and this passes the case, so I'm assuming this is correct

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int full_write(int fd, const char* s, ssize_t n) {
    ssize_t done = 0;

    while (done < n) {
        ssize_t w = write(fd, s + done, n - done);

        if (w == -1) {
            perror("write");
            
            return -1;
        }

        done += w;
    }

    return 0;
}

static int cfd(int fd) {
    if (close(fd) == -1) {
        perror("close");
        
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    struct stat st;
    
    int p1[2];
    int p2[2];

    pid_t lpid;
    pid_t spid;

    char buf[4096] = {0};
    char out[64] = {0};

    ssize_t n = 0;

    int files = 0;
    int len = 0;
    int bad = 0;
    int status = 0;
    int i = 0;

    if (argc != 2) {
        fprintf(stderr, "The first argument has to be a directory.");

        return EXIT_FAILURE;
    }

    if (access(argv[1], R_OK) == -1) {
        fprintf(stderr, "Permission denied. %s cannot be read.", argv[1]);

        return EXIT_FAILURE;
    }

    if (stat(argv[1], &st) == -1) {
        perror("stat");

        return EXIT_FAILURE;
    }

    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "The first argument has to be a directory.");

        return EXIT_FAILURE;
    }

    if (pipe(p1) == -1) {
        perror("pipe");

        return EXIT_FAILURE;
    }

    if (pipe(p2) == -1) {
        perror("pipe");

        if (cfd(p1[0]) == -1) {
            bad = 1;
        }

        if (cfd(p1[1]) == -1) {
            bad = 1;
        }

        return EXIT_FAILURE;
    }

    lpid = fork();

    if (lpid == -1) {
        perror("fork");

        if (cfd(p1[0]) == -1) {
            bad = 1;
        }

        if (cfd(p1[1]) == -1) {
            bad = 1;
        }

        if (cfd(p2[0]) == -1) {
            bad = 1;
        }

        if (cfd(p2[1]) == -1) {
            bad = 1;
        }

        return EXIT_FAILURE;
    }

    if (lpid == 0) {
        if (dup2(p1[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        if (cfd(p1[0]) == -1) {
            bad = 1;
        }

        if (cfd(p1[1]) == -1) {
            bad = 1;
        }

        if (cfd(p2[0]) == -1) {
            bad = 1;
        }

        if (cfd(p2[1]) == -1) {
            bad = 1;
        }

        if (bad == 1) {
            _exit(EXIT_FAILURE);
        }

        execlp("ls", "ls", "-ai", argv[1], (char*) NULL);
        fprintf(stderr, "Error: ls failed.");
        _exit(EXIT_FAILURE);
    }

    spid = fork();

    if (spid == -1) {
        perror("fork");

        if (cfd(p1[0]) == -1) {
            bad = 1;
        }

        if (cfd(p1[1]) == -1) {
            bad = 1;
        }

        if (cfd(p2[0]) == -1) {
            bad = 1;
        }

        if (cfd(p2[1]) == -1) {
            bad = 1;
        }

        if (waitpid(lpid, NULL, 0) == -1) {
            perror("waitpid");
        }

        return EXIT_FAILURE;
    }

    if (spid == 0) {
        if (dup2(p1[0], STDIN_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        if (dup2(p2[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }

        if (cfd(p1[0]) == -1) {
            bad = 1;
        }

        if (cfd(p1[1]) == -1) {
            bad = 1;
        }

        if (cfd(p2[0]) == -1) {
            bad = 1;
        }

        if (cfd(p2[1]) == -1) {
            bad = 1;
        }

        if (bad == 1) {
            _exit(EXIT_FAILURE);
        }

        execlp("sort", "sort", "-k", "1", (char*) NULL);
        fprintf(stderr, "Error: sort failed.");
        _exit(EXIT_FAILURE);
    }

    if (cfd(p1[0]) == -1) {
        bad = 1;
    }

    if (cfd(p1[1]) == -1) {
        bad = 1;
    }

    if (cfd(p2[1]) == -1) {
        bad = 1;
    }

    while (bad != 1 && (n = read(p2[0], buf, sizeof(buf))) > 0) {
        for (i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                files++;
            }
        }

        if (full_write(STDOUT_FILENO, buf, n) == -1) {
            bad = 1;
        }
    }

    if (n == -1) {
        perror("read");
        bad = 1;
    }

    if (cfd(p2[0]) == -1) {
        bad = 1;
    }

    len = snprintf(out, sizeof(out), "Total files: %d\n", files);

    if (bad != 1 && (len < 0 || len >= (int) sizeof(out))) {
        fprintf(stderr, "Error: snprintf failed.");
        bad = 1;
    }

    if (bad != 1 && full_write(STDOUT_FILENO, out, len) == -1) {
        bad = 1;
    }

    if (waitpid(lpid, &status, 0) == -1) {
        perror("waitpid");
        bad = 1;
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        bad = 1;
    }

    if (waitpid(spid, &status, 0) == -1) {
        perror("waitpid");
        bad = 1;
    } else if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
        bad = 1;
    }

    if (bad == 1) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
