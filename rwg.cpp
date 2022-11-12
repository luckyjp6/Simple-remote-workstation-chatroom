#include "functions.h"
#include "rwg.h"

std::vector<my_cmd> C; // after read one line of commands, stores them here
std::map< size_t, args> args_of_cmd; // pid, args
std::map< size_t, int > pipe_num_to; // pipe_num, counter
my_cmd tmp; // used in clear_tmp() and process_pipe_info()

int execute_line(int index, char *line)
{
    parse_line(line);

    for (int i = 0; i < C.size(); i++)
    {
        if (i % 50 == 0 && i != 0) conditional_wait();
        
        if (execute_command(index, C[i]) < 0) return -1; // exit
    }
    return 0;
}

int execute_command(int index, my_cmd &command)
{
    int connfd = client[index].connfd;

    if (command.argv[0] == "exit") {
        C.clear();
        return -1;
    }
    if (command.argv[0] == "setenv") 
    {
        my_setenv(command);
        return 0;
    }
    if (command.argv[0] == "printenv") 
    {
        my_printenv(command);
        return 0;
    }
    if (command.argv[0] == "who")
    {
        print_all_user(index);
        return 0;
    }
    if (command.argv[0] == "tell")
    {
        if (command.argv.size() < 3) 
        {
            char err[] = "Usage: tell <to whom> <msg>\n";
            Writen(connfd, err, strlen(err));
            return 0;
        }
        // std::string msg = "";
        // for (int i = 2; i < command.argv.size(); i++) msg += command.argv[i];
        tell_msg(index, atoi(command.argv[1].data()), command.argv[2]);
        return 0;
    }
    if (command.argv[0] == "yell")
    {
        if (command.argv.size() < 2) 
        {
            char err[] = "Usage: yel <msg>\n";
            Writen(connfd, err, strlen(err));
            return 0;
        }
        yell_msg(index, command.argv[1]);
        return 0;
    }
    if (command.argv[0] == "name")
    {
        if (command.argv.size() < 2) 
        {
            char err[] = "Usage: name <name>\n";
            Writen(connfd, err, strlen(err));
            return 0;
        }
        change_name(index, command.argv[1]);
        return 0;
    }

    args cmd;
    cmd.argc = command.argv.size();
    cmd.argv = (char**) malloc(sizeof(char*) * (cmd.argc+1));
    cmd.number_pipe = command.number_pipe;

    bool need_data, need_pipe = (command.pipe_to > 0);
    bool u_pipe_to = false, u_pipe_from = false;
    size_t pid;
    int p_num[2], data_pipe[2], u_pipe[2];
    std::vector<int> data_list;

    // record pipe descripter for the command behind to read
    if (need_pipe) 
    {
        pipe(p_num);
        cmd.p_num = p_num[0];
    }
    
    // received data from user pipe
    int from = command.user_pipe_from;
    if (from >= 0)
    {
        if (check_user_pipe_from(from, index))
        {
            char msg[MY_LINE_MAX +50];
            memset(msg, '\0', MY_LINE_MAX+50);
            sprintf(msg, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", client[from].name, from+1, client[index].name, index+1, user_pipe[from][index].command.data());
            broadcast(msg);

            u_pipe_from = true;
        }
        else 
        {
            close(user_pipe[from][index].pipe_num);
            user_pipe[from][index].reset();
        }
    }
    
    // user pipe to other user
    if (command.user_pipe_to >= 0)
    {
        int to = command.user_pipe_to;
        if (check_user_pipe_to(index, to))
        {
            pipe(u_pipe);
            std::string cmd_line = command.user_pipe_command;
            cmd_line.erase(cmd_line.end()-1);
            user_pipe[index][to].set(u_pipe[0], cmd_line);
            
            char msg[MY_LINE_MAX +50];
            memset(msg, '\0', MY_LINE_MAX+50); 
            sprintf(msg, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", client[index].name, index+1, cmd_line.data(), client[to].name, to+1);
            broadcast(msg);

            u_pipe_to = true;
        }        
    }

    check_need_data(need_data, data_pipe, data_list);
    
    // fork
    pid = fork();
    if (pid < 0) 
    {
        char *err;
        sprintf(err, "failed to fork\n");
        Writen(STDERR_FILENO, err, strlen(err));
        return 0;
    }

    // parent do
    if (pid > 0)
    {
        if (need_pipe) close(p_num[1]);
        if (need_data) close(data_pipe[0]);
        if (u_pipe_to) close(u_pipe[1]);
        if (u_pipe_from) 
        {
            close(user_pipe[from][index].pipe_num);
            user_pipe[from][index].reset();
        }

        // record memory allocate address
        args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));
        
        // update when new line
        if (command.number_pipe) update_pipe_num_to();
        
        // store read id of pipe
        if (command.number_pipe) pipe_num_to.insert(std::pair<size_t, int>(p_num[0], command.pipe_to-1));
        else if (need_pipe) pipe_num_to.insert(std::pair<size_t, int>(p_num[0], 0));

        // process data from multiple pipe
        if (need_data && data_list.size() != 0) 
            if (handle_data_from_multiple_pipe(data_pipe, data_list) == 0) return 0;
        
        return 0;
    }
    //child do
    else {
        int connfd = client[index].connfd;

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
        else if (u_pipe_from) dup2(user_pipe[from][index].pipe_num, STDIN_FILENO);
        else dup2(connfd, STDIN_FILENO);

        // pipe stderr
        if (command.err) dup2(p_num[1], STDERR_FILENO);
        else dup2(connfd, STDERR_FILENO);

        // pipe data to other process
        if (need_pipe) {
            close(p_num[0]);
            dup2(p_num[1], STDOUT_FILENO);
        }
        else if (u_pipe_to) 
        {
            close(u_pipe[0]);
            dup2(u_pipe[1], STDOUT_FILENO);
        }
        else dup2(connfd, STDOUT_FILENO);

        // store data to a file
        if (command.store_addr.size() > 0) {
            if (set_output_to_file(command) == 0)
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
            char err[1024];
            sprintf(err, "Unknown command: [%s].\n", cmd.argv[0]);
            Writen(STDERR_FILENO, err, strlen(err));

            // close pipe
            if (need_data) close(data_pipe[0]);
            if (need_pipe) close(p_num[1]);
            return 0;
        }
    }
    return 0;
}

void parse_line(char *line)
{
    // user pipe
    std::string s_line(line);
    
	bool storage_flg = false;
	const char *new_args = " \n\r";
	char *command = strtok(line, new_args);
	while(command != NULL){
		std::string s(command);

		// store the path
		if (storage_flg) {
			tmp.store_addr = s;
			storage_flg = false;
			C.push_back(tmp);
			clear_tmp();
			
			command = strtok(NULL, new_args);
			continue;
		}

        // parse msg
        if (s == "tell")
        {
            tmp.argv.push_back(s);

            char *to = strtok(NULL, " ");
            std::string t(to);
            tmp.argv.push_back(t);

            char *msg = strtok(NULL, "\n\r\0");
            std::string m(msg);
            tmp.argv.push_back(m);

            C.push_back(tmp);
            clear_tmp();
            return;
        }
        if (s == "yell")
        {
            tmp.argv.push_back(s);

            char *msg = strtok(NULL, "\n\r\0");
            std::string m(msg);
            tmp.argv.push_back(m);

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
		else if (command[0] == '|' || command[0] == '!') 
		{
			process_pipe_info(command);
			
			// add commands
			C.push_back(tmp);
			clear_tmp();
		}
        else if (s[0] == '<')
        {
            s.erase(0, 1);
            tmp.user_pipe_from = atoi(s.data())-1;
        }
		else if (s[0] == '>') 
        {
            if (s.size() == 1) storage_flg = true;
            else 
            {
                s.erase(0, 1);
                tmp.user_pipe_to = atoi(s.data())-1;
                tmp.user_pipe_command = s_line;
            }
        }
		else tmp.argv.push_back(s);

		command = strtok(NULL, new_args);
	}
	C.push_back(tmp);
	clear_tmp();

	// for (auto command: C)
	// {
    //     for (auto arg: command.argv)
	// 	    printf("%s ", arg.data());
    //     printf("\n-----------------------\n");
	// }
}

int check_user_pipe_from(int from, int index)
{
    int connfd = client[index].connfd;

    if (client[from].connfd < 0)
    {
        char msg[MY_LINE_MAX +50];
        memset(msg, '\0', MY_LINE_MAX+50);  
        sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", from+1);
        Writen(connfd, msg, strlen(msg));

        return 0;
    }
    if (user_pipe[from][index].pipe_num < 0)
    {
        char msg[MY_LINE_MAX +50];
        memset(msg, '\0', MY_LINE_MAX+50);
        sprintf(msg, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", from+1, index+1);
        Writen(connfd, msg, strlen(msg));

        return 0;
    }

    return 1;
}

int check_user_pipe_to(int index, int to)
{
    int connfd = client[index].connfd;
    
    if (client[to].connfd < 0)
    {
        char msg[MY_LINE_MAX +50];
        memset(msg, '\0', MY_LINE_MAX+50); 
        sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", to+1);
        Writen(connfd, msg, strlen(msg));
        return 0;
    }
    if (user_pipe[index][to].pipe_num > 0)
    {
        char msg[MY_LINE_MAX +50];
        memset(msg, '\0', MY_LINE_MAX+50); 
        sprintf(msg, "*** Error: the pipe #%d->#%d already exists. ***\n", index+1, to+1);
        Writen(connfd, msg, strlen(msg));
        return 0;
    }

    return 1;
}

void my_setenv(my_cmd &command)
{
	if (command.argv.size() != 3) {
		char err[] = "Usage: setenv [Variable] [Value].\n";
		Writen(STDERR_FILENO, err, sizeof(err));
	}
	setenv(command.argv[1].data(), command.argv[2].data(), 1);
}

void my_printenv(my_cmd &command)
{
	if (command.argv.size() != 2) {
		char err[] = "Usage: printenv [Variable].\n";
		Writen(STDERR_FILENO, err, sizeof(err));
	}
	char* env_info = getenv(command.argv[1].data());
	if (env_info != NULL) printf("%s\n", env_info);
}

void print_all_user(int index)
{
    int connfd = client[index].connfd;
    char msg[MSG_MAX];

    memset(msg, '\0', MSG_MAX);
    sprintf(msg, "<ID\t<nickname>\t<IP:port>\t<indicate me>\n");
    Writen(connfd, msg, strlen(msg));
    for (int i = 0; i < maxi; i++)
    {
        if (client[i].connfd < 0) continue;
        memset(msg, '\0', MSG_MAX);
        sprintf(msg, "%d\t%s\t%s:%d\t%s\n", i+1, client[i].name, inet_ntoa(client[i].addr.sin_addr), client[i].addr.sin_port, (i == index)?" <-me":"");
        Writen(connfd, msg, strlen(msg));
    }
}

void tell_msg(int from, int to, std::string msg)
{
    char snd[MSG_MAX];
    memset(snd, '\0', MSG_MAX);

    to -= 1;

    if (client[to].connfd < 0)
    {
        sprintf(snd, "*** Error: user #%d does not exist yet. ***\n", to);
        Writen(client[from].connfd, snd, strlen(snd));
        return;
    }

    sprintf(snd, "*** %s told you ***: %s\n", client[from].name, msg.data());
    Writen(client[to].connfd, snd, strlen(snd));
}

void yell_msg(int from, std::string msg)
{
    char snd[MSG_MAX];
    memset(snd, '\0', MSG_MAX);
    sprintf(snd, "*** %s yelled ***: <%s>\n", client[from].name, msg.data());
    broadcast(snd);
}

void change_name(int index, std::string name)
{
    char snd[MSG_MAX];
    memset(snd, '\0', MSG_MAX);

    int i;
    for (i = 0; i < maxi; i++)
    {   
        if (i == index) continue;
        if (strcmp(client[i].name, name.c_str()) == 0) 
        {
            sprintf(snd, "*** User '%s' already exists. ***\n", name.data());
            Writen(client[index].connfd, snd, strlen(snd));
            return;
        }
    }

    strcpy(client[index].name, name.data());
    sprintf(snd, "*** User from %s:%d is named '%s'. ***\n", inet_ntoa(client[index].addr.sin_addr), client[index].addr.sin_port, client[index].name);
    broadcast(snd);
}

int handle_data_from_multiple_pipe(int data_pipe[2], std::vector<int> data_list)
{
	// fork a child to process
	int pid = fork();
	if (pid < 0) {
		char err[] = "failed to fork\n";
		Writen(STDERR_FILENO, err, sizeof(err));
		return 0;
	}

	if (pid == 0) {
		// read data from each pipe (FIFO)
		for (auto id: data_list) {
			char read_data[1024];
			int read_length;
			while ((read_length = Readn(id, read_data, 1024)) > 0) {
				Writen(data_pipe[1], read_data, read_length);
			}
			close(id);
		}
		close(data_pipe[1]);
		return 1;
	}else{
		close(data_pipe[1]);
		for(auto id: data_list) close(id);
		data_list.clear();
		
		// information for signal handler about how to handle this child
		args cmd;
		cmd.argv = NULL; cmd.argc = 0;
		args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));
		return 1;
	}

	return 1;
}

int set_output_to_file(my_cmd &command)
{
	size_t file_id = open(command.store_addr.data(), O_WRONLY|O_CREAT|O_TRUNC, 00777);
	// clear the file content
	ftruncate(file_id, 0);
	lseek(file_id, 0, SEEK_SET);
	if (file_id < 0) {
		char err[1024];
		sprintf(err, "Failed to open file: %s\n", command.store_addr.data());
		Writen(STDERR_FILENO, err, strlen(err));
		return 0;
	}
	dup2(file_id, STDOUT_FILENO);
	return 1;
}

void check_need_data(bool &need, int (&data_pipe)[2], std::vector<int> &data_list) {
	// get all needed data for current command into datalist 
	// (datalist stores the needed read end of the pipe)
	
	data_list.clear();

	// counter == 0 => pipe the data to the command that execute next
	for (auto s: pipe_num_to) {
		if (s.second == 0) data_list.push_back(s.first);		
	}

	need = (data_list.size() > 0);
	if (!need) return;

	// if only one pipe of data needed
	// directly assign read p_id to data_pipe
	if(data_list.size() == 1) {
		data_pipe[0] = data_list[0];
		pipe_num_to.erase(data_list[0]);
		data_list.clear();
		return;
	}

	// parent will tackle the data from multiple pipe
	pipe(data_pipe);
	for (auto s: data_list) {
		pipe_num_to.erase(s);
	}
}

void update_pipe_num_to() { 
	std::vector<int> wait_to_erase;
	// update pipe counter
	for (auto &s: pipe_num_to) {
		s.second--;
		if (s.second < 0) wait_to_erase.push_back(s.first);
	}

	// erase unnecessary pipe id
	for (auto s: wait_to_erase) pipe_num_to.erase(s);
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
	for (int i = 1; i < s.size(); i++)
		tmp.pipe_to = tmp.pipe_to *10 + (int)(s[i] - '0');
}

void wait_all_children() {
	while (!args_of_cmd.empty()) {
		sig_chld(SIGCHLD);			
	}
}

void conditional_wait() {
	while(!args_of_cmd.empty()) {
		sig_chld(SIGCHLD);

		// stop waiting if remaining are all commands with number pipe
		// the output are not immediately needed
		bool must_wait = false;
		for (auto s:args_of_cmd) {
			int p_num = s.second.p_num;
			if (pipe_num_to.find(p_num) == pipe_num_to.end()) {
				must_wait = true;
				break;
			}
		}
		if (!must_wait) {
			break;
		}
	}
}

void sig_chld(int signo)
{
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		
		// free the memory!!
		char **cmd_argv = args_of_cmd[pid].argv;
		args_of_cmd.erase(pid);
		
		if (cmd_argv != NULL) free(cmd_argv);
    }

	return;
}

void err_sys(const char* x) { 
    perror(x); 
    exit(1); 
}

ssize_t	writen(int fid, const char *buf, size_t size) {
	size_t	nremain;
	ssize_t	nwritten;
	const char *buf_now;

	buf_now = buf;
	nremain = size;
	while (nremain > 0) {
		if ( (nwritten = write(fid, buf_now, nremain)) <= 0) {
			if (nwritten < 0 && errno == EINTR) nwritten = 0; // the error is not from write()
			else return -1; // write error
		}

		nremain -= nwritten;
		buf_now += nwritten;
	}
	return(size);
}

void Writen(int fid, char *buf, size_t size) {
	if (writen(fid, buf, size) != size)
		err_sys("writen error");
}

ssize_t	readn(int fid, char *buf, size_t size) {
	size_t nremain;
	ssize_t	nread;
	char *buf_now;

	buf_now = buf;
	nremain = size;
	while (nremain > 0) {
		if ( (nread = read(fid, buf_now, nremain)) < 0) {
			if (errno == EINTR) nread = 0; // the error is not from read
			else return -1; // read error
		} 
		else if (nread == 0) break; // EOF

		nremain -= nread;
		buf_now += nread;
	}
	return(size - nremain);
}

ssize_t Readn(int fid, char *buf, size_t size) {
	size_t	n;

	if ( (n = readn(fid, buf, size)) < 0)
		err_sys("readn error\n");
	return(n);
}