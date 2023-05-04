#include "conn_func.h"
#include "proc_client.h"

void sig_chld(int signo);
void sig_int(int signo);

int maxi = 0; // user id
int msg_server_pid = -1;

int main(int argc, char **argv)
{
	if (argc < 3) 
    {
        printf("Usage: ./a.out [port] [msg server port] [data server IP] [data server port]\n");
        return -1;
    }
    
    int					connfd, listenfd, epollfd;
	pid_t				childpid;
	sockaddr_in	        servaddr;

    msg_server_pid = init(argv[2]);

    setenv("PATH", ".", 1);
    signal(SIGCHLD, sig_chld); // handle close connection
    signal(SIGUSR1, sig_broadcast); // handle broadcast
    signal(SIGUSR2, sig_tell); // handle tell
    signal(SIGINT,  sig_int); // handle server termination
    signal(SIGTERM,  sig_int); // handle server termination

    my_connect(listenfd, argv[1], servaddr);
    
	for ( ; ; ) 
    {
        int new_id = handle_new_connection(connfd, listenfd);
        if (new_id >= maxi) maxi = new_id+1;    

        char **cli_argv = (char**) malloc(sizeof(char*)*2);
        int pid = fork();

        if (pid == 0) 
        {
            setenv("PATH", "bin:.", 1);
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

            // connect to msg server
            int msg_fd;
            sockaddr_in msg_addr;
            msg_addr.sin_family = AF_INET;
            msg_addr.sin_port = htons(atoi(argv[2]));
            inet_pton(AF_INET, "127.0.0.1", &msg_addr.sin_addr);
            msg_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(msg_fd, (sockaddr*)&msg_addr, sizeof(sockaddr));
            write(msg_fd, &new_id, sizeof(new_id)); // register

            // exec child
            char tmp[5];
            sprintf(tmp, "%d", msg_fd);
            execl("proc_client", tmp, argv[3], argv[4]);
            // to_client(new_id);

            exit(0);
            return 0;
        }
        else
        {
            cp[new_id].set(pid, cli_argv);
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
		for (int i = 0; i < maxi; i++) 
        {
            if (pid == cp[i].pid)
            {
                free(cp[i].argv);
                close_client(i);
                
                return;
            }
        }
    }

	return;
}

void sig_int(int signo) /* server terminate by terminal ctrl +c */
{
    printf("in sig_int, maxi = %d\n", maxi);

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

    exit(1);
}