/* Top level for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/main.c,v 1.70 91/01/01 21:18:18 robertb Exp $
   $Locker:  $

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

#if	defined (DG_HACK) && defined (USG)
/* changed name of getwd to getwoid to avoid syscall confilicts -- jfs */
#endif /* not DG_HACK or not USG */
#ifdef BSD
#define getwoid getwd
#endif

#include "defs.h"
#include "command.h"
#include "param.h"
#include "ui.h"

#ifdef USG
#include <sys/types.h>
#include <unistd.h>
#endif

#include <sys/file.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef SET_STACK_LIMIT_HUGE
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>

int original_stack_limit;
#endif

/* If this definition isn't overridden by the header files, assume
   that isatty and fileno exist on this system.  */
#ifndef ISATTY
#define ISATTY(FP)	(isatty (fileno (FP)))
#endif

/* Version number of GDB, as a string.  */

extern char *version;

/*
 * Declare all cmd_list_element's
 */

/* Chain containing all defined commands.  */

struct cmd_list_element *cmdlist;

/* Chain containing all defined info subcommands.  */

struct cmd_list_element *infolist;
struct cmd_list_element *info_cmd;

/* Chain containing all defined enable subcommands. */

struct cmd_list_element *enablelist;
struct cmd_list_element *enable_cmd;

/* Chain containing all defined disable subcommands. */

struct cmd_list_element *disablelist;
struct cmd_list_element *disable_cmd;

/* Chain containing all defined delete subcommands. */

struct cmd_list_element *deletelist;
struct cmd_list_element *gdbdelete_cmd;

/* Chain containing all defined "enable breakpoint" subcommands. */

struct cmd_list_element *enablebreaklist;
struct cmd_list_element *enablebreak_cmd;

/* Chain containing all defined set subcommands */

struct cmd_list_element *setlist;
struct cmd_list_element *set_cmd;

/* Chain containing all defined \"set history\".  */

struct cmd_list_element *sethistlist;
struct cmd_list_element *sethist_cmd;

/* Chain containing all defined \"unset history\".  */

struct cmd_list_element *unsethistlist;
struct cmd_list_element *unsethist_cmd;

/* stdio stream that command input is being read from.  */

FILE *instream;

/* Current working directory.  */

char *current_directory;

/* The directory name is actually stored here (usually).  */
static char dirbuf[MAXPATHLEN];

/* Stack of currently executing source files. */
struct source_stack_structure
{
  struct source_stack_structure *next;
  FILE *previous_instream;
  char file[1];		/* is actually longer than this */
} *source_stack = 0;

/* The number of lines on a page, and the number of spaces
   in a line.  */
int linesize, pagesize;

#ifdef TEK_HACK
/* Nonzero if we should repeat the previous command on empty input line */

int autorepeat = 1;

/* Pointer to 'echo' command; checked in execute_command */
static struct cmd_list_element *echo_p;
#endif /* TEK_HACK */

/* Nonzero if we should refrain from using an X window.  */

int inhibit_windows = 0;

#ifdef TEK_HACK
/* One if using the X user interface. DL 8/2/89 */
int usingX = 0;
#endif /* TEK_HACK */

/* Function to call before reading a command, if nonzero.
   The function receives two args: an input stream,
   and a prompt string.  */
   
void (*window_hook) ();

/* Last signal that the inferior received (why it stopped).  */
extern int stop_signal;

extern int frame_file_full_name;
int xgdb_verbose;

void free_command_lines ();
char *gdb_readline ();
char *command_line_input ();
static void initialize_main ();
static void initialize_cmd_lists ();
void command_loop ();
static void source_command ();
static void quit_command ();
static void print_gdb_version ();
static void float_handler ();
static void cd_command ();

extern int errno;
char *getenv ();

/* gdb prints this when reading a command interactively */
#ifdef TEK_PROG_HACK
char *gbl_prompt;

extern char *macro_expand ();
extern struct cmd_list_element *my_lookup_cmd();
extern void free_macro_expansion ();
extern struct command_line *read_command_lines ();
extern struct cleanup *push_match_expansions ();
extern struct cleanup *push_def_expansions ();
extern void nested_define_command ();
extern void nested_document_command ();
extern void nested_while_command ();

extern char *index();

#else /* not TEK_PROG_HACK */
static char *prompt;
#endif /* not TEK_PROG_HACK */

/* Buffer used for reading command lines, and the size
   allocated for it so far.  */

char *line;
int linesize;


/* Signal to catch ^Z typed while reading a command: SIGTSTP or SIGCONT.  */

#ifndef STOP_SIGNAL
#ifdef SIGTSTP
#define STOP_SIGNAL SIGTSTP
#endif
#endif

/* This is how `error' returns to command level.  */

jmp_buf to_top_level;

void
return_to_top_level ()
{
  quit_flag = 0;
  immediate_quit = 0;
  clear_breakpoint_commands ();
  clear_momentary_breakpoints ();
  disable_current_display ();
  do_cleanups (0);
#ifdef TEK_PROG_HACK
  clear_cond_stack ();
  update_prompt (2);
#endif /* TEK_PROG_HACK */
  longjmp (to_top_level, 1);
}

/* Call FUNC with arg ARG, catching any errors.
   If there is no error, return the value returned by FUNC.
   If there is an error, return zero after printing ERRSTRING
    (which is in addition to the specific error message already printed).  */

int
catch_errors (func, arg, errstring)
     int (*func) ();
     int arg;
     char *errstring;
{
  jmp_buf saved;
  int val;
  struct cleanup *saved_cleanup_chain;

  saved_cleanup_chain = save_cleanups ();

  bcopy (to_top_level, saved, sizeof (jmp_buf));

  if (setjmp (to_top_level) == 0)
    val = (*func) (arg);
  else
    {
      ui_fprintf (stderr, "%s\n", errstring);
      val = 0;
    }

  restore_cleanups (saved_cleanup_chain);

  bcopy (saved, to_top_level, sizeof (jmp_buf));
  return val;
}

/* Handler for SIGHUP.  */

static void
disconnect ()
{
  kill_inferior_fast ();
  signal (SIGHUP, SIG_DFL);
  kill (getpid (), SIGHUP);
}

/* Clean up on error during a "source" command (or execution of a
   user-defined command).
   Close the file opened by the command
   and restore the previous input stream.  */

/* Clean up on error during a "source" command (or execution of a
   user-defined command).
   Close the file opened by the command
   and restore the previous input stream.  */

#ifndef TEK_PROG_HACK
static
#endif /* not TEK_PROG_HACK */
void
source_cleanup (stream)
     FILE *stream;
{
  /* Instream may be 0; set to it when executing user-defined command. */
  if (instream)
    fclose (instream);
  instream = stream;
}

static void
source_stack_cleanup (s)
	struct source_stack_structure *s;
{
  /* Discard source_stack_structures until we get to where we want to be. */
  while (s != source_stack)
    {
      register struct source_stack_structure *t = source_stack->next;

      source_cleanup(source_stack->previous_instream);
      free(source_stack);
      source_stack = t;
    }
}

int
main (argc, argv, envp)
     int argc;
     char **argv;
     char **envp;
{
  extern void request_quit ();
  int count;
  int inhibit_gdbinit = 0;
  int quiet = 0;
  int batch = 0;
  register int i;
  char *p;
  extern char *rindex();
#ifdef TEK_HACK
  int emulate_dbx = 0;
  char *source_command_file = NULL;
#endif /* TEK_HACK */

  quit_flag = 0;
  linesize = 100;
  line = (char *) xmalloc (linesize);
  instream = stdin;

  getwoid (dirbuf);
  current_directory = dirbuf;

#ifdef SET_STACK_LIMIT_HUGE
  {
    struct rlimit rlim;

    /* Set the stack limit huge so that alloca (particularly stringtab
     * in dbxread.c) does not fail. */
    getrlimit (RLIMIT_STACK, &rlim);
    original_stack_limit = rlim.rlim_cur;
    rlim.rlim_cur = rlim.rlim_max;
    setrlimit (RLIMIT_STACK, &rlim);
  }
#endif /* SET_STACK_LIMIT_HUGE */

  /* Look for flag arguments.  */

#ifdef TEK_HACK
	/*
	 * Process command line for user interface options
         */

  p = rindex(argv[0], '/');
  if (!p) p = argv[0];
  while (*p == '.' || *p == '/') p++;
  if ((*p == 'x')
  || (*p == 'X')) {
          usingX = 1;
  }
#endif /* TEK_HACK */

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-q") || !strcmp (argv[i], "-quiet"))
	quiet = 1;
      else if (!strcmp (argv[i], "-nx"))
	inhibit_gdbinit = 1;
#ifdef TEK_HACK
      else if (!strcmp (argv[i], "-na"))
	autorepeat = 0;
      else if (!strcmp (argv[i], "-dbx"))
	emulate_dbx = 1;
#endif /* TEK_HACK */
      else if (!strcmp (argv[i], "-nw"))
	inhibit_windows = 1;
      else if (!strcmp (argv[i], "-batch"))
	batch = 1, quiet = 1, usingX = 0;
      else if (!strcmp (argv[i], "-fullname"))
	frame_file_full_name = 1;
      else if (!strcmp (argv[i], "-xgdb_verbose"))
	xgdb_verbose = 1;
#ifdef TEK_HACK
      else if ((!strcmp (argv[i], "-c")) && usingX) {
   /* Workaround for the fact that the X toolkit intrinsics will see '-c' as
    * an abbreviation for '-color' and try to use it.  So we'll simply replace
    * '-c' with '-core' before the toolkit knows about it.
    */
        argv[i] = savestring("-core", 5);
        i++;
      }
#endif /* TEK_HACK */
      else if (argv[i][0] == '-')
	i++;
    }

  /* Run the init function of each source file */

  initialize_cmd_lists ();	/* This needs to be done first */
  initialize_all_files ();
  initialize_main ();		/* But that omits this file!  Do it now */
  initUsrInt (usingX, &argc, argv);

  signal (SIGINT, request_quit);
  signal (SIGQUIT, SIG_IGN);
  if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
    signal (SIGHUP, disconnect);
  signal (SIGFPE, float_handler);

  if (!quiet)
    print_gdb_version ();


  /* Process the command line arguments.  */

  count = 0;
  for (i = 1; i < argc; i++)
    {
      register char *arg = argv[i];
      /* Args starting with - say what to do with the following arg
	 as a filename.  */
      if (arg[0] == '-')
	{
	  /*void exec_file_command (), symbol_file_command ();
	  void core_file_command (), directory_command ();
	  void tty_command ();
*/

	  if (!strcmp (arg, "-q") || !strcmp (arg, "-nx")
#ifdef TEK_HACK
              || !strcmp (arg, "-na") || !strcmp (arg, "-dbx")
#endif /* TEK_HACK */
	      || !strcmp (arg, "-quiet") || !strcmp (arg, "-batch")
	      || !strcmp (arg, "-fullname") || !strcmp (arg, "-nw")
	      || !strcmp (arg, "-xgdb_verbose"))
	    /* Already processed above */
	    continue;

	  if (++i == argc)
	    ui_fprintf (stderr, "No argument follows \"%s\".\n", arg);
	  if (!setjmp (to_top_level))
	    {
	      /* -s foo: get syms from foo.  -e foo: execute foo.
		 -se foo: do both with foo.  -c foo: use foo as core dump.  */
	      if (!strcmp (arg, "-se"))
		{
		  exec_file_command (argv[i], !batch);
		  symbol_file_command (argv[i], !batch);
		}
	      else if (!strcmp (arg, "-s") || !strcmp (arg, "-symbols"))
		symbol_file_command (argv[i], !batch);
	      else if (!strcmp (arg, "-e") || !strcmp (arg, "-exec"))
		exec_file_command (argv[i], !batch);
	      else if (!strcmp (arg, "-c") || !strcmp (arg, "-core"))
		core_file_command (argv[i], !batch);
	      /* -x foo: execute commands from foo.  */
	      else if (!strcmp (arg, "-x") || !strcmp (arg, "-command")
		       || !strcmp (arg, "-commands"))
#ifdef TEK_HACK
  /* We may want to read in dbx emulation file before anything else */
                source_command_file = savestring(argv[i], strlen(argv[i]));
#else
		source_command (argv[i]);
#endif /* TEK_HACK */
	      /* -d foo: add directory `foo' to source-file directory
		         search-list */
	      else if (!strcmp (arg, "-d") || !strcmp (arg, "-dir")
		       || !strcmp (arg, "-directory"))
		directory_command (argv[i], 0);
	      /* -cd FOO: specify current directory as FOO.
		 GDB remembers the precise string FOO as the dirname.  */
	      else if (!strcmp (arg, "-cd"))
		{
		  cd_command (argv[i], 0);
		  init_source_path ();
		}
	      /* -t /def/ttyp1: use /dev/ttyp1 for inferior I/O.  */
	      else if (!strcmp (arg, "-t") || !strcmp (arg, "-tty"))
		tty_command (argv[i], 0);
	      else
		ui_badnews(-1,"Unknown command-line switch: \"%s\"\n", arg);
	    }
	}
      else
	{
	  /* Args not thus accounted for
	     are treated as, first, the symbol/executable file
	     and, second, the core dump file.  */
	  count++;
	  if (!setjmp (to_top_level))
	    switch (count)
	      {
	      case 1:
		exec_file_command (arg, !batch);
		symbol_file_command (arg, !batch);
		break;

	      case 2:
		core_file_command (arg, !batch);
		break;

	      case 3:
		ui_fprintf (stderr, "Excess command line args ignored. (%s%s)\n",
			 arg, (i == argc - 1) ? "" : " ...");
	      }
	}
    } /* end 'for' */
  {
    struct stat homebuf, cwdbuf;
    char *homedir, *homeinit;

    /* Read init file, if it exists in home directory  */
    homedir = getenv ("HOME");

#ifdef TEK_HACK
  /* Now we read all the command files in.  The order is:
   *    1. dbx emulation files, if desired (-dbx)
   *    2. any files specified by name (-x)
   *    3. gdbinit file, unless not present or inhibited (-nx)
   */
	homeinit = (char *) alloca (strlen (getenv ("HOME")) + 20);
        if (emulate_dbx) {
           sprintf(homeinit, "%s/gdb-dbx_script", getenv ("HOME"));
           if (access(homeinit, R_OK) == 0) {
		emulate_dbx = 0;
		ui_fprintf(stdout,"Reading %s for dbx emulation script.\n",
			homeinit);
		if (!setjmp (to_top_level))
		    source_command(homeinit);
           }
  	   else if (access("./gdb-dbx_script", R_OK) == 0) {
   		ui_fprintf(stdout,"Reading ./gdb-dbx_script for dbx emulation script.\n");
		if (!setjmp (to_top_level))
   		    source_command ("./gdb-dbx_script");
           }
           else if (access("/usr/local/bin/gnu/gdb-dbx_script", R_OK) == 0) {
   		ui_fprintf(stdout,"Reading /usr/local/bin/gnu/gdb-dbx_script for dbx emulation script.\n");
		if (!setjmp (to_top_level))
   		    source_command ("/usr/local/bin/gnu/gdb-dbx_script");
           }
           else if (access("/usr/local/bin/gdb-dbx_script", R_OK) == 0) {
   	      ui_fprintf(stdout,"Reading /usr/local/bin/gdb-dbx_script for dbx emulation script.\n");
		if (!setjmp (to_top_level))
   		    source_command ("/usr/local/bin/gdb-dbx_script");
           }
       	   else if (access("/usr/local/gdb-dbx_script", R_OK) == 0) {
   	ui_fprintf(stdout,"Reading /usr/local/gdb-dbx_script for dbx emulation script.\n");
		if (!setjmp (to_top_level))
   		    source_command ("/usr/local/gdb-dbx_script");
        }
        else ui_fprintf(stderr, "Can't find gdb-dbx_script to use in setting up dbx emulation.\n");
     }
/* Now read -x file, if any */
     if (source_command_file) {
	if (!setjmp (to_top_level))
	    source_command(source_command_file);
        free(source_command_file);
     }
/* Finally, read .gdbinit file if not inhibitied */
#endif /* TEK_HACK */
    if (homedir)
      {
#ifndef TEK_HACK
	homeinit = (char *) alloca (strlen (getenv ("HOME")) + 20);
#endif
	strcpy (homeinit, getenv ("HOME"));
	strcat (homeinit, "/.gdbinit");
	if (!inhibit_gdbinit && access (homeinit, R_OK) == 0)
	  if (!setjmp (to_top_level))
	    source_command (homeinit);

	/* Do stats; no need to do them elsewhere since we'll only
	   need them if homedir is set.  Make sure that they are
	   zero in case one of them fails (guarantees that they
	   won't match if either exits).  */
	
	bzero (&homebuf, sizeof (struct stat));
	bzero (&cwdbuf, sizeof (struct stat));
	
	stat (homeinit, &homebuf);
	stat ("./.gdbinit", &cwdbuf); /* We'll only need this if
					 homedir was set.  */
    
    /* Read the input file in the current directory, *if* it isn't
       the same file (it should exist, also).  */

    if (!homedir
	|| bcmp ((char *) &homebuf,
		 (char *) &cwdbuf,
		 sizeof (struct stat)))
      if (!inhibit_gdbinit && access (".gdbinit", R_OK) == 0)
	if (!setjmp (to_top_level))
	  source_command (".gdbinit");
  }


  if (batch)
    ui_badnews(1,"Attempt to read commands from stdin in batch mode.");

  if (!quiet) {
    ui_fprintf(stdout, "Type \"help\" for a list of commands.\n");
    ui_fflush(stdout);
  }
  ui_doneinit();
 }

  /* The command loop.  */

  while (1)
    {
      if (!setjmp (to_top_level))
	{
	  command_loop ();
        }
      clearerr (stdin);		/* Don't get hung if C-d is typed.  */
    }
}


/* Execute the line P as a command.
   Pass FROM_TTY as second argument to the defining function.  */

void
#ifdef TEK_PROG_HACK
execute_command (c, p, from_tty)
     register struct cmd_list_element *c;
#else /* not TEK_PROG_HACK */
execute_command (p, from_tty)
#endif /* not TEK_PROG_HACK */
     char *p;
     int from_tty;
{
#ifndef TEK_PROG_HACK
  register struct cmd_list_element *c;
#endif /* not TEK_PROG_HACK */
  register struct command_line *cmdlines;

  free_all_values ();
  while (*p == ' ' || *p == '\t') p++;
#ifdef TEK_PROG_HACK
  if ((! c) && *p)
      c = my_lookup_cmd (&p, cmdlist, "", 0, 1);
  if (c)
#else /* not TEK_PROG_HACK */
  if (*p)
#endif /* not TEK_PROG_HACK */
    {
#ifndef TEK_PROG_HACK
      c = lookup_cmd (&p, cmdlist, "", 0, 1);

/* Added to insure no reference through null pointer.  DL 8/1/89 */
      if (c == NULL) return;
#endif /* not TEK_PROG_HACK */
#ifdef TEK_HACK 
/* Allow commands separated by a colon */
      if (*p && c != echo_p) {
	char *semicolon = index(p, ';');

        if (semicolon) {
            char *newp = savestring(p, strlen(p));
            char *ldquote = index(newp, '"');
	    char *newcmd;
            char *rdquote;
/* If the semicolon is within double quotes, it's part of a macro 
 * definition or some other quoted string; ignore it.
 */
            semicolon = index(newp, ';');
            newcmd = semicolon + 1;
            if (ldquote) {
                if (*(ldquote + 1))
		  rdquote = index(ldquote + 1, '"');
            }
	    if ((!ldquote) || semicolon < ldquote || semicolon > rdquote) {
		*semicolon = '\0';
    		while (*newcmd && (*newcmd == ' ' || 
		    *newcmd == '\t')) newcmd++;
    		if (*newcmd) {
			execute_command(c, newp, from_tty);
			execute_command(0, newcmd, from_tty);
                        free(newp);
			return;
	    	}
	   }
           free(newp);
        }
      }
#endif /* TEK_HACK */ 

      if (c->function == 0)
	ui_badnews(-1,"That is not a command, just a help topic.");
      else if (c->class == (int) class_user)
	{
	  struct cleanup *old_chain;
          char *tem;
	  
#ifndef TEK_PROG_HACK
	  if (*p)
	    ui_badnews(-1,"User-defined commands cannot take arguments (yet).");
#endif /* not TEK_PROG_HACK */
	  cmdlines = (struct command_line *) c->function;
	  if (cmdlines == (struct command_line *) 0)
	    /* Null command */
	    return;

	  /* Set the instream to 0, indicating execution of a
	     user-defined function.  */
#ifdef TEK_PROG_HACK

          if (skip_execute (c))
            return;
          tem = macro_expand (p, c);
          (void) make_cleanup (free_macro_expansion, tem);
	  if (cmdlines->cmd == (struct cmd_list_element *) INVALID_CORE_ADDR)
	    {
	      if (cmdlines->next == (struct command_line *)0)
		/* Null command (with delimiters!?) */
		return;
	      old_chain = push_def_expansions (c->name, tem, cmdlines->line);
	      cmdlines = cmdlines->next;
	    }
	  else
	    old_chain = push_def_expansions (c->name, tem, 0);
          (void) push_match_expansions ();
	  (void) make_cleanup (source_cleanup, instream);
#else /* not TEK_PROG_HACK */
	  old_chain =  make_cleanup (source_cleanup, instream);
#endif /* not TEK_PROG_HACK */
	  instream = (FILE *) 0;
	  while (cmdlines)
	    {
#ifdef TEK_PROG_HACK
	      execute_command (cmdlines->cmd, cmdlines->line, 0);
#else /* not TEK_PROG_HACK */
	      execute_command (cmdlines->line, 0);
#endif /* not TEK_PROG_HACK */
	      cmdlines = cmdlines->next;
	    }
	  do_cleanups (old_chain);
	}
#ifdef TEK_PROG_HACK
      else if (skip_execute (c))
        return;
      else if (c->class == (int) class_nested)
        (*c->function) (p, from_tty);
      else
        {
          char *tem;
          tem = macro_expand (p, c);
          (void) make_cleanup (free_macro_expansion, tem);
          (*c->function) (*tem ? tem : 0, from_tty);
        }
#else /* not TEK_PROG_HACK */
      else 
        /* Pass null arg rather than an empty one.  */
        (*c->function) (*p ? p : 0, from_tty);
#endif /* not TEK_PROG_HACK */
    }
}

static void
do_nothing ()
{
}

/* Read commands from `instream' and execute them
   until end of file.  */
void
command_loop ()
{
  register char *p;

  struct cleanup *old_chain;
  while (!feof (instream))
    {
#ifdef TEK_PROG_HACK /* -rcb 6/90 */
      if (window_hook && instream == stdin)
	(*window_hook) (instream, gbl_prompt);
#endif

      quit_flag = 0;
      if (instream == stdin && ISATTY (stdin))
	reinitialize_more_filter ();
      old_chain = make_cleanup (do_nothing, 0);
#ifdef TEK_PROG_HACK /* -rcb 6/90 */
      p = command_line_input (instream == stdin ? gbl_prompt : 0,
#else
      p = command_line_input (instream == stdin ? prompt : 0,
#endif
			      instream == stdin);

      if (p == (char *)EOF)
	{
	  if (instream != stdin)
	    return;
	  if (!varvalue("ignoreeof")) {
            quit_command();
          }
	  continue;
	}
      execute_command (0, p, instream == stdin);
      /* Do any commands attached to breakpoint we stopped at.  */
      
      do_breakpoint_commands ();

      do_cleanups (old_chain);
    }
}

/* Commands call this if they do not want to be repeated by null lines.  */

void
dont_repeat ()
{
  /* If we aren't reading from standard input, we are saving the last
     thing read from stdin in line and don't want to delete it.  Null lines
     won't repeat here in any case.  */
  if (instream == stdin)
    *line = 0;
}

#ifdef STOP_SIGNAL
static void
stop_sig ()
{
#if STOP_SIGNAL == SIGTSTP
  signal (SIGTSTP, SIG_DFL);
#ifdef	TEK_HACK
#ifdef	BSD
#ifndef	BSD4_1
  sigsetmask (0);
#endif
#endif
#endif	/* TEK_HACK */
  kill (getpid (), SIGTSTP);
  signal (SIGTSTP, stop_sig);
#else
  signal (STOP_SIGNAL, stop_sig);
#endif
  ui_newprompt();
#ifdef TEK_PROG_HACK
  ui_fprintf(stdout, "%s", gbl_prompt);
#else /* not TEK_PROG_HACK */
  ui_fprintf(stdout, "%s", prompt);
#endif /* not TEK_PROG_HACK */
  ui_fflush (stdout);

  /* Forget about any previous command -- null line now will do nothing.  */
  dont_repeat ();
}
#endif /* STOP_SIGNAL */

/* This routine reads a line from the stream "instream" via fgetc.
   If RETURN_RESULT is set it allocates
   space for whatever the user types and returns the result.
   Return value is (char *)EOF if end-of-file (control-D). */
/* Revised to use ui_fgets to accommodate X interface DL 8/1/89 */
char *
gdb_readline (prompt, return_result)
     char *prompt;
     int return_result;
{
  char *result;
  int result_size;

  if (prompt)
    {
      ui_fflush (stdout);
      ui_newprompt();
      ui_fprintf(stdout, "%s", prompt);
      ui_fflush (stdout);
    }

  if (instream == stdin)
    {
      char *mark;

      ui_tickleMe();
  
      result_size = 320;
      result = (char *) xmalloc (result_size);
 
      /* Loop until we do a read with no I/O error.
	 We'll get an error each time the user types ^Z, fg. */
      for (;;)
	{
	  errno = 0;
	  if (ui_fgets(result, result_size, instream))
	    break;

	  /* If EOF, return EOF.  Otherwise go around the loop again. */
	  if (errno==0)
	    {
	      free (result);
	      ui_putc('\n', stdout);
	      return (char *) EOF;
	    }
	}

      if (return_result)
        {
          mark = index(result, '\n');
          if (mark) *mark = '\0';
          else result[result_size - 1] = '\0';
          return result;
        }
      else
        {
          free (result);
          return (char *) 0;
        }
    }
  else
    {
      int c;
      int input_index = 0;

      result_size = 80;

      if (return_result)
        result = (char *) xmalloc (result_size);

      while (1)
        {
          c = fgetc (instream);
          if (c == EOF || c == '\n')
            break;
          if (return_result)
            {
              result[input_index++] = c;
              while (input_index >= result_size)
                {
                  result_size *= 2;
                  result = (char *) xrealloc (result, result_size);
                }
            }
        }
      if (c == EOF)
	{
	  free (result);
	  return (char *) EOF;
	}
      else if (return_result)
        {
          result[input_index++] = '\0';
          return result;
        }
      else
	{
	  free (result);
          return (char *) 0;
	}
    }
}

/* Declaration for fancy readline with command line editing.  */
char *readline ();

/* Variables which control command line editing and history
   substitution.  These variables are given default values at the end
   of this file.  */

/* This is always off for now if we're using the X user interface. DL 8/2/89 */
static int command_editing_p;
static int history_expansion_p;
static int write_history_p;
static int history_size;
static char *history_filename;

/* Variables which are necessary for fancy command line editing.  */
char *gdb_completer_word_break_characters =
  " \t\n!@#$%^&*()-+=|~`}{[]\"';:?/>.<,";

/* Functions that are used as part of the fancy command line editing.  */

/* Generate symbol names one by one for the completer.  If STATE is
   zero, then we need to initialize, otherwise the initialization has
   already taken place.  TEXT is what we expect the symbol to start
   with.  RL_LINE_BUFFER is available to be looked at; it contains the
   entire text of the line.  RL_POINT is the offset in that line of
   the cursor.  You should pretend that the line ends at RL_POINT.  */
char *
symbol_completion_function (text, state)
     char *text;
     int state;
{
  char **make_symbol_completion_list ();
  static char **list = (char **)NULL;
  static int index;
  char *output;
  extern char *rl_line_buffer;
  extern int rl_point;
  char *tmp_command, *p;
  struct cmd_list_element *c, *result_list;

  if (!state)
    {
      /* Free the storage used by LIST, but not by the strings inside.  This is
	 because rl_complete_internal () frees the strings. */
      if (list)
	free (list);
      list = 0;
      index = 0;

      /* Decide whether to complete on a list of gdb commands or on
	 symbols.  */
      tmp_command = (char *) alloca (rl_point + 1);
      p = tmp_command;
      
      strncpy (tmp_command, rl_line_buffer, rl_point);
      tmp_command[rl_point] = '\0';
      
      c = lookup_cmd_1 (&p, cmdlist, &result_list, 1);

      /* Move p up to the next interesting thing.  */
      while (*p == ' ' || *p == '\t')
	p++;

      if (!c)
	/* He's typed something unrecognizable.  Sigh.  */
	list = (char **) 0;
      else if (c == (struct cmd_list_element *) -1)
	{
	  if (p + strlen(text) != tmp_command + rl_point)
	    ui_badnews(-1,"Unrecognized command.");
	  
	  /* He's typed something ambiguous.  This is easier.  */
	  if (result_list)
	    list = complete_on_cmdlist (result_list, text);
	  else
	    list = complete_on_cmdlist (cmdlist, text);
	}
      else
	{
	  /* If we've gotten this far, gdb has recognized a full
	     command.  There are several possibilities:

	     1) We need to complete on the command.
	     2) We need to complete on the possibilities coming after
	     the command.
	     2) We need to complete the text of what comes after the
	     command.   */

	  if (!*p && *text)
	    /* Always (might be longer versions of thie command).  */
	    list = complete_on_cmdlist (result_list, text);
	  else if (!*p && !*text)
	    {
	      if (c->prefixlist)
		list = complete_on_cmdlist (*c->prefixlist, "");
	      else
		list = make_symbol_completion_list ("");
	    }
	  else
	    {
	      if (c->prefixlist && !c->allow_unknown)
		{
		  *p = '\0';
		  ui_badnews(-1,"\"%s\" command requires a subcommand.",
			 tmp_command);
		}
	      else
		list = make_symbol_completion_list (text);
	    }
	}
    }

  /* If the debugged program wasn't compiled with symbols, or if we're
     clearly completing on a command and no command matches, return
     NULL.  */
  if (!list)
    return ((char *)NULL);

  output = list[index];
  if (output)
    index++;

  return (output);
}

/* Read one line from the command input stream `instream'
   into the local static buffer `linebuffer' (whose current length
   is `linelength').
   The buffer is made bigger as necessary.
   Returns the address of the start of the line.
   If end-of-file (control-D or true file EOF) occurs, returns (char *)EOF.

   *If* the instream == stdin & stdin is a terminal, the line read
   is copied into the file line saver (global var char *line,
   length linesize) so that it can be duplicated.

   This routine either uses fancy command line editing or
   simple input as the user has requested.  */

char *
command_line_input (prompt, repeat)
     char *prompt;
     int repeat;
{
  static char *linebuffer = 0;
  static int linelength = 0;
  register char *p;
  register char *p1, *rl;
  char *local_prompt = prompt;
  register int c;
  char *nline;

  if (linebuffer == 0)
    {
      linelength = 80;
      linebuffer = (char *) xmalloc (linelength);
    }

  p = linebuffer;

  /* Control-C quits instantly if typed while in this loop
     since it should not wait until the user types a newline.  */
  immediate_quit++;
#ifdef STOP_SIGNAL
  signal (STOP_SIGNAL, stop_sig);
#endif

  while (1)
    {
      /* Don't use fancy stuff if not talking to stdin.  */
      if (command_editing_p && instream == stdin
	  && ISATTY (instream))
	rl = readline (local_prompt);
      else
	rl = gdb_readline (local_prompt, 1);

      /* On error or EOF, set the line buffer to a null string (so
	 carriage return has nothing to repeat) and return EOF. */
      if (!rl || rl == (char *) EOF)
	{
	  linebuffer[0] = '\0';
	  return (char *)EOF;
	}
      if (strlen(rl) + 1 + (p - linebuffer) > linelength)
	{
	  linelength = strlen(rl) + 1 + (p - linebuffer);
	  nline = (char *) xrealloc (linebuffer, linelength);
	  p += nline - linebuffer;
	  linebuffer = nline;
	}
      p1 = rl;
      /* Copy line.  Don't copy null at end.  (Leaves line alone
         if this was just a newline)  */
      while (*p1)
	*p++ = *p1++;

      free (rl);			/* Allocated in readline.  */

      if (p == linebuffer || *(p - 1) != '\\')
	break;

      p--;			/* Put on top of '\'.  */
      local_prompt = (char *) 0;
  }

#ifdef STOP_SIGNAL
  signal (SIGTSTP, SIG_DFL);
#endif
  immediate_quit--;

  /* Do history expansion if that is wished.  */
  if (history_expansion_p && instream == stdin
      && ISATTY (instream) && linebuffer[0] == '!')
    {
      char *history_value;
      int expanded;

      *p = '\0';		/* Insert null now.  */
      expanded = history_expand (linebuffer, &history_value);
      if (expanded)
	{
	  /* Print the changes.  */
	  ui_fprintf(stdout, "%s\n", history_value);

	  /* If there was an error, call this function again.  */
	  if (expanded < 0)
	    {
	      free (history_value);
	      return command_line_input (prompt, repeat);
	    }
	  if (strlen (history_value) > linelength)
	    {
	      linelength = strlen (history_value) + 1;
	      linebuffer = (char *) xrealloc (linebuffer, linelength);
	    }
	  strcpy (linebuffer, history_value);
	  p = linebuffer + strlen(linebuffer);
	  free (history_value);
	}
    }

  /* If we just got an empty line, and that is supposed
     to repeat the previous command, return the value in the
     global buffer.  */

#ifdef TEK_HACK
  if (repeat && autorepeat)
#else
  if (repeat)
#endif /* not TEK_HACK */

    {
      if (p == linebuffer)
	return line;
      p1 = linebuffer;
      while (*p1 == ' ' || *p1 == '\t')
	p1++;
      if (!*p1)
	return line;
    }

  /* If line is a comment, clear it out.  */
  p1 = linebuffer;
  while ((c = *p1) == ' ' || c == '\t') p1++;
  if (c == '#')
    p = linebuffer;

  *p = 0;

  /* Add line to history if appropriate.  */
  if (instream == stdin
      && ISATTY (stdin) && *linebuffer)
    add_history (linebuffer);

  /* Save into global buffer if appropriate.  */
  if (repeat)
    {
      if (linelength > linesize)
	{
	  line = xrealloc (line, linelength);
	  linesize = linelength;
	}
      strcpy (line, linebuffer);
      return line;
    }

  return linebuffer;
}

/* Read lines from the input stream
   and accumulate them in a chain of struct command_line's
   which is then returned.  */

#ifndef TEK_PROG_HACK 
struct command_line *
read_command_lines ()
{
  struct command_line *first = 0;
  register struct command_line *next, *tail = 0;
  register char *p, *p1;
  struct cleanup *old_chain = 0;

  while (1)
    {
      dont_repeat ();
      p = command_line_input (0, instream == stdin);
      /* Remove leading and trailing blanks.  */
      while (*p == ' ' || *p == '\t') p++;
      p1 = p + strlen (p);
      while (p1 != p && (p1[-1] == ' ' || p1[-1] == '\t')) p1--;

      /* Is this "end"?  */
      if (p1 - p == 3 && !strncmp (p, "end", 3))
	break;

      /* No => add this line to the chain of command lines.  */
      next = (struct command_line *) xmalloc (sizeof (struct command_line));
      next->line = savestring (p, p1 - p);
      next->next = 0;
      if (tail)
	{
	  tail->next = next;
	}
      else
	{
	  /* We just read the first line.
	     From now on, arrange to throw away the lines we have
	     if we quit or get an error while inside this function.  */
	  first = next;
	  old_chain = make_cleanup (free_command_lines, &first);
	}
      tail = next;
    }

  dont_repeat ();

  /* Now we are about to return the chain to our caller,
     so freeing it becomes his responsibility.  */
  if (first)
    discard_cleanups (old_chain);
  return first;
}
#endif

/* Free a chain of struct command_line's.  */

void
free_command_lines (lptr)
      struct command_line **lptr;
{
  register struct command_line *l = *lptr;
  register struct command_line *next;

  while (l)
    {
      next = l->next;
#ifdef TEK_PROG_HACK
      if (l->cmd && (l->cmd == (struct cmd_list_element *) INVALID_CORE_ADDR))
	free_string_list (l->line);
      else if (l->cmd && (l->cmd->class == class_nested))
        free_command_lines ((struct command_line **)&(l->line));
      else
#endif /* TEK_PROG_HACK */
      free (l->line);
      free (l);
      l = next;
    }
}

/* Add an element to the list of info subcommands.  */

void
add_info (name, fun, doc)
     char *name;
     void (*fun) ();
     char *doc;
{
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;

  new = 
#endif /* TEK_PROG_HACK */
  add_cmd (name, no_class, fun, doc, &infolist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) info_cmd;
#endif /* TEK_PROG_HACK */
}

/* Add an alias to the list of info subcommands.  */

void
add_info_alias (name, oldname, abbrev_flag)
     char *name;
     char *oldname;
     int abbrev_flag;
{
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;

  new =
#endif /* TEK_PROG_HACK */
  add_alias_cmd (name, oldname, 0, abbrev_flag, &infolist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) info_cmd;
#endif /* TEK_PROG_HACK */
}

/* The "info" command is defined as a prefix, with allow_unknown = 0.
   Therefore, its own definition is called only for "info" with no args.  */

static void
info_command ()
{
  ui_fprintf(stdout, "\"info\" must be followed by the name of an info command.\n");
  help_list (infolist, "info ", -1, stdout);
}

/* Add an element to the list of commands.  */

void
add_com (name, class, fun, doc)
     char *name;
     int class;
     void (*fun) ();
     char *doc;
{
  add_cmd (name, class, fun, doc, &cmdlist);
}

/* Add an alias or abbreviation command to the list of commands.  */

void
add_com_alias (name, oldname, class, abbrev_flag)
     char *name;
     char *oldname;
     int class;
     int abbrev_flag;
{
  add_alias_cmd (name, oldname, class, abbrev_flag, &cmdlist);
}

void
error_no_arg (why)
     char *why;
{
  ui_badnews(-1,"Argument required (%s).", why);
}

static void
help_command (command, from_tty)
     char *command;
     int from_tty; /* Ignored */
{
  help_cmd (command, stdout);
}

#ifdef TEK_PROG_HACK
char *
#else /* not TEK_PROG_HACK */
static void
#endif /* not TEK_PROG_HACK */
validate_comname (comname)
     char *comname;
{
  register char *p;

  if (comname == 0)
    error_no_arg ("name of command to define");

  p = comname;
  while (*p)
    {
#ifdef TEK_PROG_HACK
      if ((*p == ' ') || (*p == '\t'))
	break;
#endif /* TEK_PROG_HACK */      
      if (!(*p >= 'A' && *p <= 'Z')
	  && !(*p >= 'a' && *p <= 'z')
	  && !(*p >= '0' && *p <= '9')
	  && *p != '-')
	ui_badnews(-1,"Junk in argument list: \"%s\"", p);
      p++;
    }
#ifdef TEK_PROG_HACK
  return p;
#endif /* TEK_PROG_HACK */  
}

static void
define_command(comname, from_tty)
     char *comname;
     int from_tty;
{
  register struct command_line *cmds;
  register struct cmd_list_element *c;
  char *tem = comname;
#ifdef TEK_PROG_HACK
  char *extra;

  extra = validate_comname (comname);
  comname = savestring (comname, extra - comname);
#else /* not TEK_PROG_HACK */
  validate_comname (comname);
#endif /* not TEK_PROG_HACK */

  c = lookup_cmd (&tem, cmdlist, "", -1, 1);
  if (c)
    {
      if (c->class == (int) class_user || c->class == (int) class_alias)
	tem = "Redefine command \"%s\"? ";
      else
	tem = "Really redefine built-in command \"%s\"? ";
      if (!query (tem, comname))
	ui_badnews(-1,"Command \"%s\" not redefined.", comname);
    }

  if (from_tty)
    {
      ui_fprintf(stdout, "Type commands for definition of \"%s\".\n\
End with a line saying just \"end\".\n", comname);
      ui_fflush (stdout);
    }

#ifdef TEK_PROG_HACK
  if (*extra)
    {
      cmds = (struct command_line *) xmalloc (sizeof (struct command_line));
      cmds->cmd = (struct cmd_list_element *) INVALID_CORE_ADDR;
      cmds->line = (char *) parse_string_list (extra);
      cmds->next = read_command_lines (3);
    }
  else
    cmds = read_command_lines (3);
#else /* not TEK_PROG_HACK */
  comname = savestring (comname, strlen (comname));

  cmds = read_command_lines ();
#endif /* not TEK_PROG_HACK */

  if (c && c->class == (int) class_user)
    free_command_lines (&c->function);

  add_com (comname, class_user, cmds,
	   (c && c->class == (int) class_user)
	   ? c->doc : savestring ("User-defined.", 13));
}

static void
document_command (comname, from_tty)
     char *comname;
     int from_tty;
{
  struct command_line *doclines;
  register struct cmd_list_element *c;
  char *tem = comname;
#ifdef TEK_PROG_HACK
  char *extra;

  extra = validate_comname (comname);
  if (*extra)
    ui_badnews(-1,"Junk in argument list: \"%s\"", extra);
#else /* not TEK_PROG_HACK */
  validate_comname (comname);
#endif /* not TEK_PROG_HACK */

  c = lookup_cmd (&tem, cmdlist, "", 0, 1);

  if (c->class != (int) class_user)
    ui_badnews(-1,"Command \"%s\" is built-in.", comname);

  if (from_tty)
    ui_fprintf(stdout, "Type documentation for \"%s\".\n\
End with a line saying just \"end\".\n", comname);

#ifdef TEK_PROG_HACK
  doclines = read_command_lines (0);
#else /* not TEK_PROG_HACK */
  doclines = read_command_lines ();
#endif /* not TEK_PROG_HACK */
  if (c->doc) free (c->doc);

  {
    register struct command_line *cl1;
    register int len = 0;

    for (cl1 = doclines; cl1; cl1 = cl1->next)
      len += strlen (cl1->line) + 1;
 
   c->doc = (char *) xmalloc (len + 1);
    *c->doc = 0;

    for (cl1 = doclines; cl1; cl1 = cl1->next)
      {
	strcat (c->doc, cl1->line);
	if (cl1->next)
	  strcat (c->doc, "\n");
      }
  }

  free_command_lines (&doclines);
} 

static void
copying_info ()
{
  immediate_quit++;
  printf_filtered ("		    GDB GENERAL PUBLIC LICENSE\n\
		    (Clarified 11 Feb 1988)\n\
\n\
 Copyright (C) 1988 Richard M. Stallman\n\
 Everyone is permitted to copy and distribute verbatim copies\n\
 of this license, but changing it is not allowed.\n\
 You can also use this wording to make the terms for other programs.\n\
\n\
  The license agreements of most software companies keep you at the\n\
mercy of those companies.  By contrast, our general public license is\n\
intended to give everyone the right to share GDB.  To make sure that\n\
you get the rights we want you to have, we need to make restrictions\n\
that forbid anyone to deny you these rights or to ask you to surrender\n\
the rights.  Hence this license agreement.\n\
\n\
  Specifically, we want to make sure that you have the right to give\n\
away copies of GDB, that you receive source code or else can get it\n\
if you want it, that you can change GDB or use pieces of it in new\n\
free programs, and that you know you can do these things.\n");

  printf_filtered ("\
  To make sure that everyone has such rights, we have to forbid you to\n\
deprive anyone else of these rights.  For example, if you distribute\n\
copies of GDB, you must give the recipients all the rights that you\n\
have.  You must make sure that they, too, receive or can get the\n\
source code.  And you must tell them their rights.\n\
\n\
  Also, for our own protection, we must make certain that everyone\n\
finds out that there is no warranty for GDB.  If GDB is modified by\n\
someone else and passed on, we want its recipients to know that what\n\
they have is not what we distributed, so that any problems introduced\n\
by others will not reflect on our reputation.\n\
\n\
  Therefore we (Richard Stallman and the Free Software Foundation,\n\
Inc.) make the following terms which say what you must do to be\n\
allowed to distribute or change GDB.\n");

  printf_filtered ("\
			COPYING POLICIES\n\
\n\
  1. You may copy and distribute verbatim copies of GDB source code as\n\
you receive it, in any medium, provided that you conspicuously and\n\
appropriately publish on each copy a valid copyright notice \"Copyright\n\
\(C) 1988 Free Software Foundation, Inc.\" (or with whatever year is\n\
appropriate); keep intact the notices on all files that refer\n\
to this License Agreement and to the absence of any warranty; and give\n\
any other recipients of the GDB program a copy of this License\n\
Agreement along with the program.  You may charge a distribution fee\n\
for the physical act of transferring a copy.\n\
\n\
  2. You may modify your copy or copies of GDB or any portion of it,\n\
and copy and distribute such modifications under the terms of\n\
Paragraph 1 above, provided that you also do the following:\n\
\n\
    a) cause the modified files to carry prominent notices stating\n\
    that you changed the files and the date of any change; and\n");

  printf_filtered ("\
    b) cause the whole of any work that you distribute or publish,\n\
    that in whole or in part contains or is a derivative of GDB\n\
    or any part thereof, to be licensed to all third parties on terms\n\
    identical to those contained in this License Agreement (except that\n\
    you may choose to grant more extensive warranty protection to some\n\
    or all third parties, at your option).\n\
\n");
  printf_filtered ("\
    c) if the modified program serves as a debugger, cause it\n\
    when started running in the simplest and usual way, to print\n\
    an announcement including a valid copyright notice\n\
    \"Copyright (C) 1988 Free Software Foundation, Inc.\" (or with\n\
    the year that is appropriate), saying that there is no warranty\n\
    (or else, saying that you provide a warranty) and that users may\n\
    redistribute the program under these conditions, and telling the user\n\
    how to view a copy of this License Agreement.\n\
\n\
    d) You may charge a distribution fee for the physical act of\n\
    transferring a copy, and you may at your option offer warranty\n\
    protection in exchange for a fee.\n\
\n\
Mere aggregation of another unrelated program with this program (or its\n\
derivative) on a volume of a storage or distribution medium does not bring\n\
the other program under the scope of these terms.\n");

  printf_filtered ("\
  3. You may copy and distribute GDB (or a portion or derivative of it,\n\
under Paragraph 2) in object code or executable form under the terms of\n\
Paragraphs 1 and 2 above provided that you also do one of the following:\n\
\n\
    a) accompany it with the complete corresponding machine-readable\n\
    source code, which must be distributed under the terms of\n\
    Paragraphs 1 and 2 above; or,\n\
\n\
    b) accompany it with a written offer, valid for at least three\n\
    years, to give any third party free (except for a nominal\n\
    shipping charge) a complete machine-readable copy of the\n\
    corresponding source code, to be distributed under the terms of\n\
    Paragraphs 1 and 2 above; or,\n\n");

  printf_filtered ("\
    c) accompany it with the information you received as to where the\n\
    corresponding source code may be obtained.  (This alternative is\n\
    allowed only for noncommercial distribution and only if you\n\
    received the program in object code or executable form alone.)\n\
\n\
For an executable file, complete source code means all the source code for\n\
all modules it contains; but, as a special exception, it need not include\n\
source code for modules which are standard libraries that accompany the\n\
operating system on which the executable file runs.\n");

  printf_filtered ("\
  4. You may not copy, sublicense, distribute or transfer GDB\n\
except as expressly provided under this License Agreement.  Any attempt\n\
otherwise to copy, sublicense, distribute or transfer GDB is void and\n\
your rights to use the program under this License agreement shall be\n\
automatically terminated.  However, parties who have received computer\n\
software programs from you with this License Agreement will not have\n\
their licenses terminated so long as such parties remain in full compliance.\n\
\n\
");
  printf_filtered ("\
  5. If you wish to incorporate parts of GDB into other free\n\
programs whose distribution conditions are different, write to the Free\n\
Software Foundation at 675 Mass Ave, Cambridge, MA 02139.  We have not yet\n\
worked out a simple rule that can be stated here, but we will often permit\n\
this.  We will be guided by the two goals of preserving the free status of\n\
all derivatives of our free software and of promoting the sharing and reuse\n\
of software.\n\
\n\
In other words, go ahead and share GDB, but don't try to stop\n\
anyone else from sharing it farther.  Help stamp out software hoarding!\n\
");
  immediate_quit--;
}

static void
warranty_info ()
{
  immediate_quit++;
  printf_filtered ("			 NO WARRANTY\n\
\n\
  BECAUSE GDB IS LICENSED FREE OF CHARGE, WE PROVIDE ABSOLUTELY NO\n\
WARRANTY, TO THE EXTENT PERMITTED BY APPLICABLE STATE LAW.  EXCEPT\n\
WHEN OTHERWISE STATED IN WRITING, FREE SOFTWARE FOUNDATION, INC,\n\
RICHARD M. STALLMAN AND/OR OTHER PARTIES PROVIDE GDB \"AS IS\" WITHOUT\n\
WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT\n\
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n\
A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY AND\n\
PERFORMANCE OF GDB IS WITH YOU.  SHOULD GDB PROVE DEFECTIVE, YOU\n\
ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.\n\n");

  printf_filtered ("\
 IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW WILL RICHARD M.\n\
STALLMAN, THE FREE SOFTWARE FOUNDATION, INC., AND/OR ANY OTHER PARTY\n\
WHO MAY MODIFY AND REDISTRIBUTE GDB, BE LIABLE TO\n\
YOU FOR DAMAGES, INCLUDING ANY LOST PROFITS, LOST MONIES, OR OTHER\n\
SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR\n\
INABILITY TO USE (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA\n\
BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY THIRD PARTIES OR A\n\
FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) GDB, EVEN\n\
IF YOU HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES, OR FOR\n\
ANY CLAIM BY ANY OTHER PARTY.\n");
  immediate_quit--;
}

  
static void
print_gdb_version ()
{
  printf_filtered ("\
This version of GDB contains a simulator of a 4-processor, 8-CMMU 88000 system.\n");
}

#ifdef TEK_HACK
extern char _rcsid[];
#endif /* TEK_HACK */

static void
version_info ()
{
  immediate_quit++;
  print_gdb_version ();
#ifdef TEK_HACK
    ui_fprintf(stdout, "%s\n", _rcsid);
#endif /* TEK_HACK */
  immediate_quit--;
}

/* xgdb calls this to reprint the usual GDB prompt.  */

void
print_prompt ()
{
  extern int selected_processor;

  ui_fflush();
  ui_newprompt();
#ifdef TEK_PROG_HACK
  ui_fprintf(stdout, "[%d] %s", selected_processor, gbl_prompt);
#else /* not TEK_PROG_HACK */
  ui_fprintf(stdout, "%s", prompt);
#endif /* not TEK_PROG_HACK */
  ui_fflush (stdout);
}

/* Command to specify a prompt string instead of "(gdb) ".  */

#ifndef TEK_PROG_HACK
static void
set_prompt_command (text)
     char *text;
{
  char *p, *q;
  register int c;
  char *new;

  if (text == 0)
    error_no_arg ("string to which to set prompt");

  new = (char *) xmalloc (strlen (text) + 2);
  p = text; q = new;
  while (c = *p++)
    {
      if (c == '\\')
	{
	  /* \ at end of argument is used after spaces
	     so they won't be lost.  */
	  if (*p == 0)
	    break;
	  c = parse_escape (&p);
	  if (c == 0)
	    break; /* C loses */
	  else if (c > 0)
	    *q++ = c;
	}
      else
	*q++ = c;
    }
  if (*(p - 1) != '\\')
    *q++ = ' ';
  *q++ = '\0';
  new = (char *) xrealloc (new, q - new);
  free (prompt);
  prompt = new;
}
#endif /* not TEK_PROG_HACK */

static void
quit_command ()
{
  extern int simulator;
  if (have_inferior_p () && !remote_debugging)
    {
#ifdef TEK_HACK
      /* If the process stopped because of SIGKILL, then it cannot be continued,
	 so skip the message asking whether to quit.
	 This addresses Tektronix bug BLK1450. */
      if (stop_signal == SIGKILL ||
	  query ("The program is running.  Quit anyway? "))
#else
      if (query ("The program is running.  Quit anyway? "))
#endif
	{
	  /* Prevent any warning message from reopen_exec_file, in case
	     we have a core file that's inconsistent with the exec file.  */
	  exec_file_command (0, 0);
	  kill_inferior_fast ();
	}
      else
	ui_badnews(-1,"Not confirmed.");
    }
  remote_close(1);
  /* Save the history information if it is appropriate to do so.  */
  if (write_history_p && history_filename)
    write_history (history_filename);
  ui_byebye();
  exit (0);
}

int
input_from_terminal_p ()
{
  return instream == stdin;
}

static void
pwd_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  char getwd_path[MAXPATHLEN];

  if (arg) ui_badnews(-1,"The \"pwd\" command does not take an argument: %s", arg);
  getwoid (getwd_path);
  if (strcmp (getwd_path, current_directory))
    ui_fprintf(stdout, "Working directory %s\n(actually %s)\n",
	    current_directory, getwd_path);
  else
    ui_fprintf(stdout, "Working directory %s\n", current_directory);
}

static void
cd_command (dir, from_tty)
     char *dir;
     int from_tty;
{
  auto char newdirbuf[MAXPATHLEN];
  int dirlen;
  int change;
  char *p;

  if (dir == 0)
    error_no_arg ("new working directory");
  dirlen = strlen(dir);
  if (dirlen >= MAXPATHLEN)
    ui_badnews(-1, "Directory name too long\n");

  /* Build the new directory name. */
  if (dir[0] == '/')
    strcpy(newdirbuf, dir);
  else
    {
      int oldlen = strlen(current_directory);
      if (oldlen + dirlen + 1 /* for '/' */ >= MAXPATHLEN)
	ui_badnews(-1, "Directory name too long\n");
      strcpy(newdirbuf, current_directory);
      newdirbuf[oldlen] = '/';
      strcpy(&newdirbuf[oldlen+1], dir);
    }

  /* Reduce the constructed path name.
     Keep looping until no more reductions can be applied. */
  do
    {
      change = 0;

      /* Replace leading "/../" with "/". */
      while (!strncmp(newdirbuf, "/../", 4))
	{
	  strcpy(newdirbuf, newdirbuf+3);
	  change = 1;
	}

      /* Change path "/.." to "/". */
      if (!strcmp(newdirbuf, "/.."))
	{
	  newdirbuf[1] = '\0';
	  break;
	}

      /* Loop to handle substring reductions. */
      p = newdirbuf;
      while (*p)
	{
		/* Eliminate trailing "/" or "/." */
	  if (p != newdirbuf && (!strcmp (p, "/") || !strcmp (p, "/.")))
	    *p = '\0', change = 1;
		/* Change "//" to "/" */
	  else if (!strncmp (p, "//", 2))
	    strcpy(p, p+1), change = 1;
		/* Change "/./" to "/" */
	  else if (!strncmp (p, "/./", 3))
	    strcpy (p, p+2), change = 1;
		/* Change "/dir/.." to "/" */
	  else if (!strncmp (p, "/..", 3) && (p[3] == 0 || p[3] == '/') &&
		   p != newdirbuf)
	    {
	      char *q = p;
	      while (q != newdirbuf && q[-1] != '/') q--;
	      if (q-1 == newdirbuf)
		{
			/* Path begins "/dir/..".
			   Replace substring with "/". */
		  strcpy (q, p+3);
		  p = q;
		  change = 1;
		}
	      else if (q != newdirbuf)
		{
			/* "/dir/.." occurs after beginning of string.
			   Delete entire substring. */
		  strcpy (q-1, p+3);
		  p = q-1;
		  change = 1;
		}
	    }
	  else p++;
	}
    }
  while (change);

  /* Issue the chdir and check that it works before committing to the new
     path name. */
  if (chdir (newdirbuf) < 0)
    perror_with_name (newdirbuf);

  /* Install the new path name. */
  strcpy(dirbuf, newdirbuf);

  if (from_tty)
    pwd_command ((char *) 0, 1);
}

static void
source_command (file)
     char *file;
{
  FILE *stream;
  struct cleanup *cleanups;
  struct source_stack_structure *s;

  if (file == 0)
    error_no_arg ("file to read commands from");

  stream = fopen (file, "r");
  if (stream == 0)
    perror_with_name (file);

  cleanups = make_cleanup (source_stack_cleanup, source_stack);

  /* Stack the name of this file, for "info files". */
  s = (struct source_stack_structure *)
      xmalloc(sizeof(struct source_stack_structure) + strlen(file));
  strcpy(s->file, file);
  s->previous_instream = instream;
  s->next = source_stack;
  source_stack = s;

  instream = stream;

  command_loop ();

  do_cleanups (cleanups);
}

info_active_sources()
{
  register struct source_stack_structure *s = source_stack;

  if (!s)
    ui_fprintf(stdout, "No active source scripts.\n");
  else if (!s->next)	/* one-line report */
    ui_fprintf(stdout, "Active source script is \"%s\".\n", s->file);
  else
  {
    /* Loop through sources and report filenames. */
    ui_fprintf(stdout,
	       "Active source scripts (first is most recently started):\n");
    while (s)
    {
      ui_fprintf(stdout, "  \"%s\"\n", s->file);
      s = s->next;
    }
  }
}

static void
echo_command (text)
     char *text;
{
  char *p = text;
  register int c;

  if (text)
    while (c = *p++)
      {
	if (c == '\\')
	  {
	    /* \ at end of argument is used after spaces
	       so they won't be lost.  */
	    if (*p == 0)
	      return;

	    c = parse_escape (&p);
	    if (c >= 0)
	      ui_putc (c, stdout);
	  }
	else
	  ui_putc (c, stdout);
      }
}
 
static void
dump_me_command ()
{
  if (query ("Should GDB dump core? "))
    {
      signal (SIGQUIT, SIG_DFL);
      kill (getpid (), SIGQUIT);
    }
}

int
parse_binary_operation (caller, arg)
     char *caller, *arg;
{
  int length;

  if (!arg || !*arg)
    return 1;

  length = strlen (arg);

  while (arg[length - 1] == ' ' || arg[length - 1] == '\t')
    length--;

  if (!strncmp (arg, "on", length)
      || !strncmp (arg, "1", length)
      || !strncmp (arg, "yes", length))
    return 1;
  else
    if (!strncmp (arg, "off", length)
	|| !strncmp (arg, "0", length)
	|| !strncmp (arg, "no", length))
      return 0;
    else
      ui_badnews(-1,"\"%s\" not given a binary valued argument.", caller);
}

/* Functions to manipulate command line editing control variables.  */

static void
set_editing (arg, from_tty)
     char *arg;
     int from_tty;
{
#ifdef TEK_HACK
  if (! usingX)
#endif /* TEK_HACK */
    command_editing_p = parse_binary_operation ("set command-editing", arg);
}

static void
editing_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  int offset;

  printf_filtered ("Interactive command editing is %s.\n",
	  command_editing_p ? "on" : "off");

  printf_filtered ("History expansion of command input is %s.\n",
	  history_expansion_p ? "on" : "off");
  printf_filtered ("Writing of a history record upon exit is %s.\n",
	  write_history_p ? "enabled" : "disabled");
  printf_filtered ("The size of the history list (number of stored commands) is %d.\n",
	  history_size);
  printf_filtered ("The name of the history record is \"%s\".\n\n",
	  history_filename ? history_filename : "");
  printf_filtered ("This list of the last %d commands is:\n\n",
	  history_size);
  for (offset = 0; offset < history_size; offset++)
    {
      struct _hist_entry {
	char *line;
	char *data;
      } *retval, *history_get();
      extern int history_base;

      retval = history_get (history_base + offset);
      if (!retval) break;

      printf_filtered ("%5d  %s\n", history_base + offset,
	      retval->line);
    }
}

static void
set_history_expansion (arg, from_tty)
     char *arg;
     int from_tty;
{
  history_expansion_p = parse_binary_operation ("set history expansion", arg);
}

static void
set_history_write (arg, from_tty)
     char *arg;
     int from_tty;
{
  write_history_p = parse_binary_operation ("set history write", arg);
}

static void
set_history (arg, from_tty)
     char *arg;
     int from_tty;
{
  ui_fprintf(stdout, "\"set history\" must be followed by the name of a history subcommand.\n");
  help_list (sethistlist, "set history ", -1, stdout);
}

static void
set_history_size (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (!*arg)
    error_no_arg ("set history size");

  history_size = atoi (arg);
}

static void
set_history_filename (arg, from_tty)
     char *arg;
     int from_tty;
{
  int i = strlen (arg) - 1;
  
  free (history_filename);
  
  while (i > 0 && (arg[i] == ' ' || arg[i] == '\t'))
    i--;

  if (!*arg)
    history_filename = (char *) 0;
  else
    history_filename = savestring (arg, i + 1);
}

int info_verbose;

static void
set_verbose_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  info_verbose = parse_binary_operation ("set verbose", arg);
}

static void
verbose_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (arg)
    ui_badnews(-1,"\"info verbose\" does not take any arguments.\n");
  
  ui_fprintf(stdout, "Verbose printing of information is %s.\n",
	  info_verbose ? "on" : "off");
}

static void
float_handler ()
{
  ui_badnews(-1,"Invalid floating value encountered or computed.");
}


static void
initialize_cmd_lists ()
{
  cmdlist = (struct cmd_list_element *) 0;
  infolist = (struct cmd_list_element *) 0;

  info_cmd =
  add_prefix_cmd ("info", class_info, info_command,
		  "Generic command for printing status.",
		  &infolist, "info ", 0, &cmdlist);

  enablelist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  enable_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_HACK */
  disablelist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  disable_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_HACK */
  deletelist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  gdbdelete_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_PROG_HACK */
  enablebreaklist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  enablebreak_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_PROG_HACK */
  setlist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  set_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_PROG_HACK */
  sethistlist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  sethist_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_PROG_HACK */
  unsethistlist = (struct cmd_list_element *) 0;
#ifdef TEK_PROG_HACK
  unsethist_cmd = (struct cmd_list_element *) 0;
#endif /* TEK_PROG_HACK */
}

#ifdef TEK_HACK

reset_stdout(oldfd)
  int oldfd;
{
  fflush(stdout);
  close(1);
  if (dup(oldfd) != 1) {
    ui_badnews(-2, "Standard out dup failed!");
  }
  close(oldfd);
  ui_endRedirectOut();
}

redirect_out_command(args)
    char *args;
{
  register int tty;
  char *filename, *cptr = args;
  int len = 0;
  int oldstdout;
  struct cleanup *old_chain;

  /* Get filename from args */
  while (cptr && *cptr != ' ' && *cptr != '\t') {
	len++;
        cptr++;
  }
  filename = (char *) xmalloc(len + 1);
  strncpy(filename, args, len);
  filename[len] = '\0';
  oldstdout = dup(1);
  close(1);
  tty = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0775);
  free(filename);
  if (tty == -1) {
    ui_badnews(-1, "Can't open %s for output redirection", filename);
  }
  ui_startRedirectOut();
  old_chain = make_cleanup(reset_stdout, oldstdout);
  execute_command(0, args + len, 0);
  do_cleanups(old_chain);
}
#endif /*TEK_HACK*/
void
make_button_commands ()
{
/* Commands added to enable button adding/deleting.  User should only
 * access these from a startup file, although it is possible to use these
 * from the command line.  These are undefined if no X interface is running.
 * DL 8/2/89
 */
  add_com("button", class_obscure, ui_button, "Add a button associated with a command.");
  add_com("unbutton", class_obscure, ui_unbutton, "Delete a button associated with a command.");
}

static void
initialize_main ()
{
  char *tmpenv;
  struct cmd_list_element *new;
  /* Command line editing externals.  */
  extern int (*rl_completion_entry_function)();
  extern char *rl_completer_word_break_characters;
  
  /* Set default verbose mode on.  */
  info_verbose = 1;

#ifndef TEK_PROG_HACK
  prompt = savestring ("(gdb) ", 6);
#endif /* not TEK_PROG_HACK */

  /* Set the important stuff up for command editing.  */
  if (! usingX) command_editing_p = 1;
  history_expansion_p = 1;
  write_history_p = 0;
  
  if (tmpenv = getenv ("HISTSIZE"))
    history_size = atoi (tmpenv);
  else
    history_size = 256;

  stifle_history (history_size);

  if (tmpenv = getenv ("GDBHISTFILE"))
    history_filename = savestring (tmpenv, strlen(tmpenv));
  else
    history_filename = savestring ("./.gdb_history", 14);

  read_history (history_filename);

  /* Setup important stuff for command line editing.  */
  rl_completion_entry_function = (int (*)()) symbol_completion_function;
  rl_completer_word_break_characters = gdb_completer_word_break_characters;

  /* Define the classes of commands.
     They will appear in the help list in the reverse of this order.  */

  add_cmd ("obscure", class_obscure, 0, "Obscure features.", &cmdlist);
#ifdef TEK_PROG_HACK
  add_cmd ("conditionals", class_conditional, 0, "Commands for conditional execution.\n\
These include a command for executing \"while loops\" and commands that\n\
implement an \"if-else\" like facility.", &cmdlist);
#endif /* TEK_PROG_HACK */
  add_cmd ("alias", class_alias, 0, "Aliases of other commands.", &cmdlist);
#ifdef  TEK_PROG_HACK
  add_cmd ("user", class_user, 0, "User-defined commands.\n\
The commands in this class are those defined by the user.\n\
Use the \"define\" command to define a command.\n\
Use \"info user COMMAND\" to print the definition of COMMAND.\n\
Use \"info user\" with no argument to print the definitions of all\n\
user-defined commands.", &cmdlist);
#else /* not TEK_PROG_HACK */
  add_cmd ("user", class_user, 0, "User-defined commands.\n\
The commands in this class are those defined by the user.\n\
Use the \"define\" command to define a command.", &cmdlist);
#endif /* not TEK_PROG_HACK */
  add_cmd ("support", class_support, 0, "Support facilities.", &cmdlist);
  add_cmd ("status", class_info, 0, "Status inquiries.", &cmdlist);
  add_cmd ("files", class_files, 0, "Specifying and examining files.", &cmdlist);
  add_cmd ("breakpoints", class_breakpoint, 0, "Making program stop at certain points.", &cmdlist);
  add_cmd ("data", class_vars, 0, "Examining data.", &cmdlist);
  add_cmd ("stack", class_stack, 0, "Examining the stack.\n\
The stack is made up of stack frames.  Gdb assigns numbers to stack frames\n\
counting from zero for the innermost (currently executing) frame.\n\n\
At any time gdb identifies one frame as the \"selected\" frame.\n\
Variable lookups are done with respect to the selected frame.\n\
When the program being debugged stops, gdb selects the innermost frame.\n\
The commands below can be used to select other frames by number or address.",
           &cmdlist);
  add_cmd ("running", class_run, 0, "Running the program.", &cmdlist);

  add_com ("pwd", class_files, pwd_command,
	   "Print working directory.  This is used for your program as well.");
  add_com ("cd", class_files, cd_command,
	   "Set working directory to DIR for debugger and program being debugged.\n\
The change does not take effect for the program being debugged\n\
until the next time it is started.");

#ifdef TEK_HACK
  add_com ("redirect-output", class_support, redirect_out_command, "Redirect output for this command to a file.");
  add_com_alias (">", "redirect-output", class_support, 1);
#endif /* not TEK_HACK */
#ifndef TEK_PROG_HACK
  add_cmd ("prompt", class_support, set_prompt_command,
	   "Change gdb's primary prompt from the default of \"(gdb)\"",
	   &setlist);
#endif /* not TEK_PROG_HACK */
  add_com ("echo", class_support, echo_command,
	   "Print a constant string.  Give string as argument.\n\
C escape sequences may be used in the argument.\n\
No newline is added at the end of the argument;\n\
use \"\\n\" if you want a newline to be printed.\n\
Since leading and trailing whitespace are ignored in command arguments,\n\
if you want to print some you must use \"\\\" before leading whitespace\n\
to be printed or after trailing whitespace.");
  add_com ("document", class_support, document_command,
	   "Document a user-defined command.\n\
Give command name as argument.  Give documentation on following lines.\n\
End with a line of just \"end\".");
  add_com ("define", class_support, define_command,
	   "Define a new command name.  Command name is argument.\n\
Definition appears on following lines, one command per line.\n\
End with a line of just \"end\".\n\
Use the \"document\" command to give documentation for the new command.\n\
Commands defined in this way do not take arguments.");

  add_com ("source", class_support, source_command,
	   "Read commands from a file named FILE.\n\
Note that the file \".gdbinit\" is read automatically in this way\n\
when gdb is started.");
  add_com ("quit", class_support, quit_command, "Exit gdb.");
  add_com ("help", class_support, help_command, "Print list of commands.");
  add_com_alias ("q", "quit", class_support, 1);
  add_com_alias ("h", "help", class_support, 1);

#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("verbose", class_support, set_verbose_command,
	   "Change the number of informational messages gdb prints.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */
  add_info ("verbose", verbose_info,
	    "Status of gdb's verbose printing option.\n");

  add_com ("dump-me", class_obscure, dump_me_command,
	   "Get fatal error; make debugger dump its core.");

#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("editing", class_support, set_editing,
	   "Enable or disable command line editing.\n\
Use \"on\" to enable to enable the editing, and \"off\" to disable it.\n\
Without an argument, command line editing is enabled.", &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;

  new =
#endif /* TEK_PROG_HACK */
  add_prefix_cmd ("history", class_support, set_history,
		  "Generic command for setting command history parameters.",
		  &sethistlist, "set history ", 0, &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    {
      new->aux = (char *) set_cmd;
      sethist_cmd = new;
    }

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("expansion", no_class, set_history_expansion,
	   "Enable or disable history expansion on command input.\n\
Without an argument, history expansion is enabled.", &sethistlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) sethist_cmd;

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("write", no_class, set_history_write,
	   "Enable or disable saving of the history record on exit.\n\
Use \"on\" to enable to enable the saving, and \"off\" to disable it.\n\
Without an argument, saving is enabled.", &sethistlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) sethist_cmd;

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("size", no_class, set_history_size,
	   "Set the size of the command history, \n\
ie. the number of previous commands to keep a record of.", &sethistlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) sethist_cmd;

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("filename", no_class, set_history_filename,
	   "Set the filename in which to record the command history\n\
 (the list of previous commands of which a record is kept).", &sethistlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) sethist_cmd;
#endif /* TEK_PROG_HACK */

  add_com_alias ("i", "info", class_info, 1);

  add_info ("editing", editing_info, "Status of command editor.");

  add_info ("copying", copying_info, "Conditions for redistributing copies of GDB.");
  add_info ("warranty", warranty_info, "Various kinds of warranty you do not have.");
  add_info ("version", version_info, "Report what version of GDB this is.");
#ifdef TEK_PROG_HACK
  init_cmd_pointers ();
#endif /* TEK_PROG_HACK */
#ifdef TEK_HACK
  {  char *p;
  
     p = "echo";
     echo_p = lookup_cmd (&p, cmdlist, "", -1, 1);
     if (echo_p == 0)
	ui_badnews(1, "Internal: \"Echo\" not defined.");
  }
#endif /* TEK_HACK */
}
