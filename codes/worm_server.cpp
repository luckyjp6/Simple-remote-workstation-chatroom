#include "conn_func.h"
#include "proc_client.h"

#define WORM_PORT 8787

void sig_chld(int signo);
void sig_int(int signo);

int maxi = 0;

int main(int argc, char **argv) {
	if (argc > 2) {
        printf("Usage: ./worm_server\n");
        return -1;
    }
    
    int					connfd, listenfd;
	pid_t				childpid;
	sockaddr_in	        servaddr;
    int	                i, nready;

    init();
    setenv("PATH", ".", 1);
    signal(SIGCHLD, sig_chld); // handle close connection
    signal(SIGINT,  sig_int); // handle server termination
    signal(SIGTERM,  sig_int); // handle server termination

    if (my_connect(listenfd, WORM_PORT, servaddr) == 0) return -1;

	for ( ; ; ) {
        int new_id = handle_new_connection(connfd, listenfd);
        if (new_id < 0) {
            close(connfd);
            continue;
        }
        if (new_id >= maxi) maxi = new_id+1;
        
        int pid = fork();
        if (pid == 0) 
        {
            // setenv("PATH", "bin:.", 1);
            signal(SIGCHLD, sig_cli_chld);
            signal(SIGINT, sig_cli_int);
            signal(SIGTERM, sig_cli_int);

            close(listenfd);

            for (auto c:cp)
            {
                if (c.connfd > 0 && c.connfd != connfd) close(c.connfd);
            }
            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);
            
            /* set root dir */
            set_root_dir(cp[new_id].name);
            /* set user id */
            if (setuid(cp[new_id].uid) < 0) err_sys("setuid");
            
            to_client(new_id);
            
            exit(0);
            return 0;
        }
        else
        {
            cp[new_id].set(pid);
            close(cp[new_id].connfd);
            write_user_info(cp[new_id]);
            printf("new client!! pid: %d, id: %d\n", pid, new_id);
        }   
    }
    /* parent closes connected socket */
    close(listenfd);

    return 0;
}

void sig_chld(int signo) {
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		for (int i = 0; i < maxi; i++) 
        {
            if (pid == cp[i].pid)
            {
                close_client(i);                
                return;
            }
        }
    }

	return;
}

void sig_int(int signo)  {
    /* server terminate by terminal ctrl +c */
    printf("in sig_int, maxi = %d\n", maxi); fflush(stdout);

    /* kill all children */
    bool remain = true;
    for (int i = 0; i < maxi; i++)
    {
        if (cp[i].pid > 0) 
        {printf("%d still alive\n", i);
            remain = true;
            kill(cp[i].pid, SIGINT);
            close_client(i);
            sig_chld(0);
            break;
        }
    }
    while (remain)
    {
        remain = false;
        for (int i = 0; i < maxi; i++)
            if (cp[i].pid > 0) 
            {
                sig_chld(0);
                break;
            }
    }

    /* clear share memory */
    for (int i = 0; i < 3; i++) 
        if (shmctl(shm_id[i], IPC_RMID, 0) < 0) perror("shmctl rm id fail");

    exit(1);
}