// apt-get install libreadline-dev libbsd-dev

#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <bsd/string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct {
    union {
        int descriptor;
        char *filename;
    };
    bool append;
    enum {
        no_replacement,
        descriptor_replacement,
        filename_replacement
    } type;
} file_replacement;

void set_descriptor(file_replacement *f, int descriptor) {
    if (f->type == no_replacement) {
        f->type = descriptor_replacement;
        f->descriptor = descriptor;
    } else {
        printf("error: cannot set multiple descriptors\n");
        exit(1);
    }
}

void set_filename(file_replacement *f, char *filename) {
    if (f->type == no_replacement) {
        f->type = filename_replacement;
        f->filename = filename;
    } else {
        printf("error: cannot set multiple descriptors\n");
        exit(1);
    }
}

typedef struct {
    char **argv;
    file_replacement stdin;
    file_replacement stdout;
    file_replacement stderr;
} command;

typedef enum {
    arg_type,
    in_file_type,
    out_file_type,
    err_file_type
} token_type;

typedef enum {
    awaiting_token_state,
    consuming_token_state
} parse_state;

void execute_command(command cmd, int* input_fds, int* output_fds) {
	// Check for internal commands first
	if ((strcmp(cmd.argv[0], "cd") == 0) || (strcmp(cmd.argv[0], "chdir") == 0)) {
		char *dest = cmd.argv[1] ? cmd.argv[1] : getenv("HOME");  // If no argument is given, go to user's homedir
		if (chdir(dest) < 0) {
			perror("Chdir error");
		}
		return;
	}
	if (strcmp(cmd.argv[0], "exit") == 0) {
		exit(0);
	}
    if (strcmp(cmd.argv[0], "history") == 0) {
        int i = 1;
        for (HIST_ENTRY **h = history_list(); *h != NULL; i++, h++) {
            printf("   %i  %s\n", i, (*h)->line);
        }
        return;
    }
	
	// It's not an internal command, so fork out an external command
	pid_t pid = fork();
	if (pid < 0) {                     // error
		perror("Forking error");
	} else if (pid == 0) {             // child process
		if (input_fds) {
			close(input_fds[1]);
			dup2(input_fds[0], STDIN_FILENO);
			close(input_fds[0]);
		} else if (cmd.stdin.type == filename_replacement) {  // pipe input/output overrides chevron input/output
			int fd = open(cmd.stdin.filename, O_RDONLY);
            if (fd < 0) return perror("File input error");
            dup2(fd, STDIN_FILENO); // replace STDIN with file
            close(fd);              // decrement reference count
		}
		if (output_fds) {
			close(output_fds[0]);
			dup2(output_fds[1], STDOUT_FILENO);
			close(output_fds[1]);
		} else if (cmd.stdout.type == filename_replacement) {
			int flag = cmd.stdin.append ? O_APPEND : O_TRUNC;
            int fd = open(cmd.stdout.filename, O_CREAT | flag | O_WRONLY, 0);
            if (fd < 0) return perror("File output error");
            dup2(fd, STDOUT_FILENO); // replace STDOUT with file
            close(fd);               // decrement reference count
		}
		if (cmd.stderr.type == filename_replacement) {
            int flag = cmd.stderr.append ? O_APPEND : O_TRUNC;
            int fd = open(cmd.stderr.filename, O_CREAT | flag | O_WRONLY, 0);
            if (fd < 0) return perror("File output error");
            dup2(fd, STDERR_FILENO); // replace STDERR with file
            close(fd);               // decrement reference count
        }
		if (execvp(cmd.argv[0], cmd.argv) < 0) {
			perror("Exec error");
		}
	} else {}    // parent process
}

void execute_commands(command *cmds, command *cmds_end) {
	int num_cmds = cmds_end - cmds;
	if (num_cmds == 1) {
		execute_command(cmds[0], NULL, NULL);
	} else {
		int pipes[num_cmds - 1][2];
		for (int i = 0; i < num_cmds - 1; i++) {
			if (pipe(pipes[i]) < 0) {
				perror("Pipe error");
			}
		}
		execute_command(cmds[0], NULL, pipes[0]);
		for (int i = 1; i < num_cmds - 1; i++) {
			execute_command(cmds[i], pipes[i-1], pipes[i]);
			close(pipes[i-1][0]);
			close(pipes[i-1][1]);
		}
		execute_command(cmds[num_cmds - 1], pipes[num_cmds - 2], NULL);
		close(pipes[num_cmds - 2][0]);
		close(pipes[num_cmds - 2][1]);
	}

	int pid, status;
	while ((pid = wait(&status)) != -1){
		#if DEBUG
		fprintf(stderr, "process %d exits with %d\n", pid, WEXITSTATUS(status));
		#endif // DEBUG
	}
}

// Returns pointer after last command
// arg_buffer will store a NULL-seperated collection of arg lists
command *parse_commands(char *line, command *cmds, char **arg_buffer) {
    // Set up loop variables
    char **curr_arg = arg_buffer;
    command *curr_cmd = cmds;
    
    // Parse input
    token_type next_type = arg_type;
    parse_state state = awaiting_token_state;
    
    for (char *c = line; *c != (char)0; c++) {
        bool is_token_separator = true;
        switch (*c) {
            case '|':
                // End of command
                *curr_arg++ = NULL; // NULL-terminate arg list and increment
                curr_cmd++;         // Increment to next command
                
                switch (next_type) {
                    case arg_type:
                        break;
                        
                    default:
                        // Handle invalid syntax
                        printf("syntax error: expected descriptor\n");
                        exit(1);
                        break;
                }
                break;
                
            case ' ':
            case '\t':
                // Skip
                break;
                
            case '<':
                // Handle non-token in-chevron
                switch (next_type) {
                    case arg_type:
                        next_type = in_file_type;
                        break;
                        
                    default:
                        // Handle invalid syntax
                        printf("syntax error: unexpected <\n");
                        exit(1);
                }
                break;
                
            case '>':
                // Handle non-token out-chevron
                switch (next_type) {
                    case arg_type:
                        next_type = out_file_type;
                        break;
                        
                    case out_file_type:
                        if (!curr_cmd->stdout.append) {
                            curr_cmd->stdout.append = true;
                        } else {
                            printf("syntax error: unexpected >\n");
                            exit(1);
                        }
                        break;
                        
                    case in_file_type:
                        if (!curr_cmd->stdin.append) {
                            curr_cmd->stdin.append = true;
                        } else {
                            printf("syntax error: unexpected >\n");
                            exit(1);
                        }
                        break;
                        
                    default:
                        printf("syntax error: unexpected >\n");
                        exit(1);
                }
                break;
                
            case '&':
                c++; // increment to number
                int num = *c - '0';
                if (num < 0 || num > 2) {
                    printf("syntax error: invalid file descriptor\n");
                    exit(1);
                }
                switch (next_type) {
                    case in_file_type:
                        set_descriptor(&(curr_cmd->stdin), num);
                        break;
                        
                    case out_file_type:
                        set_descriptor(&(curr_cmd->stdout), num);
                        break;
                        
                    case err_file_type:
                        set_descriptor(&(curr_cmd->stderr), num);
                        break;
                        
                    default:
                        printf("syntax error: unexpected &\n");
                        exit(1);
                }
                next_type = arg_type;
                break;
                
            case '0':
            case '1':
            case '2':
            {
                if (*(c + 1) == '>') {
                    char num = *c++;
                    switch (num) {
                        case '0':
                            next_type = in_file_type;
                            break;
                            
                        case '1':
                            next_type = out_file_type;
                            break;
                            
                        case '2':
                            next_type = err_file_type;
                            break;
                            
                        default:
                            break;
                    }
                    break;
                } // otherwise read the number normally
            }
                
            default:
                is_token_separator = false;
                switch (state) {
                    case awaiting_token_state:
                        state = consuming_token_state;
                        
                        // Initialize the command if it has no argv list
                        if (!curr_cmd->argv) {
                            // Set argv list
                            curr_cmd->argv = curr_arg;
                        }
                        
                        switch (next_type) {
                            case arg_type:
                                *curr_arg++ = c; // Add argument to list and increment
                                break;
                                
                            case in_file_type:
                                set_filename(&(curr_cmd->stdin), c);
                                break;
                                
                            case out_file_type:
                                set_filename(&(curr_cmd->stdout), c);
                                break;
                                
                            case err_file_type:
                                set_filename(&(curr_cmd->stderr), c);
                                break;
                                
                            default:
                                exit(1);
                        }

                        break;
                        
                    default:
                        break;
                }
                break;
        }
        if (is_token_separator) {
            *c = (char)0;
            state = awaiting_token_state;
        }
    }
    return curr_cmd + 1;
}

int main() {
    char *line;
    char prompt[80], login_buf[80], cwd_buf[80];
    while(true) {
        // Get login and cwd
        getlogin_r(login_buf, sizeof(login_buf));
        getcwd(cwd_buf, sizeof(cwd_buf));
        
        // Write login and cwd into prompt
        strlcpy(prompt, login_buf, sizeof(prompt));
        strlcat(prompt, ":", sizeof(prompt));
        strlcat(prompt, cwd_buf, sizeof(prompt));
        strlcat(prompt, "> ", sizeof(prompt));
        
        // Display prompt and read line
        line = readline(prompt);
        
        // Add line to history
        add_history(line);
        
        // Set up array of commands and arguments
        command cmds[100] = {{0}};
        char *arg_buffer[100] = {0};
        
        // Parse commands
        command *cmds_end = parse_commands(line, cmds, arg_buffer);
        
        // Display line back
		execute_commands(cmds, cmds_end);
        
        // Free line
        free(line);
    }
    return 0;
}
