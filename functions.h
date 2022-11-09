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
    char user_name[NAME_LENGTH];
    char real_name[NAME_LENGTH];
    sockaddr_in addr;
}; 

struct broadcast_msg
{
    int except;
    char msg[MSG_SIZE];
    int msg_size;
    void set(int e) {
        except = e;
        msg_size = strlen(msg);
    }
};

// use map to impliment the channel
struct channel_info
{
    bool is_private = false;
    char* key;
    std::string topic;
    std::vector<std::string> ban_list;
    std::vector<int> connected; // connfd
};

extern int maxi, num_user;
extern std::map<std::string, Client_info> name_client;
extern std::string fd_name[OPEN_MAX];
extern pollfd client[OPEN_MAX];
extern std::vector<broadcast_msg> b_msg;
extern std::map<std::string, channel_info> channels;

void init();
int my_connect(int &listenfd, char *port, sockaddr_in &servaddr);
void handle_new_connection(int &connfd, const int listenfd, Client_info new_client);

void tolower_str(char *str);

void close_client(int index);

#endif