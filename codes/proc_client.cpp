#include "conn_func.h"
#include "proc_client.h"

#include <dirent.h>

std::vector<my_cmd> C; // after read one line of commands, stores them here
std::map< size_t, args > args_of_cmd; // pid, args
my_cmd tmp;

int normal_pipe = -1;
client_pid me;

char home_path[MY_NAME_MAX+10];
bool print_prefix = false;

int to_client(int id) {
    /* get connection info */
    me = cp[id];
    sprintf(home_path, "/home/%s", me.name);

    /* clean FIFO_open */
    C.clear();
    args_of_cmd.clear();
    tmp.clear();
        
    /* show welcome message */
    printf("\033[35m __       __       __   ______    __ __    ____  ____\033[0m\n");
    printf("\033[35m \\ \\     /  \\     / /  /  __  \\   | '__/  / __ `' __  \\\033[0m\n");
    printf("\033[35m  \\ \\   / /\\ \\   / /  /  /  \\  \\  | |    |  | |  |  |  |\033[0m\n");
    printf("\033[35m   \\ \\_/ /  \\ \\_/ /   \\  \\__/  /  | |    |  | |  |  |  |\033[0m\n");
    printf("\033[35m    \\___/    \\___/     \\______/   |_|    |__| |__|  |__|\033[0m\n");
    printf("\n");
    printf("\033[35m{\\__/}\033[0m\n");
    printf("\033[35m( • - •)\033[0m\n");
    printf("\033[35m/ > \033[0m\033[36mWelcome back\033[0m %s\n", me.name);
    printf("\033[36mOnline users:\033[0m %d\n", get_shm_num(shm_id[0]));    
    printf("\n");
    printf("\n");
    fflush(stdout);

    /* start to execute */
    while (true) {
        if (std::cin.eof())  {
            printf("cin eof\n");
            break;
        }

        char *now_path = getcwd(NULL, 0);
        if (memcmp(home_path, now_path, strlen(home_path)) == 0) {
            now_path += strlen(home_path)-1;
            now_path[0] = '~';
        }
        printf("\033[32m%s@worm_server:\033[36m%s\033[0m > ", me.name, now_path); fflush(stdout);

        char buf[MY_LINE_MAX];
        memset(buf, 0, MY_LINE_MAX);
        int len = read(STDIN_FILENO, buf, MY_LINE_MAX);

        if (len < 0) {
            err_sys("failed to read\n");
            return -1;
        }
        else if (len <= 1) continue; // blank line

        int status;
        if ((status = execute_line(buf)) == -1) break;
        // else if (status == -2) return 0;
    }

    wait_all_children();
    sig_cli_int(0);

    return 0;
}

int execute_line(char *line) {
    parse_line(line);

    bool interrupt = false;
    int status = 0;
    for (int i = 0; (i < C.size()); i++)
    {
        if (i % 50 == 0 && i != 0) wait_all_children();
        if ((status = execute_command(C[i])) < 0) {
            // printf("status: %d\n", status); fflush(stdout);
            switch (status)
            {
            case -1: // exit
                return -1;             
            case -2: // error of input file (file redirection)
                interrupt = true;
                break;
            case -3: // for fork test
                printf("num inst: %d\n", i); fflush(stdout);
                interrupt = true;
                break;
            }
            break;
        }
    }
    if (normal_pipe >= 0) {
        if (interrupt) close(normal_pipe);
        else {
            // ???
        }
    }
    wait_all_children();
    C.clear();
    
    return 0;
}

int execute_command(my_cmd &command) {
    if (command.argv[0] == "exit") {
        C.clear();
        return -1;
    }
    if (command.argv[0] == "setenv") 
    {
        if (command.argv.size() < 3)
        {
            printf("Usage: setenv [env_name] [value].\n");
	        return 0;
        }
        setenv(command.argv[1].data(), command.argv[2].data(), 1);
        return 0;
    }
    if (command.argv[0] == "printenv") 
    {
        if (command.argv.size() < 2) 
        {
            printf("Usage: printenv [env_name].\n");
            return 0;
        }

        char* env_info = getenv(command.argv[1].data());
        if (env_info != NULL) printf("%s\n", env_info);

        return 0;
    }
    if (command.argv[0] == "cd") {
        if (command.argv.size() < 2) {

            command.argv.push_back(home_path);
        }
        if (command.argv[1] == "~") command.argv[1] = home_path;
        if (chdir(command.argv[1].c_str()) < 0) perror("cd");
        return 0;
    }
    if (command.argv[0] == "who")
    {
        print_all_user();
        return 0;
    }

    args cmd;
    cmd.argc = command.argv.size();
    cmd.argv = (char**) malloc(sizeof(char*) * (cmd.argc+1));

    bool need_input=(command.input_path.size() > 0), need_output=(command.output_path.size() > 0);
    size_t pid;
    int in_fd, out_fd, out_pipe[2];
    std::vector<int> data_list;

    /* file redirection & normal pipe */
    if (need_input) {
        in_fd = open(command.input_path.c_str(), O_RDONLY, 0);
        if (in_fd < 0) {
            perror("worm_server: ");
            return -2;
        }
    }
    if (need_output) {
        int open_flg = O_RDWR | O_CREAT;
        if (command.append) open_flg |= O_APPEND;
        else open_flg |= O_TRUNC;
        out_fd = open(command.output_path.c_str(), open_flg, 00700);
        if (out_fd < 0) {
            if (need_input) close(in_fd);
            perror("worm_server: ");
            return -2;
        }
    }
    if (command.pipe) { pipe(out_pipe); cmd.out = out_pipe[1]; }
    
    /* fork */
    while ((pid = fork()) < 0)
    // if (pid < 0) 
    {
        printf("failed to fork\n"); fflush(stdout);
        // return -3;
    }

    /* parent do */
    if (pid > 0)
    {
        if (need_input) close(in_fd);
        if (need_input && normal_pipe >= 0) to_dev_null();
        if (need_output) close(out_fd);
        if (command.pipe) { close(out_pipe[1]); normal_pipe = out_pipe[0]; }

        /* record memory allocate address */
        args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));

        return 0;
    }
    /* child do */
    else {
        /* process input */
        if (need_input) dup2(in_fd, STDIN_FILENO);
        else if (normal_pipe >= 0) dup2(normal_pipe, STDIN_FILENO);
        /* process output */
        if (need_output) dup2(out_fd, STDOUT_FILENO);
        else if (command.pipe) { close(out_pipe[0]); dup2(out_pipe[1], STDOUT_FILENO); }

        // pipe stderr
        // if (command.err) dup2(p_num[1], STDERR_FILENO);

        // get the arguments ready
        for (int j = 0; j < cmd.argc; j++){
            cmd.argv[j] = (char*) malloc(sizeof(char) * command.argv[j].size()+1);
            strcpy(cmd.argv[j], command.argv[j].c_str());
        }
        cmd.argv[cmd.argc] = new char;
        cmd.argv[cmd.argc] = NULL;

        // exec!!!!
        if (execvp(cmd.argv[0], cmd.argv) < 0) {
            printf("Unknown command: [%s].\n", cmd.argv[0]);

            // close pipe
            if (need_input) close(in_fd);
            if (need_output) close(out_fd);
            else if (command.pipe) close(out_pipe[0]);

            exit(-1);
        }
    }
    return 0;
}

void parse_line(char *line) {
    // user pipe
    std::string s_line(line);
    
	// bool storage_flg = false;
	const char *new_args = " \n\r";
	char *command = strtok_r(line, new_args, &line);
	while(command != NULL) {
		std::string s(command);

        // parse msg
		if (s == "|") {
			tmp.pipe = true;
		
			// add commands
			C.push_back(tmp);
			tmp.clear();
		}
        else if (s == "<") {
            char *path = strtok_r(line, new_args, &line);
            tmp.input_path = path;
        }
		else if (s == ">") {
            char *path = strtok_r(line, new_args, &line);
            tmp.output_path = path;
            tmp.append = false;
        }
        else if (s == ">>") {
            char *path = strtok_r(line, new_args, &line);
            tmp.output_path = path;
            tmp.append = true;
        }
		else tmp.argv.push_back(s);

		command = strtok_r(line, new_args, &line);
	}
	if (tmp.argv.size() > 0) {
        C.push_back(tmp);
    	tmp.clear();
    }
}

void print_all_user() {
    int num_user = get_shm_num(shm_id[0]);
    
    char snd[MSG_MAX], now[SHM_SIZE];
    memset(snd, '\0', MSG_MAX);
    
    printf("%5s%12s%26s%18s\n", "<ID>", "<nickname>", "<IP:port>", "<indicate me>\n");  
    
    for (int i = 0; i < NUM_USER && num_user > 0; i++)
    { 
        client_pid c;
        // read_user_info(c, i);
        if (!read_user_info(c, i)) continue;

        num_user--;
        
        printf("%5d%12s%20s:%5d%17s\n", i+1, c.name, c.addr, c.port, (i == me.id)?"<-me":"");
    }
}

void to_dev_null() {

    if (fork() == 0) {
        char buf[MY_LINE_MAX];
        while (read(normal_pipe, buf, MY_LINE_MAX) > 0) ;
    }else {
        close(normal_pipe);
        normal_pipe = -1;
    }
}

void wait_all_children() {
	while (!args_of_cmd.empty()) {
		sig_cli_chld(SIGCHLD);			
	}
}

void sig_cli_chld(int signo) {
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		// free the memory!!
		char **cmd_argv = args_of_cmd[pid].argv;
        if (cmd_argv != NULL) free(cmd_argv);        
        args_of_cmd.erase(pid);		
    }

	return;
}

void sig_cli_int(int signo) {    
    /* disconnection, close by server */  
    /* kill all children */
    for (auto arg:args_of_cmd)
    {
        kill(arg.first, 9);
        sig_cli_chld(0);
    }

    while (args_of_cmd.size() > 0) sig_cli_chld(0);

    exit(0);
}

ssize_t	writen(int connfd, const char *buf, size_t size) {
	size_t	nremain;
	ssize_t	nwritten;
	const char *buf_now;

	buf_now = buf;
	nremain = size;
	while (nremain > 0) {
		if ( (nwritten = write(connfd, buf_now, nremain)) <= 0) {
			if (nwritten < 0 && errno == EINTR) nwritten = 0; // the error is not from write()
			else return -1; // write error
		}

		nremain -= nwritten;
		buf_now += nwritten;
	}
	return(size);
}

void Writen(int connfd, char *buf, size_t size) {
	if (writen(connfd, buf, size) != size)
		err_sys("writen error");
}

ssize_t	readn(int connfd, char *buf, size_t size) {
	size_t nremain;
	ssize_t	nread;
	char *buf_now;

	buf_now = buf;
	nremain = size;
	while (nremain > 0) {
		if ( (nread = read(connfd, buf_now, nremain)) < 0) {
			if (errno == EINTR) nread = 0; // the error is not from read
			else return -1; // read error
		} 
		else if (nread == 0) break; // EOF

		nremain -= nread;
		buf_now += nread;
	}
	return(size - nremain);
}

ssize_t Readn(int connfd, char *buf, size_t size) {
	size_t	n;

	if ( (n = readn(connfd, buf, size)) < 0)
		err_sys("readn error\n");
	return(n);
}
