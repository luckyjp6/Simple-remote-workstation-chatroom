#include "functions.h"

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
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
    
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);//INADDR_ANY);
	servaddr.sin_port        = htons(atoi(argv[1]));

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
		printf("failed to bind\n");
		return 0;
	}

    listen(listenfd, 1024);

    Client_info tmp_client;
    socklen_t clilen = sizeof(cliaddr);

    while(true)
    {
        connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);
	
        int pid = fork();
        if (pid < 0) 
        {
            printf("Fork error\n");
            return -1;
        }
        if (pid == 0) 
        {
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);
            
            char **npshell = (char**) malloc(sizeof(char*));
            npshell[0] = new char;
            strcpy(npshell[0], "npshell");
            setenv("PATH", ".", 1);
            if (execvp("npshell", npshell) < 0)
            {
                char err[1024];
                sprintf(err, "Fail to exec\n");
                write(STDERR_FILENO, err, strlen(err));

                close(connfd);
                return -1;
            }
        }
        else 
        {   
            close(connfd);
            wait(NULL);
        }
    }

    
    return 0;
}