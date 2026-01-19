Jack Stewart and Owen Marcione
SLOsh program



current issues:


Working Functionality:
- redirecting stdout to new file
- redirecting stdout to append to file
- 'cd' changing directory and printing correct CWD
- 'exit' exiting shell
- entering a blank line (empty and with whitespace)
- basic commands (ls, echo one two three, pwd)
- absolute path (/bin/echo hello, /bin/ls)
- relative path/shell within shell (ran './slosh' within shell)
- built in commands
- sigint during command (sleep 10 ^C)
- single pipe
- chained pipes (working with redirects and appends)
- improper operator use (| ls, echo hi |, echo hi >) correctly errors with statement
- nonexistenet command says command not found

Requirements: (commented out requirements have been tested)
<!-- Basic command execution (can run simple commands like 'ls') -->

<!-- 'cd' built-in command (can change directories) -->

<!-- 'exit' command (shell terminates cleanly) -->

<!-- Child signal handling (SIGINT kills child but shell survives) -->
<!-- Empty command handling (handles empty lines) -->

<!-- Nonexistent command (reports command not found) -->

Status reporting (shows non-zero exit codes)

<!-- Pipe command execution (connects output to input with |) -->

<!-- Complex pipe chain (handles multi-stage pipes)
    Does not work with redirecting right now -->

<!-- Prompt displays current directory (shows working directory) -->

<!-- Executing commands with full path (handles absolute paths) -->

<!-- Output redirection (redirects stdout to file with >) -->

<!-- Append redirection (appends stdout to file with >>) -->

<!-- Overwrite redirection (> replaces existing files) -->

<!-- Code maintainability / cleanliness -->


