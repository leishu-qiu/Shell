```
 ____  _          _ _   ____
/ ___|| |__   ___| | | |___ \
\___ \| '_ \ / _ \ | |   __) |
 ___) | | | |  __/ | |  / __/
|____/|_| |_|\___|_|_| |_____|
```

# Shell 2
Shell 2 is a simple shell program that provides basic command-line functionality. It allows you to run commands, perform simple input/output redirection, and run jobs in foreground and background.


## Program Structure

The program is organized into multiple functions, each with a specific purpose. Here's an overview of the program's structure:

- `parse`: Parses the command-line input, separates it into tokens and arguments, and checks for input/output redirection. It also detects the presence of an ampersand (&) to determine if the command should be run in the background.

- `error_handler_system`: Handles errors for system calls by printing error messages and exiting the program. It provides informative error messages to diagnose and address issues in the program.

- `process_redirection`: Handles input and output redirection by closing and opening file descriptors for commands with redirection symbols.

- `reaping`: Reaps background processes using `waitpid` with `WNOHANG`, `WUNTRACED`, and `WCONTINUED` options to perform non-blocking waits for child processes. It handles the status of background processes and prints corresponding messages.

- `addBgJob`: Adds background jobs to a job list data structure when a command is intended to run in the background. It checks the `background` flag and adds the job with the appropriate status.

- `waitFgProcess`: Waits for a foreground process to complete and handles its status. It uses `waitpid` with the `WUNTRACED` option to process the status of the foreground process and update job information. It also reports suspensions, terminations, or resumptions.

## How to Compile and Run

To compile shell 2, type in "make all" in the terminal.
To run shell 2, type in "./33sh" in the terminal.

## Limitation

This shell has some limitations. Supports basic functionality only.
