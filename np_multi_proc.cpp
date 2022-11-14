#include "functions_multi_proc.h"

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
    signal(SIGCHLD, sig_chld);
    signal(SIGINT,  sig_int);

    if (my_connect(listenfd, argv[1], servaddr) == 0) return -1;

	for ( ; ; ) 
    {
        int new_id = handle_new_connection(connfd, listenfd);
        if (new_id > maxi) maxi = new_id;
        
        printf("new client!!\n");

        char **cli_argv = (char**) malloc(sizeof(char*)*2);
        int pid = fork();

        if (pid == 0) 
        {
            close(listenfd);
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);

            char rwg[] = "rwg_multi_proc";
            cli_argv[0] = (char*) malloc(sizeof(char) * strlen(rwg)+1);
            strcpy(cli_argv[0], rwg);

            char char_id[4];
            sprintf(char_id, "%d", new_id);
            cli_argv[1] = (char*) malloc(sizeof(char) * sizeof(int)+1);
            strcpy(cli_argv[1], char_id);

            if (execvp(cli_argv[0], cli_argv) < 0)
            {
                char err[1024];
                sprintf(err, "Unknown command: [%s].\n", cli_argv[0]);
                write(STDERR_FILENO, err, strlen(err));
                return -1;
            }
        }
        else
        {
            // kill(pid, SIGIO);
            cp[new_id].set(connfd, pid, cli_argv);
            printf("server received pid: %d\n", pid);
            write_user_info(new_id);
            client_pid c(new_id);
            read_user_info(c);

            // broadcast message
            char msg[MSG_MAX];
            memset(msg, '\0', MSG_MAX);
            sprintf(msg, "*** User '(no name)' entered from %s:%d. ***\n", cp[new_id].addr, cp[new_id].port);
            // broadcast(msg);close(connfd);
        }
	}
    // while(cp.size() > 0) sig_chld(0);
    /* parent closes connected socket */
    close(listenfd);

    return 0;
}

void sig_chld(int signo)
{
printf("in sig chld\n");
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		for (int i = 0; i <= maxi; i++) 
        {
            if (pid == cp[i].pid)
            {
                printf("goodbye\n");
                free(cp[i].argv);
                cp[i].reset(i);
                break;
            }
        }
    }

	return;
}

void sig_int(int signo)
{
    /* clear share memory */
    for (int i = 0; i < 2; i++) 
        if (shmctl(shm_id[i], IPC_RMID, 0) < 0) perror("shmctl rm id fail");

    /* kill all process */
    bool remain = true;
    // while (remain)
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