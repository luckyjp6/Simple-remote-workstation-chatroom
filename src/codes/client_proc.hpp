#ifndef CLIENT_PROC_HPP
#define CLIENT_PROC_HPP

#include <unistd.h>
#include <vector>
#include <map>
#include <optional>

#include "util.hpp"
#include "shm_manager.hpp"

struct args{
	char **argv;
	int argc;
	int in = -1;
    int out = -1;
};

class my_cmd{
private:
	std::vector<std::string> argv;
	char input_path[MY_LINE_MAX] = "";
	char output_path[MY_LINE_MAX] = "";
	bool append = false;
	bool pipe_next = false;	
	
	/* consume useless data */
	void to_dev_null();
public:
	my_cmd() {}
	void add_arg(std::string arg) { argv.push_back(arg); }
	void set_arg(int idx, std::string value) { argv[idx] = value; }
	void set_input_path(char *p) { strcpy(input_path, p); }
	void set_output_path(char *p) { strcpy(output_path, p); }
	void set_pipe() { pipe_next = true; }
	void set_append() { append = true; }

	ssize_t get_arg_num() { return argv.size(); }
	std::string get_arg(int idx) { return argv[idx]; }
	std::optional<args> get_args();

	bool is_pipe() { return pipe_next; }
	bool need_input() { return strlen(input_path); }
	bool need_output() { return strlen(output_path); }
	bool not_empty() { return argv.size(); }

	void clear();
};

class client_proc {
private:
	/* self identity */
	int my_id;
	char my_name[MY_NAME_MAX];
	char home_path[MY_LINE_MAX];

	/* commands & args */
    std::vector<my_cmd> C;

	/* shared memory*/ 
	shm_manager shm_mgr;
	
	/* print messages */
	void show_welcome_msg();
	void show_goodbye_msg();
	void show_prompt();

	/* useful tool */
	char* get_path();

	/* execution */
	void parse_line(char* line);
	int execute_line(char* line);
	int execute_command(my_cmd cmd);
	void wait_all_children();

	/* built-in command */
	void print_all_user();

public:
	client_proc() {};
	client_proc(int id, const char* name);
	void run();

	static client_proc& getInstance() {
		static client_proc instance;
		return instance;
	}
};

void process_pipe_info(std::string s); // process input command's pipe info
void conditional_wait(); // conditionally wait for some children at the end of each line

/* signal handler */
void sig_cli_chld(int signo); 
void sig_cli_int(int signo);

#endif