#ifndef RWG_MULTI_PROC_H
#define RWG_MULTI_PROC_H

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
	int user_pipe_to = -1; // user pipe to whom (id)
	int user_pipe_from = -1; // user pipe from whom
	std::string user_pipe_command = "";
};

struct args{
	char **argv;
	int argc;
	bool number_pipe = false;
	int p_num = -1; // pipe id, used in conditional_wait()
};

int execute_line(int input_index, char *line);
void parse_line(char *line);
int check_user_pipe_from(int from, int index);
int check_user_pipe_to(int index, int to);
int execute_command(my_cmd &command);

void my_setenv(int connfd, my_cmd &command);
void my_printenv(int connfd, my_cmd &command);
void print_all_user(int index);
void tell_msg(int from, int to, std::string msg);
void yell_msg(int from, std::string msg);
void change_name(int index, std::string name);

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