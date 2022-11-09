#include "functions.h"

int maxi, num_user;
std::map<std::string, Client_info> name_client;
std::string fd_name[OPEN_MAX];
pollfd client[OPEN_MAX];
std::vector<broadcast_msg> b_msg;
std::map<std::string, channel_info> channels;

void init() 
{
    maxi = 0; num_user = 0;
    name_client.clear();
    for (int i = 1; i < OPEN_MAX; i++) client[i].fd = -1; /* -1: available entry */
    for (int i = 0; i < OPEN_MAX; i++) fd_name[i] = "";
    b_msg.clear();

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
    
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;

    return 1;
}

void handle_new_connection(int &connfd, const int listenfd, Client_info new_client)
{
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);
    num_user ++;

    // save descriptor
    int i;
    for (i = 1; i < OPEN_MAX; i++)
    {
        if (client[i].fd < 0) 
        {
            client[i].fd = connfd;
            new_client.addr = cliaddr;
            new_client.connfd = connfd;
            break;
        }
    }

    if (i == OPEN_MAX) 
    {
        printf("too many clients\n");
        return;
    }

    client[i].events = POLLRDNORM;
    if (i > maxi) maxi = i;
}

void close_client(int index) 
{
    int connfd = client[index].fd;

    close(connfd);
    client[index].fd = -1;
    num_user--;

    name_client.erase(fd_name[connfd]);
    fd_name[connfd] = "";
}