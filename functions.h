#ifndef FUNSTIONS_H
#define FUNSTIONS_H

#include <cerrno>
#include <iostream>
#include <sys/types.h> 
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
#define MY_LINE_MAX 15000
#define MSG_MAX 1024
#define COMMAND_MAX 256
#define MY_NAME_MAX 20
#define NUM_USER 30

struct Client_info 
{
    int connfd;
    sockaddr_in addr;
    char name[MY_NAME_MAX+5];

    void reset()
    {
        connfd = -1;
        strcpy(name, "(no name)");
    }

    void set(int fd, sockaddr_in add)
    {
        connfd = fd;
        addr = add;
    }
}; 

struct pipe_info
{
    int pipe_num;
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
void handle_new_connection(int &connfd, const int listenfd);

void close_client(int index);
void broadcast(char *msg);

#endif