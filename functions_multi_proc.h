#ifndef FUNSTIONS_MULTI_PROC_H
#define FUNSTIONS_MULTI_PROC_H

#include <cerrno>
#include <iostream>
#include <sys/types.h> 
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <algorithm>

#define SEM_FOLDER "./shm"
#define PERMS 0666
#define SHM_SIZE 100

#define OPEN_MAX 1000
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

struct client_pid
{
    int id;
    int connfd = -1;
    int pid;
    char **argv;
    char name[MY_NAME_MAX];
    char addr[20];
    int port;
    
    client_pid () {};
    client_pid (int i) {id = i;}

    void reset(int i)
    {
        id = i;
        connfd = -1;
        pid = -1;
        argv = NULL;
        strcpy(name, "(no name)");
        strcpy(addr, "0.0.0.0");
        port = -1;
    }

    void set(int p, char **a)
    {
        pid = p;
        argv = a;
    }

    void setaddr(int fd, sockaddr_in a)
    {
        connfd = fd;
        strcpy(addr, inet_ntoa(a.sin_addr));
        port = (int)ntohs(a.sin_port);
    }
};

extern client_pid cp[OPEN_MAX];

extern key_t shm_key[2]; // user data, broadcast
extern int shm_id[2];
void init();

int my_connect(int &listenfd, char *port, sockaddr_in &servaddr);
int handle_new_connection(int &connfd, const int listenfd);

void close_client(int index);
void broadcast(char *msg);

void alter_num_user(int amount);
void write_user_info(client_pid c);
void read_user_info(client_pid &c);
int get_shm_num(int s_id);

void err_sys(const char *err);
#endif