```
 ____  _          _ _   ____
/ ___|| |__   ___| | | |___ \
\___ \| '_ \ / _ \ | |   __) |
 ___) | | | |  __/ | |  / __/
|____/|_| |_|\___|_|_| |_____|
```

# Shell 2

The primary source code is located in sh.c. Shell 2 is a terminal-based shell that mimics the functionality of the Bash shell. It supports a variety of features including safe signal handling, input/output redirection, and the ability to manage jobs in both the foreground and background. The shell also includes implementations of numerous Bash builtins.

## Program Structure

The program is organized into multiple functions, each with a specific purpose. Here's an overview of the program's structure:

- `parse`: This function processes command-line inputs by tokenizing the input and identifying arguments. It checks for input/output redirection and detects if a command should run in the background (indicated by an ampersand &).

- `error_handler_system`: Manages system call errors by displaying relevant error messages and exiting the program when necessary. This function is crucial for troubleshooting and fixing issues that arise during operation.

- `process_redirection`: Responsible for handling input and output redirections. It manages file descriptors to ensure that commands with redirection symbols (<, >) operate correctly.

- `reaping`: Uses the waitpid system call with options like WNOHANG, WUNTRACED, and WCONTINUED to reap background processes in a non-blocking manner. It also manages the status updates and messaging for these processes.

- `addBgJob`: Adds commands intended to run in the background to a job list. This function checks the background flag and assigns the correct status to each job.

- `waitFgProcess`: Waits for foreground processes to complete, utilizing waitpid with the WUNTRACED option to handle their statuses efficiently. This function is key in managing and reporting on process suspensions, terminations, or resumptions.

Here's a clearer and more polished version for the "How to Compile and Run" and "Limitation" sections of your GitHub README:

---

## How to Compile and Run

To compile Shell 2, open your terminal and execute the following command:

```
make all
```

To run Shell 2 after compilation, type:

```
./33sh
```
