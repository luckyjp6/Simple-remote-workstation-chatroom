#include "functions_multi_proc.h"

client_pid cp[OPEN_MAX];

key_t shm_key[2]; // user data, broadcast
int shm_id[2];



void init()
{
    /* set user info */
    shm_key[0] = (key_t)(1234);    
    if ((shm_id[0] = shmget(shm_key[0], SHM_SIZE*OPEN_MAX, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");

    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    memset(shm_addr, '\0', SHM_SIZE*OPEN_MAX);

    char n[4];
    sprintf(n, "%d", 0);
    memcpy(shm_addr, n, 4);

    char now[SHM_SIZE];
    sprintf(now, "%d %d %s %d", -1, -1, "0.0.0.0", 0);
    for (int i = 0; i < OPEN_MAX; i++)
    {
        memcpy(shm_addr + i*SHM_SIZE+4, now, SHM_SIZE);
    }

    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    /* set broadcast msg */
    shm_key[1] = (key_t)(1234+1);    
    if ((shm_id[1] = shmget(shm_key[1], MY_LINE_MAX, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");
    char *broadcast_shm = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_shm == NULL) err_sys("shmat fail");

    memset(broadcast_shm, '\0', MY_LINE_MAX);

    if (shmdt(broadcast_shm) < 0) err_sys("shmdt fail");

    /* clear client-pid */
    for (int i = 0; i < OPEN_MAX; i++) cp[i].reset(i);
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

    return 1;
}

int handle_new_connection(int &connfd, const int listenfd)
{
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    connfd = accept(listenfd, (sockaddr *) &cliaddr, &clilen);

    alter_num_user(1);

    // save descriptor
    int new_id = -1;
    /* get new client an id */ 
    for (int i = 0; i < OPEN_MAX; i++) {
        if (cp[i].connfd < 0) 
        {
            new_id = i;
            cp[new_id].setaddr(cliaddr);
            break;
        }
    }
    
    if (new_id < 0) 
    {
        printf("too many clients\n");
        return -1;
    }

    char msg[MSG_MAX];
    
    // show welcome message
    memset(msg, '\0', MSG_MAX);
    sprintf(msg, "****************************************\n** Welcome to the information server. **\n****************************************\n");
    write(connfd, msg, strlen(msg));

    return new_id;
}

void broadcast(char *msg)
{
// printf("in broadcast\n");
    /* broad cast message in share memory */
    char *broadcast_addr = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_addr == NULL) err_sys("shmat fail");
    char msg_len[4];
    sprintf(msg_len, "%d", (int)strlen(msg));
    memcpy(broadcast_addr, msg_len, 4);

    memcpy(broadcast_addr+4, msg, strlen(msg));
    if (shmdt(broadcast_addr) < 0) err_sys("shmdt fail");

    /* send signal to each client */
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    char n[4];
    memcpy(n, shm_addr, 4);
    int num_user = atoi(n);
write(STDOUT_FILENO, "before kill\n", strlen("before kill\n"));
    char now[SHM_SIZE];
    
    for (int i = 0; i < OPEN_MAX && num_user > 0; i++)
    {      
        client_pid c(i);

        read_user_info(c);
        if (c.connfd < 0) continue;
        printf("pid: %d\n", c.pid);  
        num_user--;

        kill(c.pid, SIGIO);
    }
    
    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");
printf("end broadcast\n");
}

void close_client(int index) 
{
    int connfd/* = get client connfd*/;
    
    close(connfd);
    alter_num_user(-1);
    kill(cp[index].pid, SIGINT);

    read_user_info(cp[index]);

    char msg[NAME_MAX + 20];
    memset(msg, '\0', NAME_MAX + 20);
    sprintf(msg, "*** User '%s' left. ***\n", cp[index].name);
    
    /* release client id */
    cp[index].reset(cp[index].id);
    write_user_info(cp[index].id);

    broadcast(msg);
}

void alter_num_user(int amount)
{
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");
    
    char now[4];
    memcpy(now, shm_addr, 4);
    
    int num_user = atoi(now) + amount;
    
    memset(now, '\0', sizeof(now));
    sprintf(now, "%d", num_user); 
    memcpy(shm_addr, &now, strlen(now));

    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

void write_user_info(int client_id)
{
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    char now[SHM_SIZE];
    memset(now, '\0', SHM_SIZE);
    memset(shm_addr + client_id*SHM_SIZE+4, '\0', SHM_SIZE);
    sprintf(now, "%d %d %s %d", cp[client_id].connfd, cp[client_id].pid, cp[client_id].addr, cp[client_id].port); 
    memcpy(shm_addr + client_id*SHM_SIZE, &now, strlen(now));


    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

void read_user_info(client_pid &c)
{
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");    
    char now[SHM_SIZE];
    memcpy(now, shm_addr + c.id*SHM_SIZE, SHM_SIZE+4);
    sscanf(now, "%d %d %s %d", &c.connfd, &c.pid, c.addr, &c.port); 

    // char msg[1024];
    // sprintf(msg, "new user: connfd: %d, pid: %d, addr: %s, port: %d\n", c.connfd, c.pid, c.addr, c.port);
    // write(STDOUT_FILENO, msg, strlen(msg));
    
    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

// int sem_create(int index)
// {
//     register int id, semval;
    
//     sembuf op_lock[2] = 
//     {
//         2, 0, 0,
//         2, 1, SEM_UNDO
//     };

// again:
//     if (sem_id[index] = semget(sem_key[index], 3, PERMS | IPC_CREAT) < 0) return -1;
//     if (semop(sem_id[index], &op_lock[0], 2) < 0)
//     {
//         if (errno == EINVAL) goto again;
        
//         err_sys("can't lock");
//     }

//     if ( (semval = semctl(id, 1, GETVAL, 0)) < 0) err_sys("can't GETVAL");

//     if (semval == 0)
//     {
//         if (sen)
//     }
// }

void err_sys(const char *err)
{
    perror(err);
    exit(-1);
}