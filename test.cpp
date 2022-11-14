#include <cerrno>
#include <iostream>
#include <sys/types.h> 
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <time.h>
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

using namespace std;

struct client_pid
{
    int connfd = -1;        // 4
    int pid;                // 4
    char **argv;            
    char name[MY_NAME_MAX]; // 32
    sockaddr_in addr;       // 14 + 4
    
    void reset()
    {
        connfd = pid = -1;
        argv = NULL;
        strcpy(name, "(no name)");
    }

    void set(int c, int p, char **a)
    {
        connfd = c;
        pid = p;
        argv = a;
    }
};

int main(){
    client_pid c;
	cout << sizeof(int) << endl;
	cout << sizeof(char**) << endl;
    char msg[20];
    sprintf(msg, "%s", inet_ntoa(c.addr.sin_addr));
	cout << strlen(msg) << endl;
    return 0;
}

