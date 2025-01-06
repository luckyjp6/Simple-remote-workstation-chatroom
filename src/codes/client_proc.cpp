#include "client_proc.hpp"
#include <dirent.h>

/* read-end pipe fd for next command */
int pipe_fd = -1;

/* all currently executing commands */
std::map<size_t, args> args_of_cmd;

std::optional<args> my_cmd::get_args() {
    args cmd;

    /* prepare the arguments */
    cmd.argc = argv.size();
    cmd.argv = new char*[cmd.argc+1];
    for (size_t i = 0; i < cmd.argc; i++) {
        cmd.argv[i] = new char[argv[i].size() + 1];
        strcpy(cmd.argv[i], argv[i].c_str());
    }
    cmd.argv[cmd.argc] = nullptr; // end label

    /* file direction & normal pipe */
    if (need_input()) {
        /* open */
        cmd.in = open(input_path, O_RDONLY, 0);

        /* error handling */
        if (cmd.in < 0) {
            err_sys_cli("Invalid input source");
            return std::nullopt;
        }

        if (pipe_fd >= 0) {
            /* 
            If both input redirection & normal pipe are specified for the same process,
            use dev_null to consume the data in normal pipe.
            Input redirection has higher priority than pipe, which will be neglect if input file redirection is specified.
            (pipe has the same priority with stdin)
            */
            close(pipe_fd);
            to_dev_null();
        }
    } else if (pipe_fd >= 0) {
        cmd.in = pipe_fd;
        pipe_fd = -1;
    }
    if (need_output()) {
        /* prepare open options */
        int open_flg = O_WRONLY | O_CREAT;
        if (append) {
            open_flg |= O_APPEND;
        } else {
            open_flg |= O_TRUNC;
        }

        /* open */
        cmd.out = open(output_path, open_flg, 00700);

        /* error handling */
        if (cmd.out) {
            err_sys_cli("Invalid output source");
            return std::nullopt;
        }
    } else if (pipe_next) {
        int output_pipe[2];
        pipe(output_pipe);
        cmd.out = output_pipe[1];
        pipe_fd = output_pipe[0];
    }
}

void my_cmd::clear() {
    argv.clear();
    memset(input_path, 0, sizeof(input_path));
    memset(output_path, 0, sizeof(output_path));
    append = false;
    pipe_next = false;
}

client_proc::client_proc(int id, const char* name): my_id(id) {
/*
    Do: Initialize client process
    Return: client_proc instance
*/
    /* self identity*/
    strcpy(my_name, name);

    /* set home path */
    sprintf(home_path, "/home/%s", name);

}

void client_proc::show_welcome_msg() {
/* 
    Do: Show welcome message
    Return: None
*/
    printf("\033[35m __       __       __   ______    __ __    ____  ____\033[0m\n");
    printf("\033[35m \\ \\     /  \\     / /  /  __  \\   | '__/  / __ `' __  \\\033[0m\n");
    printf("\033[35m  \\ \\   / /\\ \\   / /  /  /  \\  \\  | |    |  | |  |  |  |\033[0m\n");
    printf("\033[35m   \\ \\_/ /  \\ \\_/ /   \\  \\__/  /  | |    |  | |  |  |  |\033[0m\n");
    printf("\033[35m    \\___/    \\___/     \\______/   |_|    |__| |__|  |__|\033[0m\n");
    printf("\n");
    printf("\033[35m{\\__/}\033[0m\n");
    printf("\033[35m( • - •)\033[0m\n");
    printf("\033[35m/ > \033[0m\033[36mWelcome back\033[0m %s\n", my_name);
    printf("\033[36mOnline users:\033[0m %d\n", shm_mgr.get_num_data(0));
    printf("\n");
    printf("\n");

    fflush(stdout);
}
void client_proc::show_goodbye_msg() {
    // printf("See you again!");
    // printf("\n");

    fflush(stdout);
}
void client_proc::show_prompt() {
    char *now_path = get_path();
    printf("\033[32m%s@worm_server:\033[36m%s\033[0m > ", my_name, now_path); 

    fflush(stdout);
}

char* client_proc::get_path() {
    char *now_path = getcwd(NULL, 0);
    if (memcmp(home_path, now_path, strlen(home_path)) == 0) {
        memmove(now_path, now_path + strlen(home_path), strlen(home_path) - strlen(home_path) + 1);
        now_path[0] = '~';
    }

    return now_path;
}

void client_proc::run() {
/*
    Do: Recv commands from stdin and execute.
    Return: None
*/
    /* start msg */
    show_welcome_msg();

    /* loop, recv new command and execute */
    for ( ; ; ) { 
        /* STDIN end of file */
        if (std::cin.eof()) {
            show_goodbye_msg();
            break;
        }

        /* show prompt */
        show_prompt();

        /* get command */
        char buf[MY_LINE_MAX] = {0};
        int len = read(STDIN_FILENO, buf, sizeof(buf));
        
        if (len < 0) {
            err_sys_cli("failed to read command");
            break;
        } else if (len <= 1) {
            /* blank line */
            continue;
        }

        /* execute command */
        int status = execute_line(buf);
        if (status == -1) {
            /* something very severe happened, s.t. the remote connection would be closed. */
            break;
        }
    }

    wait_all_children();
    sig_cli_int(0);
}

int client_proc::execute_line(char *line) {
/*
    Do: Parse line and execute commands.
    Return: status code
*/
    /* parse commands */
    parse_line(line);

    bool interrupt = false;
    int status = 0;
    for (int i = 0; (i < C.size()); i++) {
        /* pause & wait every 50 commands */
        if (i % 50 == 0 && i != 0) wait_all_children();

        /* execute command */
        if ((status = execute_command(C[i])) < 0) {
            switch (status) {
                case -1: // exit
                    return -1;             
                case -2: // error of input file (file redirection)
                    interrupt = true;
                    break;
            }
            break;
        }
    }
    if (pipe_fd >= 0 && interrupt) {
        close(pipe_fd);
    }
    wait_all_children();
    C.clear();
    
    return 0;
}

int client_proc::execute_command(my_cmd command) {
/*
    Do: Execute single command.
        - built-in command: exit, cd, setenv, printenv, who, ...
        - call executable @ PATH
*/
    /* built-in command */
    if (command.get_arg(0) == "exit") {
        C.clear();
        return -1;
    } else if (command.get_arg(0) == "setenv") {
        /* error usage */
        if (command.get_arg_num() < 3)
        {
            printf("Usage: setenv [env_name] [value].\n");
	        return 0;
        }

        /* execute */
        setenv(command.get_arg(1).c_str(), command.get_arg(2).c_str(), 1);
        return 0;
    } else if (command.get_arg(0) == "printenv") {
        /* error usage */
        if (command.get_arg_num() < 2) 
        {
            printf("Usage: printenv [env_name].\n");
            return 0;
        }

        /* execute */
        char* env_info = getenv(command.get_arg(1).c_str());
        if (env_info != NULL) {
            printf("%s\n", env_info);
        }

        return 0;
    } else if (command.get_arg(0) == "cd") {
        std::string destination(command.get_arg(1));
        /* cd to home directory */
        if (command.get_arg_num() < 2) {
            destination = home_path;
        }
        else if (command.get_arg(1) == "~") {
            destination = home_path;
        }
        else if (command.get_arg(1)[0] == '~') {
            destination.erase(0, 1);
            destination = home_path + destination;
        }

        /* execute */
        if (chdir(destination.c_str()) < 0) {
            err_sys_cli("cd");
        }
        return 0;
    } else if (command.get_arg(0) == "who") {
        /* execute */
        print_all_user();
        return 0;
    }
    
    /* executable @ PATH */
    auto cmd = command.get_args();
    if (!cmd) {
        /* error in file redirection */
        return -2;
    }
    
    /* fork */
    int pid = fork();
    if (pid < 0) {
        err_sys_cli("Too many process, failed to fork\n");
        return -2;
    }

    if (pid > 0) {
        /* parent side */
        /* close client's input/output */
        if (cmd->in >= 0) {
            close(cmd->in);
        }
        if (cmd->out >= 0) {
            close(cmd->out);
        }
        
        /* record memory allocate address */
        args_of_cmd.insert(std::pair<size_t, args> (pid, cmd.value()));

        return 0;
    } else {
        /* child side */

        /* set input/output */
        if (cmd->in >= 0) {
            dup2(cmd->in, STDIN_FILENO);
        }
        if (cmd->out >= 0) {
            dup2(cmd->out, STDOUT_FILENO);
        }
        if (pipe_fd >= 0) { 
            close(pipe_fd); 
        }

        // pipe stderr
        dup2(cmd->out, STDERR_FILENO);

        // execute
        if (execvp(cmd->argv[0], cmd->argv) < 0) {
            /* execution fall */
            err_sys_cli("Unknown command: [", cmd->argv[0],"].\n");

            // close pipe
            if (cmd->in >= 0) close(cmd->in);
            if (cmd->out >= 0) close(cmd->out);

            exit(-1);
        }
    }
    return 0;
}

void client_proc::parse_line(char *line) {
    // user pipe
    std::string s_line(line);
    
	// bool storage_flg = false;
    my_cmd temp;
	const char *new_args = " \n\r"; 
    char *word;
	while((word = strtok_r(line, new_args, &line)) != NULL) {
		std::string s(word);
        
        // parse msg
		if (s == "|") {
			temp.set_pipe();
		
			// add commands
			C.push_back(temp);
            temp.clear();
		} else if (s == "<") {
            /* input redirection */
            char *path = strtok_r(line, new_args, &line);
            temp.set_input_path(path);
        } else if (s == ">") {
            /* output redirection */
            char *path = strtok_r(line, new_args, &line);
            temp.set_output_path(path);
        } else if (s == ">>") {
            /* output redirection, append mode */
            char *path = strtok_r(line, new_args, &line);
            temp.set_output_path(path);
            temp.set_append();
        } else if (s[0] == '$') {
            /* substitute environment variable */
            
            /* get value */
            s.erase(0, 1);
            char *env_value = getenv(s.c_str());

            std::string v = "";
            if (env_value == NULL) {
                std::string v = "";
                temp.add_arg(v);
            } else {
                std::string v(env_value);
                temp.add_arg(v);
            }
        } else {
            /* ordinary arg */
            temp.add_arg(s);
        }

		word = strtok_r(line, new_args, &line);
	}
	if (temp.not_empty() > 0) {
        C.push_back(temp);
    	temp.clear();
    }
}

void my_cmd::to_dev_null() {
/*
    Do: Consume data from useless pipe
    Return: None
*/
    if (fork() == 0) {
        /* fork a child to consume the useless data in normal pipe */
        char buf[MY_LINE_MAX];
        while (read(pipe_fd, buf, MY_LINE_MAX) > 0) ;
        exit(0);
    }else {
        close(pipe_fd);
        pipe_fd = -1;
    }
}

void client_proc::print_all_user() {
/*
    Do: Built-in command "who", print all user online.
    Return: None
*/
    int num_user = shm_mgr.get_num_data(0);

    /* subtitle */
    printf("%5s%12s%26s%18s\n", "<ID>", "<nickname>", "<IP:port>", "<indicate me>\n");  
    
    for (int i = 0; i < MAX_USER && num_user > 0; i++)
    { 
        auto c = shm_mgr.read_user_info(i);
        if (!c) continue;

        num_user--;
        
        printf("%5d%12s%20s:%5d%17s\n", i+1, c->get_name(), c->get_addr(), c->get_port(), (i == my_id)?"<-me":"");
    }
}

void client_proc::wait_all_children() {
/* 
    Do: Wait for all children at the end of parent process.
    Return: None
*/
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