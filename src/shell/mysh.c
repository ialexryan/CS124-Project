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

#define DEBUG 0

typedef struct {
    char **argv;
    char *in_filename;
    char *out_filename;
    char *error_filename;
    bool appending;
} command;

typedef enum {
    TokenTypeBeginArgument,
    TokenTypeBeginInFile,
    TokenTypeBeginOutFile,
    TokenTypeBeginErrorFile,
    TokenTypeOther
} token_type;

void execute_command(command cmd, int* input_fds, int* output_fds) {
	// Check for internal commands first
	if ((strcmp(cmd.argv[0], "cd") == 0) || (strcmp(cmd.argv[0], "chdir") == 0)) {
		if (chdir(cmd.argv[1]) < 0) {
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
		} else if (cmd.in_filename) {  // pipe input/output overrides chevron input/output
			int fd = open(cmd.in_filename, O_RDONLY);
            if (fd < 0) return perror("File input error");
            dup2(fd, STDIN_FILENO); // replace STDIN with file
            close(fd);              // decrement reference count
		}
		if (output_fds) {
			close(output_fds[0]);
			dup2(output_fds[1], STDOUT_FILENO);
			close(output_fds[1]);
		} else if (cmd.out_filename) {
			int flag = cmd.appending ? O_APPEND : O_TRUNC;
            int fd = open(cmd.out_filename, O_CREAT | flag | O_WRONLY, 0);
            if (fd < 0) return perror("File output error");
            dup2(fd, STDOUT_FILENO); // replace STDOUT with file
            close(fd);               // decrement reference count
		}
		if (cmd.error_filename) {
            int fd = open(cmd.error_filename, O_CREAT | O_TRUNC | O_WRONLY, 0);
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
        char *arg_buffer[100] = {0}; // NULL-separated arg lists
        
        // Set up loop variables
        char **curr_arg = arg_buffer;
        command *curr_cmd = cmds;
        
        // Parse input
        token_type next_type = TokenTypeBeginArgument;
        for (char *c = line; *c != (char)0; c++) {
            switch (*c) {
                case '|':
                    // End of command
                    *curr_arg++ = NULL; // NULL-terminate arg list and increment
                    curr_cmd++;         // Increment to next command
                    
                    // Handle non-token pipe
                    switch (next_type) {
                        case TokenTypeOther:
                        case TokenTypeBeginArgument:
                            next_type = TokenTypeBeginArgument;
                            break;
                            
                        default:
                            printf("syntax error\n");
                            exit(1);
                    }
                    *c = (char)0;
                    break;
                    
                case ' ':
                case '\t':
                    // Handle non-token whitespace
                    switch (next_type) {
                        case TokenTypeOther:
                            next_type = TokenTypeBeginArgument;
                            break;
                            
                        default:
                            // Extraneous spaces are O.K.
                            break;
                    }
                    *c = (char)0;
                    break;
                    
                case '<':
                    // Handle non-token in-chevron
                    switch (next_type) {
                        case TokenTypeOther:
                        case TokenTypeBeginArgument:
                            next_type = TokenTypeBeginInFile;
                            break;
                            
                        default:
                            printf("syntax error\n");
                            exit(1);
                    }
                    *c = (char)0;
                    break;
                    
                case '>':
                    // Handle non-token out-chevron
                    switch (next_type) {
                        case TokenTypeOther:
                        case TokenTypeBeginArgument:
                            next_type = TokenTypeBeginOutFile;
                            break;
                        
                        case TokenTypeBeginOutFile:
                            if (!curr_cmd->appending) {
                                curr_cmd->appending = true;
                                break;
                            }
                            // fallthrough
                            
                        default:
                            printf("syntax error");
                            exit(1);
                    }
                    *c = (char)0;
                    break;
                    
                case '0':
                case '1':
                case '2':
                {
                    char num = *c++;
                    if (*c == '>') {
                        switch (num) {
                            case '0':
                                next_type = TokenTypeBeginInFile;
                                break;
                                
                            case '1':
                                next_type = TokenTypeBeginOutFile;
                                break;
                                
                            case '2':
                                next_type = TokenTypeBeginErrorFile;
                                break;
                                
                            default:
                                break;
                        }
                        break;
                    } // otherwise read the number normally
                }
                    
                default:
                    if (next_type == TokenTypeOther) break;
                    
                    // Initialize the command if it has no argv list
                    if (!curr_cmd->argv) {
                        // Set argv list
                        curr_cmd->argv = curr_arg;
                    }

                    switch (next_type) {
                        case TokenTypeBeginArgument:
                            *curr_arg++ = c; // Add argument to list and increment
                            break;
                            
                        case TokenTypeBeginInFile:
                            curr_cmd->in_filename = c;
                            break;
                        
                        case TokenTypeBeginOutFile:
                            curr_cmd->out_filename = c;
                            break;

                        case TokenTypeBeginErrorFile:
                            curr_cmd->error_filename = c;
                            break;

                        default:
                            exit(1);
                    }

                    next_type = TokenTypeOther;
                    break;
            }
        }
        command *cmds_end = curr_cmd + 1;
        
#if DEBUG
        for (command *c = cmds; c < cmds_end; c++) {
            printf("COMMAND: %s\n", c->argv[0]);
            for (char **arg = c->argv + 1; *arg != NULL; arg++) {
                printf("ARG: %s\n", *arg);
            }
            if (c->in_filename) printf("IN: %s\n", c->in_filename);
            if (c->out_filename) printf("OUT: %s\n", c->out_filename);
        }
#endif
        
        // Display line back
		execute_commands(cmds, cmds_end);
        
        // Free line
        free(line);
    }
    return 0;
}
