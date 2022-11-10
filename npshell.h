#ifndef NPSHELL_H
#define NPSHELL_H

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <sys/wait.h>
#include <map>

struct my_cmd{
	std::vector<std::string> argv;
	int pipe_to = 0; 
	std::string store_addr = "";
	bool err = false;
	bool number_pipe = false;
};

struct args{
	char **argv;
	int argc;
	bool number_pipe = false;
	int p_num = -1; // pipe id, used in conditional_wait()
};

// extern std::vector<my_cmd> C; // after read one line of commands, stores them here
// extern std::map< size_t, args> args_of_cmd; // pid, args
// extern std::map< size_t, int > pipe_num_to; // pipe_num, counter
// extern my_cmd tmp; // used in clear_tmp() and process_pipe_info()

void parse_line(char *line);
int execute_command(my_cmd &command);

void my_setenv(my_cmd &command);
void my_printenv(my_cmd &command);

int handle_data_from_multiple_pipe(int data_pipe[2], std::vector<int> data_list);

int set_output_to_file(my_cmd &command);

void err_sys(const char* x); // used in W/writen() and R/readn()
void sig_chld(int signo); // signal handler
void wait_all_children(); // wait for all children at the end of parent process
void conditional_wait(); // conditionally wait for some children at the end of each line

// safer read and write
ssize_t writen(int fid, const char *buf, size_t size);
void Writen(int fid, char *buf, size_t size);
ssize_t	readn(int fid, char *buf, size_t size);
ssize_t Readn(int fid, char *buf, size_t size);

void check_need_data(bool &need, int (&p_num)[2], std::vector<int> &data_list); 
void update_pipe_num_to();

void clear_tmp();
void process_pipe_info(std::string s); // process input command's pipe info

#endif