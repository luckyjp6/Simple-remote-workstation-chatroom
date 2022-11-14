#include "functions.h"
#include "rwg.h"

int maxi;
Client_info client[OPEN_MAX];

int maxfd;
fd_set afds, rfds;

pipe_info user_pipe[NUM_USER][NUM_USER];

void init() 
{
    maxi = 0; maxfd = 0;
    for (auto &c:client) c.reset();
    FD_ZERO(&afds);

    for (auto &a:user_pipe)
        for(auto &b:a)
            b.reset();
}

int my_connect(int &listenfd, char *port, sockaddr_in &servaddr)
{
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    int buf_size = MY_LINE_MAX;
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(buf_size));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

    socklen_t length; 
    int ss = 0;
    length = sizeof(ss);
    if (getsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &ss, &length) < 0) printf("failed\n");
    printf("rcv buf size: %d\n", ss);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);//INADDR_ANY);
	servaddr.sin_port        = htons(atoi(port));

	while (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
        if (errno == EINTR) continue;
		printf("failed to bind\n");
		return 0;
	}

	listen(listenfd, 1024);
    
    FD_SET(listenfd, &afds);
    maxfd = listenfd;

    return 1;
}

void handle_new_connection(int &connfd, const int listenfd, bool welcome)
{
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);    

    // save descriptor
    int i;
    for (i = 0; i < OPEN_MAX; i++)
    {
        if (client[i].connfd < 0) 
        {
            client[i].set(connfd, cliaddr);
            FD_SET(connfd, &afds);
            if (welcome)
            {
                if (i >= maxi) maxi = i+1;
                if (connfd > maxfd) maxfd = connfd;
            }

            break;
        }
    }
    
    if (i == OPEN_MAX) 
    {
        printf("too many clients\n");
        return;
    }
    
    if (welcome)
    {
        char msg[MSG_MAX];
    
        // show welcome message
        memset(msg, '\0', MSG_MAX);
        sprintf(msg, "****************************************\n** Welcome to the information server. **\n****************************************\n");
        Writen(client[i].connfd, msg, strlen(msg));

        // broadcast message
        memset(msg, '\0', MSG_MAX);
        sprintf(msg, "*** User '%s' entered from %s:%d. ***\n", client[i].name, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        broadcast(msg);
    }
    

    char pa[] = "% ";
    Writen(connfd, pa, strlen(pa));
}

void broadcast(char *msg)
{
    for (int i = 0; i < maxi; i++)
    {
        int connfd = client[i].connfd;
        if (connfd < 0) continue;
        Writen(connfd, msg, strlen(msg));
    }
}

void close_client(int index, bool goodbye) 
{
    int connfd = client[index].connfd;
    
    char msg[NAME_MAX + 20];
    close(connfd);
    memset(msg, '\0', NAME_MAX + 20);
    sprintf(msg, "*** User '%s' left. ***\n", client[index].name);
    
    FD_CLR(connfd, &afds);
    client[index].reset();
    
    if (goodbye)
    {        
        for (int i = 0; i < NUM_USER; i++) 
        {
            if (user_pipe[index][i].pipe_num > 0) close(user_pipe[index][i].pipe_num);
            user_pipe[index][i].reset();
        }
        for (int i = 0; i < NUM_USER; i++) 
        {
            if (user_pipe[i][index].pipe_num > 0) close(user_pipe[i][index].pipe_num);
            user_pipe[i][index].reset();
        }
        broadcast(msg);
    }
}