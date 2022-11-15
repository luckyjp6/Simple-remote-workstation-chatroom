#include "functions_multi_proc.h"
#include "rwg_multi_proc.h"

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
    sprintf(now, "%d %d %s %d %s", -1, -1, "0.0.0.0", 0, "(no name)");
    for (int i = 0; i < OPEN_MAX; i++)
    {
        memcpy(shm_addr + i*SHM_SIZE+5, now, SHM_SIZE);
    }

    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    /* set broadcast msg */
    shm_key[1] = (key_t)(1234+1);    
    if ((shm_id[1] = shmget(shm_key[1], MY_LINE_MAX, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");
    char *broadcast_shm = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_shm == NULL) err_sys("shmat fail");

    memset(broadcast_shm, '\0', MY_LINE_MAX);
    
    memcpy(broadcast_shm, n, 4);
    
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

    // socklen_t length; 
    // int ss = 0;
    // length = sizeof(ss);
    // if (getsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &ss, &length) < 0) printf("failed\n");
    // printf("rcv buf size: %d\n", ss);

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
            cp[new_id].setaddr(connfd, cliaddr);
            break;
        }
    }
    
    if (new_id < 0) 
    {
        printf("too many clients\n");
        return -1;
    }

    return new_id;
}

void broadcast(char *msg)
{
// printf("in broadcast\n");
    /* broad cast message in share memory */
    
    char *broadcast_addr = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_addr == NULL) err_sys("shmat fail");
    memset(broadcast_addr, '\0', MY_LINE_MAX);
    
    char nn[4]; sprintf(nn, "%d", (int)strlen(msg));
    memcpy(broadcast_addr, nn, 4);
    memcpy(broadcast_addr+5, msg, strlen(msg));
    if (shmdt(broadcast_addr) < 0) err_sys("shmdt fail");

    /* send signal to each client */
    int num_user = get_shm_num(shm_id[0]);

    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");    
    char now[SHM_SIZE];
    
    for (int i = 0; i < OPEN_MAX && num_user > 0; i++)
    {
        client_pid c(i);
        read_user_info(c);

        if (c.connfd < 0) continue;
        
        num_user--;
        
        if (kill(c.pid, SIGUSR1) < 0)
        {
            err_sys("signal havn't been init\n");
        }
    }
    
    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");
    
// printf("leave broadcast\n");
    return;
}

void close_client(int index) 
{    
// printf("close client %d\n", index);
    close(cp[index].connfd);
    alter_num_user(-1);

    read_user_info(cp[index]);

    char msg[NAME_MAX + 20];
    memset(msg, '\0', NAME_MAX + 20);
    sprintf(msg, "*** User '%s' left. ***\n", cp[index].name);
    
    /* release client id */
    cp[index].reset(index);
    write_user_info(cp[index]);

    broadcast(msg);
}

void alter_num_user(int amount)
{
    int num_user = get_shm_num(shm_id[0]);
    num_user += amount;

    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");
    
    char now[4];
    memset(now, '\0', sizeof(now));
    sprintf(now, "%d", num_user); 
    memcpy(shm_addr, &now, strlen(now));

    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

void write_user_info(client_pid c)
{
// printf("in write user info\n");
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");
    memset(shm_addr + c.id*SHM_SIZE +5, '\0', SHM_SIZE);
    
    char now[SHM_SIZE];
    memset(now, '\0', SHM_SIZE);
    sprintf(now, "%d %d %s %d %s", c.connfd, c.pid, c.addr, c.port, c.name); 
// printf("write: %s\n",now);
    memcpy(shm_addr + c.id*SHM_SIZE +5, &now, strlen(now));


    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

void read_user_info(client_pid &c)
{
// printf("read client %d\n", c.id);
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");    
    char now[SHM_SIZE];
    
    memcpy(now, shm_addr + c.id*SHM_SIZE +5, SHM_SIZE);
    // char *tmp_char = strtok(now, " \n\r");

    sscanf(now, "%d %d %s %d", &c.connfd, &c.pid, c.addr, &c.port); 
    
    char *ss = strtok(now, " ");
    for (int i = 0; i < 3; i++) ss = strtok(NULL, " ");
    ss = strtok(NULL, "\0");
    strcpy(c.name, ss);

// printf("%s\n", now);
    // printf("read user: connfd: %d, pid: %d, addr: %s, port: %d\n", c.connfd, c.pid, c.addr, c.port);
    
    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

int get_shm_num(int s_id)
{
    char *shm_addr = (char *)shmat(s_id, 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    char n[4];
    memcpy(n, shm_addr, 4);
    int len = atoi(n);

    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    return len;
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

