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
    setenv("PATH", "bin:.", 1);

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
            handle_new_connection(connfd, listenfd, true);
            printf("new client!!!!!!!!!!!!!!!\n");
            nready--;
        }

        /* check all clients */
		int sockfd, n;
        char buf[MY_LINE_MAX];
        for (i = 0; i < maxi && nready > 0; i++) 
        {
            if (!FD_ISSET(client[i].connfd, &rfds)) continue;
            setenv("PATH", client[i].env["PATH"].data(), 1);

            /* read input*/
            memset(buf, '\0', MY_LINE_MAX);

            if ( (n = read(client[i].connfd, buf, MY_LINE_MAX)) < 0) 
            { 
                if (errno != ECONNRESET) err_sys("read error");
                close_client(i, true); /* connection reset by client */
            } 
            else if (n == 0) close_client(i, true); /* connection closed by client */
            else if (n == 1)
            {
                write(connfd, "% ", strlen("% ")); fflush(stdout);
            }
            else 
            {
                int status;
                /* read command */
                if ((status = execute_line(i, buf)) < 0) 
                {
                    // exit
                    if (status == -1) {
                        close_client(i, true);
                        continue; 
                    }
                    // children
                    else 
                    {
                        close_client(i, false);
                        return 0; 
                    }
                }
                write(client[i].connfd, "% ", strlen("% ")); fflush(stdout);
            }

            --nready;
            
        }
	}

    /* parent closes connected socket */
    close(listenfd);
}