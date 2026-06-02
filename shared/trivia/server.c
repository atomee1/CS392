/*
*   Name: William Ee
*   Course: CS 392
*   Date: Due 5/7/2026
*   Pledge: I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct Entry {
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};

struct Player {
    int fd;
    int score;
    char name[128];
};

int read_file_line(int fd, char *buf, int size) {
    int i = 0;
    int bytes;
    char c;

    while (i < size - 1) {
        bytes = read(fd, &c, 1);

        if (bytes == 0) {
            if (i == 0) {
                return -1;
            }

            break;
        }

        if (bytes < 0) {
            return -1;
        }

        if (c == '\n') {
            break;
        }

        buf[i] = c;
        i++;
    }

    buf[i] = '\0';

    return i;
}

void help(char* program) {
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n", program);
    printf("\n");
    printf("-f question_file Default to \"qshort.txt\";\n");
    printf("-i IP_address Default to \"127.0.0.1\";\n");
    printf("-p port_number Default to 25555;\n");
    printf("-h Display this help info.\n");
}

int read_questions(struct Entry* arr, char* filename) {
    int fd, result;
    int count = 0;
    char opt_line[256];
    char answer[50];

    fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while (count < 50) {
        result = read_file_line(fd, arr[count].prompt, 1024);

        if (result == -1) {
            break;
        }

        if (arr[count].prompt[0] == '\0') {
            continue;
        }

        read_file_line(fd, opt_line, 256);

        sscanf(opt_line, "%s %s %s", arr[count].options[0], arr[count].options[1], arr[count].options[2]);

        read_file_line(fd, answer, 50);

        if (strcmp(answer, arr[count].options[0]) == 0) {
            arr[count].answer_idx = 0;
        } else if (strcmp(answer, arr[count].options[1]) == 0) {
            arr[count].answer_idx = 1;
        } else {
            arr[count].answer_idx = 2;
        }

        count++;
    }

    close(fd);

    return count;
}

int read_line(int fd, char* buf, int size) {
    int i = 0;
    char c;
    int n;

    while (i < size - 1) {
        n = read(fd, &c, 1);

        if (n <= 0) {
            return n;
        }

        if (c == '\n') {
            break;
        }

        if (c != '\r') {
            buf[i] = c;
            i++;
        }
    }

    buf[i] = '\0';

    return i;
}

void write_players(struct Player players[], char* message) {
    int i;

    for (i = 0; i < 2; i++) {
        write(players[i].fd, message, strlen(message));
    }
}

void close_all(int server_fd, struct Player players[]) {
    int i;

    for (i = 0; i < 2; i++) {
        if (players[i].fd > 0) {
            close(players[i].fd);
        }
    }

    if (server_fd > 0) {
        close(server_fd);
    }
}

void lost(int server_fd, struct Player players[]) {
    printf("Lost connection!\n");
    close_all(server_fd, players);
    exit(1);
}

int make_server(char* ip, int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_fd, (struct sockaddr* ) &server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 2) == -1) {
        perror("listen");
        close(server_fd);
        exit(1);
    }

    printf("Welcome to 392 Trivia!\n");

    return server_fd;
}

void get_players(int server_fd, struct Player players[]) {
    int connected = 0;
    int named = 0;
    int i;

    for (i = 0; i < 2; i++) {
        players[i].fd = -1;
        players[i].score = 0;
        players[i].name[0] = '\0';
    }

    while (named < 2) {
        fd_set readfds;
        int maxfd = server_fd;

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        for (i = 0; i < 2; i++) {
            if (players[i].fd != -1 && players[i].name[0] == '\0') {
                FD_SET(players[i].fd, &readfds);

                if (players[i].fd > maxfd) {
                    maxfd = players[i].fd;
                }
            }
        }

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd;
            struct sockaddr_in in_addr;
            socklen_t addr_size = sizeof(in_addr);

            new_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);

            if (connected == 2) {
                printf("Max connection reached!\n");
                close(new_fd);
            } else {
                printf("New connection detected!\n");
                players[connected].fd = new_fd;
                write(new_fd, "Please type your name: ", strlen("Please type your name: "));
                connected++;
            }
        }

        for (i = 0; i < 2; i++) {
            if (players[i].fd != -1 && players[i].name[0] == '\0' && FD_ISSET(players[i].fd, &readfds)) {
                int n = read_line(players[i].fd, players[i].name, 128);

                if (n <= 0) {
                    lost(server_fd, players);
                }

                printf("Hi %s!\n", players[i].name);
                named++;
            }
        }
    }

    printf("The game starts now!\n");
}

void write_questions(int server_fd, struct Player players[], struct Entry questions[], int question_count) {
    int q;
    int i;

    for (q = 0; q < question_count; q++) {
        char server_msg[1400];
        char client_msg[1400];
        char answer_msg[100];
        int answered = 0;

        sprintf(server_msg, "Question %d: %s\n1: %s\n2: %s\n3: %s\n",
                q + 1, questions[q].prompt,
                questions[q].options[0], questions[q].options[1], questions[q].options[2]);

        sprintf(client_msg, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n",
                q + 1, questions[q].prompt,
                questions[q].options[0], questions[q].options[1], questions[q].options[2]);

        printf("%s", server_msg);
        write_players(players, client_msg);

        while (answered == 0) {
            fd_set readfds;
            int maxfd = players[0].fd;

            FD_ZERO(&readfds);

            for (i = 0; i < 2; i++) {
                FD_SET(players[i].fd, &readfds);

                if (players[i].fd > maxfd) {
                    maxfd = players[i].fd;
                }
            }

            select(maxfd + 1, &readfds, NULL, NULL, NULL);

            for (i = 0; i < 2; i++) {
                if (FD_ISSET(players[i].fd, &readfds)) {
                    char buf[32];
                    int n = read_line(players[i].fd, buf, 32);
                    int choice;

                    if (n <= 0) {
                        lost(server_fd, players);
                    }

                    choice = buf[0] - '1';

                    if (choice == questions[q].answer_idx) {
                        players[i].score++;
                    } else {
                        players[i].score--;
                    }

                    sprintf(answer_msg, "Correct answer: %s\n", questions[q].options[questions[q].answer_idx]);
                    printf("%s", answer_msg);
                    write_players(players, answer_msg);
                    answered = 1;

                    break;
                }
            }
        }
    }
}

void show_winners(struct Player players[]) {
    int high = players[0].score;
    int i;
    char msg[200];

    for (i = 1; i < 2; i++) {
        if (players[i].score > high) {
            high = players[i].score;
        }
    }

    for (i = 0; i < 2; i++) {
        if (players[i].score == high) {
            sprintf(msg, "Congrats, %s!\n", players[i].name);
            printf("%s", msg);
            write_players(players, msg);
        }
    }
}

int main(int argc, char** argv) {
    struct Entry questions[50];
    struct Player players[2];
    char* question_file = "qshort.txt";
    char* ip = "127.0.0.1";
    int port = 25555;
    int opt;
    int server_fd;
    int question_count;

    opterr = 0;

    while ((opt = getopt(argc, argv, "f:i:p:h")) != -1) {
        if (opt == 'f') {
            question_file = optarg;
        } else if (opt == 'i') {
            ip = optarg;
        } else if (opt == 'p') {
            port = atoi(optarg);
        } else if (opt == 'h') {
            help(argv[0]);

            return 0;
        } else {
            printf("Error: Unknown option '-%c' received.\n", optopt);
            
            return 1;
        }
    }

    question_count = read_questions(questions, question_file);
    server_fd = make_server(ip, port);

    get_players(server_fd, players);
    write_questions(server_fd, players, questions, question_count);
    show_winners(players);
    close_all(server_fd, players);

    return 0;
}
