#include "conn_func.h"
#include "proc_client.h"

#include <dirent.h>

std::vector<my_cmd> C; // after read one line of commands, stores them here
std::map< size_t, args > args_of_cmd; // pid, args
my_cmd tmp; // used in clear_tmp() and process_pipe_info()
std::vector<pnt> pipe_num_to; // pipe_num, counter   

client_pid me;

char home_path[MY_NAME_MAX+10];
bool print_prefix = false;

int to_client(int id) {
    /* get connection info */
    me = cp[id];
    sprintf(home_path, "/home/%s", me.name);

    /* clean FIFO_open */
    // memset(FIFO_open, -1, 30);
    C.clear();
    args_of_cmd.clear();
    clear_tmp();
    pipe_num_to.clear();

    /* broadcast message */
    // char msg[MSG_MAX];
    // memset(msg, '\0', MSG_MAX);
    // sprintf(msg, "\b\b*** User '%s' jump into the server. ***\n", me.name);
    // broadcast(msg);
        
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
        else if (status == -2) return 0;
    }

    wait_all_children();
    sig_cli_int(0);

    return 0;
}

int execute_line(char *line) {
    parse_line(line);

    int status = 0;
    for (int i = 0; i < C.size(); i++)
    {
        if (i % 50 == 0 && i != 0) conditional_wait();
        if ((status = execute_command(C[i])) < 0) return status; // exit        
    }
    if (status == 0) update_pipe_num_to();
    conditional_wait();
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
            printf("Usage: setenv [Variable] [Value].\n");
	        return 0;
        }
        setenv(command.argv[1].data(), command.argv[2].data(), 1);
        return 0;
    }
    if (command.argv[0] == "printenv") 
    {
        if (command.argv.size() < 2) 
        {
            printf("Usage: printenv [Variable].\n");
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
    cmd.number_pipe = command.number_pipe;

    bool need_data, need_pipe = (command.pipe_to > 0);
    bool u_pipe_to = false, u_pipe_from = false;
    int u_to, u_from;
    size_t pid;
    int p_num[2], data_pipe[2];
    std::vector<int> data_list;

    // record pipe descripter for the command behind to read
    if (need_pipe) 
    {
        pipe(p_num);
        cmd.p_num = p_num[0];
    }

    // received data from user pipe
    // int from = command.user_pipe_from;
    // if (from >= 0)
    // {
    //     u_pipe_from = true;

    //     if (check_user_pipe_from(from, u_from) < 0) u_from = open("/dev/null", O_RDONLY);
    //     else 
    //     {
    //         std::string cmd_line = command.user_pipe_command;
            
    //         char msg[MY_LINE_MAX +50];
    //         memset(msg, '\0', MY_LINE_MAX+50);

    //         client_pid c(from);
    //         read_user_info(c);
            
    //         cmd_line.erase(cmd_line.end()-1);
    //         sprintf(msg, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", me.name, me.id+1, c.name, from+1, cmd_line.data());
    //         broadcast(msg);

    //         cmd.from = from;
    //     }

        
    // }

    // user pipe to other user
    // int to = command.user_pipe_to;
    // if (to >= 0)
    // {
    //     u_pipe_to = true;

    //     if (check_user_pipe_to(to, u_to, command.user_pipe_command) < 0) u_to = open("/dev/null", O_WRONLY);
    //     else 
    //     {
    //         cmd.to = to;
    //     }
    // }

    check_need_data(need_data, data_pipe, data_list);
  
    // fork
    pid = fork();
    if (pid < 0) 
    {
        printf("failed to fork\n");
        return 0;
    }

    // parent do
    if (pid > 0)
    {
        if (need_pipe) close(p_num[1]);
        if (need_data) close(data_pipe[0]);

        if (u_pipe_to) close(u_to);
        if (u_pipe_from) close(u_from);

        // record memory allocate address
        args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));
        
        // update when new line
        if (command.number_pipe) update_pipe_num_to();
        
        // store read id of pipe
        pnt tmp;
        if (command.number_pipe) {
            tmp.set(p_num[0], command.pipe_to-1);
            pipe_num_to.push_back(tmp);
        }
        else if (need_pipe) 
        {
            tmp.set(p_num[0], 0);
            pipe_num_to.push_back(tmp);
        }

        int status;
        // process data from multiple pipe
        if (need_data && data_list.size() != 0) 
            if ((status = handle_data_from_multiple_pipe(data_pipe, data_list)) < 0) return status;
        
        // update when new line
        if (command.number_pipe) return 1;
        else return 0;
    }
    //child do
    else {

        // received data from other process
        if (need_data) 
        {
            // parent will handle multiple pipes of data
            if (data_list.size() != 0) 
            {
                close(data_pipe[1]);
                for(auto id: data_list) close(id);
            }

            dup2(data_pipe[0], STDIN_FILENO);
        }
        else if (u_pipe_from) dup2(u_from, STDIN_FILENO);

        // pipe stderr
        if (command.err) dup2(p_num[1], STDERR_FILENO);

        // pipe data to other process
        if (need_pipe) {
            close(p_num[0]);
            dup2(p_num[1], STDOUT_FILENO);
        }
        else if (u_pipe_to) 
        {
            dup2(u_to, STDOUT_FILENO);
        }

        // store data to a file
        if (command.store_addr.size() > 0) {
            if (set_output_to_file(command) < 0)
            {
                free(cmd.argv);
                return 0;
            }
        }

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
            if (need_data) close(data_pipe[0]);
            if (need_pipe) close(p_num[1]);

            return -2;
        }
    }
    return 0;
}

void parse_line(char *line) {
    // user pipe
    std::string s_line(line);
    
	// bool storage_flg = false;
	const char *new_args = " \n\r";
	char *command = strtok(line, new_args);
	while(command != NULL){
		std::string s(command);

        // parse msg
        if (s == "tell")
        {
            tmp.argv.push_back(s);

            char *to = strtok(NULL, " ");
            if (to != NULL) {
                std::string t(to);
                tmp.argv.push_back(t);

                char *msg = strtok(NULL, "\n\r\0");
                if (msg != NULL)
                {
                    std::string m(msg);
                    tmp.argv.push_back(m);
                }
            }
            C.push_back(tmp);
            clear_tmp();
            return;
        }
        if (s == "yell")
        {
            tmp.argv.push_back(s);

            char *msg = strtok(NULL, "\n\r\0");
            if (msg  != NULL) 
            {
                std::string m(msg);
                tmp.argv.push_back(m);
            }
            

            C.push_back(tmp);
            clear_tmp();
            return;
        }

		if (s == "|")
		{
			tmp.pipe_to = 1;
		
			// add commands
			C.push_back(tmp);
			clear_tmp();
		}
        else if (s == "<")
        {

        }
		else if (s == ">") 
        {

        }
		else tmp.argv.push_back(s);

		command = strtok(NULL, new_args);
	}
	if (tmp.argv.size() > 0) 
    {
        C.push_back(tmp);
    	clear_tmp();
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

int handle_data_from_multiple_pipe(int data_pipe[2], std::vector<int> data_list) {

	// fork a child to process
	int pid = fork();
    
	if (pid < 0) {
		char err[] = "failed to fork\n";
		write(STDERR_FILENO, err, strlen(err));
		return -1;
	}

	if (pid == 0) {
		// read data from each pipe (fifo)
		for (auto id: data_list) {
			char read_data[1024];
			int read_length;
            memset(read_data, 0, 1024);
			while ((read_length = read(id, read_data, 1024)) > 0) {
				write(data_pipe[1], read_data, read_length);
			}
			close(id);
		}
		close(data_pipe[1]);
		return -2;
	}else{
		close(data_pipe[1]);
		for(auto id: data_list) close(id);
		data_list.clear();
		
		// information for signal handler about how to handle this child
		args cmd;
		cmd.argv = NULL; cmd.argc = 0;
		args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));

		return 0;
	}

	return 0;
}

int set_output_to_file(my_cmd &command) {
	size_t file_id = open(command.store_addr.data(), O_WRONLY|O_CREAT|O_TRUNC, 00777);
	// clear the file content
	ftruncate(file_id, 0);
	lseek(file_id, 0, SEEK_SET);
	if (file_id < 0) {
		char err[1024];
		sprintf(err, "Failed to open file: %s\n", command.store_addr.data());
		write(STDERR_FILENO, err, strlen(err));
		return -1;
	}
	dup2(file_id, STDOUT_FILENO);
	return 0;
}

void check_need_data(bool &need, int (&data_pipe)[2], std::vector<int> &data_list) {
	// get all needed data for current command into datalist 
	// (datalist stores the needed read end of the pipe)
	data_list.clear();

	// counter == 0 => pipe the data to the command that execute next
	for (auto i = pipe_num_to.begin(); i < pipe_num_to.end(); ) {
    	if (i->remain <= 0) {
            data_list.push_back(i->pipe_num);
            pipe_num_to.erase(i);
        }
        else i++;
	}

    need = (data_list.size() != 0);
	if (!need) return;
	// if only one pipe of data needed
	// directly assign read p_id to data_pipe
	else if(data_list.size() == 1) {
		data_pipe[0] = data_list[0];
		data_list.clear();
		return;
	}
	// parent will tackle the data from multiple pipe
	else pipe(data_pipe);
}

void update_pipe_num_to() { 

	for (auto i = pipe_num_to.begin(); i < pipe_num_to.end(); ) {
        i -> remain--;
		if (i->remain < 0) {
            pipe_num_to.erase(i);
        }
        else 
            i++;
	}
}

void clear_tmp() {
	tmp.argv.clear();
	tmp.pipe_to = 0;
	tmp.store_addr = "";
	tmp.err = false;
	tmp.number_pipe = false;
    tmp.user_pipe_to = -1;
    tmp.user_pipe_from = -1;
    tmp.user_pipe_command = "";
}

void process_pipe_info(std::string s) {
	// only process number pipe
	tmp.number_pipe = true;
	if (s[0] == '!') tmp.err = true;
	
    s.erase(0, 1);
	tmp.pipe_to = atoi(s.c_str());
}

void wait_all_children() {
	while (!args_of_cmd.empty()) {
		sig_cli_chld(SIGCHLD);			
	}
}

void conditional_wait() {
	while(!args_of_cmd.empty()) {
		sig_cli_chld(SIGCHLD);

		// stop waiting if remaining are all commands with number pipe
		// the output are not immediately needed
		bool must_wait = false;
		for (auto s:args_of_cmd) {
			int p_num = s.second.p_num;
            bool find = false;
			for (auto p:pipe_num_to) {
                if (p.pipe_num == p_num)
                {
                    find = true;
                    break;
                }
			}
            if (!find)
            {
                must_wait = true;
                break;
            }
		}
		if (!must_wait) {
			break;
		}
	}
}

void sig_cli_chld(int signo) {
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		// free the memory!!
		char **cmd_argv = args_of_cmd[pid].argv;
        if (cmd_argv != NULL) free(cmd_argv);
		
        if (args_of_cmd[pid].from >= 0)
        {
            char FIFO_name[50];
            sprintf(FIFO_name, "user_pipe/%d_%d.txt", args_of_cmd[pid].from, me.id);
            
            if (remove(FIFO_name) < 0)
            {
                printf("can't remove %s\n", FIFO_name);
            }
        }

        // if (args_of_cmd[pid].to >= 0)
        // {
        //     char FIFO_name[50];
        //     sprintf(FIFO_name, "user_pipe/%d_%d.txt", me.id, args_of_cmd[pid].to);
            
        //     if (remove(FIFO_name) < 0)
        //     {
        //         printf("can't remove %s\n", FIFO_name);
        //     }
        // }
        
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
