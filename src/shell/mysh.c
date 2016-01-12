#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

int main() {
    char* line;
    line = readline("Enter a string: ");
    printf("%s\n", line);
    free(line);
    return 0;
}
