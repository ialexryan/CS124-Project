// apt-get install libreadline-dev libbsd-dev

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
} command;

typedef enum {
    TokenTypeBeginArgument,
    TokenTypeBeginInFile,
    TokenTypeBeginOutFile,
    TokenTypeOther
} token_type;

void execute_command(command cmd) {
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
	
	// It's not an internal command, so fork out an external command
	pid_t pid;
	pid = fork();
	if (pid < 0) {                     // error
		perror("Forking error");
	} else if (pid == 0) {             // child process
		if (execvp(cmd.argv[0], cmd.argv) < 0) {
			perror("Exec error");
		}
	} else {                           // parent process
		int status;
		if (wait(&status) < 0) {
			perror("Waiting error");
		}
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
                            
                        default:
                            printf("syntax error");
                            exit(1);
                    }
                    *c = (char)0;
                    break;
                    
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
		execute_command(cmds[0]);
        
        // Free line
        free(line);
    }
    return 0;
}
