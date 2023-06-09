#include "conn_func.h"
#include "proc_client.h"
#include <pwd.h>
#include <grp.h>

client_pid cp[NUM_USER];

key_t shm_key[3]; // user data, broadcast
int shm_id[3];
int server_gid = -1;

void init() {
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

    /* check group exist */
    group* grp = getgrnam("worm_server");
    if (grp == nullptr) {
        char err_msg[] = "Please create a group named worm_server\n";
        write(2, err_msg, strlen(err_msg));
        exit(-1);
    }
    server_gid = grp->gr_gid;
}

int my_connect(int &listenfd, int port, sockaddr_in &servaddr) {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    int buf_size = MY_LINE_MAX;
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(buf_size));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(0);
	servaddr.sin_port        = htons(port);

	while (bind(listenfd, (const sockaddr *) &servaddr, sizeof(servaddr)) < 0) 
    {
        if (errno == EINTR) continue;
		printf("failed to bind\n");
		return 0;
	}

	listen(listenfd, 1024);

    return 1;
}

int handle_new_connection(int &connfd, const int listenfd) {
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
    char name[MY_LINE_MAX] = {0};
    memset(name, 0, MY_LINE_MAX);
    cp[new_id].uid = -1;
    while (cp[new_id].uid < 0) {
        write(connfd, "User name: ", strlen("User name: ")); fflush(stdout);
        if (read(connfd, name, MY_NAME_MAX) <= 0) return -1;
        
        int uid;
        std::string name_s(name);
        size_t next_line = name_s.find("\n");
        if (next_line != std::string::npos) name_s.erase(next_line);

        if (memcmp(name, "load", 4) == 0) {
            std::string user_name = name_s.substr(5);
            uid = check_usr_exist(connfd, (char *)user_name.c_str());
            if (uid < 0) return -1;
            
            if (fork() == 0) {
                close(listenfd);
                for (auto c:cp)
                {
                    if (c.connfd > 0 && c.connfd != connfd) close(c.connfd);
                }

                /* set root dir */
                set_root_dir((char *)user_name.c_str());
                /* set user id */
                if (setuid(uid) < 0) err_sys("setuid");
                dup2(connfd, STDERR_FILENO);

                handle_load_file(connfd);
                exit(0);
            } else {
                close(connfd);
                return -1;
            }
        }

        if (name_s.size() > MY_NAME_MAX) name_s = name_s.substr(0, MY_NAME_MAX);

        strcpy(cp[new_id].name, name_s.c_str());
        uid = check_usr_exist(connfd, cp[new_id].name);
        if (uid < 0) return -1;
        cp[new_id].uid = uid;
    }

    alter_num_user(1);
    
    cp[new_id].setaddr(connfd, cliaddr);
    return new_id;
}

int check_usr_exist(int connfd, char *name) {
    int user_id = -1;
    struct passwd* pw = getpwnam(name);
    if (pw != nullptr) {
        if (pw->pw_gid == server_gid) user_id = pw->pw_uid;
    }
    int passwd_len;
    char msg[200] = {0};
    sprintf(msg, "%s@140.113.141.90's password: ", name);
    if (write(connfd, msg, strlen(msg)) <= 0) return -1;
    memset(msg, 0, sizeof(msg));
    if (read(connfd, msg, sizeof(msg)) < 0) return -1;
    if (user_id < 0) {
        write(connfd, "Wrong password\n", 15);
        return -1;
    }

    std::string passwd(msg);
    size_t next_line = passwd.find("\n");
    if (next_line != std::string::npos) passwd.erase(next_line);
    int fd = open("./name_passwd.txt", O_RDONLY);
    char data[MY_LINE_MAX], *line, *d; 
    d = data;
    read(fd, data, sizeof(data));
    while ((line = strtok_r(d, "\r\n", &d)) != NULL) {
        if (strstr(line, name) == NULL) continue;
        char *tmp = strtok_r(line, " ", &line);
        if (strcmp(line, passwd.c_str()) == 0) return user_id;
        else break;
    }   
    write(connfd, "Wrong password\n", 15);
    return -1;
}

void set_root_dir(char *name) {
    char root_path[MY_LINE_MAX];
    char home_path[MY_LINE_MAX];
    sprintf(home_path, "/home/%s", name);
    sprintf(root_path, "%s/user_space", getcwd(NULL, 0));
    

    if (chdir(root_path) < 0) err_sys("chdir");
    if (chroot(root_path) < 0) err_sys("chroot");
    if (chdir(home_path) < 0) err_sys("chdir");

    setenv("PATH", "/bin", 1);
}   

void handle_load_file(int connfd) {
    write(connfd, "ok", 2);
    int len;
    char buf[MY_LINE_MAX] = {0};

    // get upload/download command
    len = read(connfd, buf, MY_LINE_MAX);
    if (len <= 0) exit(-1);
    
    if (memcmp(buf, "worm_upload", strlen("worm_upload")) == 0) {
        char file_path[500] = {0};
        memcpy(file_path, buf+12, len-12);
        if (file_path[0] == '~') file_path[0] = '.';
        int fd = open(file_path, O_RDWR | O_CREAT, 00700);
        if (fd < 0) err_sys("destination");
        write(connfd, "ok", 2);

        size_t len;
        char buf[MY_LINE_MAX];
        while( (len = read(connfd, buf, MY_LINE_MAX)) > 0) write(fd, buf, len);
        if (len < 0) err_sys("read");

        close(fd);
        close(connfd);
        exit(0);
    } else if (memcmp(buf, "worm_download", strlen("worm_download")) == 0) {
        char file_path[500] = {0};
        memcpy(file_path, buf+14, len-14);
        if (file_path[0] == '~') file_path[0] = '.';

        int fd = open(file_path, O_RDONLY);
        if (fd < 0) err_sys("source file");
        write(connfd, "ok", 2);
        size_t len;
        char buf[MY_LINE_MAX] = {0};
        while ( (len = read(fd, buf, MY_LINE_MAX)) > 0 ) write(connfd, buf, len);
        if (len < 0) err_sys("read");
        
        close(fd);
        close(connfd);
        exit(0);
    } else {
        printf("Invalid cp command\n"); fflush(stdout);
        exit(-1);
    }
}

void close_client(int index) {    
    if (cp[index].connfd > 0)
    {
        epoll_event ev;
        ev.data.fd = cp[index].request;
        // close(cp[index].connfd);
        close(cp[index].msg);
        close(cp[index].request);

        alter_num_user(-1);

        read_user_info(cp[index], index);

        char msg[NAME_MAX + 20];
        memset(msg, '\0', NAME_MAX + 20);
        sprintf(msg, "*** User '%s' left. ***\n", cp[index].name);
        
        /* release client id */
        cp[index].reset(index);
        write_user_info(cp[index]);
        
        printf("goodbye\n");
    }
}

void alter_num_user(int amount) {
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

void write_user_info(client_pid c) {
    char *shm_addr = (char *)shmat(shm_id[0], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");
    memset(shm_addr + c.id*SHM_SIZE +5, '\0', SHM_SIZE);
    
    char now[SHM_SIZE];
    memset(now, '\0', SHM_SIZE);
    sprintf(now, "%d %s %d %s %d", c.connfd, c.name, c.pid, c.addr, c.port); 
    memcpy(shm_addr + c.id*SHM_SIZE +5, &now, strlen(now));


    if (shmdt(shm_addr) < 0) perror("shmdt fail");
}

bool read_user_info(client_pid &c, int id) {
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

int get_shm_num(int s_id) {
    char *shm_addr = (char *)shmat(s_id, 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    char n[4];
    memcpy(n, shm_addr, 4);
    int len = atoi(n);

    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    return len;
}

void err_sys(const char *err) {
    perror(err);
    exit(-1);
}