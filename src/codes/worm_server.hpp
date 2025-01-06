// #ifndef CLIENT_PID_CLASS
// #define CLIENT_PID_CLASS

// #include "util.hpp"
// #include <cstring>

// class client_pid {
//     int id;
//     int uid, pid;
//     int connfd, request, msg;
//     char name[MY_NAME_MAX];
//     char addr[20];
//     int port;

// public: 
//     client_pid (int i = -1) { reset(i); }
//     client_pid (client_pid &other) {
//         set_id(other.get_id());
//         set_uid(other.get_uid());
//         set_pid(other.get_pid());
//         set_connfd(other.get_connfd());
//         set_request(other.get_request());
//         set_msg(other.get_msg());
//         set_name((char *)other.get_name());
//         set_addr((char *)other.get_addr());
//         set_port(other.get_port());
//     }

//     void reset(int i);
//     void set_id(int i) { id = i; }
//     void set_uid(int i) { uid = i; }
//     void set_pid(int i) { pid = i; }
//     void set_connfd(int i) { connfd = i; }
//     void set_request(int i) { request = i; }
//     void set_msg(int i) { msg = i; }
//     void set_name(std::string s) { strcpy(name, s.c_str()); }
//     void set_addr(std::string s) { strcpy(addr, s.c_str()); }
//     void set_port(int i) { port = i; }

//     int get_id() { return id; }
//     int get_uid() { return uid; }
//     int get_pid() { return pid; }
//     int get_connfd() { return connfd; }
//     int get_request() { return request; }
//     int get_msg() { return msg; }
//     const char* get_name() { return name; }
//     const char* get_addr() { return addr; }
//     int get_port() { return port; }
// };

// #endif

#ifndef WORM_SERVER_HPP
#define WORM_SERVER_HPP

/* basic */
#include <cstring>
#include <fcntl.h>

/* socket */
#include <sys/socket.h> // socket, connet, bind
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h> // inet_pton, inet_ntoa

/* fork, wait */
#include <wait.h>

/* signal */
#include <csignal>

/* group id */
#include <grp.h>

/* password */
#include <pwd.h>

#include "util.hpp"
#include "shm_manager.hpp"

#define MAX_USER 30
#define MAX_EVENT MAX_USER*2


class worm_server {
private:
    int server_gid = -1;
    int listenfd;
    
    /* shared memory */
    shm_manager shm_mgr;


    /* socket */
    void connect(int port);
    int handle_new_connection(int &connfd);
    int authentication(int idx); 
    
    /* client operation */
    void client_init(int idx);
    void client_load_file(int idx);

public:
    worm_server();
    void run();
};

void close_client(int idx);
/* signal handler */
void sig_child(int signo);
void sig_int(int signo);

#endif