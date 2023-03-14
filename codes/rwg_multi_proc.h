#ifndef RWG_MULTI_PROC_H
#define RWG_MULTI_PROC_H

struct my_cmd{
	std::vector<std::string> argv;
	int pipe_to = 0; 
	std::string store_addr = "";
	bool err = false;
	bool number_pipe = false;
	int user_pipe_to = -1; // user pipe to whom (id)
	int user_pipe_from = -1; // user pipe from whom
	std::string user_pipe_command = "";
};

struct args{
	char **argv;
	int argc;
	bool number_pipe = false;
	int p_num = -1; // pipe id, used in conditional_wait()
	
	int to = -1, from = -1;
};

extern client_pid me;

int client(int id);
int execute_line(char *line);
int execute_command(my_cmd &command);
void parse_line(char *line);

int check_user_pipe_from(int from, int &u_from);
int check_user_pipe_to(int to, int &u_to, std::string &cmd_line);

void print_all_user();
void change_name(std::string name);

int handle_data_from_multiple_pipe(int data_pipe[2], std::vector<int> data_list);

int set_output_to_file(my_cmd &command);

void check_need_data(bool &need, int (&p_num)[2], std::vector<int> &data_list); 
void update_pipe_num_to();

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