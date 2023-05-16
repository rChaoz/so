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
static int shell_cd(word_t *dir)
{
    // First, get current working directory
    char oldDir[1024];
    int saveOldDir = getcwd(oldDir, 1024) != NULL;

    int res;

    // If no argument is provided, change directory to $HOME
    if (dir == NULL) {
        const char *home = getenv("HOME");
        if (home == NULL) {
            fputs("The $HOME variable is not set.\n", stderr);
            return EXIT_FAILURE;
        } else res = chdir(home);

    } else {
        // If the argument is "-", change directory to $OLDPWD
        char *path = parse_word(dir);
        if (strcmp(path, "-") == 0) {
            const char *old = getenv("OLDPWD");
            if (old == NULL) {
                fputs("The $OLDPWD variable is not set.\n", stderr);
                return EXIT_FAILURE;
            } else res = chdir(old);
        } else res = chdir(path);
        free(path);
    }

    // Print error (if any) and return
    if (res != 0) {
        switch (errno) {
            case ENOENT:
                fputs("The specified path cannot be found.\n", stderr);
                break;
            case EACCES:
                fputs("Permission denied.\n", stderr);
                break;
            default:
                fputs("Unknown error.\n", stderr);
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
        fprintf(stderr, "Unable to open file \"%s\" for input redirection.", fileName);
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
    if (strcmp(command, "cd") == 0)
        return shell_cd(s->params);

    // Fork process
    int pid = fork();
    DIE(pid < 0, "fork() failed");

    // Main process will wait for child process to end and return
    if (pid > 0) {
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

    if (s->err != NULL)
        if (open_file(s->err, stderr, s->io_flags & IO_ERR_APPEND ? "a" : "w") == NULL)
            exit(errno);

    int ret;

    // If built-in command, execute the command
    if (strcmp(command, "pwd") == 0)
        ret = shell_pwd();
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
        ret = shell_exit();

    else {
        // Create command string
        size_t capacity = strlen(command) + 1024, len = 0;
        char *buf = malloc(capacity);
        DIE(buf == NULL, "malloc() failed");
        word_t *arg = s->params;

        len += sprintf(buf + len, "%s", command);
        while (arg != NULL) {
            char *v = parse_word(arg);
            char *toAdd;
            if (v[0] == '\0') toAdd = "\"\"";
            else toAdd = v;
            size_t toAddLength = strlen(toAdd);

            // Add argument to command string
            if (len + toAddLength + 1 >= capacity) {
                buf = realloc(buf, capacity = len + toAddLength + 1025);
                DIE(buf == NULL, "realloc() failed");
            }
            len += sprintf(buf + len, " %s", toAdd);

            free(v);
            arg = arg->next_word;
        }

        // Execute command with system call
        int status = system(buf);
        free(buf);
        ret = WEXITSTATUS(status);
    }

    // Exit child process
    exit(ret);
}

/**
 * Process two commands in parallel, by creating two children.
 */
static int run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
                           command_t *father)
{
    /* TODO: Execute cmd1 and cmd2 simultaneously. */

    return 0; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static int run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
                       command_t *father)
{
    /* TODO: Redirect the output of cmd1 to the input of cmd2. */

    return 0; /* TODO: Replace with actual exit status. */
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
            if (ret == 0)
                ret = parse_command(c->cmd2, level, c);
            break;

        case OP_CONDITIONAL_ZERO:
            ret = parse_command(c->cmd1, level, c);
            if (ret != 0)
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
