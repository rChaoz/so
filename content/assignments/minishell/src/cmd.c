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

#define READ		0
#define WRITE		1


/**
 * Parse a word, expanding variables as necessary
 */
static char* parse_word(word_t *word) {
    char *res = malloc(1024);
    res[0] = '\0';
    size_t bufLength = 0, length = 0;

    while (word != NULL) {
        const char *toAdd = word->string;

        if (word->expand) { /* TODO */ }

        size_t toAddLength = strlen(toAdd);
        if (length + toAddLength >= bufLength) {
            res = realloc(res, length + toAddLength + 1024);
            DIE(res == NULL, "Unable to allocate memory");
        }

        strcat(res, toAdd);
        length += toAddLength;
        word = word->next_part;
    }

    return res;
}

const word_t HOME_WORD = { "HOME", true, NULL, NULL };

/**
 * Internal show or change-directory command.
 */
static int shell_cd(word_t *dir, FILE *out, FILE *err)
{
    if (dir == &HOME_WORD) {} // TODO check if $HOME is set, show error otherwise

    char *path = parse_word(dir);

    if (strcmp(path, "-") == 0) {
        // TODO check if $OLDPWD is set and change dir to that
    }

    // First, save current working directory into $OLDPWD
    char buf[1024];
    // If pwd fails, fail silently (don't change $OLDPWD)
    if (getcwd(buf, 1024) != NULL) {
        // TODO set $OLDPWD to BUF
    }

    int res = chdir(path);

    free(path);
    if (res != 0) {
        switch (errno) {
            case ENOENT:
                fprintf(err, "The specified path cannot be found.\n");
                break;
            case EACCES:
                fprintf(err, "Permission denied.\n");
                break;
            default:
                fprintf(err, "Unknown error.\n");
                break;
        }
        return errno;
    }
	return 0;
}

/**
 * Internal show working-directory command
 */
static int shell_pwd(FILE *out, FILE *err) {
    char buf[1024];
    if (getcwd(buf, 1024) == NULL) {
        fprintf(err, "Unable to get working directory: possibly path too long\n");
        return errno;
    } else {
        fputs(buf, out);
        fputc('\n', out);
    }
    return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void) {
	return SHELL_EXIT;
}

static FILE* open_file(word_t *word, char *mode) {
    char *fileName = parse_word(word);
    FILE *f = fopen(fileName, mode);
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

    // Resolve input/outputs redirections
    FILE *in = stdin;
    FILE *out = stdout;
    FILE *err = stderr;
    if (s->in != NULL) {
        in = open_file(s->in, "r");
        if (in == NULL) return errno;
    }
    if (s->out != NULL) {
        out = open_file(s->out, s->io_flags & IO_OUT_APPEND ? "a" : "w");
        if (out == NULL) {
            if (in != stdin) fclose(in);
            return errno;
        }
    }
    if (s->err != NULL) {
        err = open_file(s->err, s->io_flags & IO_ERR_APPEND ? "a" : "w");
        if (err == NULL) {
            if (in != stdin) fclose(in);
            if (out != stdout) fclose(out);
            return errno;
        }
    }

    // If built-in command, execute the command
    char *command = parse_word(s->verb);
    int ret = 0;
    if (strcmp(command, "cd") == 0)
        ret = shell_cd(s->params, out, err);
    else if (strcmp(command, "pwd") == 0)
        ret = shell_pwd(out, err);
    else if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0)
        ret = shell_exit();
    else if (1) {
        /* TODO: If variable assignment, execute the assignment and return
         * the exit status.
         */
    } else if (2) {
        /* TODO: If external command:
         *   1. Fork new process
         *     2c. Perform redirections in child
         *     3c. Load executable in child
         *   2. Wait for child
         *   3. Return exit status
         */
    }

    // Close input/output files and free memory
    if (in != stdin) fclose(in);
    if (out != stdout) fclose(out);
    if (err != stderr) fclose(err);
    free(command);

	return ret;
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
        return parse_simple(c->scmd, level, father);

	case OP_SEQUENTIAL:
        if (parse_command(c->cmd1, level, c) == SHELL_EXIT) return SHELL_EXIT;
        return parse_command(c->cmd2, level, c);

	case OP_PARALLEL:
        return run_in_parallel(c->cmd1, c->cmd2, level, c);

	case OP_CONDITIONAL_NZERO:
        ret = parse_command(c->cmd1, level, c);
		if (ret == 0) return parse_command(c->cmd2, level, c);
		return ret;

	case OP_CONDITIONAL_ZERO:
        ret = parse_command(c->cmd1, level, c);
        if (ret != 0) return parse_command(c->cmd2, level, c);
        return ret;

	case OP_PIPE:
		return run_on_pipe(c->cmd1, c->cmd2, level, c);

	default:
		return SHELL_EXIT;
	}
}
