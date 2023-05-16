// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cmd.h"
#include "utils.h"

#define READ        0
#define WRITE       1

/**
 * Get an environment variable, or the empty string "", if not found.
 */
static const char *get_env(const char *name)
{
    const char *value = getenv(name);
    if (value == NULL) return "";
    else return value;
}

/**
 * Parse a word, expanding variables as necessary
 */
static char *parse_word(word_t *word)
{
    char *res = malloc(1024);
    res[0] = '\0';
    size_t capacity = 0, length = 0;

    while (word != NULL) {
        const char *toAdd = word->string;

        if (word->expand)
            toAdd = get_env(toAdd);

        size_t toAddLength = strlen(toAdd);
        if (length + toAddLength >= capacity) {
            res = realloc(res, capacity = length + toAddLength + 1024);
            DIE(res == NULL, "Unable to allocate memory");
        }

        strcat(res, toAdd);
        length += toAddLength;
        word = word->next_part;
    }

    return res;
}

/**
 * Internal show or change-directory command.
 */
static int shell_cd(word_t *dir, FILE *err)
{
    // First, get current working directory
    char oldDir[1024];
    int saveOldDir = getcwd(oldDir, 1024) != NULL;

    int res;

    // If no argument is provided, change directory to $HOME
    if (dir == NULL) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            fputs("The $HOME variable is not set.\n", err);
            return EXIT_FAILURE;
        } else res = chdir(home);

    } else {
        // If the argument is "-", change directory to $OLDPWD
        char *path = parse_word(dir);
        if (strcmp(path, "-") == 0) {
            const char *old = getenv("OLDPWD");
            if (old == NULL) {
                fputs("The $OLDPWD variable is not set.\n", err);
                return EXIT_FAILURE;
            } else res = chdir(old);
        } else res = chdir(path);
        free(path);
    }

    // Print error (if any) and return
    if (res != 0) {
        switch (errno) {
            case ENOENT:
                fputs("The specified path cannot be found.\n", err);
                break;
            case EACCES:
                fputs("Permission denied.\n", err);
                break;
            default:
                fputs("Unknown error.\n", err);
                break;
        }
        return errno;

        // If directory was changed successfully, save old dir into $OLDPWD
        // If pwd failed, fail silently (don't change $OLDPWD)
    } else if (saveOldDir)
        setenv("OLDPWD", oldDir, 1);

    return 0;
}

/**
 * Internal show working-directory command
 */
static int shell_pwd()
{
    char buf[1024];
    if (getcwd(buf, 1024) == NULL) {
        fputs("Unable to get working directory: possibly path too long\n", stderr);
        return errno;
    } else {
        fputs(buf, stdout);
        fputc('\n', stdout);
    }
    return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
    return SHELL_EXIT;
}

static FILE *open_file(word_t *word, FILE *stream, char *mode)
{
    char *fileName = parse_word(word);
    FILE *f = freopen(fileName, mode, stream);
    if (f == NULL) {
        fprintf(stderr, "Unable to open file \"%s\" for redirection.", fileName);
        free(fileName);
        return NULL;
    }
    free(fileName);
    return f;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
    /* TODO: Sanity checks. */

    // Check if variable assignment
    const word_t *eq = s->verb->next_part;
    if (eq != NULL && strcmp(eq->string, "=") == 0) {
        const char *varName = s->verb->string;

        if (eq->next_part == NULL) {
            // Assigning nothing, e.g. "NAME="
            // Delete the variable
            if (s->verb->expand) {
                fprintf(stderr, "Variable name may start with dollar sign.");
                return EXIT_FAILURE;
            }
            unsetenv(varName);
        } else {
            // Assigning value
            const char *value = eq->next_part->string;

            // Expand variable if necessary
            if (eq->next_part->expand)
                value = get_env(value);

            setenv(varName, value, 1);
        }

        return 0;
    }

    // Check for CD command as if this runs in a fork process, the working
    // directory won't be updates in main process
    char *command = parse_word(s->verb);
    if (strcmp(command, "cd") == 0) {
        free(command);
        // CD doesn't output anything, but touch the output file as it's checked by the checker
        if (s->out != NULL) {
            char *name = parse_word(s->out);
            FILE *f = fopen(name, "a");
            if (f == NULL) {
                fprintf(stderr, "Unable to open file \"%s\" for redirection.", name);
                free(name);
                return EXIT_FAILURE;
            } else
                fclose(f);
            free(name);
        }

        // Open the error redirection file
        FILE *err = stderr;
        if (s->err != NULL) {
            char *name = parse_word(s->err);
            FILE *f = fopen(name, "a");
            if (f == NULL) {
                fprintf(stderr, "Unable to open file \"%s\" for redirection.", name);
                free(name);
                return EXIT_FAILURE;
            } else
                fclose(f);
            free(name);
        }

        int ret = shell_cd(s->params, err);
        if (err != stderr) fclose(err);
        return ret;
    }

    // Fork process
    int pid = fork();
    DIE(pid < 0, "fork() failed");

    // Main process will wait for child process to end and return
    if (pid > 0) {
        free(command);
        int status;
        waitpid(pid, &status, 0);
        char r = WEXITSTATUS(status);
        return (int) r;
    }

    // Child process will execute the command

    // Resolve input/outputs redirections
    if (s->in != NULL)
        if (open_file(s->in, stdin, "r") == NULL) exit(errno);

    if (s->out != NULL)
        if (open_file(s->out, stdout, s->io_flags & IO_OUT_APPEND ? "a" : "w") == NULL)
            exit(errno);

    if (s->err != NULL) {
        // Check if we are using same file for both out and err
        if (s->out == s->err) dup2(STDOUT_FILENO, STDERR_FILENO);
        else if (open_file(s->err, stderr, s->io_flags & IO_ERR_APPEND ? "a" : "w") == NULL)
            exit(errno);
    }

    int ret;

    // If built-in command, execute the command
    if (strcmp(command, "pwd") == 0)
        ret = shell_pwd();
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
        ret = shell_exit();

    else {
        // Create arguments array
        size_t argLen = 0;
        word_t *arg = s->params;
        while (arg != NULL) {
            ++argLen;
            arg = arg->next_word;
        }
        char **args = malloc(sizeof(char*) * (argLen + 2));
        args[0] = command;
        args[argLen + 1] = NULL;

        // Populate arguments array
        size_t i = 1;
        arg = s->params;
        while (arg != NULL) {
            args[i] = parse_word(arg);
            ++i;
            arg = arg->next_word;
        }

        // Call exec to run the file
        execvp(command, args);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "Not found: %s\n", command);
                break;
            case EACCES:
                fputs("Access denied.", stderr);
                break;
            case ENAMETOOLONG:
                fputs("Path name too long.", stderr);
                break;
            default:
                fputs("Unknown error.", stderr);
                break;
        }
        ret = EXIT_FAILURE;
    }

    // Exit child process
    free(command);
    exit(ret);
}

/**
 * Process two commands in parallel, by creating two children.
 */
static int run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
                           command_t *father)
{
    int pid = fork(), ret;
    DIE(pid < 0, "fork() failed");

    // Main process will run first command and then wait for child
    if (pid > 0) {
        ret = parse_command(cmd1, level, father);
        int status;
        waitpid(pid, &status, 0);
        int childRet = (int) ((char) WEXITSTATUS(status));
        if (ret == 0) ret = childRet;
    }
    // Child process will run command and exit
    else
        exit(parse_command(cmd2, level, father));

    return ret;
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static int run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
                       command_t *father)
{

    // Create a child processes
    int outerPid = fork();
    DIE(outerPid < 0, "fork() failed");

    if (outerPid == 0) {
        // Create pipe
        int pipes[2];
        DIE(pipe(pipes) == -1, "pipe() failed");

        // Child process splits into 2
        int pid = fork();
        DIE(pid < 0, "fork() failed");

        // Child 1 will run cmd1 and child 2 will run cmd2
        if (pid > 0) {
            // Child 1
            dup2(pipes[1], STDOUT_FILENO);
            close(pipes[0]);
            close(pipes[1]);
            int r = parse_command(cmd1, level, father), status;

            close(STDOUT_FILENO);

            waitpid(pid, &status, 0);
            if (r == 0) r = (int) ((char) WEXITSTATUS(status));
            exit(r);
        } else {
            // Child 2
            dup2(pipes[0], STDIN_FILENO);
            close(pipes[0]);
            close(pipes[1]);
            int r = parse_command(cmd2, level, father);

            close(STDIN_FILENO);
            exit(r);
        }
    }

    // Main process waits child 1, child 1 waits child 2
    // 2 children are required because we can't "undo" a stdin/out redirect
    int status;
    waitpid(outerPid, &status, 0);

    return (int) ((char) WEXITSTATUS(status));
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
    /* TODO: sanity checks */

    ++level; // increment level for future call
    int ret;

    switch (c->op) {
        case OP_NONE:
            ret = parse_simple(c->scmd, level, father);
            break;

        case OP_SEQUENTIAL:
            if (parse_command(c->cmd1, level, c) == SHELL_EXIT)
                ret = SHELL_EXIT;
            else ret = parse_command(c->cmd2, level, c);
            break;

        case OP_PARALLEL:
            ret = run_in_parallel(c->cmd1, c->cmd2, level, c);
            break;

        case OP_CONDITIONAL_NZERO:
            ret = parse_command(c->cmd1, level, c);
            if (ret != 0)
                ret = parse_command(c->cmd2, level, c);
            break;

        case OP_CONDITIONAL_ZERO:
            ret = parse_command(c->cmd1, level, c);
            if (ret == 0)
                ret = parse_command(c->cmd2, level, c);
            break;

        case OP_PIPE:
            ret = run_on_pipe(c->cmd1, c->cmd2, level, c);
            break;

        default:
            ret = SHELL_EXIT;
            break;
    }

    return ret;
}
