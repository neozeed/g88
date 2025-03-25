/*
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/ui.c,v 1.10 90/08/19 21:18:44 robertb Exp $
 * $Locker:  $
 *
 */
/* Tektronix programming extensions to GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc.


This file is part of GDB.

GDB is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GDB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GDB; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */



/* 
 * Changes made by Tektronix are marked by TEK_HACK, TEK_PROG_HACK, 
 * and GHSFORTRAN.
 * Changes made by Data General are marked by DG_HACK.  Compiling without these
 * #defines should be equivalent to compiling vanilla 3.2, more or less.
 *
 * The changes done by Tektronix fit into three catagories:
 *	TEK_HACK -- these were done just to get GDB to work in our environment,
 *		    including work done to support the Green Hills C compiler.
 *	TEK_PROG_HACK -- These were done to extend GDB by adding programming
 *			 support: things like if-elif-else-endif, while, and
 *			 arguments to user defined commands.
 *	GHSFORTRAN -- These changes make (will make) GDB work with the Green
 *		      Hills Fortran compiler.
 *
 *
 * In addition, I/O routines were renamed so that I/O could be directed
 * to/from the X interface if used.  See the files ui.c and ui.h for
 * more information.  Here are the routines renamed:
 *    Old name:			Renamed to:
 *	fprintf			ui_fprintf
 *	printf			ui_fprintf(stdout
 *	putchar			ui_putchar
 *	putc			ui_putc
 *	fputc			ui_putc
 *	fputs			ui_fputs
 *	puts			ui_puts
 *	gets			ui_gets
 *	fgets			ui_fgets
 *	fflush			ui_fflush
 *	system			ui_system
 *	wait			ui_wait
 *	error			ui_badnews(-1
 *	fatal			ui_badnews(1
 *	getc, fgetc		replaced with ui_gets, ui_fgets
 * These changes are not demarcated by ifdef.   
 *
 *					November 16, 1989
 */




/*
 *  Generic user interface
 */

#include <stdio.h>
#define	ui_c
#include "ui.h"

#include <signal.h>

#define BADEXEC 127     /* exec fails */
#define	streq(a,b)	(strcmp(a,b) == 0)

void    doNothing(), help(), gripe(), loc_edit(), dflt_badnews(); 
int     doNothing_int();

#ifdef	TEK4107
void	tek4107_strtSubProc(), tek4107_getClientInfo(), 
	tek4107_doButtons(), tek4107_byebye(), tek4107_tickleMe(),
	tek4107_procflush(), tek4107_badnews(), tek4107_newprompt();
int	tek4107_initialize(), tek4107_fflush(), tek4107_call(), tek4107_wait();
#endif

#ifndef	NOXWINDOWS
void	xx_gripe(), xx_strtSubProc(), xx_edit(), xx_getClientInfo(), 
	xx_doButtons(), xx_byebye(), xx_tickleMe(), xx_help(), 
	xx_procflush(), xx_badnews(), xx_newprompt(), xx_endSubProc(),
        xx_button(), xx_unbutton(), xx_startRedirectOut(), xx_endRedirectOut(),
        xx_doneinit();
int	xx_initialize_gdb(), xx_fprintf(), xx_fputc(), xx_puts(), xx_fputs(),
	xx_fflush(), xx_call(), xx_wait(), xx_system(), xx_query();
char	*xx_gets(), *xx_fgets();
#endif

//int     fgetc(), fprintf(), fputc(), puts(), fputs(), fflush(), loc_call(),
int     fgetc(), fputc(), puts(), fputs(), fflush(), loc_call(),
	wait(), dflt_initialize(), retNothing(), system();
char    *gets(), *fgets();

UIVecStruct    standardVector = {
    "standard",
    dflt_initialize,     /* ui_initialize */
    doNothing_int,	 /* ui_newprompt */
    fprintf,
    fputc,
    puts,
    fputs,
    gets,
    fgets,
    fflush,
    doNothing,	    /* help  */
    doNothing,	    /* gripe */
    loc_call,       /* ui_call */
    wait,
    doNothing,      /* ui_strtSubProc */
    doNothing,      /* ui_endSubProc */
    loc_edit,       /* ui_edit */
    doNothing,      /* ui_getClientInfo */
    doNothing,      /* ui_doButtons */
    doNothing,      /* ui_byebye */
    dflt_badnews,
    doNothing,      /* ui_procflush */
    doNothing,      /* ui_tickleMe */
    NULL,      /* ui_system; nullop so the shell command will use execl
			if this is not defined */
    doNothing,	    /* ui_button */
    doNothing,      /* ui_unbutton */
    doNothing_int,  /* ui_query */
    doNothing,      /* ui_startRedirectOut */
    doNothing,      /* ui_endRedirectOut */
    doNothing,      /* ui_doneinit */
};

UIVector    uiVector = &standardVector;

#ifdef	TEK4107

UIVecStruct    tek4107Vector = {
    "tek4107",
    tek4107_initialize,
    tek4107_newprompt,
    fprintf,
    fputc,
    puts,
    fputs,
    gets,
    fgets,
    tek4107_fflush,
    doNothing,     /* help  */
    doNothing,     /* gripe */
    tek4107_call,   /* ui_call */
    tek4107_wait,
    tek4107_strtSubProc,
    tek4107_endSubProc,		/* ui_endSubProc */
    loc_edit,       /* ui_edit */
    tek4107_getClientInfo,
    tek4107_doButtons,
    tek4107_byebye,
    tek4107_badnews,
    doNothing,      /* ui_procflush */
    tek4107_tickleMe,
    system,
    doNothing,	    /* ui_button */
    doNothing,      /* ui_unbutton */
    doNothing,      /* ui_query */
    doNothing,      /* ui_startRedirectOut */
    doNothing,      /* ui_endRedirectOut */
    doNothing,      /* ui_doneinit */
};
#endif

#ifdef	NOXWINDOWS
char    *noXWin =
   "X-Window System interface not implimented in this version.\n";
#else
UIVecStruct    xwindowsVector = {
    "X-Window System",
    xx_initialize_gdb,
    xx_newprompt,
    xx_fprintf,
    xx_fputc,
    xx_puts,
    xx_fputs,
    xx_gets,
    xx_fgets,
    xx_fflush,
    xx_help,
    xx_gripe,
    xx_call,
    xx_wait,
    xx_strtSubProc,
    xx_endSubProc,
    xx_edit,
    xx_getClientInfo,
    xx_doButtons,
    xx_byebye,
    xx_badnews,
    xx_procflush,
    xx_tickleMe,
    xx_system,
    xx_button,
    xx_unbutton,
    xx_query,
    xx_startRedirectOut,
    xx_endRedirectOut,
    xx_doneinit,
};
#endif
 
char	badInit[] =
    "could not initialize %s user interface: reverting to standard.\n";

initUsrInt (choice, argc, argv)
    int     choice, *argc;
    char    **argv;
{
    static int	initialized = 0;
    char    *bad_tag = 0;

    if (initialized)  return;
    initialized = 1;

    switch (choice) {
    case 0:	/* standard user interface */
	uiVector = &standardVector;
	goto do_init;
    case 1:	/* X-Window System user interface */
#ifdef	NOXWINDOWS
	ui_fprintf(stderr, noXWin);
	uiVector = &standardVector;
#else
        uiVector = &xwindowsVector;
#endif
	goto do_init;
#ifdef	TEK4107
    case 2:	/* Terminal is a Tektronix 4107 */
	uiVector = &tek4107Vector;
	goto do_init;
#endif
default:
	bad_tag = "unknown";
	/* fall through */
do_init:
	if (! bad_tag) {
	    if (ui_initialize(argc, argv)) {
	        bad_tag = uiVector->uiv_name;
	    }
	}
	if (bad_tag) {
	    uiVector = &standardVector;
            ui_fprintf(stderr, badInit, bad_tag);
	    (void)ui_initialize(argc, argv);
        }
	break;
    }
}

void    doNothing() {}
int     doNothing_int() {}  /* -rcb 8/19/90 to get rid of warning */

int    retNothing() {return 0;}

/* VARARGS4 */
loc_call(sync, inF, outF, name, args)
    FILE    *inF, *outF;
    char    *name, *args;
{
    typedef void Operation();
    Operation   *old;
    int         pid, status;

    old = signal(SIGINT, SIG_DFL);
    pid = backv(name, inF, outF, &name);
    signal(SIGINT, SIG_IGN);
    if (sync) {
	    wait(pid, &status);
	    signal(SIGINT, old);
	    return pid;
    }
    signal(SIGINT, old);
    return status;
}

fswap(oldfd, newfd)
int oldfd;
int newfd;
{
    if (oldfd != newfd) {
        close(oldfd);
        dup(newfd);
        close(newfd);
    }
}

backv(name, in, out, argv)
char	*name;
FILE	*in;
FILE	*out;
char	**argv;
{
    int pid;

    ui_fflush(stdout);
    if ((pid = fork())) {
        fswap(0, fileno(in));
        fswap(1, fileno(out));
        execvp(name, argv);
        _exit(BADEXEC);
    }
    return pid;
}

void
loc_edit(sync, ed, lineno, src)
    char    *ed, *src;
{
    char    lineCmd[10];

    if (streq(ed, "vi") || streq(ed, "ex") || streq(ed, "emacs")) {
        sprintf(lineCmd, "+%d", lineno);
        loc_call(sync, stdin, stdout, ed, lineCmd, src, 0);
    } else {
        loc_call(sync, stdin, stdout, ed, src, 0);
    }
}


#ifndef	STARTUP_FILE
#define	STARTUP_FILE	".gdbinit"
#endif

int
dflt_initialize(argc, argv)
int	*argc;
char	**argv;
{
	char	*initial_script = STARTUP_FILE;
	char	*home;
	char	filnambuf[1024];
        int	argcount = *argc;

	argcount--; argv++;
	while (argcount > 0 && argv[1] != (char *)0) {
		if (argv[1][0] == '-' && streq(argv[1], "-startupFile")) {
			if (argcount > 2 ) {
				argcount--; argv++;
				initial_script = argv[1];
			} else {
				ui_badnews(0,
					"gdb: no file name given for -startupFile flag");
			}
		}
		argcount--; argv++;
	}

	/*
	 * try to open the startup file
	 */

#ifdef NOTSURE
	inputfile = fopen(initial_script, "r");
	if (inputfile == NULL) {
		home = (char *)getenv("HOME");
		if (home != NULL) {
			sprintf(filnambuf, "%s/%s", home, initial_script);
			inputfile = fopen(filnambuf, "r");
		}
	}
	if (inputfile == NULL) {
		inputfile = stdin;
	}
#endif
	return 0;
}


void
dflt_badnews(n, s, a, b, c, d, e, f, g, h)
int	n;
char	*s;
{
	switch (n) {
        case -1:
		terminal_ours();
		ui_fflush(stdout);
		ui_fprintf(stdout, s, a, b, c, d, e, f, g, h);
                ui_fprintf(stdout,"\n");
		ui_fflush(stdout);
		return_to_top_level();
	case 0:
		ui_fprintf(stdout, s, a, b, c, d, e, f, g, h);
                ui_fprintf(stdout,"\n");
		ui_fflush(stdout);
		break;
	default:
		ui_fprintf(stdout, s, a, b, c, d, e, f, g, h);
		ui_fprintf(stdout, "exiting: return code=%d\n", n);
		exit(n);
	}
}

#ifdef	TEK4107
static	doing_prompt = 0;
static	char	*dbg_name = "";
int	child_pid;

void
tek4107_strtSubProc(pid)
{
	if (pid == 0) {
		return;
	}
	child_pid = pid;
	printf("\0337");    /* save cursor */
	printf("\033[?6l"); /* set origin mode absolute */
	printf("\033[1;9H");   /* move cursor to row 1 column 9 */
	printf("\033[=4;>0m"); /* set dialog indexes */
	printf("pid: 0x%08x", child_pid);
	printf("\0338");    /* restore cursor */
	printf("\033[=0;>0m"); /* set dialog indexes */
	fflush(stdout);
}

void
tek4107_endSubProc()
{
	child_pid = pid;
	printf("\0337");    /* save cursor */
	printf("\033[?6l"); /* set origin mode absolute */
	printf("\033[1;9H");   /* move cursor to row 1 column 9 */
	printf("\033[=4;>0m"); /* set dialog indexes */
	printf("pid: terminated");
	printf("\0338");    /* restore cursor */
	printf("\033[=0;>0m"); /* set dialog indexes */
	fflush(stdout);
}

void
tek4107_getClientInfo()
{
}

void
tek4107_doButtons()
{
}

void
tek4107_byebye()
{
	printf("\033[r");   /* reset top margin */
	printf("\033[2J");  /* erase screen */
	fflush(stdout);
}

void
tek4107_tickleMe(infop, docmd)
char	*infop;
char	*docmd;
{
	char	prompt[1024], *cp;
	char	reply[1024];

	printf("\0337");    /* save cursor */
	printf("\033[?6l"); /* set origin mode absolute */
	printf("\033[1;25H");   /* move cursor to row 1 column 25 */
	printf("\033[=4;>0m"); /* set dialog indexes */
	sprintf(prompt, "tickled");
	printf("%s", prompt);
	fflush(stdout);
	(void) gets(reply);
	printf("\033[1;25H");   /* move cursor to row 1 column 25 */
	for (cp=prompt; *cp; cp++) {
		putchar(' ');
	}
	printf("\0338");    /* restore cursor */
	printf("\033[=0;>0m"); /* set dialog indexes */
	fflush(stdout);
}

void
tek4107_procflush()
{
}

void
tek4107_badnews()
{
}

void
tek4107_newprompt()
{
	doing_prompt = 1;
	printf("\033[1m[%s]", dbg_name);
}

int
tek4107_initialize(argc, argv)
int	*argc;
char	**argv;
{
	char	*envptr;
        int	argcount = *argc;

	envptr = (char *) getenv("TERM");
	if ((envptr == NULL) || (! streq(envptr,"4107"))) {
		return 1;
	}
	if (argcount > 0) {
		dbg_name = *argv;
	}
	printf("\033[5;r"); /* set top margin */
	printf("\033[2J");  /* erase screen */
	printf("\033[=4;>4m"); /* set dialog indexes */
	printf("\033[?6l"); /* set origin mode absolute */
	printf("\033[H");   /* move cursor to row 1 column 1 */
	printf("\033[2K\n");  /* erase the line; go to next */
	printf("\033[2K\n");  /* erase the line; go to next */
	printf("\033[2K\n");  /* erase the line; go to next */
	printf("\033[2K\n");  /* erase the line; go to next */
	printf("\033[=0;>0m"); /* reset dialog indexes */
	fflush(stdout); 
	return 0;
}

int
tek4107_fflush(file)
FILE	*file;
{
	if ((file == stdout) && doing_prompt) {
		printf("\033[0m");	/* reset rendition */
		doing_prompt = 0;
	}
	fflush(file);
}

tek4107_call()
{
}

int
tek4107_wait(status_p)
int	*status_p;
{
	int	pid;

	printf("\0337");    /* save cursor */
	printf("\033[?6l"); /* set origin mode absolute */
	printf("\033[H");   /* move cursor to row 1 column 1 */
	printf("\033[=3;>0m"); /* set dialog indexes */
	printf("running");
	printf("\0338");    /* restore cursor */
	printf("\033[=0;>0m"); /* reset dialog indexes */
	fflush(stdout);
	pid = wait(status_p);
	if (pid != 0) {
		printf("\0337");    /* save cursor */
		printf("\033[?6l"); /* set origin mode absolute */
		printf("\033[H");   /* move cursor to row 1 column 1 */
		printf("\033[=2;>0m"); /* set dialog indexes */
		printf("stoped ");
		printf("\0338");    /* restore cursor */
		printf("\033[=0;>0m"); /* reset dialog indexes */
		fflush(stdout);
	}
	return pid;
}
#endif
