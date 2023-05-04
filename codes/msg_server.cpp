
#include <sys/epoll.h>
#include "conn_func.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc < 2) {
        cout << "Usage ./msg_server [port]" << endl;
        exit(-1);
    }

    int                 listen_fd, fd_to_id[NUM_USER];
    sockaddr_in         saddr;
    int	                epollfd, nfds;
    epoll_event         ev, events[NUM_USER];

    my_connect(listen_fd, argv[1], saddr);

    epollfd = epoll_create1(0);
    if (epollfd < 0) err_sys("epoll create1");

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD,  listen_fd, &ev) < 0) err_sys("epoll_ctl: listen_sock");

    for ( ; ; ) 
    {
        nfds = epoll_wait(epollfd, events, NUM_USER+1, -1);
        if (nfds < 0) err_sys("epoll wait");

        for (int n = 0; n < nfds; n++) {
            if (events[n].data.fd == listen_fd) {
                /* new connection */
                int connfd = accept(listen_fd, )
            }
            /* get message */
        }
    }
}