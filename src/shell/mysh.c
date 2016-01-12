// apt-get install libreadline-dev libbsd-dev

#include <stdlib.h>
#include <stdbool.h>
#include <bsd/string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* line;
    char prompt[80], login_buf[80], cwd_buf[80];
    while(true) {
        getlogin_r(login_buf, sizeof(login_buf));
        getcwd(cwd_buf, sizeof(cwd_buf));
        strlcpy(prompt, login_buf, sizeof(prompt));
        strlcat(prompt, ":", sizeof(prompt));
        strlcat(prompt, cwd_buf, sizeof(prompt));
        strlcat(prompt, "> ", sizeof(prompt));
        line = readline(prompt);
        add_history(line);
        printf("%s\n", line);
        free(line);
    }
    return 0;
}
