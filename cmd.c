// SPDX-License-Identifier: BSD-3-Clause

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "utils.h"

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO: Execute cd. */
	if (dir == NULL)
		return false;

	if (chdir(dir->string) == -1)
		return false;

	return true;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO: Execute exit/quit. */
	return SHELL_EXIT; /* TODO: Replace with actual exit code. */
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	/* TODO: Sanity checks. */
	if (s == NULL || s->verb == NULL)
		return 1;

	if (strcmp(s->verb->string, "exit") == 0 || strcmp(s->verb->string, "quit") == 0)
		return shell_exit();

	/* TODO: If builtin command, execute the command. */

	if (strcmp(s->verb->string, "cd") == 0) {
		if (shell_cd(s->params) == true)
			return 0;
		return 1;
	}

	/* TODO: If variable assignment, execute the assignment and return
	 * the exit status.
	 */

	/* TODO: If external command:
	 *   1. Fork new process
	 *     2c. Perform redirections in child
	 *     3c. Load executable in child
	 *   2. Wait for child
	 *   3. Return exit status
	 */

	int pid, status;

	pid = fork();
	if (pid == -1)
		return 1;

	if (pid == 0) {
		int in_fd, out_fd, err_fd;

		if (s->in) {
			char *in_filename = get_word(s->in);

			in_fd = open(in_filename, O_RDONLY | O_CREAT, 0644);
			if (in_fd == -1)
				return 1;
			dup2(in_fd, STDIN_FILENO);
			close(in_fd);
		}

		if (s->out) {
			char *out_filename = get_word(s->out);

			if (s->io_flags == IO_OUT_APPEND || s->err)
				out_fd = open(out_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
			else
				out_fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (out_fd == -1)
				return 1;
			dup2(out_fd, STDOUT_FILENO);
			close(out_fd);
		}

		if (s->err) {
			char *err_filename = get_word(s->err);

			if (s->io_flags == IO_ERR_APPEND)
				err_fd = open(err_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
			else
				err_fd = open(err_filename, O_WRONLY | O_CREAT, 0644);
			if (err_fd == -1)
				return 1;
			dup2(err_fd, STDERR_FILENO);
			close(err_fd);
		}

		int argc;
		char **args;
		
		args = get_argv(s, &argc);
		execvp(s->verb->string, args);
	} else {
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}

	return 0; /* TODO: Replace with actual exit status. */
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool run_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Execute cmd1 and cmd2 simultaneously. */
	int pid1, pid2, ret, status1, status2;

	pid1 = fork();
	if (pid1 == -1)
		return false;

	if (pid1 == 0) {
		ret = parse_command(cmd1, level + 1, father);
		exit(ret);
	}

	pid2 = fork();
	if (pid2 == -1)
		return false;

	if (pid2 == 0) {
		ret = parse_command(cmd2, level + 1, father);
		exit(ret);
	}

	waitpid(pid1, &status1, 0);
	waitpid(pid2, &status2, 0);

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2).
 */
static bool run_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO: Redirect the output of cmd1 to the input of cmd2. */
	int pid1, pid2, ret, status1, status2;
	int fd[2];

	if (pipe(fd) == -1)
		return false;

	pid1 = fork();
	if (pid1 == -1)
		return false;

	if (pid1 == 0) {
		close(fd[READ]);
		dup2(fd[WRITE], STDOUT_FILENO);
		close(fd[WRITE]);
		ret = parse_command(cmd1, level + 1, father);
		exit(ret);
	}

	pid2 = fork();
	if (pid2 == -1)
		return false;

	if (pid2 == 0) {
		close(fd[WRITE]);
		dup2(fd[READ], STDIN_FILENO);
		close(fd[READ]);
		ret = parse_command(cmd2, level + 1, father);
		exit(ret);
	}

	close(fd[READ]);
	close(fd[WRITE]);

	waitpid(pid1, &status1, 0);
	waitpid(pid2, &status2, 0);

	return true; /* TODO: Replace with actual exit status. */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO: sanity checks */
	if (c == NULL)
		return shell_exit();

	int ret;

	if (c->op == OP_NONE) {
		/* TODO: Execute a simple command. */
		return parse_simple(c->scmd, level, father);; /* TODO: Replace with actual exit code of command. */
	}

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* TODO: Execute the commands one after the other. */
		ret = parse_command(c->cmd1, level + 1, c);
		ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_PARALLEL:
		/* TODO: Execute the commands simultaneously. */
		ret = run_in_parallel(c->cmd1, c->cmd2, level + 1, c);
		break;

	case OP_CONDITIONAL_NZERO:
		/* TODO: Execute the second command only if the first one
		 * returns non zero.
		 */
		if (parse_command(c->cmd1, level + 1, c) != 0)
			ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_CONDITIONAL_ZERO:
		/* TODO: Execute the second command only if the first one
		 * returns zero.
		 */
		if (parse_command(c->cmd1, level + 1, c) == 0)
			ret = parse_command(c->cmd2, level + 1, c);
		break;

	case OP_PIPE:
		/* TODO: Redirect the output of the first command to the
		 * input of the second.
		 */
		ret = run_on_pipe(c->cmd1, c->cmd2, level + 1, c);
		break;

	default:
		return SHELL_EXIT;
	}

	return ret; /* TODO: Replace with actual exit code of command. */
}
