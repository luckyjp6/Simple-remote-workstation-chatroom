#ifndef PROC_CLIENT_H
#define PROC_CLIENT_H

struct my_cmd{
	std::vector<std::string> argv;
	std::string input_path = "";
	std::string output_path = "";
	bool append;
	bool pipe;	
	void clear() {
		argv.clear();
		input_path = "";
		output_path = "";
		append = false;
		pipe = false;
	}
};

struct args{
	char **argv;
	int argc;
	int in = -1;
    int out = -1;
};

extern client_pid me;

int to_client(int id);
void set_root_dir(char *name);

int execute_line(char *line);
int execute_command(my_cmd &command);
void parse_line(char *line);

void print_all_user();

void to_dev_null(); // redirect previous pipe output to /dev/null
// int get_file_fd(std::string path, int open_flag, int mode); // open file for redirection in/out
//                                                             // may have to consider truncate the file if it is already exist
int set_output_to_file(my_cmd &command);

void clear_tmp();

void process_pipe_info(std::string s); // process input command's pipe info

void wait_all_children(); // wait for all children at the end of parent process
void conditional_wait(); // conditionally wait for some children at the end of each line

// void err_sys(const char* x); // used in W/writen() and R/readn()
void sig_cli_chld(int signo); // signal handler
void sig_cli_int(int signo);
void sig_tell(int signo);
void sig_broadcast(int signo);
void FIFO_read(int from);

#endif