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
#define MSG_SIZE 550
#define NAME_LENGTH 10
#define SERVER_NAME "jp6jp6"

struct Client_info 
{
    int connfd;
    sockaddr_in addr;
    char name[25];

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

extern int maxi;
extern Client_info client[OPEN_MAX];

extern int maxfd;
extern fd_set afds, rfds;

void init();
int my_connect(int &listenfd, char *port, sockaddr_in &servaddr);
void handle_new_connection(int &connfd, const int listenfd);

void close_client(int index);
void broadcast(char *msg);

#endif