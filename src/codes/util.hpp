#ifndef UTIL_HPP
#define UTIL_HPP

#include <iostream>
#include <typeinfo>
#include <cxxabi.h>
#include <unistd.h>

/* Glabal Constents */
#define WORM_PORT 8787
// regular size + buffer
#define MY_LINE_MAX 15000 + 100
#define MY_NAME_MAX 20 + 10
#define MSG_MAX 1024 +100
#define COMMAND_MAX 256 +50

/* upload/download keyword */
#define LOAD_FILE_KEYWORD "load"
#define UPLOAD_COMMAND "worm_upload"
#define DOWNLOAD_COMMAND "worm_download"

class client_pid {
    int id;
    int uid, pid;
    int connfd, request, msg;
    char name[MY_NAME_MAX];
    char addr[20];
    int port;

public: 
    client_pid (int i = -1) { reset(i); }
    client_pid (client_pid &other) {
        set_id(other.get_id());
        set_uid(other.get_uid());
        set_pid(other.get_pid());
        set_connfd(other.get_connfd());
        set_request(other.get_request());
        set_msg(other.get_msg());
        set_name((char *)other.get_name());
        set_addr((char *)other.get_addr());
        set_port(other.get_port());
    }

    void reset(int i);
    void set_id(int i) { id = i; }
    void set_uid(int i) { uid = i; }
    void set_pid(int i) { pid = i; }
    void set_connfd(int i) { connfd = i; }
    void set_request(int i) { request = i; }
    void set_msg(int i) { msg = i; }
    void set_name(std::string s) { strcpy(name, s.c_str()); }
    void set_addr(std::string s) { strcpy(addr, s.c_str()); }
    void set_port(int i) { port = i; }

    int get_id() { return id; }
    int get_uid() { return uid; }
    int get_pid() { return pid; }
    int get_connfd() { return connfd; }
    int get_request() { return request; }
    int get_msg() { return msg; }
    const char* get_name() { return name; }
    const char* get_addr() { return addr; }
    int get_port() { return port; }
};

template <typename T, typename... Args>
void err_sys(const T* instance, const char* func, Args... args) {
    int status;
    char* demangledName = abi::__cxa_demangle(typeid(*instance).name(), nullptr, nullptr, &status);
    std::string className = (status == 0) ? demangledName : typeid(*instance).name();
    free(demangledName); 

    std::cerr << "[" << className << "::" <<  func << "] ";
    (std::cerr << ... << args);

    if (errno != 0) {
        perror(" ");
    } else {
        std::cerr << std::endl;
    }
}

template <typename... Args>
void err_sys_cli(Args... args) {
    std::cerr << "[worm_workstation]: ";
    (std::cerr << ... << args);

    if (errno != 0) {
        perror(" ");
    } else {
        std::cerr << std::endl;
    }

    exit(-1);
}

ssize_t my_write(int connfd, std::string msg) {
    ssize_t res = write(connfd, msg.c_str(), msg.size());
    fflush(stdout);

    return res;
}

#endif