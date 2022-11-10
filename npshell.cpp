#include "npshell.h"

#define LINE_MAX 10000

std::vector<my_cmd> C; // after read one line of commands, stores them here
std::map< size_t, args> args_of_cmd; // pid, args
std::map< size_t, int > pipe_num_to; // pipe_num, counter
my_cmd tmp; // used in clear_tmp() and process_pipe_info()

int main(int argc, char **argv)
{
	setenv("PATH", "bin:.", 1);
	signal(SIGCHLD, sig_chld);

	bool should_run = true;
	while(should_run)
	{
		printf("%% "); fflush(stdout);
		char line[LINE_MAX];
		int input_length = read(STDIN_FILENO, line, LINE_MAX);

		if (input_length < 0)
		{
			err_sys("failed to read\n");
			return -1;
		}
		else if (input_length <= 1) continue; // blank line

		parse_line(line);

		int i = 0;
		for (auto command: C)
		{
			if (i++ % 50 == 0 && i != 0) conditional_wait();

			if (command.argv[0] == "exit") {
				update_pipe_num_to();
				C.clear();
				should_run = false;
				break;
			}
			if (command.argv[0] == "setenv") 
			{	
				update_pipe_num_to();
				my_setenv(command);
				continue;
			}
			if (command.argv[0] == "printenv") 
			{
				update_pipe_num_to();
				my_printenv(command);
				continue;
			}

			args cmd;
			cmd.argc = command.argv.size();
			cmd.argv = (char**) malloc(sizeof(char*) * (cmd.argc+1));
			cmd.number_pipe = command.number_pipe;

			bool need_data, need_pipe = (command.pipe_to > 0);
			size_t pid;
			int p_num[2], data_pipe[2];
			std::vector<int> data_list;

			// record pipe descripter for the command behind to read
			if (need_pipe) 
			{
				pipe(p_num);
				cmd.p_num = p_num[0];
			}

			check_need_data(need_data, data_pipe, data_list);
			
			// fork
			pid = fork();
			if (pid < 0) 
			{
				char *err;
				sprintf(err, "failed to fork\n");
				Writen(STDERR_FILENO, err, strlen(err));
				break;
			}

			// parent do
			if (pid > 0)
			{
				if (need_pipe) close(p_num[1]);
				if (need_data) close(data_pipe[0]);

				// record memory allocate address
				args_of_cmd.insert(std::pair<size_t, args> (pid, cmd));
				
				// update when new line
				if (command.number_pipe) update_pipe_num_to();
				
				// store read id of pipe
				if (command.number_pipe) pipe_num_to.insert(std::pair<size_t, int>(p_num[0], command.pipe_to-1));
				else if (need_pipe) pipe_num_to.insert(std::pair<size_t, int>(p_num[0], 0));

				// process data from multiple pipe
				if (need_data && data_list.size() != 0) 
					if (handle_data_from_multiple_pipe(data_pipe, data_list) == 0) break;
				
				continue;
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

				// pipe stderr
				if (command.err) dup2(p_num[1], STDERR_FILENO);

				// pipe data to other process
				if (need_pipe) {
					close(p_num[0]);
					dup2(p_num[1], STDOUT_FILENO);
				}

				// store data to a file
				if (command.store_addr.size() > 0) {
					if (set_output_to_file(command) == 0)
					{
						free(cmd.argv);
						return -1;
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
					return -1;
				}
			}
		}

		// wait for all children, except for command with number pipe
		conditional_wait();
		C.clear();
	}

	// wait for all children
	wait_all_children();
	return 0;
}

void parse_line(char *line)
{
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

		if (s == "|")
		{
			tmp.pipe_to = 1;
		
			// add commands
			C.push_back(tmp);
			clear_tmp();
		}
		else if(command[0] == '|' || command[0] == '!') 
		{
			process_pipe_info(command);
			
			// add commands
			C.push_back(tmp);
			clear_tmp();
		}
		else if(s == ">") storage_flg = true;
		else tmp.argv.push_back(s);

		command = strtok(NULL, new_args);
	}
	C.push_back(tmp);
	clear_tmp();

	for (int i = 0; i < C.size(); i++)
	{
		printf("command: %s\n", C[i].argv[0].data());
	}
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