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
#define SHM_SIZE sizeof(client_pid)

#define OPEN_MAX 50
#define MY_LINE_MAX 15005
#define MSG_MAX 1030
#define COMMAND_MAX 260
#define MY_NAME_MAX 25
#define NUM_USER 30

struct client_pid
{
    int id;
    int connfd = -1;
    int pid;
    char **argv;
    char name[MY_NAME_MAX];
    char addr[20];
    uint16_t port;
    
    client_pid () {};
    client_pid (int i) {id = i;}

    void reset(int i)
    {
        id = i;
        connfd = -1;
        pid = -1;
        argv = NULL;
        strcpy(name, "(no name)");
    }

    void set(int c, int p, char **a)
    {
        connfd = c;
        pid = p;
        argv = a;
    }

    void setaddr(sockaddr_in a)
    {
        strcpy(addr, inet_ntoa(a.sin_addr));
        port = ntohs(a.sin_port);
    }
};
void sig_broadcast(int signo);
int main(){
    signal(SIGUSR1, sig_broadcast); // handle broadcast
    int pid = fork();
    
    if (pid == 0)
    {
        sleep(10);
    }else {
        char FIFO_name[20];
        sprintf(FIFO_name, "user_pipe/0_0.txt");
                    
        if (mknod(FIFO_name, S_IFIFO | 0777, 0) < 0)
        {
            if (errno == EEXIST) 
            {
                printf("*** Error: the pipe #0->#0 already exists. ***\n");
                return -1;
            }
            printf("can't create fifo: %s\n", FIFO_name);
            return -1;
        }  

        /* tell the reader to open FIFO read */
        kill(pid, SIGUSR1);
        int u_to;
        if ((u_to = open(FIFO_name, O_WRONLY)) < 0)
        {
            char err[MSG_MAX];
            sprintf(err, "can't open write fifo: %s\n", FIFO_name);
            write(STDOUT_FILENO, err, strlen(err));
            return -1;
        }
        write(u_to, "hello", strlen("hello"));
        printf("write open\n");
    }
    
    return 0;
}

void sig_broadcast(int signo)
{
    /* open FIFO read */
    char FIFO_name[20];
    sprintf(FIFO_name, "user_pipe/0_0.txt");
    
    int nn;
    if ((nn = open(FIFO_name, O_RDONLY)) < 0)
    {
        char err[MSG_MAX];
        sprintf(err, "can't open read fifo: %s\n", FIFO_name);
        write(STDOUT_FILENO, err, strlen(err));
        return;
    }

    printf("read open\n");
    char msg[254];
    read(nn, msg, 250);
    printf("%s\n", msg);

    return;
}