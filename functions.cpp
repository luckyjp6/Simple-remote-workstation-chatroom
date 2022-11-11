#include "functions.h"
#include "rwg.h"

int maxi;
Client_info client[OPEN_MAX];

int maxfd;
fd_set afds, rfds;

void init() 
{
    maxi = 0; maxfd = 0;
    for (auto &c:client) c.reset();
    FD_ZERO(&afds);

    setenv("PATH", "bin", 1);
}

int my_connect(int &listenfd, char *port, sockaddr_in &servaddr)
{
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));
    
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);//INADDR_ANY);
	servaddr.sin_port        = htons(atoi(port));

	if (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
		printf("failed to bind\n");
		return 0;
	}

	listen(listenfd, 1024);
    
    FD_SET(listenfd, &afds);
    maxfd = listenfd;

    return 1;
}

void handle_new_connection(int &connfd, const int listenfd)
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
            if (i >= maxi) maxi = i+1;
            if (connfd > maxfd) maxfd = connfd;

            break;
        }
    }
    
    if (i == OPEN_MAX) 
    {
        printf("too many clients\n");
        return;
    }
    
    char msg[MSG_SIZE];
    
    // show welcome message
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, "****************************************\n** Welcome to the information server. **\n****************************************\n");
    Writen(client[i].connfd, msg, strlen(msg));

    // broadcast message
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, "*** User '%s' entered from %s:%d. ***\n", client[i].name, inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
    broadcast(msg);

    char pa[] = "% ";
    Writen(connfd, pa, sizeof(pa));
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

void close_client(int index) 
{
    int connfd = client[index].connfd;

    close(connfd);

    char msg[MSG_SIZE];
    memset(msg, '\0', MSG_SIZE);
    sprintf(msg, "*** User '%s left. ***\n", client[index].name);
    
    client[index].reset();

    broadcast(msg);
}