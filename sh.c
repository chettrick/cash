/* sh.c
 * SSI: Simple Shell Interpreter
 *
 * CSC360 - 2018 Spring
 * Dr. Jianping Pan
 *
 * Christopher Hettrick
 */

#include <err.h>		/* err(3) */
#include <limits.h>		/* PATH_MAX */
#include <stdio.h>		/* printf(3), snprintf(3), readline(3) */
#include <stddef.h>		/* size_t */
#include <stdlib.h>		/* exit(3), free(3), getenv(3), calloc(3) */
#include <string.h>		/* strdup(3) */
#include <strings.h>		/* strcasecmp(3) */
#include <unistd.h>		/* getcwd(3), fork(2) */

#include <readline/readline.h>	/* readline(3) */

#define PROMPT_SIZE	(5 + PATH_MAX + 3 + 1)

enum proc_state {
	STATE_FG,
	STATE_BG
};

struct args {
	char	 *file;
	char	**argv;
	int	  argc;
	enum	  proc_state ps;
};

struct proc {
	struct	  proc *next;
	pid_t	  pid;
	struct	  args *a;
};

static void		 cwd_prompt(char *, size_t);
static struct args	*parse_args(char *);
static void		 usage(void) __attribute__ ((__noreturn__));

/*
 * SSI: Simple Shell Interpreter
 *
 * Very basic Bourne Shell functionality.
 *
 * Caveats: Quoted arguments are not supported.
 *          Backslash escapes are not supported.
 */
int
main(int argc, char *argv[])
{
	char		*line;
	char		 prompt[PROMPT_SIZE];
	const char	*home_dir;
	struct args	*args;

	if (argc > 1) {
		usage();
	}
	argc--;
	argv++;			/* XXX To satiate the compiler. */

#if 0
	printf("\n%s\n", getenv("PATH"));
#endif

	if ((home_dir = getenv("HOME")) == NULL) {
		fprintf(stderr, "HOME environment variable not set");
		err(1, "getenv");
	}

	cwd_prompt(prompt, PROMPT_SIZE);
	while ((line = readline(prompt)) != NULL) {
		printf("Line length: %lu\n", strlen(line));	/* XXX */
		if ((args = parse_args(line)) == NULL) {
			free(line);
			continue;		/* Skip blank lines. */
		}

/* XXX */
int i;
	for (i = 0; args->argv[i] != '\0'; i++) {
		printf("argv[%d]: %s\n", i, args->argv[i]);
	}
printf("Command name: %s\n", args->file);
/* XXX */

/* XXX Get cmd from struct */
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
cwd_prompt(char *prompt, size_t promptsize)
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
	if (ret == -1 || ret >= (int)promptsize) {
		err(1, "snprintf");
	}
	free(buf);
}

/* Does not work with quotes, yet. */
struct args *
parse_args(char *line)
{
	enum state {
			 DELIM,
			 OTHER
	} state;

	const char	*delim = " \t";	/* Delimiters between arguments. */
	int		 argc;		/* Count of arguments in string. */
	char		*c;		/* Current token in string. */

	char		 *p;
	char		**ap;
	char		**newargv;
	struct args	 *args;
	int		  i;			/* XXX */

	if (strlen(line) == 0) {	/* Only work on strings with tokens. */
		return NULL;
	}

	/* Choose initial state as DELIM if first char is in delim. */
	(strspn(line, delim) > 0) ? (state = DELIM) : (state = OTHER);

	argc = 0;
	for (c = line; *c != '\0'; c++) {
		switch (state) {
		case DELIM:
			c += strspn(c, delim) - 1;
			state = OTHER;
			break;
		case OTHER:
			argc++;
			c += strcspn(c, delim) - 1;
			state = DELIM;
			break;
		/* No default: since state is an enum; no other cases. */
		}
	}

	printf("argc: %d\n", argc);		/* XXX */

	char		 *argv[argc + 1];	/* XXX - Here bc needs argc. */

	/* Need a copy of the line, since it will be clobbered. */
	if ((p = strdup(line)) == NULL) {
		err(1, "strdup");
	}

	/* Now build argv. */
	for (ap = argv; ap < &argv[argc] &&
	    (*ap = strsep(&p, delim)) != NULL;) {
		if (**ap != '\0') {
			ap++;
		}
	}
	*ap = NULL;

	argv[argc] = NULL;


	/* Need newargv on the heap, not on the stack like argv. */
	if ((newargv = calloc((size_t)argc + 1, sizeof(*newargv))) == NULL) {
		err(1, "calloc");
	}

	for (i = 0; i < argc; i++) {
		if ((newargv[i] = strdup(argv[i])) == NULL) {
			err(1, "strdup");
		}
	}
	newargv[argc] = NULL;

	/* Now make the struct to return with argc and newargv. */
	if ((args = calloc(1, sizeof(args))) == NULL) {
		err(1, "calloc");
	}

	if (!strcmp(newargv[0], "bg")) {
		args->ps = STATE_BG;
		args->file = newargv[1];
	} else {
		args->ps = STATE_FG;
		args->file = newargv[0];
	}
	args->argv = newargv;
	args->argc = argc;


	return args;
}






#if 0
done	args usage();

n/a	char * to mallocd array of paths = parse_path($PATH);
n/a	if char * is NULL then
n/a		use _PATH_DEFPATH from paths.h

done	save $HOME directory for use with ~

done	make up initial custom cwd prompt with function cwd_prompt(pre, post)

done	while get a line from readline using custom cwd prompt
XXX actually have a flag set in proc struct, not a global

		struct proc p = parse_line(line); /* Ret alloc struct or NULL */
			proc_state = fg;	/* Child in fg by default */
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
			else /* Two or more args */
				error "ssi: cd: too many arguments"
		else if cmd is 'bglist' (or 'jobs')
			/* print out list of background jobs */
			while (struct.next != NULL) {
				print curproc "pid: path options"
				curstruct = struct.next
			}
			print "Total Background Jobs:\t%d"
		else
			if cmd is 'bg'
				shift first arg of arg string to be struct cmd
				set proc_state to bg
				fork and exec, but dont wait.
				detach child from stdin, stdout, stderr
				add to processes linked list
			else	/* fg. Just regular fork() and exec(). */
				fork and exec
				wait on child exit.

		if child exits then
			print "pid: cmd options has terminated."
		update cwd (because could be different now)
#endif

static void
usage(void)
{
	extern char	*__progname;

	(void)fprintf(stderr, "usage: %s\n", __progname);

	exit(1);
}
