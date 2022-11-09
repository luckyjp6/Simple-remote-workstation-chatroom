#include "functions.h"
#include "npshell.h"

int main(int argc, char **argv)
{
	if (argc < 2) 
    {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }
    
    int					listenfd, connfd;
	pid_t				childpid;
	sockaddr_in	        servaddr;
    int	                i, nready;

    init();
	signal(SIGCHLD, sig_chld);
    
    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

    Client_info tmp_client;
	for ( ; ; ) 
    {
        nready = poll(client, maxi+1, -1);
        
        // new client
        if (client[0].revents & POLLRDNORM) 
        {

            handle_new_connection(connfd, listenfd, tmp_client);
        }
        
        /* check all clients */
		int sockfd, n;
        char buf[MSG_SIZE];        
        for (i = 1; i <= maxi; i++) 
        {
            if ( (sockfd = client[i].fd) < 0) continue;
            if (client[i].revents & (POLLRDNORM | POLLERR)) 
            {
                /* read input*/
                memset(buf, '\0', MSG_SIZE);

                if ( (n = read(sockfd, buf, MSG_SIZE-50)) < 0) 
                { 
                    if (errno == ECONNRESET) close_client(i); /* connection reset by client */
                    // else err_sys("read error");
                } 
                else if (n == 0) close_client(i); /* connection closed by client */
                else 
                {
                    /* read command */
                    parse_line(buf);

                    // execute commands
                    for (int i = 0; i < C.size(); i++)
                    {
                        if (i % 50 == 0 && i != 0) conditional_wait();
                        if (execute_command(C[i]) == 0) break;
                    }
                    update_pipe_num_to();
                    conditional_wait();
                    C.clear();
                }

                if (--nready <= 0) break; /* no more readable descs */
            }
        }
        wait_all_children();
	}
    /* parent closes connected socket */
    close(connfd);
}