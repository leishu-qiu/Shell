#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "./jobs.h"

// flag for input redirection symbol
int input_redirection = 0;
// flag for output redirection symbol
int output_redirection = 0;
// flag for output redirection symbol in append mode
int append = 0;
char *path;
char *outputfile = NULL;
char *inputfile = NULL;
// flag for ampersand
int background;
job_list_t *joblist;
int jid;
int jd_bg;
pid_t pid_child;

/*
 * parse()
 *
 * Description:
 * Parses the command line input provided in the 'buffer' and separates it into
 * tokens and arguments for use in later parts of the code. The function also
 * parses for IO redirection and checks for errors related to IO redirection.
 * Additionally, it checks for the presence of an ampersand at the end of the
 * command line to determine if the command should be executed in the
 * background.
 *
 * @param buffer   A character array containing the command line input to be
 * parsed.
 * @param tokens   An array of character pointers (strings) where the parsed
 * tokens will be stored.
 * @param argv     An array of character pointers (strings) where the parsed
 * arguments will be stored.
 */
void parse(char buffer[1024], char *tokens[512], char *argv[512])
{
    if (buffer == NULL)
        return;
    // reset the global variables for multiple lines
    path = NULL;
    inputfile = NULL;
    outputfile = NULL;
    input_redirection = 0;
    output_redirection = 0;
    append = 0;

    // flag for full path found
    int fullpath = 0;
    char *str = buffer;
    char *token;
    int i = 0, j = 0, k = 0;
    // tokenize the input
    while ((token = strtok(str, " \t\n")) != NULL)
    {
        if (strncmp(token, "/", 1) == 0 && fullpath == 0)
        {
            path = token;
            fullpath = 1;
        }
        tokens[i] = token;
        i++;
        str = NULL;
    }

    // checking ampersand
    if (i > 0)
    {
        size_t len = strlen(tokens[i - 1]);
        if (len == 1 && strcmp(tokens[i - 1], "&") == 0)
        {
            background = 1;
            tokens[i - 1] = '\0';
        }
    }

    for (j = 0, k = 0; tokens[j] != NULL; j++)
    {
        // Input redirection
        if (strcmp(tokens[j], "<") == 0)
        {
            input_redirection = 1;
            // Move to next index
            j++;
            if (tokens[j] == NULL)
                fprintf(stderr, "syntax error: no input file\n");
            else if (strcmp(tokens[j], ">") == 0 ||
                     strcmp(tokens[j], ">>") == 0 ||
                     strcmp(tokens[j], "<") == 0)
                fprintf(stderr,
                        "syntax error: input file is a redirection symbol\n");
            if (inputfile != NULL)
                fprintf(stderr, "syntax error: multiple input files");
            else
                inputfile = tokens[j];
        }
        // Output Redirection
        else if (strcmp(tokens[j], ">") == 0 ||
                 strcmp(tokens[j], ">>") == 0)
        {
            output_redirection = 1;
            // Append mode
            if (strcmp(tokens[j], ">>") == 0)
                append = 1;
            j++;
            if (tokens[j] == NULL)
                fprintf(stderr, "syntax error: no output file\n");
            else if (strcmp(tokens[j], ">>") == 0 ||
                     strcmp(tokens[j], ">") == 0 || strcmp(tokens[j], "<") == 0)
                fprintf(stderr,
                        "syntax error: output file is a redirection symbol\n");
            if (outputfile != NULL)
                fprintf(stderr, "syntax error: multiple output files");
            else
                outputfile = tokens[j];
        }
        else
        {
            if (tokens[j] && path && strcmp(tokens[j], path) == 0)
            {
                char *slash = strrchr(tokens[j], '/');
                if (slash != NULL)
                {
                    slash++;
                    argv[k] = slash;
                }
            }
            else
            {
                argv[k] = tokens[j];
            }
            k++;
        }
    }
    // Null-terminate argv
    argv[k] = NULL;
    return;
}

/*
 * error_handler_system()
 *
 * Description:
 * Handles errors for system calls by printing out an error message using
 * `perror` and exiting the program. This function is typically used to report
 * errors that occur during system calls and provide informative error messages
 * to help diagnose and address issues in the program.
 *
 * @param msg   A constant character pointer that contains the error message to
 * be printed.
 *
 */
void error_handler_system(const char *msg)
{
    perror(msg);
    exit(1);
}

/*
 * process_redirection()
 *
 * Description:
 * Handles IO redirection by closing the corresponding file descriptors and
 * opening the specified files. This function is typically called after parsing
 * a command line input to set up the necessary IO redirection for executing a
 * command.
 *
 */
void process_redirection()
{
    // Input Redirection
    if (input_redirection)
    {
        if (close(0) == -1)
            error_handler_system("close failed");
        if (open(inputfile, O_RDONLY, 0600) == -1)
            error_handler_system("open failed");
    }
    // Output Redirection
    if (output_redirection)
    {
        if (close(1) == -1)
            error_handler_system("close failed");
        // Append Mode
        if (append)
        {
            if (open(outputfile, O_WRONLY | O_APPEND | O_CREAT, 0600) == -1)
                error_handler_system("open failed");
        }
        else
        {
            if (open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0600) == -1)
                error_handler_system("open failed");
        }
    }
    return;
}

/*
 * reaping()
 *
 * Description:
 *
 * Reap background processes using waitpid with WNOHANG, WUNTRACED, WCONTINUED
 * options to non-blocking wait for child processes. Handle the status of
 * background processes and prints corresponding message.
 *
 * @param tokens   An array of character pointers (strings) that contain parsed
 * tokens. The token at index 0 containing the orignal command that started the
 * background job is being passed when calling add_job function.
 *
 */

void reaping(char *tokens[512])
{
    int status_waitpid;
    pid_t ppid;
    while ((ppid = waitpid(-1, &status_waitpid,
                           WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        if (WIFSTOPPED(status_waitpid))
        {
            // bg job stopped by a signal
            if (get_job_jid(joblist, ppid) == -1)
            {
                if (add_job(joblist, jid, ppid, STOPPED, tokens[0]) == 0)
                    jid++;
            }
            else
            {
                update_job_pid(joblist, ppid, STOPPED);
            }
            fprintf(stderr, "[%d] (%d) suspended by signal %d\n",
                    get_job_jid(joblist, ppid), ppid, WSTOPSIG(status_waitpid));
        }
        else if (WIFSIGNALED(status_waitpid))
        {
            // bg job terminated by signal
            int jobid = get_job_jid(joblist, ppid);
            if (jobid == -1)
            {
                fprintf(stderr, "[%d] (%d) terminated by signal %d\n", jd_bg++,
                        ppid, WTERMSIG(status_waitpid));
                remove_job_pid(joblist, ppid);
            }
            else
            {
                fprintf(stderr, "[%d] (%d) terminated by signal %d\n", jobid,
                        ppid, WTERMSIG(status_waitpid));
                remove_job_pid(joblist, ppid);
            }
        }
        else if (WIFCONTINUED(status_waitpid))
        {
            // bg job is resumed
            int jd = get_job_jid(joblist, ppid);
            if (jd == -1)
            {
                if (add_job(joblist, jid, ppid, RUNNING, tokens[0]) == 0)
                {
                    fprintf(stderr, "[%d] (%d) resumed\n", jid, ppid);
                    jid++;
                }
            }
            else if (jd)
            {
                update_job_pid(joblist, ppid, RUNNING);
                fprintf(stderr, "[%d] (%d) resumed\n", jd, ppid);
            }
        }
        else if (WIFEXITED(status_waitpid))
        {
            // bg job exits normally
            int jobid = get_job_jid(joblist, ppid);
            if (jobid == -1)
            {
                fprintf(stderr, "[%d] (%d) terminated with exit status %d\n",
                        jd_bg++, ppid, WEXITSTATUS(status_waitpid));
            }
            else
            {
                fprintf(stderr, "[%d] (%d) terminated with exit status %d\n",
                        jobid, ppid, WEXITSTATUS(status_waitpid));
            }
            remove_job_jid(joblist, jobid);
        }
    }
}

/*
 * addBgJob()
 *
 * Description:
 * Adds a background job to a joblist data structure, provided that the command
 * is intended to run in the background. The function checks if the 'background'
 * flag is set, indicating that the command should be executed in the
 * background. If so, it adds the job to the joblist with the appropriate status
 * (RUNNING) and associated information.
 *
 * @param tokens   An array of character pointers (strings) that contains parsed
 * tokens. The token at index 0 containing the orignal command that started the
 * background job is being passed when calling add_job function.
 *
 */
void addBgJob(char *tokens[512])
{
    if (background)
    {
        if (tokens[0])
        {
            int result = add_job(joblist, jid, pid_child, RUNNING, tokens[0]);
            if (result == -1)
            {
                fprintf(stderr, "add job error\n");
            }
            else if (result == 0)
            {
                fprintf(stderr, "[%d] (%d)\n", jid, pid_child);
                jid++;
            }
        }
    }
}

/*
 * waitFgProcess()
 *
 * Description:
 * Waits for a foreground process to complete and handles its status. This
 * function uses the waitpid system call with the WUNTRACED option. It then
 * processes the status of the process and updates job information accordingly,
 * reporting any suspensions, terminations, or resumptions. When waitpid is
 * done, transfer terminal control back to shell.
 *
 * @param tokens   An array of character pointers (strings) that contains parsed
 * tokens. The token at index 0 containing the orignal command that started the
 * background job is being passed when calling add_job function.
 */
void waitFgProcess(char *tokens[512])
{
    int status;
    pid_t cur_pid = waitpid(pid_child, &status, WUNTRACED);
    if (cur_pid < 0)
        error_handler_system("wait");
    else if (cur_pid > 0)
    {
        if (WIFSTOPPED(status))
        {
            int jobid = get_job_jid(joblist, cur_pid);
            if ((jobid == -1))
            {
                if (add_job(joblist, jid, cur_pid, STOPPED, tokens[0]) == 0)
                {
                    jobid = jid;
                    jid++;
                }
            }
            else
            {
                update_job_jid(joblist, cur_pid, STOPPED);
            }
            fprintf(stderr, "[%d] (%d) suspended by signal %d\n", jobid,
                    cur_pid, WSTOPSIG(status));
        }
        else if (WIFSIGNALED(status))
        {
            remove_job_pid(joblist, cur_pid);
            fprintf(stderr, "(%d) terminated by signal %d\n", cur_pid,
                    WTERMSIG(status));
        }
        else if (WIFCONTINUED(status))
        {
            int jd = get_job_jid(joblist, cur_pid);
            if (jd == -1)
            {
                if (add_job(joblist, jid, cur_pid, RUNNING, tokens[0]) == 0)
                {
                    fprintf(stderr, "[%d] (%d) resumed\n", jid, cur_pid);
                    jid++;
                }
            }
            else
            {
                update_job_pid(joblist, cur_pid, RUNNING);
                fprintf(stderr, "[%d] (%d) resumed\n", jd, cur_pid);
            }
        }
    }
    // transfer control back to shell
    tcsetpgrp(0, getpgrp());
}

int main()
{
    // Ignore Signals
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    joblist = init_job_list();
    char buf[1024];
    ssize_t n;
    char *tokens[512];
    char *argv[512];

    // Initialization
    input_redirection = 0;
    output_redirection = 0;
    append = 0;
    outputfile = NULL;
    inputfile = NULL;
    jid = 1;
    jd_bg = 1;

    while (1)
    {
#ifdef PROMPT
        if (printf("33sh> ") < 0)
        {
            /* handle error */
            fprintf(stderr, "33 shell error");
        }
        if (fflush(stdout) < 0)
        {
            /* handle error */
            error_handler_system("fflush failed");
        }
#endif
        background = 0;
        reaping(tokens);

        n = read(0, buf, sizeof(buf));
        if (n == 1 && buf[0] == '\n')
        {
            continue;
        }
        else if (n == 0)
        {
            cleanup_job_list(joblist);
            exit(0);
        }
        else if (n < 0)
        {
            error_handler_system("read failed");
        }
        buf[n] = 0;
        for (int i = 0; i < 512; i++)
        {
            tokens[i] = NULL;
            argv[i] = NULL;
        }
        inputfile = NULL;
        outputfile = NULL;

        parse(buf, tokens, argv);

        // Handle Built-in Commands
        if (tokens[0] == NULL)
            continue;
        if (strcmp(tokens[0], "cd") == 0)
        {
            if (tokens[2] != NULL || tokens[1] == NULL)
                fprintf(stderr, "syntax error\n");
            else
            {
                if (chdir(tokens[1]) == -1)
                    error_handler_system("cd");
            }
        }
        else if (strcmp(tokens[0], "ln") == 0)
        {
            if (tokens[1] != NULL && tokens[2] != NULL)
            {
                if (link(tokens[1], tokens[2]) == -1)
                    error_handler_system("link");
            }
        }
        else if (strcmp(tokens[0], "rm") == 0)
        {
            if (tokens[1] != NULL)
            {
                if (unlink(tokens[1]) == -1)
                    error_handler_system("unlink");
            }
            else
                fprintf(stderr, "missing operand\n");
        }
        else if (strcmp(tokens[0], "exit") == 0)
        {
            cleanup_job_list(joblist);
            exit(0);
        }
        else if (strcmp(tokens[0], "jobs") == 0)
        {
            jobs(joblist);
        }
        else if (strcmp(tokens[0], "fg") == 0)
        {
            // parse jid
            int fg_jid = atoi(tokens[1] + 1);
            int fg_pid = get_job_pid(joblist, fg_jid);
            if (get_job_pid(joblist, fg_jid) == -1)
            {
                fprintf(stderr, "job not found\n");
            }
            // send SIGCONT to the process group
            kill(-fg_pid, SIGCONT);
            // give control of terminal to the process
            tcsetpgrp(0, fg_pid);

            int status_fg;
            pid_t retval;
            if ((retval = waitpid(fg_pid, &status_fg, WUNTRACED)) > 0)
            {
                if (WIFSTOPPED(status_fg))
                {
                    update_job_pid(joblist, fg_pid, STOPPED);
                    fprintf(stderr, "[%d] (%d) suspended by signal %d\n",
                            fg_jid, fg_pid, WSTOPSIG(status_fg));
                }
                else if (WIFEXITED(status_fg))
                {
                    remove_job_pid(joblist, fg_pid);
                }
                else if (WIFSIGNALED(status_fg))
                {
                    fprintf(stderr, "(%d) terminated by signal %d\n", fg_pid,
                            WTERMSIG(status_fg));
                    remove_job_pid(joblist, fg_pid);
                }
            }
            // transfer control back to shell
            tcsetpgrp(0, getpgrp());
        }
        else if (strcmp(tokens[0], "bg") == 0)
        {
            int bg_jid = atoi(tokens[1] + 1);
            int bg_pid = get_job_pid(joblist, bg_jid);
            if (bg_pid == -1)
            {
                fprintf(stderr, "job not found\n");
            }
            kill(-bg_pid, SIGCONT);
            update_job_pid(joblist, bg_pid, RUNNING);
        }
        else
        {
            // Fork child process
            if ((pid_child = fork()) == 0)
            {
                setpgid(0, 0);
                if (!background)
                {
                    tcsetpgrp(0, getpgrp());
                }
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTOU, SIG_DFL);
                char *p = NULL;
                if (input_redirection || output_redirection)
                {
                    process_redirection();
                    p = path;
                }
                else
                    p = tokens[0];
                execv(p, argv);
                error_handler_system("execv");
            }
            else if (pid_child == -1)
            {
                error_handler_system("fork");
            }
            else
            {
                // Add bg jobs
                addBgJob(tokens);

                // Wait for fg process
                if (!background)
                    waitFgProcess(tokens);
            }
        }
    }
    cleanup_job_list(joblist);
    return 0;
}
