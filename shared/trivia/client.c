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
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void help(char* program) {
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", program);
    printf("\n");
    printf("-i IP_address Default to \"127.0.0.1\";\n");
    printf("-p port_number Default to 25555;\n");
    printf("-h Display this help info.\n");
}

void parse_connect(int argc, char** argv, int* server_fd) {
    char* ip = "127.0.0.1";
    int port = 25555;
    int opt;
    struct sockaddr_in server_addr;

    opterr = 0;

    while ((opt = getopt(argc, argv, "i:p:h")) != -1) {
        if (opt == 'i') {
            ip = optarg;
        } else if (opt == 'p') {
            port = atoi(optarg);
        } else if (opt == 'h') {
            help(argv[0]);
            exit(0);
        } else {
            printf("Error: Unknown option '-%c' received.\n", optopt);
            exit(1);
        }
    }

    *server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (*server_fd == -1) {
        perror("socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(*server_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(*server_fd);
        exit(1);
    }
}

int main(int argc, char** argv) {
    int server_fd;
    int running = 1;

    parse_connect(argc, argv, &server_fd);

    while (running == 1) {
        fd_set readfds;
        int maxfd;

        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(server_fd, &readfds);

        maxfd = server_fd;

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            char buf[2048];
            int n = read(server_fd, buf, 2047);

            if (n <= 0) {
                running = 0;
            } else {
                buf[n] = '\0';
                printf("%s", buf);
                fflush(stdout);
            }
        }

        if (running == 1 && FD_ISSET(0, &readfds)) {
            char buf[256];
            int n = read(0, buf, 255);

            if (n <= 0) {
                running = 0;
            } else {
                buf[n] = '\0';
                write(server_fd, buf, strlen(buf));
            }
        }
    }

    close(server_fd);
    
    return 0;
}
