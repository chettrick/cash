/* sh.c
 * SSI: Simple Shell Interpreter
 *
 * CSC360 - 2018 Spring
 * Dr. Jianping Pan
 *
 * Christopher Hettrick
 */

#include <err.h>		/* err(3) */
#include <libgen.h>		/* basename(3) */
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
static void		 cmd_run(struct args *, const char *);
static void		 usage(void) __attribute__ ((__noreturn__));

extern char	 *__progname;

/* XXX char	 *oldpwd = NULL; */	/* Old working directory. */

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

		printf("argc: %d\n", args->argc);		/* XXX */
		int i;						/* XXX */
		for (i = 0; args->argv[i] != '\0'; i++) {	/* XXX */
			printf("argv[%d]: %s\n", i, args->argv[i]); /* XXX */
		}						/* XXX */
		printf("Command name: %s\n", args->file);	/* XXX */
		printf("You wrote: %s\n", line);		/* XXX */

		/* Exit shell. */
		if (!strcasecmp(args->argv[0], "exit")) {
			free(line);
			return 0;
		}

		cmd_run(args, home_dir);

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
static struct args *
parse_args(char *line)
{
	enum lex_state {
			  DELIM,
			  OTHER
	} l_state;

	const char	 *delim = " \t";	/* Delimiters between args. */
	int		  argc;		/* Count of arguments in string. */
	char		**argv;		/* Pointer to array of arg vectors. */
	char		 *c;		/* Current token in string. */

	char		 *p;		/* Pointer to strdup'd line. */
	char		**ap;
	struct args	 *args;		/* All arg details from this line. */

	if (strlen(line) == 0) {	/* Only work on strings with tokens. */
		return NULL;
	}

	/* Choose initial state as DELIM if first char is in delim. */
	(strspn(line, delim) > 0) ? (l_state = DELIM) : (l_state = OTHER);

	argc = 0;
	for (c = line; *c != '\0'; c++) {
		switch (l_state) {
		case DELIM:
			c += strspn(c, delim) - 1;
			l_state = OTHER;
			break;
		case OTHER:
			argc++;
			c += strcspn(c, delim) - 1;
			l_state = DELIM;
			break;
		/* No default: since state is an enum; no other cases. */
		}
	}

	printf("argc: %d\n", argc);		/* XXX */

	/* No args, just whitespace. Do nothing. */
	if (argc == 0) {
		return NULL;
	}

	/* Need an argv on the heap, not on the stack, so malloc(). */
	if ((argv = calloc((size_t)argc + 1, sizeof(*argv))) == NULL) {
		err(1, "calloc");
	}

	/* Need a copy of the line, since it will be clobbered. */
	if ((p = strdup(line)) == NULL) {
		err(1, "strdup");
	}

	/* Build argv. */
	ap = argv;
	while (ap < &argv[argc] && (*ap = strsep(&p, delim)) != NULL) {
		if (**ap != '\0') {
			ap++;
		}
	}
	argv[argc] = (char *)NULL;		/* Last item must be NULL. */

	/* Allocate space on heap for the struct to return. */
	if ((args = calloc(1, sizeof(args))) == NULL) {
		err(1, "calloc");
	}

	/* Populate the args struct. */
	if (!strcmp(argv[0], "bg")) {
		args->ps = STATE_BG;
		args->file = argv[1];
	} else {
		args->ps = STATE_FG;
		args->file = argv[0];
	}
	args->argv = argv;
	args->argc = argc;

	return args;
}

static void
cmd_run(struct args *a, const char *home_dir)
{
	const char	*cmd;
/* XXX	char		*tempdir; */

	cmd = basename(a->argv[0]);

/* XXX - Need to add cwd to path for err() and warn(). */
	if (!strcmp(cmd, "cd")) {
		if (a->argc == 1) {		/* No args to cd. */
			if (chdir(home_dir) == -1) {
				warn("%s: %s", cmd, home_dir);
			}
		} else if (a->argc == 2) {	/* Only one arg to cd. */
			if (!strcmp(a->argv[1], "-")) {
#if 0 /* XXX - Complete later. */
				if (oldpwd != NULL) {

				} else {
					warnx("%s: no OLDPWD\n", cmd);
				}
#endif
			} else if (!strcmp(a->argv[1], "~")) {
				if (chdir(home_dir) == -1) {
					warn("%s: %s", cmd, home_dir);
				}
			} else {
				/* XXX - Save pwd in oldpwd. */
				if (chdir(a->argv[1]) == -1) {
					warn("%s: %s", cmd, a->argv[1]);
				}
			}
		} else {			/* More than one arg to cd. */
			warnx("%s: too many arguments", cmd);
			return;
		}
	}

#if 0
done	if cmd is 'cd' then
done		if no args then
done			cd to home dir
done		else if there is only one arg
XXXskip			if arg is '-' then
XXXskip				if OLDPWD exists then
XXXskip					save curdir into tempdir
XXXskip					cd to OLDPWD
XXXskip					set OLDPWD to tempdir
XXXskip					print the now pwd
XXXskip				else
XXXskip					error "ssi: cd: no OLDPWD"
done			else if arg is '~' then
done				cd to home dir
done			else
XXXskip				save pwd in oldpwd
done				chdir() to new dir in arg
done		else /* Two or more args */
done			error "ssi: cd: too many arguments"
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
#endif
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

done		struct proc p = parse_line(line); /* Ret alloc struct or NULL */
done			proc_state = fg;	/* Child in fg by default */
done		if (p == NULL)
done			error message;

		cmd logic
		fork
		exec

		if child exits then
			print "pid: cmd options has terminated."
done		update cwd (because could be different now)
#endif

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s\n", __progname);

	exit(1);
}
