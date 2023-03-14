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
    int	                nready;
    
    init();
    setenv("PATH", "bin:.", 1);

    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

    for( ; ; )
    {
        handle_new_connection(connfd, listenfd, false);
        // FD_CLR(listenfd, &afds);

        int n;
        char buf[MY_LINE_MAX];

        for ( ; ; )
        {
            memset(buf, '\0', MY_LINE_MAX);
            
            if ( (n = read(connfd, buf, MY_LINE_MAX)) < 0) 
            { 
                if (errno == ECONNRESET) err_sys("read error");
                close_client(0, false); /* connection reset by client */
                break;
            } 
            else if (n == 0) 
            {
                close_client(0, false);
                break; /* connection closed by client */
            }
            else if (n == 1)
            {
                write(connfd, "% ", sizeof("% "));
            }
            else 
            {
                // printf("%d\n", n);
                /* read command */
                int status;
                if ((status = execute_line(0, buf)) < 0) {
                    if (status == -2) return 0;
                    wait_all_children();
                    close_client(0, false);
                    break; // exit
                }
                
                char pa[] = "% ";
                Writen(connfd, pa, strlen(pa));
            }
        }
    }

    close(listenfd);
    return 0;
}