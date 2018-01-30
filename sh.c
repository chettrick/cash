/* sh.c
 * SSI: Simple Shell Interpreter
 *
 * CSC360 - 2018 Spring
 * Dr. Jianping Pan
 *
 * Christopher Hettrick
 */

#include <sys/wait.h>		/* wait(2) */

#include <err.h>		/* err(3), warn(3), warnx(3) */
#include <libgen.h>		/* basename(3) */
#include <limits.h>		/* PATH_MAX */
#include <stdio.h>		/* printf(3), fprintf(3), snprintf(3) */
				/* readline(3) */
#include <stddef.h>		/* size_t */
#include <stdlib.h>		/* exit(3), free(3), getenv(3), calloc(3) */
#include <string.h>		/* strdup(3), strcmp(3), strlen(3) */
				/* strspn(3), strcspn(3), strsep(3) */
#include <unistd.h>		/* getcwd(3), fork(2), execvp(3) */

#include <readline/readline.h>	/* readline(3) */
#include <readline/history.h>	/* add_history(3) */

#define PROMPT_SIZE	(5 + PATH_MAX + 3 + 1)	/* "SSI: " + cwd + " > " + \0 */

enum proc_state {
	STATE_FG,
	STATE_BG
};

struct args {
	char	 *file;			/* (Full) path of new process file. */
	char	**realargv;		/* Immutable pointer to arg vectors. */
	char	**argv;			/* Mutable pointer to arg vectors. */
	int	  argc;			/* Argument count. */
	enum	  proc_state ps;	/* Foreground or background process. */
};

struct proc {
	struct	  proc *next;		/* Next process in process list. */
	pid_t	  pid;			/* Process id. */
	struct	  args *a;		/* Process command arguments. */
};

static void		 cwd_prompt(char *, size_t);
static struct args	**args_parse(char **);
static void		 args_free(struct args **);

static int		 builtin_run(struct args **, const char *);
static struct proc	**proc_run(struct args **, const char *);
#if 0
static void		 proc_free(struct proc **);
#endif

static void		 bg_add(struct proc **);
static void		 bg_print(struct proc **, char *);
static void		 bg_list(void);
static struct proc	**bg_find(pid_t);
static void		 bg_remove(struct proc **);
static void		 bg_free(struct proc *);

static void		 usage(void) __attribute__ ((__noreturn__));

static struct proc	*bghead = NULL;	/* Bg processes list head. */

/*
 * SSI: Simple Shell Interpreter
 *
 * Very basic Bourne Shell functionality.
 *
 * Caveats: Quoted arguments are not supported.
 *          Backslash escapes are not supported.
 *          'cd ~' will change to the user's home directory, but
 *          no filename expansion is done on '~'.
 */
int
main(int argc, char *argv[])
{
	char		*line;			/* Readline returned line. */
	char		 prompt[PROMPT_SIZE];	/* Shell prompt. PS1. */
	const char	*home_dir;		/* User's home directory. */
	struct args	**args;			/* Ptr to arguments struct. */
	struct proc	**np;			/* Ptr to new process. */
	pid_t		 ch_pid = 0;		/* Child process ID. */
	struct proc	**bg_pid;		/* Ptr to ptr to bg struct. */

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
		/* Check for processes in bglist that have finished. */
		ch_pid = waitpid(WAIT_MYPGRP, NULL, WNOHANG);
		while (ch_pid > 0) {		/* Child exited. */
			/* Remove child from background process list. */
			bg_pid = bg_find(ch_pid);
			bg_remove(bg_pid);
			free(*bg_pid);
			/* Check for more children exiting. */
			ch_pid = waitpid(WAIT_MYPGRP, NULL, WNOHANG);
		}

		/* Get arguments struct from command line. */
		if ((args = args_parse(&line)) == NULL) {
			free(line);
#if 0
			line = NULL;
#endif
			continue;		/* Skip blank lines. */
		}

#if 0
		add_history(line);		/* Readline history. */
#endif

		if ((np = proc_run(args, home_dir)) != NULL) {
			/* Background process. */
			bg_add(np);
#if 0
			np = NULL;
#endif
		}

		/* Do not need the line anymore. Free it. */
		free(line);
		line = NULL;
#if 0
#endif

		cwd_prompt(prompt, PROMPT_SIZE);
	}

	/* Free all structs for background processes, but dont kill them. */
	bg_free(bghead);

	return 0;
}

static void
cwd_prompt(char *prompt, size_t promptsize)
{
	char		*buf;
	char		*p;
	int		 ret;

	if ((buf = calloc(PATH_MAX, sizeof(char))) == NULL) {
		err(1, "calloc");
	}

	if ((p = getcwd(buf, PATH_MAX)) == NULL) {
		err(1, "getcwd");
	}

	ret = snprintf(prompt, promptsize, "SSI: %s > ", buf);
	if (ret == -1 || ret >= (int)promptsize) {
		free(buf);
#if 0
		buf = NULL;
#endif
		err(1, "snprintf");
	}
	free(buf);
#if 0
	buf = NULL;
#endif
}

/*
 * Parse supplied string of text into separate arguments.
 * First argument is the command name.
 *
 * Note: Does not work with quotes or filenames with spaces, yet.
 */
static struct args **
args_parse(char **line)
{
	enum lex_state {
			  IFS,
			  OTHER
	} l_state;

	const char	 *ifs = " \t";	/* Delimiters between args. */
	int		  argc;		/* Count of arguments in string. */
	char		**argv;		/* Pointer to array of arg vectors. */
	char		 *c;		/* Current token in string. */

	char		 *p;		/* Pointer to strdup'd line. */
	char		**ap;		/* Pointer to walk along line. */
	struct args	*args;		/* All arg details from this line. */
	struct args	**retargs;	/* All arg details from this line. */

	if (strlen(*line) == 0) {	/* Only work on strings with tokens. */
		return NULL;
	}

	/* Choose initial state as IFS if first char is in ifs. */
	(strspn(*line, ifs) > 0) ? (l_state = IFS) : (l_state = OTHER);

	/* Split line up based on ifs whitespace; count number of arguments. */
	argc = 0;
	for (c = *line; *c != '\0'; c++) {
		switch (l_state) {
		case IFS:
			c += strspn(c, ifs) - 1;
			l_state = OTHER;
			break;
		case OTHER:
			argc++;
			c += strcspn(c, ifs) - 1;
			l_state = IFS;
			break;
		/* No default case since state is an enum; no other cases. */
		}
	}

	/* No args, just whitespace. Do nothing. */
	if (argc == 0) {
		return NULL;
	}

	/* Need an argv on the heap, not on the stack, so calloc(). */
	if ((argv = calloc((size_t)argc, sizeof(*argv))) == NULL) {
		err(1, "calloc");
	}

	/* Need a copy of the line, since it will be clobbered. */
	if ((p = strdup(*line)) == NULL) {
		err(1, "strdup");
	}

	/* Build argv. */
	ap = argv;
	while (ap < &argv[argc] && (*ap = strsep(&p, ifs)) != NULL) {
		if (**ap != '\0') {
			ap++;
		}
	}
	argv[argc] = (char *)NULL;		/* Last item must be NULL. */

	/* bg without any arguments.
	 * Must be done here since accessing argv[1] is a segfault.
	 */
	if (argc == 1 && !strcmp(argv[0], "bg")) {
		warnx("%s: missing command argument", argv[0]);
		free(argv);
#if 0
		argv = NULL;
		free(p);
		p = NULL;
#endif

		return NULL;
	}

	/* Allocate space on the heap for the struct to return. */
	if ((args = calloc(1, sizeof(args))) == NULL) {
		err(1, "calloc");
	}

	/* Populate the args struct. */
	if (!strcmp(argv[0], "bg")) {
		args->file = argv[1];		/* Skip first token (bg). */
		args->realargv = argv;		/* For passing to free(). */
		args->argv = &argv[1];		/* for passing to execvp(). */
		args->argc = argc - 1;		/* - 1 because skipped bg. */
		args->ps = STATE_BG;		/* Background execution. */
	} else {
		args->file = argv[0];		/* Use first token as file. */
		args->realargv = argv;		/* For passing to free(). */
		args->argv = argv;		/* for passing to execvp(). */
		args->argc = argc;
		args->ps = STATE_FG;		/* Foreground execution. */
	}

#if 0
	free(p);
	p = NULL;
#endif
	retargs = &args;
	return retargs;
}

static void
args_free(struct args **aa)
{
	struct args *a = *aa;

	free(a->realargv);
#if 0
	a->realargv = NULL;
	a->argv = NULL;
#endif
	free(a);
}

static int
builtin_run(struct args **aa, const char *home_dir)
{
	const char	*cmd;
	struct args	*a = *aa;

	cmd = basename(a->argv[0]);

	if (!strcmp(cmd, "exit")) {		/* Exit shell. */
		args_free(&a);
#if 0
		a = NULL;
#endif

		exit(0);
	} else if (!strcmp(cmd, "cd")) {
		switch (a->argc) {
		case 1:				/* No args to cd. */
			if (chdir(home_dir) == -1) {
				warn("%s: %s", cmd, home_dir);
			}
			break;
		case 2:				/* Only one arg to cd. */
			if (!strcmp(a->argv[1], "~")) {
				if (chdir(home_dir) == -1) {
					warn("%s: %s", cmd, home_dir);
				}
			} else {		/* Plain cd dir. */
				if (chdir(a->argv[1]) == -1) {
					warn("%s: %s", cmd, a->argv[1]);
				}
			}
			break;
		default:			/* More than one arg to cd. */
			warnx("%s: too many arguments", cmd);
			return 1;
		}
	} else if (!strcmp(cmd, "bglist")) {
		/* Run through the bglist and print it out. */
		bg_list();
	} else {				/* Not a builtin. */
		return 1;
	}

	return 0;
}

static struct proc **
proc_run(struct args **aa, const char *home_dir)
{
	pid_t		 pid;
	struct proc	**np;
	struct proc	*p;
	struct args	*a = *aa;

	if (builtin_run(aa, home_dir) == 0) {	/* Try builtin cmd first. */
		args_free(aa);
#if 0
		aa = NULL;
#endif
		return NULL;			/* Was a builtin command. */
	} else {				/* fork() and exec() child. */
		if ((pid = fork()) == -1) {
			warn("fork");
			args_free(aa);
#if 0
			aa = NULL;
#endif

			return NULL;
		}

		if (a->ps == STATE_BG) {	/* Background exec(). */
			if (pid == 0) {		/* Child. */

				/* Okay, now finally run the damn thing. */
				if (execvp(a->file, a->argv) == -1) {
					warnx("%s: not found", a->file);
					args_free(aa);
#if 0
					aa = NULL;
#endif

					_exit(127);	/* 127 cmd not found. */
				}
			} else {		/* Parent. */
				/* Wait for non-blocking child. */
				if (waitpid(WAIT_MYPGRP, NULL, WNOHANG) == -1) {
					err(1, "waitpid");
				}

				/* Build up process struct. */
				if ((p = calloc(1, sizeof(p))) == NULL) {
					err(1, "calloc");
				}
				p->next = NULL;
				p->pid = pid;
				p->a = a;

				np = &p;

				return np; 	/* Return the proc struct *. */
			}
		} else {			/* Foreground exec(). */
			if (pid == 0) {		/* Child. */
				if (execvp(a->file, a->argv) == -1) {
					warnx("%s: not found", a->file);
				}
				_exit(127);	/* 127 for cmd not found. */
			} else {		/* Parent. */
				wait(NULL);	/* Block for child. */

				return NULL;	/* Nothing to send back. */
			}
		}
	}

	return NULL;		/* XXX To satiate the compiler. */
}

#if 0
static void
proc_free(struct proc **np)
{
	struct proc	*p = *np;
	args_free(&p->a);
#if 0
	p->a = NULL;
#endif
	free(p);
}
#endif

static void
bg_add(struct proc **np)
{
	struct proc	*p = *np;

	if (bghead == NULL) {	/* Add first item in list. */
		bghead = p;
	} else {		/* Add to front of list. */
		p->next = bghead;
		bghead = p;
	}
	bg_print(&p, NULL);
}

/*
 * Print out the background command and arguments followed by
 * a supplied string s.
 */
static void
bg_print(struct proc **np, char *s)
{
	int		 i;
	struct proc	*p = *np;

	printf("%d:", p->pid);
	for (i = 0; i < p->a->argc; i++) {
		printf(" %s", p->a->argv[i]);
	}
	if (s != NULL) {
		printf("%s", s);
	}
	printf("\n");
}

/*
 * Run through the bglist and print it out.
 */
static void
bg_list(void)
{
	struct proc	*p;
	int		 jobcnt = 0;

	for (p = bghead; p != NULL; p = p->next) {
		bg_print(&p, NULL);
		jobcnt++;
	}
	printf("Total Background Jobs:\t%d\n", jobcnt);
}

/*
 * Find a background process by its pid and return a pointer to it.
 */
struct proc **
bg_find(pid_t pid)
{
	struct proc	**np = &bghead;
	struct proc	 *p;

	if (np == NULL) {
		return NULL;
	}

	for (p = *np; p != NULL; p = p->next) {
		if (p->pid == pid) {
			/* Found the struct with pid 'pid'. */
			return np;
		}
	}

	/* Can't find the struct with pid 'pid'. */
	return NULL;
}

/*
 * Best effort to remove process identified by struct proc * from the
 * global background processes list.
 *
 * Note: No error code is returned if the process is not found.
 */
static void
bg_remove(struct proc **np)
{
	struct proc	*prev = NULL;
	struct proc	*curr = bghead;;
	struct proc	*p = *np;

	/* Nothing to remove. */
	if (p == NULL) {
		return;
	}

	/* Remove the head. */
	if (p == curr) {
		bg_print(&p, " has terminated.");
		curr = p->next;
		free(p);
		return;
	}

	while (curr != NULL) {
		if (p == curr) {
			bg_print(&p, " has terminated.");
			if (curr->next == NULL) {
				/* Remove the tail. */
				prev->next = NULL;
				free(curr);
			} else {
				/* Remove in the middle. */
				prev->next = curr->next;
				free(curr);
			}
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

/*
 * Free the entire background processes list, but don't kill them.
 */
static void
bg_free(struct proc *p)
{
	p++;		/* XXX */
}

static void
usage(void)
{
	extern char	*__progname;

	(void)fprintf(stderr, "usage: %s\n", __progname);

	exit(1);
}
