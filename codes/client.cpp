#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <poll.h>

#define MY_LINE_MAX 15000 +100

int sockfd;

void err_sys(const char *err) {
    perror(err);
    exit(-1);
}

void sig_int(int signo) {
    char buf[MY_LINE_MAX];
    fflush(stdin);
    // read(0, buf, MY_LINE_MAX);
    // std::cout << "clear: " << buf << std::endl;
    write(1, "\n", 1);
    write(sockfd, "\n", 1);
    return;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cout << "Usage: ./worm_client <server IP> <server port>" << std::endl;
        return 0;
    }

    signal(SIGINT, sig_int);

    sockaddr_in server_addr;
    int         nready;
    pollfd      p_fd[2];

    // set socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) < 0) err_sys("inet_pton");
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0) err_sys("connect");

    // set poll fd
    p_fd[0].fd = 0;
    p_fd[0].events = POLLRDNORM;

    p_fd[1].fd = sockfd;
    p_fd[1].events = POLLRDNORM;

    char buf[MY_LINE_MAX];
    for ( ; ; ) {
        nready = poll(p_fd, 2, -1);
        
        // user input
        if (p_fd[0].revents & POLLRDNORM) {
            int len = read(0, buf, MY_LINE_MAX);
            if (len < 0) err_sys("stdin");
            else if (len == 0) {
                write(1, "\n", 1);
                return 0;
            }
            write(sockfd, buf, len);
        } else if(p_fd[1].revents & POLLRDNORM) {
            int len = read(sockfd, buf, MY_LINE_MAX);
            if (len < 0) err_sys("remote");
            else if (len == 0) {
                write(1, "\n", 1);
                return 0;
            }
            write(1, buf, len);
        }
    }
    return 0;
}