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
volatile pid_t children_running = 0;
volatile sig_atomic_t sigint_recv = 0;

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
    // set flag for fgets to ignore CTRL+C
    (void)sig;
    sigint_recv = 1;
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
        if (argc > MAX_ARGS) { 
            fprintf(stderr, "Too many arguments provided. Limit: %x\n", MAX_ARGS);
            return -1;
        }

        while (*p && is_ws(*p)) {
            *p = '\0'; 
            p++; 
        }

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
            if (!strcmp(args[0], "|") || !strcmp(args[0], ">") || !strcmp(args[0], ">>")) {
                fprintf(stderr, "Error: Invalid input string. Cannot begin with operator '%s'\n", args[0]);
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
void execute_command(char **args, int argc) {
    /* TODO: Your implementation here */

    // first set up redirects
    int redirect; // 0: None, 1: replace (>), 2: append (>>)
    char *filepath;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(args[i], ">")) {
            redirect = 1;
            filepath = args[i+1];            
        }
        if (!strcmp(args[i], ">>")) {
            redirect = 2;
            filepath = args[i+1];            
        }
    }
    if (redirect) {
        int fd = fopen(filepath, "w" ? redirect == 1: "a" );

    }

    int i;
    int pipe_present = 0;
    for (i = 0; i < argc; i++) {
        if (!strcmp(args[i], "|")) {
            pipe_present = i;
        }
    }
    if (pipe_present) { // do piping stuff, need 2 forks, 2 execs etc
        char **cmd1, **cmd2;
        
        //divide into 2 cmds, seperated by "|"
        cmd1 = args;
        args[i++] = NULL; //make "|" into NULL to signify end of cmd1
        cmd2 = args[i]; //start cmd2 right after
    }
    else { // no pipes just run as normal

    }

    execvp(args[0], args);

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

void reap_children(void) {
    int status;
    pid_t pid;

    while((pid = waitpid(0, &status, 0)) > 0) { //wait for all children in pgid
        if (WIFSIGNALED(status)) {
            printf("Child with PID %x killed by SIGINT.", pid);
        }
        children_running--;
    }
    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
        children_running = 0;
    }
}

int main(void) {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    int status = 1;
    int builtin_result;

    /* TODO: Set up signal handling. Which signals matter to a shell? */
    struct sigaction sa = {0};
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    while (status) {

        if (children_running) // if child running, check if done/killed
            reap_children();

        display_prompt();

        /* Read input and handle signal interruption */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            /* TODO: Handle the case when fgets returns NULL. When does this happen? */
            // Happens on SIGINT, so just continue if sigint was recieved
            if (sigint_recv) {
                sigint_recv = 0;
                continue;
            }
            break;
        }

        /* Parse input */
        int argc = parse_input(input, args);
        // for (int i=0; i< argc; i++) {
        //     printf("arg[%x]: %s\n", i, args[i]);
        //     fflush(stdout);
        // }

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
        execute_command(args, argc);
    }

    printf("SLOsh exiting...\n");
    return EXIT_SUCCESS;
}
