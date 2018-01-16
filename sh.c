#include <err.h>		/* err(3) */
#include <limits.h>		/* PATH_MAX */
#include <stdio.h>		/* printf(3) */
#include <stdlib.h>		/* exit(3), free(3), getenv(3) */
#include <string.h>		/* strdup(3) */
#include <strings.h>		/* strcasecmp(3) */
#include <unistd.h>		/* getcwd(3) */

#include "linenoise.h"		/* line editing library */

#define PROMPT_SIZE	5 + PATH_MAX + 3 + 1

static void		cwd_prompt(char *, int);
static void		usage(void);

int
main(int argc, char *argv[])
{
	char		*line;
	char		 prompt[PROMPT_SIZE];
	const char	*home_dir;

	if (argc > 1) {
		usage();
	}
	argc--;
	argv++;			/* To satiate the compiler. */

#if 0
	printf("\n%s\n", getenv("PATH"));
#endif

	if ((home_dir = getenv("HOME")) == NULL) {
		fprintf(stderr, "HOME enviroment variable not set");
		err(1, "getenv");
	}

	cwd_prompt(prompt, PROMPT_SIZE);
	while ((line = linenoise(prompt)) != NULL) {
		if (!strcasecmp(line, "exit")) {
			free(line);
			return 0;
		}
		printf("You wrote: %s\n", line);

		free(line);
		cwd_prompt(prompt, PROMPT_SIZE);
	}

	return 0;
}

static void
cwd_prompt(char *prompt, int promptsize)
{
	char		*buf;
	char		*p;
	int		 ret;

	if ((buf = malloc((PATH_MAX + 1) * sizeof(char))) == NULL) {
		err(1, "malloc");
	}

	if ((p = getcwd(buf, PATH_MAX)) == NULL) {
		err(1, "getcwd");
	}

	ret = snprintf(prompt, promptsize, "SSI: %s > ", buf);
	if (ret == -1 || ret >= promptsize) {
		err(1, "snprintf");
	}
	free(buf);
}

#if 0
	args usage();

	char * to malloc'd array of paths = parse_path($PATH);
	if char * is NULL then
		use _PATH_DEFPATH from paths.h

	save $HOME directory for use with ~

//done	make up initial custom cwd prompt with function cwd_prompt(pre, post)

//done	while get a line from linenoise using custom cwd prompt {
// XXX actually have a flag set in proc struct, not a global
		bg_flag = 0;	// Child process runs in foreground by default

		struct proc p = parse_line(line); // Ret alloc'd struct or NULL
		if (p == NULL)
			error message;

		if cmd is 'cd' then
			if no args then
				cd to home dir
			else if there is only one arg
				if arg is '-' then
					if OLDPWD exists then
						save curdir into tempdir
						cd to OLDPWD
						set OLDPWD to tempdir
						print the now pwd
					else
						error "ssi: cd: no OLDPWD"
				else
					save pwd in oldpwd
					chdir() to new dir in arg
			else // Two or more args
				error "ssi: cd: too many arguments"
		else if cmd is 'bglist' (or 'jobs')
			// print out list of background jobs
			while (struct.next != NULL) {
				print curproc "pid: path options"
				curstruct = struct.next
			}
			print "Total Background Jobs:\t%d"
		else
			if cmd is 'bg'
				shift first arg of arg string to be struct cmd
				set bg_flag = 1;
				fork and exec, but dont wait.
				detach child from stdin, stdout, stderr
				add to processes linked list
			else	// fg. Just regular fork() and exec().
				fork and exec
				wait on child exit.

		if child exits then
			print "pid: cmd options has terminated."
		update cwd (because could be different now)
	}
#endif

#if 0
struct myproc parse_line(char *line);
char *	parse_path(PATH);
#endif

static void
usage(void)
{
	extern char	*__progname;

	(void)fprintf(stderr, "usage: %s\n", __progname);

	exit(1);
}
