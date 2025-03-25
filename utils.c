/* General utility routines for GDB, the GNU debugger.
   Copyright (C) 1986, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/utils.c,v 1.30 91/01/01 21:16:51 robertb Exp $
   $Locker:  $

 */

#ifdef	DG_HACK
/* Changed name of getwd to getwoid to avoid system call conflict -- jfs */
#endif /* DG_HACK */
#ifdef	TEK_HACK
/* Changed name of insque to insqueue and remque to remqueue to avoid
   conflict with X11 routines */
#endif /* TEK_HACK */

#include <stdio.h>
#include <signal.h>

#ifdef BSD
#ifndef _WIN32
#include <util.h>
#endif
#endif

#ifdef TEK_HACK
#include <assert.h>
#endif /* TEK_HACK */

#include "defs.h"
#include "param.h"
#include "ui.h"
#ifdef TEK_PROG_HACK
#include "command.h"
#endif /* TEK_PROG_HACK */

#ifdef HAVE_TERMIO
#include <termio.h>
#endif

/* If this definition isn't overridden by the header files, assume
   that isatty and fileno exist on this system.  */
#ifndef ISATTY
#define ISATTY(FP)	(isatty (fileno (FP)))
#endif

#ifdef TEK_HACK
extern int usingX;
extern char *gdb_readline();
extern FILE *instream;
#endif /* TEK_HACK */

void fatal();

/* Chain of cleanup actions established with make_cleanup,
   to be executed if an error happens.  */

static struct cleanup *cleanup_chain;

/* Nonzero means a quit has been requested.  */

int quit_flag;

/* Nonzero means quit immediately if Control-C is typed now,
   rather than waiting until QUIT is executed.  */

int immediate_quit;

/* Add a new cleanup to the cleanup_chain,
   and return the previous chain pointer
   to be passed later to do_cleanups or discard_cleanups.
   Args are FUNCTION to clean up with, and ARG to pass to it.  */

struct cleanup *
make_cleanup (function, arg)
     void (*function) ();
     int arg;
{
  register struct cleanup *new
    = (struct cleanup *) xmalloc (sizeof (struct cleanup));
  register struct cleanup *old_chain = cleanup_chain;

  new->next = cleanup_chain;
  new->function = function;
  new->arg = arg;
  cleanup_chain = new;

  return old_chain;
}

/* Discard cleanups and do the actions they describe
   until we get back to the point OLD_CHAIN in the cleanup_chain.  */

void
do_cleanups (old_chain)
     register struct cleanup *old_chain;
{
  register struct cleanup *ptr;
  while ((ptr = cleanup_chain) != old_chain)
    {
      (*ptr->function) (ptr->arg);
      cleanup_chain = ptr->next;
      free (ptr);
    }
}

/* Discard cleanups, not doing the actions they describe,
   until we get back to the point OLD_CHAIN in the cleanup_chain.  */

void
discard_cleanups (old_chain)
     register struct cleanup *old_chain;
{
  register struct cleanup *ptr;
  while ((ptr = cleanup_chain) != old_chain)
    {
      cleanup_chain = ptr->next;
      free (ptr);
    }
}

/* Set the cleanup_chain to 0, and return the old cleanup chain.  */
struct cleanup *
save_cleanups ()
{
  struct cleanup *old_chain = cleanup_chain;

  cleanup_chain = 0;
  return old_chain;
}

/* Restore the cleanup chain from a previously saved chain.  */
void
restore_cleanups (chain)
     struct cleanup *chain;
{
  cleanup_chain = chain;
}

/* This function is useful for cleanups.
   Do

     foo = xmalloc (...);
     old_chain = make_cleanup (free_current_contents, &foo);

   to arrange to free the object thus allocated.  */

void
free_current_contents (location)
     char **location;
{
  free (*location);
}

/* Generally useful subroutines used throughout the program.  */

/* Like malloc but get error if no storage available.  */
#ifdef TEK_HACK
/*
 * Revised to check for zero-length malloc -=- andrew@frip.wv.tek.com 25.Sep.89
 */
#endif /* TEK_HACK */
char *
xmalloc (size)
     long size;
{
#ifndef TEK_HACK
  register char *val = (char *) malloc (size);
#else /* TEK_HACK */
  register char *val;

  assert(size>0);
  val = (char *) malloc (size);
#endif /* TEK_HACK */
  if (!val)
    ui_badnews(1,"virtual memory exhausted.", 0);
  return val;
}

/* Like realloc but get error if no storage available.  */

char *
xrealloc (ptr, size)
     char *ptr;
     long size;
{
#ifndef TEK_HACK
  register char *val = (char *) realloc (ptr,size);
#else /* TEK_HACK */
  register char *val;

  assert(size>0);
  val = (char *) realloc (ptr, size);
#endif /* TEK_HACK */
  if (!val)
    ui_badnews(1,"virtual memory exhausted.", 0);
  return val;
}

/* Print the system error message for errno, and also mention STRING
   as the file name for which the error was encountered.
   Then return to command level.  */

void
perror_with_name (string)
     char *string;
{
  extern int sys_nerr;
//  extern char *sys_errlist[];
  extern int errno;
  char *err;
  char *combined;

#ifndef _WIN32
  if (errno < sys_nerr)
    err = sys_errlist[errno];
  else
#endif
    err = "unknown error";

  combined = (char *) alloca (strlen (err) + strlen (string) + 3);
  strcpy (combined, string);
  strcat (combined, ": ");
  strcat (combined, err);

  ui_badnews(-1,"%s.", combined);
}

/* Print the system error message for ERRCODE, and also mention STRING
   as the file name for which the error was encountered.  */

void
print_sys_errmsg (string, errcode)
     char *string;
     int errcode;
{
  extern int sys_nerr;
//  extern char *sys_errlist[];
  char *err;
  char *combined;

#ifndef _WIN32
  if (errcode < sys_nerr)
    err = sys_errlist[errcode];
  else
#endif
    err = "unknown error";

  combined = (char *) alloca (strlen (err) + strlen (string) + 3);
  strcpy (combined, string);
  strcat (combined, ": ");
  strcat (combined, err);

  ui_fprintf(stdout, "%s.\n", combined);
}

void
quit ()
{
#ifdef TEK_HACK
  extern remote_debugging;
  if (remote_debugging) {
    return_to_top_level();
  }
#endif

#ifndef _WIN32
#ifdef HAVE_TERMIO
  ioctl (fileno (stdout), TCFLSH, 1);
#else /* not HAVE_TERMIO */
  ioctl (fileno (stdout), TIOCFLUSH, 0);
#endif /* not HAVE_TERMIO */
#ifdef TIOCGPGRP
  ui_badnews(-1,"Quit");
#else
  ui_badnews(-1,"Quit (expect signal %d when inferior is resumed)", SIGINT);
#endif /* TIOCGPGRP */
#else
  ui_badnews(-1,"Quit");
#endif
}

/* Control C comes here */

void
request_quit ()
{
  quit_flag = 1;

#ifdef USG
  /* Restore the signal handler.  */
  signal (SIGINT, request_quit);
#endif

#ifdef TEK_HACK
    signal (SIGINT, request_quit);
    remote_request_quit();
#endif
  if (immediate_quit)
    quit ();
}

/* Print an error message and return to command level.
   STRING is the error message, used as a fprintf string,
   and ARG is passed as an argument to it.  */

/*VARARGS1*/
void
error(string, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
     char *string;
     int arg1, arg2, arg3, arg4, arg5, arg6, arg7;
{
  terminal_ours ();		/* Should be ok even if no inf.  */
  ui_fflush (stdout);
  ui_fprintf (stderr, string, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  ui_fprintf (stderr, "\n");
  return_to_top_level ();
  /* NOTREACHED */
}

/* Print an error message and exit reporting failure.
   This is for a error that we cannot continue from.
   STRING and ARG are passed to ui_fprintf.  */

/*VARARGS1*/
void
fatal(string, arg)
     char *string;
     int arg;
{
  ui_fprintf (stderr, "gdb: ");
  ui_fprintf (stderr, string, arg);
  ui_fprintf (stderr, "\n");
  exit (1);
}

/* Make a copy of the string at PTR with SIZE characters
   (and add a null character at the end in the copy).
   Uses malloc to get the space.  Returns the address of the copy.  */

char *
savestring (ptr, size)
     char *ptr;
     int size;
{
  register char *p = (char *) xmalloc (size + 1);
  bcopy (ptr, p, size);
  p[size] = 0;
  return p;
}

char *
concat (s1, s2, s3)
     char *s1, *s2, *s3;
{
  register int len = strlen (s1) + strlen (s2) + strlen (s3) + 1;
  register char *val = (char *) xmalloc (len);
  strcpy (val, s1);
  strcat (val, s2);
  strcat (val, s3);
  return val;
}

void
print_spaces (n, file)
     register int n;
     register FILE *file;
{
  while (n-- > 0)
    ui_putc (' ', file);
}

/* Ask user a y-or-n question and return 1 iff answer is yes.
   Takes three args which are given to printf to print the question.
   The first, a control string, should end in "? ".
   It should not say how to answer, because we do that.  */
/* Revised to use ui_fgets. DL 8/1/89 */

#ifdef TEK_HACK
/*VARARGS1*/
int
query (ctlstr, arg1, arg2)
     char *ctlstr;
{
  register char *i;
  char buff[1024];

  /* Automatically answer "yes" if input is not from a terminal.  */
  if (!input_from_terminal_p ())
    return 1;

  /* If using X interface and last input was from a mouse event, pop up
   * a dialog */
  if (usingX) 
    return (ui_query(ctlstr, arg1, arg2));

  while (1)
    {
      ui_newprompt();
      ui_fprintf(stdout, ctlstr, arg1, arg2);
      ui_fprintf(stdout, "(y or n) ");
      ui_fflush (stdout);
      i = ui_fgets (buff, 1024, stdin);
      clearerr (stdin);		/* in case of C-d */
      if (!i)			/* C-d is equivalent to yes */
	{
	  ui_putc('\n', stdout);
	  return 1;
	}
      if (buff[0] >= 'a')
	buff[0] -= 040;
      if (buff[0]== 'Y')
	return 1;
      if (buff[0] == 'N')
	return 0;
      ui_fprintf(stdout, "Please answer y or n.\n");
    }
}
#else /* TEK_HACK */
int
query (ctlstr, arg1, arg2)
     char *ctlstr;
{
  register int answer;

  /* Automatically answer "yes" if input is not from a terminal.  */
  if (!input_from_terminal_p ())
    return 1;

  while (1)
    {
      printf (ctlstr, arg1, arg2);
      printf ("(y or n) ");
      fflush (stdout);
      answer = fgetc (stdin);
      clearerr (stdin);		/* in case of C-d */
      if (answer != '\n')
	while (fgetc (stdin) != '\n') clearerr (stdin);
      if (answer >= 'a')
	answer -= 040;
      if (answer == 'Y')
	return 1;
      if (answer == 'N')
	return 0;
      printf ("Please answer y or n.\n");
    }
}
#endif /* TEK_HACK */

/* Parse a C escape sequence.  STRING_PTR points to a variable
   containing a pointer to the string to parse.  That pointer
   is updated past the characters we use.  The value of the
   escape sequence is returned.

   A negative value means the sequence \ newline was seen,
   which is supposed to be equivalent to nothing at all.

   If \ is followed by a null character, we return a negative
   value and leave the string pointer pointing at the null character.

   If \ is followed by 000, we return 0 and leave the string pointer
   after the zeros.  A value of 0 does not mean end of string.  */

int
parse_escape (string_ptr)
     char **string_ptr;
{
  register int c = *(*string_ptr)++;
  switch (c)
    {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'e':
      return 033;
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    case '\n':
      return -2;
    case 0:
      (*string_ptr)--;
      return 0;
    case '^':
      c = *(*string_ptr)++;
      if (c == '\\')
	c = parse_escape (string_ptr);
      if (c == '?')
	return 0177;
      return (c & 0200) | (c & 037);
      
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      {
	register int i = c - '0';
	register int count = 0;
	while (++count < 3)
	  {
	    if ((c = *(*string_ptr)++) >= '0' && c <= '7')
	      {
		i *= 8;
		i += c - '0';
	      }
	    else
	      {
		(*string_ptr)--;
		break;
	      }
	  }
	return i;
      }
    default:
      return c;
    }
}

/* Print the character CH on STREAM as part of the contents
   of a literal string whose delimiter is QUOTER.  */

void
printchar (ch, stream, quoter)
     unsigned char ch;
     FILE *stream;
     int quoter;
{
  register int c = ch;
  if (c < 040 || c >= 0177)
    {
      if (c == '\n')
	ui_fprintf (stream, "\\n");
      else if (c == '\b')
	ui_fprintf (stream, "\\b");
      else if (c == '\t')
	ui_fprintf (stream, "\\t");
      else if (c == '\f')
	ui_fprintf (stream, "\\f");
      else if (c == '\r')
	ui_fprintf (stream, "\\r");
      else if (c == 033)
	ui_fprintf (stream, "\\e");
      else if (c == '\a')
	ui_fprintf (stream, "\\a");
      else
	ui_fprintf (stream, "\\%03o", c);
    }
  else
    {
      if (c == '\\' || c == quoter)
	ui_putc ('\\', stream);
      ui_putc (c, stream);
    }
}

static int lines_per_page, lines_printed, chars_per_line, chars_printed;

/* Set values of page and line size.  */
static void
set_screensize_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  char *p = arg;
  char *p1;
  int tolinesize = lines_per_page;
  int tocharsize = chars_per_line;

  if (!*p)
    error_no_arg ("set screensize");

  while (*p >= '0' && *p <= '9')
    p++;

  if (*p && *p != ' ' && *p != '\t')
    ui_badnews(-1,"Non-integral argument given to \"set screensize\".");

  tolinesize = atoi (arg);

  while (*p == ' ' || *p == '\t')
    p++;

  if (*p)
    {
      p1 = p;
      while (*p1 >= '0' && *p1 <= '9')
	p1++;

      if (*p1)
	ui_badnews(-1,"Non-integral second argument given to \"set screensize\".");

      tocharsize = atoi (p);
    }

  lines_per_page = tolinesize;
  chars_per_line = tocharsize;
}

static void
prompt_for_continue ()
{
#ifdef TEK_HACK
  char *reply;

 /* Allow help or info from user-defined commands */
  if (! instream) {
	chars_printed = lines_printed = 0;
        return;
  }
#endif /* TEK_HACK */
  immediate_quit++;
#ifdef TEK_HACK
  reply = gdb_readline ("---Type <return> to continue or <q> to stop output---", 1);
  if (reply == (char *)EOF || *reply == 'q' || *reply == 'Q')
     ui_badnews(-1, "Output stopped.");
#else
  gdb_readline ("---Type <return> to continue---", 0);
#endif /* TEK_HACK */
  chars_printed = lines_printed = 0;
  immediate_quit--;
}

/* Reinitialize filter; ie. tell it to reset to original values.  */

void
reinitialize_more_filter ()
{
  lines_printed = 0;
  chars_printed = 0;
}

static void
screensize_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (arg)
    ui_badnews(-1,"\"info screensize\" does not take any arguments.");
  
  if (!lines_per_page)
    ui_fprintf(stdout, "Output more filtering is disabled.\n");
  else
    {
      ui_fprintf(stdout, "Output more filtering is enabled with\n");
      ui_fprintf(stdout, "%d lines per page and %d characters per line.\n",
	      lines_per_page, chars_per_line);
    }
}

/* Like fputs but pause after every screenful.
   Unlike fputs, fputs_filtered does not return a value.

   Note that a longjmp to top level may occur in this routine
   (since prompt_for_continue may do so) so this routine should not be
   called when cleanups are not in place.  */

void
fputs_filtered (linebuffer, stream)
     char *linebuffer;
     FILE *stream;
{
  char *lineptr;

#ifdef TEK_HACK
  /* Don't do any filtering if it is disabled.  */
  /* Or if usingX DL 8/8/89 */
#endif /* TEK_HACK */
  if (lines_per_page == 0 
#ifdef TEK_HACK
	|| usingX
#endif /* TEK_HACK */
		)
    {
      ui_fputs (linebuffer, stream);
      return;
    }

  /* Go through and output each character.  Show line extension
     when this is necessary; prompt user for new page when this is
     necessary.  */
  
  lineptr = linebuffer;
#ifdef TEK_HACK
  while (lineptr && *lineptr)
#else
  while (*lineptr)
#endif /* TEK_HACK */
    {
      /* Possible new page.  */
      if (lines_printed >= lines_per_page - 1)
	prompt_for_continue ();

      while (*lineptr && *lineptr != '\n')
	{
	  /* Print a single line.  */
	  if (*lineptr == '\t')
	    {
	      ui_putc ('\t', stream);
	      /* Shifting right by 3 produces the number of tab stops
	         we have already passed, and then adding one and
		 shifting left 3 advances to the next tab stop.  */
	      chars_printed = ((chars_printed >> 3) + 1) << 3;
	      lineptr++;
	    }
	  else
	    {
	      ui_putc (*lineptr, stream);
	      chars_printed++;
	      lineptr++;
	    }
      
	  if (chars_printed >= chars_per_line)
	    {
	      chars_printed = 0;
	      lines_printed++;
	      /* Possible new page.  */
	      if (lines_printed >= lines_per_page - 1)
		prompt_for_continue ();
	    }
	}

      if (*lineptr == '\n')
	{
	  lines_printed++;
	  ui_putc ('\n', stream);
	  lineptr++;
	  chars_printed = 0;
	}
    }
}

/* Print ARG1, ARG2, and ARG3 on stdout using format FORMAT.  If this
   information is going to put the amount written since the last call
   to INIIALIZE_MORE_FILTER or the last page break over the page size,
   print out a pause message and do a gdb_readline to get the users
   permision to continue.

   Unlike fprintf, this function does not return a value.

   Note that this routine has a restriction that the length of the
   final output line must be less than 255 characters *or* it must be
   less than twice the size of the format string.  This is a very
   arbitrary restriction, but it is an internal restriction, so I'll
   put it in.  This means that the %s format specifier is almost
   useless; unless the caller can GUARANTEE that the string is short
   enough, fputs_filtered should be used instead.

   Note also that a longjmp to top level may occur in this routine
   (since prompt_for_continue may do so) so this routine should not be
   called when cleanups are not in place.  */

/*VARARGS2*/
void
fprintf_filtered (stream, format, arg1, arg2, arg3, arg4, arg5, arg6)
     FILE *stream;
     char *format;
     int arg1, arg2, arg3, arg4, arg5, arg6;
{
  static char *linebuffer = (char *) 0;
  static int line_size;
  int format_length = strlen (format);

  /* There are some situations in which we don't want to do paging.  */
#ifdef TEK_HACK
  if (stream != stdout || !ISATTY(stdout) || lines_per_page == 0 || usingX)
#else
  if (stream != stdout || !ISATTY(stdout) || lines_per_page == 0)
#endif /* TEK_HACK */
    {
      ui_fprintf (stream, format, arg1, arg2, arg3, arg4, arg5, arg6);
      return;
    }

  /* Allocated linebuffer for the first time.  */
  if (!linebuffer)
    {
      linebuffer = (char *) xmalloc (255);
      line_size = 255;
    }

  /* Reallocate buffer to a larger size if this is necessary.  */
  if (format_length * 2 > line_size)
    {
      line_size = format_length * 2;

      /* You don't have to copy.  */
      free (linebuffer);
      linebuffer = (char *) xmalloc (line_size);
    }

  /* This won't blow up if the restrictions described above are
     followed.   */
  (void) sprintf (linebuffer, format, arg1, arg2, arg3, arg4, arg5, arg6);

  fputs_filtered (linebuffer, stream);
}

/*VARARGS1*/
void
printf_filtered (format, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
     char *format;
     int arg1, arg2, arg3, arg4, arg5, arg6, arg7;
{
  fprintf_filtered (stdout, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}

/* Print N spaces.  */
void
print_spaces_filtered (n, stream)
     int n;
     FILE *stream;
{
  register char *s = (char *) alloca (n + 1);
  register char *t = s;

  while (n--)
    *t++ = ' ';
  *t = '\0';

  fputs_filtered (s, stream);
}


#ifdef USG
bcopy (from, to, count)
char *from, *to;
{
	memcpy (to, from, count);
}

bcmp (from, to, count)
{
	return (memcmp (to, from, count));
}

#ifndef _WIN32
bzero (to, count)
char *to;
{
	while (count--)
		*to++ = 0;
}
#endif

#ifdef	DG_HACK
getwoid (buf)
#else /* not DG_HACK */
getwd (buf)
#endif	/* not DG_HACK */
char *buf;
{
  getwd (buf, MAXPATHLEN);
}


#ifdef TEK_HACK
   /* strstr finds a string in a string.  Returns the location found
      in the searched-in string, or NULL if not found */
char *
strstr (s, c)
  char *s;           /* the string to find something in */
  char *c;           /* the string that's something to find */
{
  char *tc;          /* travel repeatedly down c */
  char *ts;          /* travel repeatedly down s */
  char *found = 0;   /* return true or false */
      
  if (*c == 0 || *s == 0) return 0;

  while (*(s))
    {
      tc = c; ts = s++;
       while (*(tc++) == *(ts++)) 
        if (!(*tc)) 
          {
              found = s;
              break;
          }
    }

  return found;
}
#endif /* TEK_HACK */

#ifndef _WIN32
char *
index (s, c)
     char *s;
{
  char *strchr ();
  return strchr (s, c);
}

char *
rindex (s, c)
     char *s;
{
  char *strrchr ();
  return strrchr (s, c);
}
#endif

#ifndef USG
char *sys_siglist[NSIG] = {
	"SIG0",
	"SIGHUP",
	"SIGINT",
	"SIGQUIT",
	"SIGILL",
	"SIGTRAP",
	"SIGIOT",
	"SIGEMT",
	"SIGFPE",
	"SIGKILL",
	"SIGBUS",
	"SIGSEGV",
	"SIGSYS",
	"SIGPIPE",
	"SIGALRM",
	"SIGTERM",
	"SIGUSR1",
	"SIGUSR2",
	"SIGCLD",
	"SIGPWR",
	"SIGWIND",
	"SIGPHONE",
	"SIGPOLL",
#ifdef m88k
        "SIGSTOP",
        "SIGTSTP",
        "SIGCONT",
        "SIGTTIN",
        "SIGTTOU",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "SIGURG",
        "SIGIO",
        "SIGXCPU",
        "SIGXFSZ",
        "SIGVTALRM",
        "SIGPROF",
        "UNDEFINED",
        "SIGLOST",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
        "UNDEFINED",
#endif /* m88k */
};
#endif

/* Queue routines */

struct queue {
	struct queue *forw;
	struct queue *back;
};

#ifdef	TEK_HACK
insqueue (item, after)
#else /* not TEK_HACK */
insque (item, after)
#endif /* not TEK_HACK */
struct queue *item;
struct queue *after;
{
	item->forw = after->forw;
	after->forw->back = item;

	item->back = after;
	after->forw = item;
}

#ifdef	TEK_HACK
remqueue (item)
#else /* not TEK_HACK */
remque (item)
#endif	/* not TEK_HACK */
struct queue *item;
{
	item->forw->back = item->back;
	item->back->forw = item->forw;
}
#endif /* USG */

#ifdef USG
/* There is too much variation in Sys V signal numbers and names, so
   we must initialize them at runtime.  */
static char undoc[] = "(undocumented)";

// char *sys_siglist[NSIG];
#endif /* USG */

extern struct cmd_list_element *setlist;
#ifdef TEK_PROG_HACK
extern struct cmd_list_element *set_cmd;
#endif /* TEK_PROG_HACK */

void
_initialize_utils ()
{
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;
#endif /* TEK_PROG_HACK */
  int i;
#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("screensize", class_support, set_screensize_command,
	   "Change gdb's notion of the size of the output screen.\n\
The first argument is the number of lines on a page.\n\
The second argument (optional) is the number of characters on a line.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */
  add_info ("screensize", screensize_info,
	    "Show gdb's current notion of the size of the output screen.");

  /* Maybe this should be initialized from termcap if possible?  */
  lines_per_page = 24;
  chars_per_line = 80;

#ifndef _WIN32
#ifdef USG
  /* Initialize signal names.  */
	sys_siglist[0] = "non-existent signal 0";
	for (i = 1; i < NSIG; i++)
		sys_siglist[i] = undoc;

#ifdef SIGHUP
	sys_siglist[SIGHUP	] = "SIGHUP";
#endif
#ifdef SIGINT
	sys_siglist[SIGINT	] = "SIGINT";
#endif
#ifdef SIGQUIT
	sys_siglist[SIGQUIT	] = "SIGQUIT";
#endif
#ifdef SIGILL
	sys_siglist[SIGILL	] = "SIGILL";
#endif
#ifdef SIGTRAP
	sys_siglist[SIGTRAP	] = "SIGTRAP";
#endif
#ifdef SIGIOT
	sys_siglist[SIGIOT	] = "SIGIOT";
#endif
#ifdef SIGEMT
	sys_siglist[SIGEMT	] = "SIGEMT";
#endif
#ifdef SIGFPE
	sys_siglist[SIGFPE	] = "SIGFPE";
#endif
#ifdef SIGKILL
	sys_siglist[SIGKILL	] = "SIGKILL";
#endif
#ifdef SIGBUS
	sys_siglist[SIGBUS	] = "SIGBUS";
#endif
#ifdef SIGSEGV
	sys_siglist[SIGSEGV	] = "SIGSEGV";
#endif
#ifdef SIGSYS
	sys_siglist[SIGSYS	] = "SIGSYS";
#endif
#ifdef SIGPIPE
	sys_siglist[SIGPIPE	] = "SIGPIPE";
#endif
#ifdef SIGALRM
	sys_siglist[SIGALRM	] = "SIGALRM";
#endif
#ifdef SIGTERM
	sys_siglist[SIGTERM	] = "SIGTERM";
#endif
#ifdef SIGUSR1
	sys_siglist[SIGUSR1	] = "SIGUSR1";
#endif
#ifdef SIGUSR2
	sys_siglist[SIGUSR2	] = "SIGUSR2";
#endif
#ifdef SIGCLD
	sys_siglist[SIGCLD	] = "SIGCLD";
#endif
#ifdef SIGCHLD
	sys_siglist[SIGCHLD	] = "SIGCHLD";
#endif
#ifdef SIGPWR
	sys_siglist[SIGPWR	] = "SIGPWR";
#endif
#ifdef SIGTSTP
	sys_siglist[SIGTSTP	] = "SIGTSTP";
#endif
#ifdef SIGTTIN
	sys_siglist[SIGTTIN	] = "SIGTTIN";
#endif
#ifdef SIGTTOU
	sys_siglist[SIGTTOU	] = "SIGTTOU";
#endif
#ifdef SIGSTOP
	sys_siglist[SIGSTOP	] = "SIGSTOP";
#endif
#ifdef SIGXCPU
	sys_siglist[SIGXCPU	] = "SIGXCPU";
#endif
#ifdef SIGXFSZ
	sys_siglist[SIGXFSZ	] = "SIGXFSZ";
#endif
#ifdef SIGVTALRM
	sys_siglist[SIGVTALRM	] = "SIGVTALRM";
#endif
#ifdef SIGPROF
	sys_siglist[SIGPROF	] = "SIGPROF";
#endif
#ifdef SIGWINCH
	sys_siglist[SIGWINCH	] = "SIGWINCH";
#endif
#ifdef SIGCONT
	sys_siglist[SIGCONT	] = "SIGCONT";
#endif
#ifdef SIGURG
	sys_siglist[SIGURG	] = "SIGURG";
#endif
#ifdef SIGIO
	sys_siglist[SIGIO	] = "SIGIO";
#endif
#ifdef SIGWIND
	sys_siglist[SIGWIND	] = "SIGWIND";
#endif
#ifdef SIGPHONE
	sys_siglist[SIGPHONE	] = "SIGPHONE";
#endif
#ifdef SIGPOLL
	sys_siglist[SIGPOLL	] = "SIGPOLL";
#endif
#endif /* USG */
#endif /* _WIN32 */
}
