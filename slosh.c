/**
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 *
 * TODO: Complete the implementation according to the comments
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/wait.h>
 #include <sys/types.h>
 #include <fcntl.h>
 #include <signal.h>
 #include <limits.h>
 #include <errno.h>

/* Define PATH_MAX if it's not available */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

/* Global variable for signal handling */
volatile sig_atomic_t child_running = 0;

/* Forward declarations */
void display_prompt(void);

/**
 * Signal handler for SIGINT (Ctrl+C)
 *
 * TODO: Handle Ctrl+C appropriately. Think about what behavior makes sense
 * when the user presses Ctrl+C - should the shell exit? should a child process
 * be interrupted?
 * Hint: The global variable tracks important state.
 */
void sigint_handler(int sig) {
    /* TODO: Your implementation here */
    // child process is killed so set to 0
    if (sig == SIGINT)
        child_running = 0;
}

/**
 * Display the command prompt with current directory
 */
void display_prompt(void) {
    char cwd[PATH_MAX];
    char prompt_buf[PATH_MAX + 3];
    int len;

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        len = snprintf(prompt_buf, sizeof(prompt_buf), "%s> ", cwd);
    } else {
        len = snprintf(prompt_buf, sizeof(prompt_buf), "SLOsh> ");
    }

    if (len > 0 && len < (int)sizeof(prompt_buf)) {
        write(STDOUT_FILENO, prompt_buf, len);
    }
}

//helper funcs
static int is_ws(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static int is_special(const char *s) {
    return (s[0] == '|' && s[1] == '\0') ||
           (s[0] == '>' && s[1] == '\0') ||
           (s[0] == '>' && s[1] == '>' && s[2] == '\0');
}

/**
 * Parse the input line into command arguments
 *
 * TODO: Extract tokens from the input string. What should you do with special
 * characters like pipes and redirections? How will the rest of the code know
 * what to execute?
 * Hint: You'll need to handle more than just splitting on spaces.
 *
 * @param input The input string to parse
 * @param args Array to store parsed arguments
 * @return Number of arguments parsed
 */
int parse_input(char *input, char **args) {
    int argc = 0;
    char *p = input;

    while (*p) {
        while (*p && is_ws(*p)) { *p = '\0'; p++; }
        if (!*p) break;

        args[argc++] = p;

        while (*p && !is_ws(*p)) p++;

        if (*p) { *p = '\0'; p++; }
    }

    args[argc] = NULL;

    // validate: operator chars must appear only as standalone tokens
    for (int i = 0; i < argc; i++) {
        if (strchr(args[i], '|') || strchr(args[i], '>')) {
            if (!is_special(args[i])) {
                fprintf(stderr, "Error: Operators must be whitespace separated\n");
                return -1;
            }
        }
    }

    return argc;
}


/**
 * Execute the given command with its arguments
 *
 * TODO: Run the command. Your implementation should handle:
 * - Basic command execution
 * - Pipes (|)
 * - Output redirection (> and >>)
 *
 * What system calls will you need? How do you connect processes together?
 * How do you redirect file descriptors?
 *
 * @param args Array of command arguments (NULL-terminated)
 */
void execute_command(char **args) {
    /* TODO: Your implementation here */
    (void)args;
}

/**
 * Check for and handle built-in commands
 *
 * TODO: Implement support for built-in commands:
 * - exit: Exit the shell
 * - cd: Change directory
 *
 * @param args Array of command arguments (NULL-terminated)
 * @return 0 to exit shell, 1 to continue, -1 if not a built-in command
 */
int handle_builtin(char **args) {
    /* TODO: Your implementation here */
    if (!strcmp(args[0], "exit")) { // handle exit
        return 0;
    }
    
    if (!strcmp(args[0], "cd")){ // handle cd
        if (chdir(args[1])) { //truthy if fail
            perror("cd");
        }
        return 1;
    }

    return -1;  /* Not a builtin command */
}

int main(void) {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    int status = 1;
    int builtin_result;

    /* TODO: Set up signal handling. Which signals matter to a shell? */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    while (status) {
        display_prompt();

        /* Read input and handle signal interruption */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            /* TODO: Handle the case when fgets returns NULL. When does this happen? */
            break;
        }

        /* Parse input */
        parse_input(input, args);

        /* Handle empty command */
        if (args[0] == NULL) {
            continue;
        }

        /* Check for built-in commands */
        builtin_result = handle_builtin(args);
        if (builtin_result >= 0) {
            status = builtin_result;
            continue;
        }

        /* Execute external command */
        execute_command(args);
    }

    printf("SLOsh exiting...\n");
    return EXIT_SUCCESS;
}
