#include "functions_multi_proc.h"
#include "rwg_multi_proc.h"

std::vector<my_cmd> C; // after read one line of commands, stores them here
std::map< size_t, args > args_of_cmd; // pid, args
my_cmd tmp; // used in clear_tmp() and process_pipe_info()
std::vector<pnt> pipe_num_to; // pipe_num, counter   

client_pid me;
int FIFO_open[30];

int client(int id)
{
    return 0;
    me = cp[id];

    /* clean FIFO_open */
    memset(FIFO_open, -1, 30);
    C.clear();
    args_of_cmd.clear();
    clear_tmp();
    pipe_num_to.clear();
    
    /* show welcome message */
    char msg[MSG_MAX];
    memset(msg, '\0', MSG_MAX);
    sprintf(msg, "****************************************\n** Welcome to the information server. **\n****************************************\n");
    if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);

    /* broadcast message */
    memset(msg, '\0', MSG_MAX);
    sprintf(msg, "*** User '%s' entered from %s:%d. ***\n", me.name, me.addr, me.port);
    broadcast(msg);
    

    /* start to execute */
    while (true)
    {
        if (std::cin.eof()) break;

        if (send(STDOUT_FILENO, "% ", strlen("% "), MSG_NOSIGNAL) < 0) sig_cli_int(0); 
        fflush(stdout);

        char buf[MY_LINE_MAX];
        memset(buf, '\0', MY_LINE_MAX);
        int n = read(STDIN_FILENO, buf, MY_LINE_MAX);

		if (n < 0) {
			err_sys("failed to read\n");
			return -1;
		}
		else if (n <= 1) continue; // blank line

        int status;
        if ((status = execute_line(buf)) == -1) break; // for exit
        else if (status == -2) return 0; // for child
    }

    wait_all_children();
    
    sig_cli_int(0);

    exit(0);
    return 0;
}

int execute_line(char *line)
{
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

int execute_command(my_cmd &command)
{
    char msg[MY_LINE_MAX];
    if (command.argv[0] == "exit") {
        C.clear();
        return -1;
    }
    if (command.argv[0] == "setenv") 
    {
        if (command.argv.size() < 3)
        {
            // printf("Usage: setenv [Variable] [Value].\n");
	        return 0;
        }
        setenv(command.argv[1].data(), command.argv[2].data(), 1);
        return 0;
    }
    if (command.argv[0] == "printenv") 
    {
        if (command.argv.size() < 2) 
        {
            // printf("Usage: printenv [Variable].\n");
            return 0;
        }

        char* env_info = getenv(command.argv[1].data());
        if (env_info != NULL) 
        {
            memset(msg, '\0', MY_LINE_MAX);
            sprintf(msg, "%s\n", env_info);
            if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
        }

        fflush(stdout);
        return 0;
    }
    if (command.argv[0] == "who")
    {
        print_all_user();
        return 0;
    }
    if (command.argv[0] == "tell")
    {
        if (command.argv.size() < 3) 
        {
            // printf("Usage: tell <to whom> <msg>\n");
            return 0;
        }
        else {
            int to = atoi(command.argv[1].data())-1;

            if (to < 0) {
                memset(msg, '\0', MY_LINE_MAX);
                sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", to+1);
                if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
                return 0;
            }
            client_pid t(to);
            read_user_info(t);
                   
            if (t.connfd < 0) {
                memset(msg, '\0', MY_LINE_MAX);
                sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", to+1);
                if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
            }
            else {
                memset(msg, '\0', MSG_MAX);
                sprintf(msg, "*** %s told you ***: %s\n", me.name, command.argv[2].data());
                tell(msg, to);
            }
        }
        return 0;
    }
    if (command.argv[0] == "yell")
    {
        if (command.argv.size() < 2) 
        {
            // printf("Usage: yell <msg>\n");
            return 0;
        }
        else {
            memset(msg, '\0', MSG_MAX);
            sprintf(msg, "*** %s yelled ***: %s\n", me.name, command.argv[1].data());
            broadcast(msg);
        }

        return 0;
    }
    if (command.argv[0] == "name")
    {
        if (command.argv.size() < 2) 
        {
            // printf("Usage: name <name>\n");
            return 0;
        }
        else change_name(command.argv[1]);

        return 0;
    }

    args cmd;
    cmd.argc = command.argv.size();
    cmd.argv = (char**) malloc(sizeof(char*) * (cmd.argc+1));
    cmd.number_pipe = command.number_pipe;
    cmd.to = cmd.from = -1;

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
    int from = command.user_pipe_from;
    if (from >= 0)
    {
        u_pipe_from = true;

        if (check_user_pipe_from(from, u_from) < 0) u_from = open("/dev/null", O_RDONLY);
        else 
        {
            client_pid c(from);
            read_user_info(c);
            
            std::string cmd_line = command.user_pipe_command;
            cmd_line.erase(cmd_line.end()-1);

            memset(msg, '\0', MY_LINE_MAX+50);
            sprintf(msg, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", me.name, me.id+1, c.name, from+1, cmd_line.data());
            broadcast(msg);

            cmd.from = from;
        }        
    }

    // user pipe to other user
    
    int to = command.user_pipe_to;
    if (to >= 0)
    {
        u_pipe_to = true;

        if (check_user_pipe_to(to, u_to, command.user_pipe_command) < 0) u_to = open("/dev/null", O_WRONLY);
        else 
        {
            cmd.to = to;
        }
    }

    check_need_data(need_data, data_pipe, data_list);
  
    // fork
    pid = fork();
    if (pid < 0) 
    {
        // printf("failed to fork\n");
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
            memset(msg, '\0', MY_LINE_MAX);
            sprintf(msg, "Unknown command: [%s].\n", cmd.argv[0]);
            if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);

            // close pipe
            if (need_data) close(data_pipe[0]);
            if (need_pipe) close(p_num[1]);

            return -2;
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
            tmp.user_pipe_command = s_line;
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
	if (tmp.argv.size() > 0) 
    {
        C.push_back(tmp);
    	clear_tmp();
    }
}

int check_user_pipe_from(int from, int &u_from)
{
    char msg[MY_LINE_MAX];
    
    client_pid f(from);
    read_user_info(f);
    
    /*check if user with id 'from' exist*/
    if (f.connfd < 0) 
    { 
        memset(msg, '\0', MY_LINE_MAX);
        sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", from+1);
        if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);

        if (FIFO_open[from] > 0) {
            FIFO_open[from] = -1;

            char FIFO_name[50];
            memset(FIFO_name, '\0', 50);
            sprintf(FIFO_name, "user_pipe/%d_%d.txt", from, me.id);

            if (remove(FIFO_name) < 0) 
            {
                // printf("can't unlink\n");
                return -1;
            }
        }
        return -1;
    }

    /*check if user pipe 'from->me' exist, maybe store the information in share memory*/
    if ((u_from = FIFO_open[from]) < 0)
    {
        memset(msg, '\0', MY_LINE_MAX);
        sprintf(msg, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", from+1, me.id+1);
        if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
        
        FIFO_open[from] = -1;
        return -1;
    }

    FIFO_open[from] = -1;       

    return 0;
}

int check_user_pipe_to(int to, int &u_to, std::string &cmd_line)
{
    char msg[MY_LINE_MAX];
    /*check if user with id 'to' exist*/
    client_pid t(to);
    read_user_info(t);

    char FIFO_name[50];
    memset(FIFO_name, '\0', 50);
    sprintf(FIFO_name, "user_pipe/%d_%d.txt", me.id, to);
    if (t.connfd < 0)
    {   
        memset(msg, '\0', MY_LINE_MAX);
        sprintf(msg, "*** Error: user #%d does not exist yet. ***\n", to+1);
        if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
        return -1;
    }
                
    if (mkfifo(FIFO_name, S_IFIFO | 0777) < 0)
    {
        if (errno == EEXIST) 
        {
            memset(msg, '\0', MY_LINE_MAX);
            sprintf(msg, "*** Error: the pipe #%d->#%d already exists. ***\n", me.id+1, to+1);
            if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
            return -1;
        }
        return -1;
    }  

    /* tell the reader to open FIFO read */
    cmd_line.erase(cmd_line.end()-1);
    memset(msg, '\0', MY_LINE_MAX);
    sprintf(msg, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", me.name, me.id+1, cmd_line.data(), t.name, t.id+1);
    broadcast(msg);

    if ((u_to = open(FIFO_name, O_WRONLY)) < 0)
    {
        memset(msg, '\0', MY_LINE_MAX);
        sprintf(msg, "can't open write fifo: %s\n", FIFO_name);
        if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
        return -1;
    }

    return 0;
}

void print_all_user()
{
    int num_user = get_shm_num(shm_id[0]);
    char msg[MY_LINE_MAX], now[SHM_SIZE];
    
    memset(msg, '\0', MY_LINE_MAX);
    sprintf(msg, "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");  
    if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
    
    for (int i = 0; i < OPEN_MAX && num_user > 0; i++)
    { 
        client_pid c(i);
        read_user_info(c);
        if (c.connfd < 0) continue;

        num_user--;
        
        memset(msg, '\0', MY_LINE_MAX);
        sprintf(msg, "%d\t%s\t%s:%d\t%s\n", i+1, c.name, c.addr, c.port, (c.id == me.id)?" <-me":"");
        if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
    }
}

void change_name(std::string new_name)
{
    char msg[MY_LINE_MAX];
    int num_user = get_shm_num(shm_id[0]);

    for (int i = 0; i < OPEN_MAX && num_user > 0; i++)
    {       
        if (i == me.id) {
            num_user--;
            continue; 
        }

        client_pid c(i);
        read_user_info(c);
        if (c.connfd < 0) continue;

        num_user--;
        
        if (strcmp(c.name, new_name.c_str()) == 0) 
        {
            memset(msg, '\0', MY_LINE_MAX);
            sprintf(msg, "*** User '%s' already exists. ***\n", new_name.data());
            if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0);
            return;
        }
    }
    
    read_user_info(me);
    strcpy(me.name, new_name.c_str());
    write_user_info(me);
    
    memset(msg, '\0', MY_LINE_MAX);
    sprintf(msg, "*** User from %s:%d is named '%s'. ***\n", me.addr, me.port, me.name);
    broadcast(msg);
}

int handle_data_from_multiple_pipe(int data_pipe[2], std::vector<int> data_list)
{

	// fork a child to process
	int pid = fork();
    
	if (pid < 0) {
		// printf("failed to fork\n");
		return -1;
	}

	if (pid == 0) {
		// read data from each pipe (fifo)
		for (auto id: data_list) {
			char read_data[1024];
			int read_length;
			while ((read_length = read(id, read_data, 1024)) > 0) {
				if (send(data_pipe[1], read_data, read_length, MSG_NOSIGNAL) < 0) sig_cli_int(0);
			}
			close(id);
		}
		close(data_pipe[1]);
        exit(0);
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

int set_output_to_file(my_cmd &command)
{
	size_t file_id = open(command.store_addr.data(), O_WRONLY|O_CREAT|O_TRUNC, 00777);
	// clear the file content
	ftruncate(file_id, 0);
	lseek(file_id, 0, SEEK_SET);
	if (file_id < 0) {
		// char err[1024];
		// sprintf(err, "Failed to open file: %s\n", command.store_addr.data());
		// write(STDERR_FILENO, err, strlen(err));
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

void sig_cli_chld(int signo)
{
	int	pid, stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0){
		// free the memory!!
		char **cmd_argv = args_of_cmd[pid].argv;
        if (cmd_argv != NULL) free(cmd_argv);
		
        if (args_of_cmd[pid].from >= 0)
        {
            char FIFO_name[50];
            memset(FIFO_name, '\0', 50);
            sprintf(FIFO_name, "user_pipe/%d_%d.txt", args_of_cmd[pid].from, me.id);
            
            if (remove(FIFO_name) < 0)
            {
                // printf("can't remove %s\n", FIFO_name);
            }
        }
        args_of_cmd.erase(pid);		
    }

	return;
}

void sig_cli_int(int signo)
{    
    /* disconnection, close by server */
    
    close(me.connfd);
    /* close user FIFO */
    for (auto arg:args_of_cmd)
    {
        kill(arg.first, 9);
        sig_cli_chld(0);
    }

    while (args_of_cmd.size() > 0) sig_cli_chld(0);

    exit(0);
    return;
}

void sig_tell(int signo)
{
    char *msg_shm = (char *)shmat(shm_id[2], 0, 0);
    if (msg_shm == NULL) err_sys("shmat fail");
    
    int len = get_shm_num(shm_id[2]);
    char msg[MY_LINE_MAX];
    memset(msg, '\0', MY_LINE_MAX);
    memcpy(msg, msg_shm+5, len);
    
    char nn[4]; 
    memset(nn, '\0', 4);
    sprintf(nn, "%d", 0);
    memcpy(msg_shm, nn, 4);

    if (shmdt(msg_shm) < 0) err_sys("shmdt fail");

    if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0); 
    fflush(stdout);

    std::string f(msg);
    int jp = f.find("just piped");
    if (jp > 0) 
    {
        char one[3] = "(#", mmee[10];
        memset(mmee, '\0', 10);
        sprintf(mmee, "(#%d)", me.id+1);
        int oo = f.find(one), tt = f.find(mmee);
        if (oo > 0 && tt > 0 && oo != tt) 
        {
            int from;
            char char_f[4];
            memset(char_f, '\0', 4);            
            memcpy(char_f, msg + oo + 2, 4);
            from = atoi(char_f);
            FIFO_read(from-1);
        }
    }
    
    return;
}

void sig_broadcast(int signo)
{
    char *shm_addr = (char *)shmat(shm_id[1], 0, 0);
    if (shm_addr == NULL) err_sys("shmat fail");

    int msg_len = get_shm_num(shm_id[1]);
    char msg[SHM_SIZE];
    memcpy(msg, shm_addr+10, msg_len+1);
    
    if (shmdt(shm_addr) < 0) err_sys("shmdt fail");

    update_read_user();

    if (send(STDOUT_FILENO, msg, strlen(msg), MSG_NOSIGNAL) < 0) sig_cli_int(0); 
    fflush(stdout);

    std::string f(msg);
    int jp = f.find("just piped");
    if (jp > 0) 
    {
        char one[3] = "(#", mmee[10];
        memset(mmee, '\0', 10);
        sprintf(mmee, "(#%d)", me.id+1);
        int oo = f.find(one), tt = f.find(mmee);
        if (oo > 0 && tt > 0 && oo != tt) 
        {
            int from;
            char char_f[4];
            memset(char_f, '\0', 4);
            memcpy(char_f, msg + oo + 2, 4);
            from = atoi(char_f);
            FIFO_read(from-1);
        }
    }
}

void FIFO_read(int from)
{
    /* open FIFO read */
    char FIFO_name[50];
    memset(FIFO_name, '\0', 50);
    sprintf(FIFO_name, "user_pipe/%d_%d.txt", from, me.id);
    
    if ((FIFO_open[from] = open(FIFO_name, O_RDONLY)) < 0)
    {
        // printf("can't open read fifo: %s\n", FIFO_name);
        return;
    }

    return;
}