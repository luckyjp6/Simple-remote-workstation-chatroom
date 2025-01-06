#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include <vector>
#include <string.h>
#include <termios.h>

#include <curses.h>

#include "util.hpp"

#define MY_LINE_MAX 15000 +100
#define WORM_PORT 8787

// std::vector<std::string> cmds;

void err_sys(const char *err) {
    perror(err);
    exit(-1);
}

void err_args() {
    std::cout << "Upload: ./worm_cp <src_file> <user_name@server_IP>:<dst_file>" << std::endl;
    std::cout << "Download: ./worm_cp <user_name@server_IP>:<src_file> <dst_file>" << std::endl;
    exit(-1);
}

int main(int argc, char **argv) {
    if (argc < 3) err_args();

    int local_fd, sockfd;
    bool is_local[2];
    std::string user_name, server_IP, path[2];
    sockaddr_in server_addr;

    /* get args */
    for (int i = 0; i < 2; i++) {
        std::string s(argv[i+1]);
        size_t colon = s.find(":");

        is_local[i] = (colon == std::string::npos);
        if (is_local[i]) {
            path[i] = s;
        } else {
            path[i] = s.substr(colon+1);
            s = s.substr(0, colon);
            size_t at = s.find("@", 0);

            if (at == std::string::npos) err_args();
            user_name = s.substr(0, at);
            server_IP = s.substr(at+1);
        }

        // std::cout << path[i] << std::endl;
    }

    if (is_local[0] == is_local[1]) err_args(); // both local or both remote
    
    /* if upload, get local file */
    if (is_local[0]) {
        local_fd = open(path[0].c_str(), O_RDONLY);    
        if (local_fd < 0) err_sys(path[is_local[1]].c_str());
    }

    /* set socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(WORM_PORT);
    if (inet_pton(AF_INET, server_IP.c_str(), &server_addr.sin_addr) < 0) err_sys("inet_pton");
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0) err_sys("connect");

    int len;
    char buf[MY_LINE_MAX];

    /* input user name */
    user_name = LOAD_FILE_KEYWORD + user_name + "\n";
    read(sockfd, buf, MY_LINE_MAX);
    write(sockfd, user_name.c_str(), user_name.size());
    
    /* input passwd */
    termios old_attr, new_attr;
    tcgetattr(STDIN_FILENO, &old_attr);
    new_attr = old_attr;
    new_attr.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_attr);

    read(sockfd, buf, MY_LINE_MAX);
    write(0, buf, strlen(buf));
    read(0, buf, MY_LINE_MAX);
    write(sockfd, buf, strlen(buf));
    write(0, "\n", 1);

    tcsetattr(STDIN_FILENO, TCSANOW, &old_attr);

    /* wait server fork childern */
    len = read(sockfd, buf, MY_LINE_MAX);
        
    if (memcmp(buf, "ok", 2) != 0) {
        write(1, buf, len);

        if (is_local[0]) close(local_fd);
        exit(-1);
    }

    if (is_local[0]) { // upload
        /* pass upload command */
        std::string s;
        s = UPLOAD_COMMAND + path[1]; 
        write(sockfd, s.c_str(), s.size());
        len = read(sockfd, buf, MY_LINE_MAX);
        
        if (memcmp(buf, "ok", 2) != 0) {
            write(1, buf, len);

            close(local_fd);
            exit(-1);
        }

        while( (len = read(local_fd, buf, MY_LINE_MAX)) > 0) write(sockfd, buf, len);
        if (len < 0) err_sys("read");

    } else { // download
        /* pass download command */
        std::string s;
        s = DOWNLOAD_COMMAND" + path[0];
        write(sockfd, s.c_str(), s.size());
        len = read(sockfd, buf, MY_LINE_MAX);
        
        if (memcmp(buf, "ok", 2) != 0) {
            write(1, buf, len);

            close(local_fd);
            exit(-1);
        }

        local_fd = open(path[1].c_str(), O_RDWR | O_CREAT, 00700);
        if (local_fd < 0) err_sys(path[1].c_str());

        while( (len = read(sockfd, buf, MY_LINE_MAX)) > 0) write(local_fd, buf, len);
        if (len < 0) err_sys("read");

    }

    close(sockfd);
    return 0;
}