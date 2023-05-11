#include "conn_func.h"
#include "proc_client.h"

client_pid cp[NUM_USER];

key_t shm_key[3]; // user data, broadcast
int shm_id[3];

void init()
{
    /* set user info */
    shm_key[0] = (key_t)(1453);
    if ((shm_id[0] = shmget(shm_key[0], SHM_SIZE*NUM_USER, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    memset(shm_addr, '\0', SHM_SIZE*NUM_USER);

    char n[4];
    memset(n, '\0', 4);
    sprintf(n, "%d", 0);
    memcpy(shm_addr, n, 4);

    char now[SHM_SIZE];
    memset(now, '\0', SHM_SIZE);
    sprintf(now, "%d %d %s %d %s", -1, -1, "0.0.0.0", -1, "(no name)");
    for (int i = 0; i < NUM_USER-10; i++)
    {
        memcpy(shm_addr + i*SHM_SIZE+5, now, SHM_SIZE);
    }

    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    /* set broadcast msg */
    shm_key[1] = (key_t)(1453+1);    
    if ((shm_id[1] = shmget(shm_key[1], MY_LINE_MAX, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");
    char *broadcast_shm = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_shm == NULL) err_sys("shmat fail");

    memset(broadcast_shm, '\0', MY_LINE_MAX);
    
    memcpy(broadcast_shm, n, 4);
    
    if (shmdt(broadcast_shm) < 0) err_sys("shmdt fail");

    /* set user call */
    shm_key[2] = (key_t)(1453+2);
    if ((shm_id[2] = shmget(shm_key[2], MSG_MAX, PERMS|IPC_CREAT)) < 0) err_sys("shmget fail");
    char *msg_shm = (char *)shmat(shm_id[2], 0, 0);
    if (msg_shm == NULL) err_sys("shmat fail");

    memset(msg_shm, '\0', MSG_MAX);
    
    memcpy(msg_shm, n, 4);
    
    if (shmdt(msg_shm) < 0) err_sys("shmdt fail");

    /* clear client-pid */
    for (int i = 0; i < NUM_USER; i++) cp[i].reset(i);
}

int my_connect(int &listenfd, char *port, sockaddr_in &servaddr)
{
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    int buf_size = MY_LINE_MAX;
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(buf_size));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);
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

    /* get new client an id */
    int new_id = -1;
    for (int i = 0; i < NUM_USER; i++) {
        if (cp[i].connfd < 0) 
        {
            new_id = i;
            break;
        }
    }
    if (new_id < 0) err_sys("too many clients");

    /* check valid user */
    char name[MY_LINE_MAX];
    bool log_in = false;
    while (!log_in) {
        write(connfd, "User name: ", strlen("User name: ")); fflush(stdout);
        read(connfd, name, MY_NAME_MAX);
        
        std::string name_s(name);
        size_t next_line = name_s.find("\n");
        name_s.erase(next_line);
        if (name_s.size() > MY_NAME_MAX) name_s = name_s.substr(0, MY_NAME_MAX);

        strcpy(cp[new_id].name, name_s.c_str());
        log_in = check_usr_exist(cp[new_id].name);
    }

    alter_num_user(1);
    
    cp[new_id].setaddr(connfd, cliaddr);
    return new_id;
}

bool check_usr_exist(char *name) {
    int fd = open("./usr_list", O_RDONLY);
    if (fd < 0) err_sys("open usr_list");
    
    const size_t names_len = (MY_NAME_MAX+1)*NUM_USER+5;
    char *n, *now_at, names[MY_LINE_MAX] = {0};
    read(fd, names, MY_LINE_MAX);

    size_t name_len = strlen(name);
    n = names;
    while(now_at = strtok_r(n, ";", &n)) {
        if (strlen(now_at) != name_len) continue;
        if (strcmp(now_at, name)) continue;
        close(fd);
        return true;
    }
    close(fd);
    return false;
}

void broadcast(char *msg)
{
    /* broad cast message in share memory */
    char *broadcast_addr = (char *)shmat(shm_id[1], 0, 0);
    if (broadcast_addr == NULL) err_sys("shmat fail");
    memset(broadcast_addr, '\0', MY_LINE_MAX);
    
    char nn[4]; 
    memset(nn, '\0', 4);
    sprintf(nn, "%d", (int)strlen(msg));
    memcpy(broadcast_addr, nn, 4);
    memcpy(broadcast_addr+5, msg, strlen(msg));
    if (shmdt(broadcast_addr) < 0) err_sys("shmdt fail");

    /* send signal to each client */
    int num_user = get_shm_num(shm_id[0]);

    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");    
    char now[SHM_SIZE];
    
    for (int i = 0; i < NUM_USER && num_user > 0; i++)
    {
        client_pid c;
        if (!read_user_info(c, i)) continue;
        
        num_user--;
        
        if (kill(c.pid, SIGUSR1) < 0)
        {
            err_sys("signal haven't been init\n");
        }
    }
    
    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");
    
    return;
}

void tell(char *msg, int to)
{
    char *msg_shm = (char *)shmat(shm_id[2], 0, 0);
    if (msg_shm == NULL) err_sys("shmat fail");
    
    int len = 1;
    while (get_shm_num(shm_id[2])> 0);

    memset(msg_shm, '\0', MSG_MAX+10);

    char nn[4]; 
    memset(nn, '\0', 4);
    sprintf(nn, "%d", (int)strlen(msg));
    memcpy(msg_shm, nn, 4);
    memcpy(msg_shm+5, msg, strlen(msg));
    if (shmdt(msg_shm) < 0) err_sys("shmdt fail");

    /* send signal to each client */
    kill(to, SIGUSR2);
    
    return;
}

void close_client(int index) 
{    
    if (cp[index].connfd > 0)
    {
        close(cp[index].connfd);
        alter_num_user(-1);

        read_user_info(cp[index], index);

        char msg[NAME_MAX + 20];
        memset(msg, '\0', NAME_MAX + 20);
        sprintf(msg, "*** User '%s' left. ***\n", cp[index].name);
        
        /* release client id */
        cp[index].reset(index);
        write_user_info(cp[index]);

        broadcast(msg);
        
        printf("goodbye\n");
    }
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
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");
    memset(shm_addr + c.id*SHM_SIZE +5, '\0', SHM_SIZE);
    
    char now[SHM_SIZE];
    memset(now, '\0', SHM_SIZE);
    sprintf(now, "%d %s %d %s %d", c.connfd, c.name, c.pid, c.addr, c.port); 
    memcpy(shm_addr + c.id*SHM_SIZE +5, &now, strlen(now));


    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

bool read_user_info(client_pid &c, int id)
{
    int num_user = get_shm_num(shm_id[0]);
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");


    char now[SHM_SIZE];
    if (id >= 0) {
        memcpy(now, shm_addr + id*SHM_SIZE +5, SHM_SIZE);
        format_user_info(now, c);
        if (shmdt(shm_addr) < 0) perror("shmdt fail");
        return c.connfd >= 0;
    }
    
    char wanted_user[MY_NAME_MAX]; 
    strcpy(wanted_user, c.name);
    for (int i = 0; i < NUM_USER && num_user > 0; i++)
    { 
        memcpy(now, shm_addr + i*SHM_SIZE +5, SHM_SIZE);
        format_user_info(now, c);

        if (c.connfd < 0) {
            num_user--;
            continue;
        }
        if (strcmp(wanted_user, c.name)) {
            num_user--;
            continue;
        }

        if (shmdt(shm_addr) < 0) perror("shmdt fail");
        return true;
   }

   if (shmdt(shm_addr) < 0) perror("shmdt fail");
   return false;
    // char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    // if (shm_addr == NULL) err_sys("shmat fail");    
    // char now[SHM_SIZE];
    
    // memcpy(now, shm_addr + c.id*SHM_SIZE +5, SHM_SIZE);

    // sscanf(now, "%d %d %s %d", &c.connfd, &c.pid, c.addr, &c.port); 
    
    // char *ss = strtok(now, " ");
    // for (int i = 0; i < 3; i++) ss = strtok(NULL, " ");
    // ss = strtok(NULL, "\0");
    // strcpy(c.name, ss);

    // if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

bool format_user_info(char *row_data, client_pid &c) {
    char *n=row_data, *ss;

    c.connfd = atoi(strtok_r(n, " ", &n));
    strcpy(c.name, strtok_r(n, " ", &n));
    c.pid = atoi(strtok_r(n, " ", &n));
    strcpy(c.addr, strtok_r(n, " ", &n));
    c.port = atoi(strtok_r(n, "\0", &n));

    // printf("connfd: %d, name: %s, pid: %d, addr:port: %s:%d\n", c.connfd, c.name, c.pid, c.addr, c.port);
    fflush(stdout);
    return true;
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

void err_sys(const char *err)
{
    perror(err);
    exit(-1);
}