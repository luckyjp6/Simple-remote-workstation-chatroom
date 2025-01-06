#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <poll.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <termios.h>

#include "util.hpp"
// #include <curses.h>

#define MY_LINE_MAX 15000 +100
#define MY_NAME_MAX 20 +10
#define WORM_PORT 8787

int sockfd;

void sig_int(int signo) {
    char buf[MY_LINE_MAX];
    fflush(stdin);

    write(1, "\n", 1);
    write(sockfd, "\n", 1);
    return;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Usage: ./worm_client <user name>@<server IP>" << std::endl;
        return 0;
    }

    signal(SIGINT, sig_int);

    char        *user_name, s_IP[20];
    sockaddr_in server_addr;
    int         nready;
    pollfd      p_fd[2];

    // get args
    char *server_IP = s_IP;
    user_name = strtok_r(argv[1], "@", &server_IP);
    // printf("%s\n", server_IP);
    
    // set socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(WORM_PORT);
    if (inet_pton(AF_INET, server_IP, &server_addr.sin_addr) < 0) err_sys("inet_pton");
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0) err_sys("connect");

    // set poll fd
    p_fd[0].fd = 0;
    p_fd[0].events = POLLIN;

    p_fd[1].fd = sockfd;
    p_fd[1].events = POLLIN;
    
    char buf[MY_LINE_MAX];
    // input user name
    read(sockfd, buf, MY_LINE_MAX);
    write(sockfd, user_name, std::min((int)strlen(user_name), MY_NAME_MAX));
    // write(sockfd, "\n", 1);

    // input passwd
    memset(buf, 0, sizeof(buf));
    termios old_attr, new_attr;
    tcgetattr(STDIN_FILENO, &old_attr);
    new_attr = old_attr;
    new_attr.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_attr);

    if (read(sockfd, buf, MY_LINE_MAX) <= 0) err_sys("read socket");
    if (write(1, buf, strlen(buf)) <= 0) err_sys("write 1");
    if (read(0, buf, MY_LINE_MAX) <= 0) err_sys("read 0");
    if (write(sockfd, buf, strlen(buf)) <= 0) err_sys("write socket");
    if (strstr(buf, "Wrong password") != NULL) return -1;
    if (write(1, "\n", 1) <= 0) err_sys("write \n");

    tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);

    for ( ; ; ) {
        nready = poll(p_fd, 2, -1);
        memset(buf, 0, sizeof(buf));

        // user input
        if (p_fd[0].revents & POLLIN) {
            // printf("input\n");
            int len = read(0, buf, MY_LINE_MAX);
            
            // error & EOF
            if (len < 0) err_sys("stdin");
            else if (len == 0) {
                write(1, "\n", 1);
                printf("len == 0\n");
                return 0;
            }

            // upload file
            // if (memcmp(buf, "worm_upload ", 12) == 0) {
            //     char *filename = buf;
            //     char *tmp = strtok_r(filename, " ", &filename);
            //     int fd = open(filename, O_RDONLY, 0);
            //     if (fd < 0) perror(filename);
            //     else {
            //         printf("upload %s\n", filename);
            //     }
            //     write(sockfd, "\n", 1);
            // }
            // else 
            write(sockfd, buf, len);
        } else if(p_fd[1].revents & POLLIN) {
            // printf("socket\n");
            int len = read(sockfd, buf, MY_LINE_MAX);

            // error & EOF
            if (len < 0) err_sys("remote");
            else if (len == 0) {
                write(1, "\n", 1);
                // printf("socket len == %d\n", strlen(buf));
                // if (write(sockfd, "\n", 1) < 0) err_sys("write socket nn");
                return 0;
            }

            // print result
            write(1, buf, len);
        }
    }
    return 0;
}