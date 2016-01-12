#include <stdlib.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* line;
    while(true) {
        line = readline("Enter a string: ");
        add_history(line);
        printf("%s\n", line);
        free(line);
    }
    return 0;
}
