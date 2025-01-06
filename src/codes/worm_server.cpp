#include "worm_server.hpp"
#include "client_proc.hpp"

/* all clients */ 
client_pid cp[MAX_USER]; 

/* shared memory */
shm_manager shm_mgr;

/* epoll */
int maxi = 0;

void client_pid::reset(int i = -1) {
/*
    Do: Reset the object.
    Return: None
*/
    set_id(i);
    set_uid(-1); set_pid(-1);
    set_connfd(-1); set_request(-1); set_msg(-1);
    set_name("(no name)");
    set_addr("0.0.0.0");
    port = -1;
}


worm_server::worm_server() {
/*
    Do: Initialize the server, set signal handler, check "worm_server" group ID exists.
    Return: worm_server instance
*/
    /* set environment variable */
    setenv("PATH", ".", 1); // TODO: why?

    /* set signal handler */
    signal(SIGCHLD, sig_child); // handle close connection
    signal(SIGINT, sig_int); // handle server termination
    signal(SIGTERM, sig_int); // handle server termination
    
    /* clear client-pid */
    for (int i = 0; i < MAX_USER; i++) cp[i].reset(i);

    /* check group exist */
    group* grp = getgrnam("worm_server");
    if (grp == nullptr) {
        char err_msg[] = "Please create a group named worm_server\n";
        err_sys(this, __func__, err_msg);
        exit(-1);
    }
    server_gid = grp->gr_gid;
}

void worm_server::run() {
/*
    Do: Main logic of the server. Listen on WORM_PORT, accept (verification is included) new client and fork a process.
    Return: None
*/
    int	connfd;
    pid_t childpid;

    /* prepare socket */
    connect(WORM_PORT);

    /* loop, accept new connection and fork process */
	for ( ; ; ) {
        int new_id = handle_new_connection(connfd);
        if (new_id < 0) {
            close(connfd);
            continue;
        }
        if (new_id >= maxi) maxi = new_id+1;
        
        if ((childpid = fork()) == 0) {
            /* client side */
            // change signal handler 
            signal(SIGCHLD, sig_cli_chld);
            signal(SIGINT, sig_cli_int);
            signal(SIGTERM, sig_cli_int);

            close(listenfd);


            client_init(new_id);

            dup2(connfd, STDIN_FILENO);
            dup2(connfd, STDOUT_FILENO);
            dup2(connfd, STDERR_FILENO);
            
            client_proc client(cp[new_id].get_id(), cp[new_id].get_name());
            client.run();
            
            exit(0);
        }
        else
        {
            /* server side */
            cp[new_id].set_pid(childpid);
            close(connfd);
            shm_mgr.set_user_info(cp[new_id]);
            printf("new client!! pid: %d, id: %d\n", childpid, new_id);
        }   
    }
    
    /* end of server, impossible to happen */
    close(listenfd);

    return;
}

void worm_server::connect(int port) {
/* 
    Do: Construct socket and start to listen.
    Return: Listening socket's file descriptor
*/
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int reuse = 1;
    int buf_size = MY_LINE_MAX;
    setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (const char*)&buf_size, sizeof(buf_size));
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse, sizeof(reuse));

    sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = htonl(0);
	server_addr.sin_port        = htons(port);

	while (bind(listenfd, (const sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        if (errno == EINTR) continue;
		err_sys(this, __func__, "failed to bind\n");
        exit(-1);
	}

	listen(listenfd, MAX_USER);
}
int worm_server::handle_new_connection(int &connfd) {
/*
    Do: Process new connection, including authentication.
    Return: New client's id
*/
    sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);
    connfd = accept(listenfd, (sockaddr *)&cli_addr, &cli_addr_len);

    /* get id for new client */
    int new_id = -1;
    for (int i = 0; i < MAX_USER; i++) {
        /* get empty client id */
        if (cp[i].get_connfd() < 0) {
            new_id = i;
            cp[i].reset(i); // clear previous data
            cp[i].set_connfd(connfd);
            break;
        }
    }
    if (new_id < 0) {
        err_sys(this, __func__, "reached the maximum number of serviceable clients");
        return -1;
    }

    /* request for user info and check if its a valid user */
    for (int i = 0; i < 3; i++) { // three times to login
        /* ask user name */
        my_write(connfd, "User name: ");

        /* get response */
        char name[MY_NAME_MAX];
        if (read(connfd, name, MY_NAME_MAX) <= 0) {
            err_sys(this, __func__, "client closed connection before entering name");
            return -1;
        }

        /* check if the client is an upload/download-only client */
        bool is_load = false;
        if (memcmp(name, LOAD_FILE_KEYWORD, strlen(LOAD_FILE_KEYWORD))) {
            is_load = true;
            memmove(name, name + strlen(LOAD_FILE_KEYWORD), strlen(name) - strlen(LOAD_FILE_KEYWORD) + 1);
        }

        /* check client name */
        if (authentication(new_id) < 0) {
            // error messages handled by authentication()
            continue;
        } else {
            cp[new_id].set_name(name);
        }

        /* for normal client */
        if (!is_load) {
            return new_id;
        }

        /* for upload/download-only client */
        if (fork() == 0) {
            /* child side */
            /* transmit file and exit */
            client_init(new_id);

            dup2(connfd, STDERR_FILENO);
            client_load_file(new_id);
            exit(0);
        } else {
            /* server side */
            /* no need to maintain connection for upload/download-only client */
            return -1;
        }
    }
    return -1;
}

int worm_server::authentication(int idx) {
/*
    Do: Receive password from user and authenticate the user.
    Return: -1 if authentication fail, otherwise, 0.
*/
    /* check if user password exist && the user belongs to group "worm_server" */
    struct passwd* pw = getpwnam(cp[idx].get_name());
    if (pw != nullptr && pw->pw_gid == server_gid) {
        cp[idx].set_uid(pw->pw_uid);
    } else {
        /* no error, for security purpose */
        cp[idx].set_uid(-1);
    }

    /* ask for password */
    int passwd_len;
    if (my_write(cp[idx].get_connfd(), (std::string)cp[idx].get_name() + "@140.113.141.90's password: ") <= 0) {
        err_sys(this, __func__, "client close connection during entering password");
        return -1;
    }
    
    /* recv password */ 
    char data[MY_LINE_MAX];
    char *recv_password, *temp;
    memset(data, 0, sizeof(data));
    if (read(cp[idx].get_connfd(), data, sizeof(data)) <= 0) {
        err_sys(this, __func__, "client close connection during entering password");
        return -1;
    } else {
        recv_password = strtok_r(data, "\n", &temp);
    }

    /* for invalid user */
    if (cp[idx].get_uid() < 0) {
        err_sys(this, __func__, "invalid user: ", cp[idx].get_name());
        my_write(cp[idx].get_connfd(), "Wrong password\n");
        return -1;
    }

    /* worm server passwd list */
    /* TODO: use sha256, don't store the password in plaintext */
    int fd = open("../name_passwd.txt", O_RDONLY);
    memset(data, 0, sizeof(data));
    while(read(fd, data, sizeof(data))) {
        temp = strstr(data, cp[idx].get_name());
        if (temp == nullptr) { 
            memset(data, 0, sizeof(data));
            continue;
        }
        
        char *stored_password;
        stored_password = strtok_r(temp, "\r\n", &temp);
        temp = strtok_r(stored_password, ":", &stored_password);
        
        
        if (strcmp(stored_password, recv_password)) {
            return 0; // authentication succeed
        } else {
            my_write(cp[idx].get_connfd(), "Wrong password\n");            
            return -1;
        }        
    }
    return -1;
}

void worm_server::client_init(int idx) {
/*
    Do: After calling fork(), set root directory and child process's user id.
    Return: None
*/
    /* close listen socket and all the other socket */
    close(listenfd);
    for (auto c:cp) {
        int now_connfd = c.get_connfd();
        if (now_connfd > 0 && now_connfd != cp[idx].get_connfd()) {
            close(now_connfd);
        }
    }

    /* set root directory */
    char root_path[MY_LINE_MAX], home_path[MY_LINE_MAX];
    sprintf(home_path, "/home/%s", cp[idx].get_name());
    sprintf(root_path, "%s/user_space", getcwd(NULL, 0));
    if (chdir(root_path) < 0) {
        err_sys(this, __func__, "fail to change directory to root_path");
    }
    if (chroot(root_path) < 0) {
        err_sys(this, __func__, "fail to set root");
    }
    if (chdir(home_path) < 0) {
        err_sys(this, __func__, "fail to change directory to home_path");
    }

    /* set environment variables */
    setenv("PATH", "/bin", 1);
    setenv("HOME", home_path, 1);

    /* set user id */
    if (setuid(cp[idx].get_uid()) < 0) {
        err_sys(this, __func__, "setuid fail");
        exit(-1);
    }
}
void worm_server::client_load_file(int idx) {
/*
    Do: Handle upload/download-only client
    Return: None 
*/
    int len;
    int connfd = cp[idx].get_connfd();
    char buf[MY_LINE_MAX] = {0};

    /* respond ACK */
    my_write(connfd, "ok");

    /* recv upload/download command */
    len = read(connfd, buf, sizeof(buf));
    if (len <= 0) exit(-1);

    /* process upload or download */
    int in_fd, out_fd;
    char *command;
    char file_path[MY_LINE_MAX] = {0};
    if (memcmp(buf, UPLOAD_COMMAND, strlen(UPLOAD_COMMAND))) {
        command = UPLOAD_COMMAND;
    } else if (memcmp(buf, DOWNLOAD_COMMAND, strlen(DOWNLOAD_COMMAND))) {
        command = DOWNLOAD_COMMAND;
    }
    // only valid string are copied 
    strcpy(file_path, buf + strlen(command));
        
    /* home directory */
    if (file_path[0] == '~') {
        file_path[0] = '.';
    }

    /* open or create file */
    if (strcmp(command, UPLOAD_COMMAND)) {
        in_fd = connfd;
        out_fd = open(file_path, O_WRONLY | O_CREAT, 00700);
    }
    else if (strcmp(command, DOWNLOAD_COMMAND)) {
        in_fd = open(file_path, O_RDONLY);
        out_fd = connfd;
    }

    if (in_fd < 0 || out_fd) {
        err_sys_cli("fail to access/open the file.");
        return;
    }

    /* respond ACK */
    my_write(connfd, "ok");

    /* recv and store the file or read and send the file */
    memset(buf, 0, sizeof(buf));
    while( (len = read(in_fd, buf, sizeof(buf))) > 0) {
        write(out_fd, buf, len);
        memset(buf, 0, sizeof(buf));
    }
    if (len < 0) {
        err_sys(this, __func__, "fail to read");
        err_sys_cli("fail to read");
    }

    close(in_fd);
    close(out_fd);
}
void close_client(int idx) {    
// TODO: Not yet check

/*
    Do: close No.idx client and clear the corresponding data.
    Return: None
*/
    if (cp[idx].get_connfd() < 0) return;
    
    close(cp[idx].get_msg());
    close(cp[idx].get_request());

    shm_mgr.set_num_data(shm_manager::user_data, 
                         shm_mgr.get_num_data(shm_manager::user_data) - 1);

    // auto c = shm_mgr.read_user_info(idx);

    // char msg[NAME_MAX + 20] = {0};
    printf("*** User '%s' left. ***\n", cp[idx].get_name());
    
    /* release client id */
    client_pid temp(idx);
    shm_mgr.set_user_info(temp);
}

void sig_child(int signo) {
/*
    Do: Wait for child termination and clean its data.
        - Signal handler for connection closed by child.
        - Unit process for server termination process.
    Return: None
*/
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		for (int i = 0; i < maxi; i++) 
        {
            if (pid == cp[i].get_pid())
            {
                close_client(i);                
                return;
            }
        }
    }

	return;
}
void sig_int(int signo)  {
/* 
    Do: Server terminate by terminal ctrl +c, terminate all children .
    Return: None
*/

    /* kill all children */
    for (int i = 0; i < maxi; i++)
    {
        if (cp[i].get_pid() > 0) {
            kill(cp[i].get_pid(), SIGINT);
            // close_client(i);
            sig_child(0);
            break;
        }
    }

    /* clear share memory */
    shm_mgr.clear();

    exit(1);
}
