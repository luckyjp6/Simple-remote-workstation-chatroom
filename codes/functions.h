#ifndef FUNSTIONS_H
#define FUNSTIONS_H

#include <cerrno>
#include <iostream>
#include <sys/types.h> 
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>

#define OPEN_MAX 1024
#define MY_LINE_MAX 15005
#define MSG_MAX 1030
#define COMMAND_MAX 260
#define MY_NAME_MAX 25
#define NUM_USER 30

struct pnt // pipe number to
{
    ssize_t pipe_num;
    int remain;
    
    void set(ssize_t p, int r)
    {
        pipe_num = p;
        remain = r;        
    }
};

struct Client_info 
{
    int connfd = -1;
    char addr[20];
    uint16_t port;
    char name[MY_NAME_MAX+5];
    std::map<std::string, std::string> env;
    std::vector<pnt> pipe_num_to; // pipe_num, counter   

    void reset()
    {
        if (connfd >= 0) close(connfd);
        connfd = -1;
        
        strcpy(name, "(no name)");
        
        env.clear();
        env["PATH"] = "bin:.";

        pipe_num_to.clear();
    }

    void set(int fd, sockaddr_in add)
    {
        connfd = fd;
        strcpy(addr, inet_ntoa(add.sin_addr));
        port = ntohs(add.sin_port);
    }
}; 

struct pipe_info
{
    int pipe_num = -1;
    std::string command;

    void reset()
    {
        pipe_num = -1;
        command = "";
    }

    void set(int p, std::string c)
    {
        pipe_num = p;
        command = c;
    }
};


extern int maxi;
extern Client_info client[OPEN_MAX];

extern int maxfd;
extern fd_set afds, rfds;

extern pipe_info user_pipe[NUM_USER][NUM_USER];

void init();
int my_connect(int &listenfd, char *port, sockaddr_in &servaddr);
void handle_new_connection(int &connfd, const int listenfd, bool welcome);

void close_client(int index, bool goodbye);
void broadcast(char *msg);

#endif