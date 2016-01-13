// apt-get install libreadline-dev libbsd-dev

#include <stdlib.h>
#include <stdbool.h>
#include <bsd/string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

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
        
        // Parse arguments from input 
        char *argv[100];
        char **next = argv;
        bool record_next = true;
        for (char *c = line; *c != (char)0; c++) {
            switch (*c) {
                case ' ':
                case '\t':
                    *c = (char)0;
                    record_next = true;
                    break;
                default:
                    if (record_next) {
                        *next++ = c;
                        record_next = false;
                    }
            }
        }
        *next = NULL;
        
        // Display line back
        execvp(argv[0], argv);
        
        // Free line
        free(line);
    }
    return 0;
}
