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
        }
    }
    if (!strcmp(args[0], "|") || !strcmp(args[0], ">") || !strcmp(args[0], ">>")) {
            fprintf(stderr, "Error: Invalid input string. Cannot begin with operator '%s'\n", args[0]);
            return -1;
        }
    if (!strcmp(args[argc - 1], "|") || !strcmp(args[argc - 1], ">") || !strcmp(args[argc - 1], ">>")) {
        fprintf(stderr, "Error: Invalid input string. Cannot end with operator '%s'\n", args[0]);
        return -1;
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
    int redirect = 0; // 0: None, 1: replace (>), 2: append (>>)
    char *filepath;
    int fd = -1;
    for (int i = 0; i < argc; i++) {
        if (!strcmp(args[i], ">")) {
            redirect = 1;
            filepath = args[i+1];  
            break;          
        }
        if (!strcmp(args[i], ">>")) {
            redirect = 2;
            filepath = args[i+1];    
            break;        
        }
    }

    int flags = 0;
    if (redirect == 1) {
        flags = O_WRONLY | O_CREAT | O_TRUNC;
    }
    else if (redirect == 2) {
        flags = O_WRONLY | O_CREAT | O_APPEND;
    }
    if (flags) {
        printf("Redirect detected\n");
        fd = open(filepath, flags, 0664);
        if (fd < 0) {
            perror("open");
            exit(1);
        }
    }
    //now we have fd for child to use if needed.

    //now set up N pipes.
    int i;
    int pipes = 0;
    char **cmds[MAX_ARGS];
    cmds[0] = &args[0];
    for (i = 0; args[i] != NULL; ++i) {
        if (!strcmp(args[i], "|")) {
            args[i] = NULL;
            cmds[++pipes] = &args[i+1];
        }
    }
    cmds[pipes+1] = NULL;
    if (pipes) { // do piping stuff
        pid_t pids[pipes+1];
        int pipefds[pipes][2];

        //create all pipes
        for (i = 0; i < pipes; ++i) {
            if(pipe(pipefds[i]) == -1) {
                perror("pipe");
            }
        }

        // fork each command
        for (i = 0; i < pipes + 1; ++i) {
            pid_t pid = fork();
            if (pid == -1)
                perror("fork");

            if (pid == 0) { // child
                // set sig handlers back to default
                struct sigaction sa = {0};
                sa.sa_handler = SIG_DFL;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;

                sigaction(SIGINT, &sa, NULL);

                if (i > 0) { // not first command: stdin from previous pipe
                    if (dup2(pipefds[i-1][0], STDIN_FILENO) < 0) {
                        perror("dup2");
                    }
                }
                if (i < pipes) { // not last command: stdout to next pipe
                    if (dup2(pipefds[i][1], STDOUT_FILENO) < 0) {
                        perror("dup2");
                    }
                }

                //close both ends of all pipes in child
                for (int j = 0; j < pipes; ++j) {
                    close(pipefds[j][0]);
                    close(pipefds[j][1]);
                }
                
                execvp(cmds[i][0], cmds[i]); //exec new command
                perror("execvp");
                exit(127);
            }

            // parent
            pids[i] = pid;
            children_running++;
        }
        //close all pipes in parent
        for (i = 0; i < pipes; ++i) {
            close(pipefds[i][0]);
            close(pipefds[i][1]);
        }
    }

    else { // no pipes just run as normal
        pid_t pid = fork();
            if (pid == -1)
                perror("fork");

            if (pid == 0) { // child
                // set sig handlers back to default
                struct sigaction sa = {0};
                sa.sa_handler = SIG_DFL;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = 0;

                sigaction(SIGINT, &sa, NULL);

                execvp(cmds[0][0], cmds[0]); //exec new command
                perror("execvp");
                exit(127);
            }

            //parent
            children_running++;
    }
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

        if (children_running) // if child running, wait for done/killed
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
