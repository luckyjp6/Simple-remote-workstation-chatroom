#include "functions_multi_proc.h"
#include "rwg_multi_proc.h"

void sig_chld(int signo);
void sig_int(int signo);

int maxi = 0;

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
    setenv("PATH", ".", 1);
    signal(SIGCHLD, sig_chld); // handle close connection
    signal(SIGUSR1, sig_broadcast); // handle broadcast
    signal(SIGINT,  sig_int); // handle server termination

    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

	for ( ; ; ) 
    {
        int new_id = handle_new_connection(connfd, listenfd);
        if (new_id > maxi) maxi = new_id;
        
        

        char **cli_argv = (char**) malloc(sizeof(char*)*2);
        int pid = fork();

        if (pid == 0) 
        {
            close(listenfd);
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);

            client(new_id);
            return 0;
        }
        else
        {
            cp[new_id].set(pid, cli_argv);
            // printf("server received pid: %d\n", pid);
            write_user_info(cp[new_id]);
            printf("new client!! pid: %d, id: %d\n", pid, new_id);
        }
	}
    /* parent closes connected socket */
    close(listenfd);

    return 0;
}

void sig_chld(int signo)
{
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		for (int i = 0; i <= maxi; i++) 
        {
            if (pid == cp[i].pid)
            {
                close_client(i);
                free(cp[i].argv);
                printf("goodbye\n");
                return;
            }
        }
    }

	return;
}

void sig_int(int signo) /* server terminate by terminal ctrl +c */
{
    /* clear share memory */
    for (int i = 0; i < 2; i++) 
        if (shmctl(shm_id[i], IPC_RMID, 0) < 0) perror("shmctl rm id fail");

    /* kill all children */
    bool remain = true;
    while (remain)
    {
        remain = false;
        for (int i = 0; i < maxi; i++)
        {
            if (cp[i].pid > 0) 
            {
                remain = true;
                kill(cp[i].pid, SIGINT);
                sig_chld(0);
                break;
            }
        }
    }
    exit(1);
}