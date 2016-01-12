// apt-get install libreadline-dev libbsd-dev

#include <stdlib.h>
#include <stdbool.h>
#include <bsd/string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* line;
    char prompt[80];
    while(true) {
        char cwd_buf[80];
        getcwd(cwd_buf, sizeof(cwd_buf));
        strlcpy(prompt, cwd_buf, sizeof(prompt));
        strlcat(prompt, " ", sizeof(prompt));
        line = readline(prompt);
        add_history(line);
        printf("%s\n", line);
        free(line);
    }
    return 0;
}
