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
	sockaddr_in	        cliaddr, servaddr;
    int	                i, nready;

    init();
	signal(SIGCHLD, sig_chld);
    
    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

    Client_info tmp_client;
    socklen_t clilen = sizeof(cliaddr);

    char buf[MSG_SIZE-50];
    connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);
	while(read(connfd, buf, MSG_SIZE-50) > 0) 
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
    wait_all_children();
	
    if (errno != ECONNRESET) err_sys("read error");
    close_client(i); 
    
    /* parent closes connected socket */
    close(connfd);
}