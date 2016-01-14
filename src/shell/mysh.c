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
    union {
        int descriptor;
        char *filename;
    };
    bool append;
    int pair_descriptor;
    enum {
        no_replacement,
        descriptor_replacement,
        pipe_replacement,
        filename_replacement
    } type;
} file_replacement;

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

void set_pipe(file_replacement *f, int descriptor, int pair_descriptor) {
    if (f->type == no_replacement) {
        f->type = pipe_replacement;
        f->descriptor = descriptor;
        f->pair_descriptor = pair_descriptor;
    } else {
        printf("error: cannot set multiple descriptors\n");
        exit(1);
    }
}

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

void close_pair_pipe(file_replacement replacement) {
    if (replacement.type == pipe_replacement) {
        if (close(replacement.descriptor) < 0) {
            perror("Pipe error");
        }
    }
}

void replace_std_file(int descriptor, file_replacement replacement) {
    int fd;
    switch (replacement.type) {
        case pipe_replacement:
        case descriptor_replacement: {
            fd = replacement.descriptor;
            break;
        }
    
        case filename_replacement: {
            switch (descriptor) {
                case STDIN_FILENO: {
                    fd = open(replacement.filename, O_RDONLY);
                    break;
                }
                    
                case STDOUT_FILENO:
                case STDERR_FILENO: {
                    int flag = replacement.append ? O_APPEND : O_TRUNC;
                    fd = open(replacement.filename, O_CREAT | flag | O_WRONLY, 0);
                    break;
                }
                    
                default:
                    // Must be a std descriptor
                    exit(0);
            }
            if (fd < 0) return perror("File input error");
            break;
        }
            
        default:
            // Nothing to do
            return;
    }

    // Replace descriptor with file
    if (dup2(fd, descriptor) < 0 || close(fd) < 0) {
        perror("File descriptor error");
    }
}

void execute_command(command cmd) {
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
    if (pid < 0) { // error
        perror("Forking error");
    }
    else if (pid == 0) { // child process
        // Replace file descriptor
        replace_std_file(STDIN_FILENO, cmd.stdin);
        replace_std_file(STDOUT_FILENO, cmd.stdout);
        replace_std_file(STDERR_FILENO, cmd.stderr);
        
        // Execute command with args
        if (execvp(cmd.argv[0], cmd.argv) < 0) {
            perror(cmd.argv[0]);
            exit(1);
        }
    }
    else { // parent process
        close_pair_pipe(cmd.stdin);
        close_pair_pipe(cmd.stdout);
        close_pair_pipe(cmd.stderr);
    }
}

void execute_command_list(command *cmds, command *cmds_end) {
    // Create pipe buffer
    for (command *lhs = cmds, *rhs = cmds + 1; rhs < cmds_end; lhs++, rhs++) {
        int pipes[2];
        if (pipe(pipes) < 0) perror("Pipe error");
        set_pipe(&(lhs->stdout), pipes[1], pipes[0]);
        set_pipe(&(rhs->stdin), pipes[0], pipes[1]);
    }
    
    // Execute individual commands
    for (command *c = cmds; c < cmds_end; c++) {
        execute_command(*c);
    }

    // Wait for child processes to quit
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
                        
                    case err_file_type:
                        if (!curr_cmd->stderr.append) {
                            curr_cmd->stderr.append = true;
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
                
                // Skip the open quote if necessary
                bool is_quoted = (*c == '"');
                if (is_quoted) c++;
                
                // Handle storing reference to beginning of argument
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
                
                // Escape the quote
                if (is_quoted) {
                    while (*c != '"') c++;
                    *c = (char)0;
                    is_token_separator = true;
                }
                
                break;
        }
        if (is_token_separator) {
            *c = (char)0;
            state = awaiting_token_state;
        }
    }
    
    if (curr_cmd == cmds && curr_cmd->argv == NULL) {
        // Empty input, return nothing
        return curr_cmd;
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
		execute_command_list(cmds, cmds_end);
        
        // Free line
        free(line);
    }
    return 0;
}
