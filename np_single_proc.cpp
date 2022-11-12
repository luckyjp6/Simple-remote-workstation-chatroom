#include "functions.h"
#include "rwg.h"

int main(int argc, char **argv)
{
	if (argc < 2) 
    {
        printf("Usage: ./a.out [port]\n");
        return -1;
    }
    
    int					connfd, listenfd;
	pid_t				childpid;
	sockaddr_in	        servaddr;
    int	                i, nready;

    init();
    setenv("PATH", "bin", 1);

    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

	for ( ; ; ) 
    {
        rfds = afds;
        if ((nready = select(maxfd+1, &rfds, NULL, NULL, NULL)) < 0)
        {
            printf("select: %s\n", strerror(errno));
            return -1;
        }

        // new client
        if (FD_ISSET(listenfd, &rfds)) 
        {
            handle_new_connection(connfd, listenfd);
            nready--;
        }

        /* check all clients */
		int sockfd, n;
        char buf[LINE_MAX];
        for (i = 0; i < maxi && nready > 0; i++) 
        {
            if (!FD_ISSET(client[i].connfd, &rfds)) continue;
            
            /* read input*/
            memset(buf, '\0', LINE_MAX);

            if ( (n = read(client[i].connfd, buf, LINE_MAX)) < 0) 
            { 
                if (errno == ECONNRESET) close_client(i); /* connection reset by client */
                else err_sys("read error");
            } 
            else if (n == 0) close_client(i); /* connection closed by client */
            else 
            {
                /* read command */
                if (execute_line(i, buf) < 0) break; // exit

                update_pipe_num_to();
                conditional_wait();
                C.clear();
                write(client[i].connfd, "% ", sizeof("% ")); fflush(stdout);
            }

            --nready;
            
        }
        wait_all_children();
	}
    /* parent closes connected socket */
    close(connfd);
}