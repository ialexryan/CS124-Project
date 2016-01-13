// apt-get install libreadline-dev libbsd-dev

#include <stdlib.h>
#include <stdbool.h>
#include <bsd/string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define DEBUG 0

typedef struct {
    char **argv;
} command;

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
        bool record_next = true;
        for (char *c = line; *c != (char)0; c++) {
            switch (*c) {
                case '|':
                    // End of command
                    *curr_arg++ = NULL; // NULL-terminate arg list and increment
                    curr_cmd++;         // Increment to next command
                    // fallthrough
                case ' ':
                case '\t':
                    // Not an argument
                    *c = (char)0;       // NULL-terminate argument string
                    record_next = true; // Mark that moved to next arg
                    break;
                    
                default:
                    // Check if this is the start of an argument
                    if (record_next) {
                        // Check if command has no argv list
                        if (!curr_cmd->argv) {
                            // First argument in command so set up the argv list
                            curr_cmd->argv = curr_arg;
                        }
                        
                        // Record argument
                        *curr_arg++ = c;     // Add argument to list and increment
                        record_next = false; // Mark that argument is recorded
                    }
            }
        }
        command *cmds_end = curr_cmd + 1;
        
#if DEBUG
        for (command *c = cmds; c < cmds_end; c++) {
            printf("COMMAND: %s\n", c->argv[0]);
            for (char **arg = c->argv + 1; *arg != NULL; arg++) {
                printf("ARG: %s\n", *arg);
            }
        }
#endif
        
        // Display line back
        execvp(cmds[0].argv[0], cmds[0].argv);
        
        // Free line
        free(line);
    }
    return 0;
}
