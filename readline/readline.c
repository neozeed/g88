/* readline.c -- a general facility for reading lines of input
   with emacs style editing and completion. */

/* $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/readline/RCS/readline.c,v 1.7 90/02/20 11:40:33 andrew Exp $ */

/* Copyright (C) 1987,1989 Free Software Foundation, Inc.

   This file contains the Readline Library (the Library), a set of
   routines for providing Emacs style line input to programs that ask
   for it.

   The Library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   The Library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   The GNU General Public License is often shipped with GNU software, and
   is generally kept in a file called COPYING or LICENSE.  If you do not
   have a copy of the license, write to the Free Software Foundation,
   675 Mass Ave, Cambridge, MA 02139, USA. */



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

/* Remove these declarations when we have a complete libgnu.a. */
static char *xmalloc (), *xrealloc ();

#if defined (hpux)
#define HPUX
#endif

#include <stdio.h>
#include <sys/types.h>
#ifndef sony
#include <fcntl.h>
#endif
#include <sys/file.h>
#include <signal.h>

#ifdef __GNUC__
#define alloca __builtin_alloca
#else
#if defined (sparc) && defined (sun)
#include <alloca.h>
#endif
#endif

#ifdef SYSV
#include <termio.h>
#else
#include <sgtty.h>
#endif

#include <setjmp.h>

/* These next are for filename completion.  Perhaps this belongs
   in a different place. */
#include <sys/stat.h>

#include <pwd.h>
#ifndef	TEK_HACK
#ifdef SYSV
struct pwd *getpwuid (), *getpwent ();
#endif
#endif	/* not TEK_HACK */

#define HACK_TERMCAP_MOTION

#ifndef SYSV
#include <sys/dir.h>
#else  /* SYSV */
#ifdef HPUX
#include <ndir.h>
#else
#include <dirent.h>
#define direct dirent
#define d_namlen d_reclen
#endif  /* HPUX */
#endif  /* SYSV */

/* Some standard library routines. */
#include "readline.h"
#include "history.h"

#ifndef digit
#define digit(c)  ((c) >= '0' && (c) <= '9')
#endif

#ifndef isletter
#define isletter(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#endif

#ifndef digit_value
#define digit_value(c) ((c) - '0')
#endif

#ifndef member
char *index ();
#define member(c, s) ((c) ? index ((s), (c)) : 0)
#endif

static update_line ();
static void output_character_function ();
static delete_chars ();
static start_insert ();
static end_insert ();

#ifdef SIGWINCH
static int rl_handle_sigwinch ();
static Function *old_sigwinch = (Function *)NULL;
#endif

#if defined (NOJOBS) || defined (SYSV)
#ifndef TEK_HACK
#ifdef HANDLE_SIGNALS
#undef HANDLE_SIGNALS
#endif /* HANDLE_SIGNALS */
#endif /* TEK_HACK */
#endif



/* **************************************************************** */
/*								    */
/*			Line editing input utility		    */
/*								    */
/* **************************************************************** */

#include "keymaps.c"
#include "funmap.c"

/* A pointer to the keymap that is currently in use. */
Function **keymap = emacs_standard_keymap;

#define vi_mode 0
#define emacs_mode 1

/* The current style of editing. */
int rl_editing_mode = emacs_mode;

/* Non-zero if the previous command was a kill command. */
static int last_command_was_kill = 0;

/* The current value of the numeric argument specified by the user. */
int rl_numeric_arg = 1;

/* Non-zero if an argument was typed. */
int rl_explicit_arg = 0;

/* Temporary value used while generating the argument. */
static int arg_sign = 1;

/* Non-zero means we have been called at least once before. */
int rl_initialized = 0;

/* If non-zero, this program is running in an EMACS buffer. */
char *running_in_emacs = (char *)NULL;

/* The current offset in the current input line. */
int rl_point;

/* Length of the current input line. */
int rl_end;

/* Make this non-zero to return the current input_line. */
int rl_done;

/* The last function executed by readline. */
Function *rl_last_func = (Function *)NULL;

/* Top level environment for readline_internal (). */
static jmp_buf readline_top_level;

/* The streams we interact with. */
static FILE *in_stream, *out_stream;

/* The names of the streams that we do input and output to. */
FILE *rl_instream = stdin, *rl_outstream = stdout;

/* Non-zero means echo characters as they are read. */
int readline_echoing_p = 1;

/* Current prompt. */
char *rl_prompt;

/* What we use internally.  You should always refer to RL_LINE_BUFFER. */
static char *the_line;

/* Non-zero makes this the next keystroke to read. */
int rl_pending_input = 0;

/* Pointer to a useful terminal name. */
char *rl_terminal_name = (char *)NULL;

/* Line buffer and maintenence. */
char *rl_line_buffer = (char *)NULL;
static int rl_line_buffer_len = 0;
#define DEFAULT_BUFFER_SIZE 256


/* **************************************************************** */
/*								    */
/*			Top Level Functions			    */
/*								    */
/* **************************************************************** */

/* Read a line of input.  Prompt with PROMPT.  A NULL PROMPT means none. */
char *
readline (prompt)
     char *prompt;
{
  char *readline_internal ();
  char *value;

  rl_prompt = prompt;

  /* If we are at EOF that is what we return. */
  if (rl_pending_input == EOF) {
    rl_pending_input = 0;
    fputc('\n', rl_outstream);
    return ((char *)EOF);
  }
  
  rl_initialize ();
  rl_prep_terminal ();

#ifdef SIGWINCH
  old_sigwinch = (Function *)signal (SIGWINCH, rl_handle_sigwinch);
#endif

#ifdef HANDLE_SIGNALS
  rl_set_signals ();
#endif

  value = readline_internal ();
  rl_deprep_terminal ();

#ifdef SIGWINCH
  signal (SIGWINCH, old_sigwinch);
#endif

#ifdef HANDLE_SIGNALS
  rl_clear_signals ();
#endif

  if (value == (char *)EOF)
    fputc('\n', rl_outstream);
  return (value);
}

#ifdef SIGWINCH
static int
rl_handle_sigwinch (sig, code, scp)
     int sig, code;
     struct sigcontext *scp;
{
  char *term = rl_terminal_name, *getenv ();

  if (readline_echoing_p)
    {
      if (!term)
	term = getenv ("TERM");
      if (!term)
	term = "dumb";
      rl_reset_terminal (term);
      crlf ();
      rl_forced_update_display ();
    }

  if (old_sigwinch && old_sigwinch != (Function *)SIG_IGN &&
      old_sigwinch != (Function *)SIG_DFL)
    (*old_sigwinch)(sig, code, scp);
}
#endif  /* SIGWINCH */

#ifdef HANDLE_SIGNALS
/* Interrupt handling. */
static Function *old_int = (Function *)NULL,
	        *old_tstp = (Function *)NULL,
       		*old_ttou = (Function *)NULL,
       		*old_ttin = (Function *)NULL,
       	 	*old_cont = (Function *)NULL;
  
/* Handle an interrupt character. */
static
rl_signal_handler (sig, code, scp)
     int sig, code;
     struct sigcontext *scp;
{
  switch (sig) {

  case SIGINT:
    free_undo_list ();
    rl_clear_message ();
    rl_init_argument ();

#ifdef SIGTSTP
  case SIGTSTP:
  case SIGTTOU:
  case SIGTTIN:
#endif

    rl_clean_up_for_exit ();
    rl_deprep_terminal ();
    rl_clear_signals ();
    rl_pending_input = 0;

#ifdef BSD
#ifndef BSD4_1
    sigsetmask (0);
#endif
#endif
    kill (getpid (), sig);

    rl_prep_terminal ();
    rl_set_signals ();
  }
}

#ifdef TEK_HACK
rl_suspend_myself()
{
   rl_signal_handler(SIGTSTP);
}
#endif /* TEK_HACK */

rl_set_signals ()
{
  static int rl_signal_handler ();

  old_int = (Function *)signal (SIGINT, rl_signal_handler);
#ifdef TEK_HACK
  if (old_int == (Function *)SIG_DFL)
    signal (SIGINT, SIG_DFL);
#else
  if (old_int == (Function *)SIG_IGN)
    signal (SIGINT, SIG_IGN);
#endif /*TEK_HACK*/

#ifdef SIGTSTP
#ifdef TEK_HACK
  old_tstp = (Function *)signal (SIGTSTP, rl_signal_handler);
  if (old_tstp == (Function *)SIG_DFL)
    signal (SIGTSTP, SIG_DFL);
#else
  old_tstp = (Function *)signal (SIGTSTP, rl_signal_handler);
  if (old_tstp == (Function *)SIG_IGN)
    signal (SIGTSTP, SIG_IGN);
#endif /* TEK_HACK */
#endif /* SIGTSTP */
#ifdef SIGTTOU
  old_ttou = (Function *)signal (SIGTTOU, rl_signal_handler);
  old_ttin = (Function *)signal (SIGTTIN, rl_signal_handler);
#endif
}

rl_clear_signals ()
{
  signal (SIGINT, old_int);

#ifdef SIGTSTP
  signal (SIGTSTP, old_tstp);
#endif
#ifdef SIGTTOU
  signal (SIGTTOU, old_ttou);
  signal (SIGTTIN, old_ttin);
#endif
}
#endif  /* HANDLE_SIGNALS */

/* Read a line of input from the global rl_instream, doing output on
   the global rl_outstream.
   If rl_prompt is non-null, then that is our prompt. */
char *
readline_internal ()
{
  int c, i;

  in_stream = rl_instream; out_stream = rl_outstream;

  if (!readline_echoing_p)
    {
      if (rl_prompt)
	fprintf (out_stream, "%s", rl_prompt);
    }
  else
    {
      rl_on_new_line ();
      rl_redisplay ();
      if (rl_editing_mode == vi_mode)
	rl_vi_insertion_mode ();
    }

  while (!rl_done)
    {
      int lk = last_command_was_kill;
      int code = setjmp (readline_top_level);
      if (code) rl_redisplay ();

      if (!rl_pending_input)
	{
	  /* Then initialize the argument. */
	  rl_init_argument ();
	}

      c = rl_read_key ();

      /* If the guts return EOF, then the line has been deleted, so there's
	 nothing to return and no reason to dispatch.  Adding this "if"
	 keeps control-D from acting like newline, control-D. */
      if (rl_pending_input == EOF) {
	rl_pending_input = 0;
	return (char *)EOF;
      }

      rl_dispatch (c, keymap);

      /* If there was no change in last_command_was_kill, then no kill
	 has taken place.  Note that if input is pending we are reading
	 a prefix command, so nothing has changed yet. */
      if (!rl_pending_input)
	{
	  if (lk == last_command_was_kill)
	    last_command_was_kill = 0;
	}
      if (!rl_done)
	rl_redisplay ();
    }

  /* Restore the original of this history line, iff the line that we
     are editing was originally in the history, AND the line has changed. */
  {
    HIST_ENTRY *entry = current_history ();

    if (entry && the_undo_list)
      {
	char *temp = savestring (the_line);
	rl_revert_line ();
	entry = replace_history_entry (where_history (), the_line, (HIST_ENTRY *)NULL);
	free_history_entry (entry);
      

	strcpy (the_line, temp);
	free (temp);
      }
  }

  /* At any rate, it is highly likely that this line has an undo list.  Get
     rid of it now. */
  if (the_undo_list)
    free_undo_list ();

  return (savestring (the_line));
}

/* Read a key, including pending input. */
rl_read_key ()
{
  int c;
  if (rl_pending_input)
    {
      c = rl_pending_input;
      rl_pending_input = 0;
    }
  else
    c = rl_getc (in_stream);

  /* In the case of EOF end this input line. */
  if (c == EOF)
    {
      rl_pending_input = EOF;
      c = NEWLINE;
    }
  return (c);
}

/* Do the command associated with KEY. */
rl_dispatch (key)
     int key;
{
  register int i;

  if (keymap[key]) (*keymap[key])(rl_numeric_arg * arg_sign, key);
  else ding ();

  /* If we have input pending, then the last command was a prefix
     command.  Don't change the state of rl_last_func. */
  if (!rl_pending_input)
    rl_last_func = keymap[key];
}

/* Initliaze readline (and terminal if not already). */
rl_initialize ()
{
  extern char *rl_display_prompt;

  /* If we have never been called before, initialize the
     terminal and data structures. */
  if (!rl_initialized)
    {
      readline_initialize_everything ();
      rl_initialized++;
    }

  /* Initalize the current line information. */
  rl_point = rl_end = 0;
  the_line = rl_line_buffer;
  the_line[0] = 0;

  /* We aren't done yet.  We haven't even gotten started yet! */
  rl_done = 0;

  /* Tell the history routines what is going on. */
  start_using_history ();

  /* Make the display buffer match the state of the line. */
  {
    extern char *rl_display_prompt;
    extern int forced_display;

    rl_on_new_line ();

    rl_display_prompt = rl_prompt ? rl_prompt : "";
    forced_display = 1;
  }

  /* No such function typed yet. */
  rl_last_func = (Function *)NULL;
}

/* Initialize the entire state of the world. */
readline_initialize_everything ()
{
  /* Find out if we are running in Emacs. */
  running_in_emacs = (char *)getenv ("EMACS");

  /* Allocate data structures. */
  if (!rl_line_buffer)
    rl_line_buffer =
      (char *)xmalloc (rl_line_buffer_len = DEFAULT_BUFFER_SIZE);

  /* Initialize the terminal interface. */
  init_terminal_io ((char *)NULL);

  /* Initialize default key bindings. */
  readline_default_bindings ();

  /* Initialize the function names. */
  rl_initialize_funmap ();

  /* Read in the init file. */
  rl_read_init_file ((char *)NULL);

  /* If the completion parser's default word break characters haven't
     been set yet, then do so now. */
  {
    extern char *rl_completer_word_break_characters;
    extern char *rl_basic_word_break_characters;

    if (rl_completer_word_break_characters == (char *)NULL)
      rl_completer_word_break_characters = rl_basic_word_break_characters;
  }
}

/* Initialize the key bindings to the defaults. */
readline_default_bindings ()
{
  register int i;
  extern rl_insert ();

  /* All printing characters are self-inserting. */
  for (i = ' '; i < 127; i++)
    keymap[i] = rl_insert;

  /* All lowercase meta characters run  their uppercase equivs. */
  for (i = META('a'); i != META('z'); i++)
    keymap[i] = rl_do_uppercase_version;

  /* All meta-digits run rl_digit_argument (). */
  for (i = META('0'); i <= META('9'); i++)
    keymap[i] = rl_digit_argument;
  keymap[META('-')] = rl_digit_argument;

  keymap[CTRL('_')] = rl_do_undo;
  keymap[CTRL('A')] = rl_beg_of_line;
  keymap[CTRL('B')] = rl_backward;
  keymap[CTRL('D')] = rl_delete;
  keymap[CTRL('E')] = rl_end_of_line;
  keymap[CTRL('F')] = rl_forward;
  keymap[CTRL('G')] = rl_abort;
  keymap[CTRL('H')] = rl_backward;
  keymap[CTRL('I')] = rl_complete;
  keymap[NEWLINE] = rl_newline;
  keymap[CTRL('K')] = rl_kill_line;
  keymap[CTRL('L')] = rl_clear_screen;
  keymap[RETURN] = rl_newline;
  keymap[CTRL('N')] = rl_get_next_history;
  keymap[CTRL('P')] = rl_get_previous_history;
  keymap[CTRL('Q')] = rl_quoted_insert;
  keymap[CTRL('R')] = rl_reverse_search_history;
  keymap[CTRL('T')] = rl_transpose_chars;
  keymap[CTRL('U')] = rl_unix_line_discard;
  keymap[CTRL('V')] = rl_quoted_insert;
  keymap[CTRL('W')] = rl_unix_word_rubout;
  keymap[CTRL('Y')] = rl_yank;
#ifdef TEK_HACK
  keymap[CTRL('Z')] = rl_suspend_myself;
#endif /* TEK_HACK */
  keymap[ESC] = rl_prefix_meta;

  keymap[RUBOUT] = rl_rubout;

  keymap[META(CTRL('R'))] = rl_revert_line;
  keymap[META('<')] = rl_beginning_of_history;
  keymap[META('>')] = rl_end_of_history;
  keymap[META('B')] = rl_backward_word;
  keymap[META('C')] = rl_capitalize_word;
  keymap[META('D')] = rl_kill_word;
  keymap[META('F')] = rl_forward_word;
  keymap[META('I')] = rl_tab_insert;
  keymap[META('L')] = rl_downcase_word;
  keymap[META('R')] = rl_revert_line;
  keymap[META('U')] = rl_upcase_word;
  keymap[META('T')] = rl_transpose_words;
  keymap[META('Y')] = rl_yank_pop;
  keymap[META('?')] = rl_possible_completions;
  keymap[META(RUBOUT)] = rl_backward_kill_word;
}

/* **************************************************************** */
/*								    */
/*			Numeric Arguments			    */
/*								    */
/* **************************************************************** */

/* Handle C-u style numeric args, as well as M--, and M-digits. */

/* Add the current digit to the argument in progress. */
rl_digit_argument (ignore, key)
     int ignore, key;
{
  rl_pending_input = key;
  rl_digit_loop ();
}

/* What to do when you abort reading an argument. */
rl_discard_argument ()
{
  ding ();
  rl_clear_message ();
  rl_init_argument ();
}

/* Create a default argument. */
rl_init_argument ()
{
  rl_numeric_arg = arg_sign = 1;
  rl_explicit_arg = 0;
}

/* C-u, universal argument.  Multiply the current argument by 4.
   Read a key.  If the key has nothing to do with arguments, then
   dispatch on it.  If the key is the abort character then abort. */
rl_universal_argument ()
{
  rl_numeric_arg *= 4;
  return (rl_digit_loop ());
}

rl_digit_loop ()
{
  int key, c;
  while (1) {
    rl_message ("(arg: %d) ", arg_sign * rl_numeric_arg);
    key = c = rl_read_key ();

    if (keymap[c] == rl_universal_argument) {
      rl_numeric_arg *= 4;
      continue;
    }
    c = UNMETA (c);
    if (numeric (c)) {
      if (rl_explicit_arg)
	rl_numeric_arg = (rl_numeric_arg * 10) + (c - '0');
      else
	rl_numeric_arg = (c - '0');
      rl_explicit_arg = 1;
    } else {
      if (c == '-' && !rl_explicit_arg) {
	rl_numeric_arg = 1;
	arg_sign = -1;
      } else {
	rl_clear_message ();
	rl_dispatch (key);
	return;
      }
    }
  }
}

/* **************************************************************** */
/*								    */
/*			Display stuff				    */
/*								    */
/* **************************************************************** */

/* This is the stuff that is hard for me.  I never seem to write good
   display routines in C.  Let's see how I do this time. */

/* (PWP) Well... Good for a simple line updater, but totally ignores
   the problems of input lines longer than the screen width.

   update_line and the code that calls it makes a multiple line,
   automatically wrapping line update.  Carefull attention needs
   to be paid to the vertical position variables.

   handling of terminals with autowrap on (incl. DEC braindamage)
   could be improved a bit.  Right now I just cheat and decrement
   screenwidth by one. */

/* Keep two buffers; one which reflects the current contents of the
   screen, and the other to draw what we think the new contents should
   be.  Then compare the buffers, and make whatever changes to the
   screen itself that we should.  Finally, make the buffer that we
   just drew into be the one which reflects the current contents of the
   screen, and place the cursor where it belongs.

   Commands that want to can fix the display themselves, and then let
   this function know that the display has been fixed by setting the
   RL_DISPLAY_FIXED variable.  This is good for efficiency. */

/* Termcap variables: */
extern char *term_up, *term_dc, *term_cr;
extern int screenheight, screenwidth, terminal_can_insert;

/* What YOU turn on when you have handled all redisplay yourself. */
int rl_display_fixed = 0;

/* The visible cursor position.  If you print some text, adjust this. */
int last_c_pos = 0;
int last_v_pos = 0;

/* The last left edge of text that was displayed.  This is used when
   doing horizontal scrolling.  It shifts in thirds of a screenwidth. */
int last_lmargin = 0;

char *visible_line = (char *)NULL;
char *invisible_line = (char *)NULL;

/* Number of lines currently on screen minus 1. */
int vis_botlin = 0;

/* A buffer for `modeline' messages. */
char msg_buf[128];

/* Non-zero forces the redisplay even if we thought it was unnecessary. */
int forced_display = 0;

/* The stuff that gets printed out before the actual text of the line.
   This is usually pointing to rl_prompt. */
char *rl_display_prompt = (char *)NULL;

/* Default and initial buffer size.  Can grow. */
static int line_size = 1024;

/* Non-zero means to always use horizontal scrolling in line display. */
int horizontal_scroll_mode = 0;

/* I really disagree with this, but my boss (among others) insists that we
   support compilers that don't work.  I don't think we are gaining by doing
   so; what is the advantage in producing better code if we can't use it? */
/* The following two declarations belong inside the
   function block, not here. */
static void move_cursor_relative ();
static void output_some_chars ();

/* Basic redisplay algorithm. */
rl_redisplay ()
{
  register int in, out, c, linenum;
  register char *line = invisible_line;
  int c_pos = 0, v_pos = 0;
  int inv_botlin = 0;		/* Number of lines in newly drawn buffer. */

  extern int readline_echoing_p;

  if (!readline_echoing_p)
    return;
  
  if (!rl_display_prompt)
    rl_display_prompt = "";

  if (!invisible_line)
    {
      visible_line = (char *)xmalloc (line_size);
      invisible_line = (char *)xmalloc (line_size);
      line = invisible_line;
      for (in = 0; in < line_size; in++)
	{
	  visible_line[in] = 0;
	  invisible_line[in] = 1;
	}
      rl_on_new_line ();
    }

  /* Draw the line into the buffer. */
  c_pos = -1;

  /* Mark the line as modified or not.  We only do this for history
     lines. */
  out = 0;
  if (current_history () && the_undo_list)
    {
      line[out++] = '*';
      line[out] = '\0';
    }

  /* If someone thought that the redisplay was handled, but the currently
     visible line has a different modification state than the one about
     to become visible, then correct the callers misconception. */
  if (visible_line[0] != invisible_line[0])
    rl_display_fixed = 0;
  
  strncpy (line + out,  rl_display_prompt, strlen (rl_display_prompt));
  out += strlen (rl_display_prompt);
  line[out] = '\0';

  for (in = 0; in < rl_end; in++)
    {
      c = the_line[in];

      if (out + 1 >= line_size)
	{
	  line_size *= 2;
	  visible_line = (char *)xrealloc (visible_line, line_size);
	  invisible_line = (char *)xrealloc (invisible_line, line_size);
	  line = invisible_line;
	}

      if (in == rl_point)
	c_pos = out;

      if (c > 127)
	{
	  line[out++] = 'M';
	  line[out++] = '-';
	  line[out++] = c - 128;
	}
#define DISPLAY_TABS
#ifdef DISPLAY_TABS
      else if (c == '\t')
	{
	  register int newout = (out | (int)7) + 1;
	  while (out < newout)
	    line[out++] = ' ';
	}
#endif
      else if (c < 32)
	{
	  line[out++] = 'C';
	  line[out++] = '-';
	  line[out++] = c + 64;
	}
      else
	line[out++] = c;
    }
  line[out] = '\0';
  if (c_pos < 0)
    c_pos = out;
  
  /* PWP: now is when things get a bit hairy.  The visible and invisible
     line buffers are really multiple lines, which would wrap every
     (screenwidth - 1) characters.  Go through each in turn, finding
     the changed region and updating it.  The line order is top to bottom. */

  /* If we can move the cursor up and down, then use multiple lines,
     otherwise, let long lines display in a single terminal line, and
     horizontally scroll it. */

  if (!horizontal_scroll_mode && term_up && *term_up)
    {
      int total_screen_chars = (screenwidth * screenheight);

      if (!rl_display_fixed || forced_display)
	{
	  forced_display = 0;

	  /* If we have more than a screenful of material to display, then
	     only display a screenful.  We should display the last screen,
	     not the first.  I'll fix this in a minute. */
	  if (out >= total_screen_chars)
	    out = total_screen_chars - 1;

	  /* Number of screen lines to display. */
	  inv_botlin = out / screenwidth;

	  /* For each line in the buffer, do the updating display. */
	  for (linenum = 0; linenum <= inv_botlin; linenum++)
	    update_line (&visible_line[linenum * screenwidth],
			 &invisible_line[linenum * screenwidth],
			 linenum);

	  /* We may have deleted some lines.  If so, clear the left over
	     blank ones at the bottom out. */
	  if (vis_botlin > inv_botlin)
	    {
	      char *tt;
	      for (; linenum <= vis_botlin; linenum++)
		{
		  tt = &visible_line[linenum * screenwidth];
		  move_vert (linenum);
		  move_cursor_relative (0, tt);
		  clear_to_eol ((linenum == vis_botlin)?
				strlen (tt) : screenwidth);
		}
	    }
	  vis_botlin = inv_botlin;

	  /* Move the cursor where it should be. */
	  move_vert (c_pos / screenwidth);
	  move_cursor_relative (c_pos % screenwidth,
				&invisible_line[(c_pos / screenwidth) * screenwidth]);
	}
    }
  else				/* Do horizontal scrolling. */
    {
      int lmargin;

      /* Always at top line. */
      last_v_pos = 0;

      /* If the display position of the cursor would be off the edge
	 of the screen, start the display of this line at an offset that
	 leaves the cursor on the screen. */
      if (c_pos - last_lmargin > screenwidth - 2)
	lmargin = (c_pos / (screenwidth / 3) - 2) * (screenwidth / 3);
      else if (c_pos - last_lmargin < 1)
	lmargin = ((c_pos - 1) / (screenwidth / 3)) * (screenwidth / 3);
      else
	lmargin = last_lmargin;

      /* If the first character on the screen isn't the first character
	 in the display line, indicate this with a special character. */
      if (lmargin > 0)
	line[lmargin] = '<';

      if (lmargin + screenwidth < out)
	line[lmargin + screenwidth - 1] = '>';

      if (!rl_display_fixed || forced_display || lmargin != last_lmargin)
	{
	  forced_display = 0;
	  update_line (&visible_line[last_lmargin],
		       &invisible_line[lmargin], 0);

	  move_cursor_relative (c_pos - lmargin, &invisible_line[lmargin]);
	  last_lmargin = lmargin;
	}
    }
  fflush (out_stream);

  /* Swap visible and non-visible lines. */
  {
    char *temp = visible_line;
    visible_line = invisible_line;
    invisible_line = temp;
    rl_display_fixed = 0;
  }
}

/* PWP: update_line() is based on finding the middle difference of each
   line on the screen; vis:

			     /old first difference
	/beginning of line   |              /old last same       /old EOL
	v		     v              v                    v
old:	eddie> Oh, my little gruntle-buggy is to me, as lurgid as
new:	eddie> Oh, my little buggy says to me, as lurgid as
	^		     ^        ^			   ^
	\beginning of line   |        \new last same	   \new end of line
			     \new first difference

   All are character pointers for the sake of speed.  Special cases for
   no differences, as well as for end of line additions must be handeled.

   Could be made even smarter, but this works well enough */
static
update_line (old, new, current_line)
     register char *old, *new;
     int current_line;
{
  register char *ofd, *ols, *oe, *nfd, *nls, *ne;
  int i, lendiff, wsatend;

  /* Find first difference. */
  for (ofd = old, nfd = new;
       (ofd - old < screenwidth) && *ofd && (*ofd == *nfd);
       ofd++, nfd++)
    ;

  /* Move to the end of the screen line. */
  for (oe = ofd; ((oe - old) < screenwidth) && *oe; oe++);
  for (ne = nfd; ((ne - new) < screenwidth) && *ne; ne++);

  /* If no difference, continue to next line. */
  if (ofd == oe && nfd == ne)
    return;

  wsatend = 1;			/* flag for trailing whitespace */
  ols = oe - 1;			/* find last same */
  nls = ne - 1;
  while ((*ols == *nls) && (ols > ofd) && (nls > nfd))
    {
      if (*ols != ' ')
	wsatend = 0;
      ols--;
      nls--;
    }

  if (wsatend)
    {
      ols = oe;
      nls = ne;
    }
  else
    {
      if (*ols)			/* don't step past the NUL */
	ols++;
      if (*nls)
	nls++;
    }

  move_vert (current_line);
  move_cursor_relative (ofd - old, old);

  /* if (len (new) > len (old)) */
  lendiff = (nls - nfd) - (ols - ofd);

  /* Insert (diff(len(old),len(new)) ch */
  if (lendiff > 0)
    {
      if (terminal_can_insert)
	{
	  extern char *term_IC;

	  /* Sometimes it is cheaper to print the characters rather than
	     use the terminal's capabilities. */
	  if ((2 * (ne - nfd)) < lendiff && (!term_IC || !*term_IC))
	    {
	      output_some_chars (nfd, (ne - nfd));
	      last_c_pos += (ne - nfd);
	    }
	  else
	    {
	      if (*ols)
		{
		  start_insert (lendiff);
		  output_some_chars (nfd, lendiff);
		  last_c_pos += lendiff;
		  end_insert ();
		}
	      else
		{
		  /* At the end of a line the characters do not have to
		     be "inserted".  They can just be placed on the screen. */
		  output_some_chars (nfd, lendiff);
		  last_c_pos += lendiff;
		}
	      /* Copy (new) chars to screen from first diff to last match. */
	      if (((nls - nfd) - lendiff) > 0)
		{
		  output_some_chars (&nfd[lendiff], ((nls - nfd) - lendiff));
		  last_c_pos += ((nls - nfd) - lendiff);
		}
	    }
	}
      else
	{		/* cannot insert chars, write to EOL */
	  output_some_chars (nfd, (ne - nfd));
	  last_c_pos += (ne - nfd);
	}
    }
  else				/* Delete characters from line. */
    {
      /* If possible and inexpensive to use terminal deletion, then do so. */
      if (term_dc && (2 * (ne - nfd)) >= (-lendiff))
	{
	  if (lendiff)
	    delete_chars (-lendiff); /* delete (diff) characters */

	  /* Copy (new) chars to screen from first diff to last match */
	  if ((nls - nfd) > 0)
	    {
	      output_some_chars (nfd, (nls - nfd));
	      last_c_pos += (nls - nfd);
	    }
	}
      /* Otherwise, print over the existing material. */
      else
	{
	  output_some_chars (nfd, (ne - nfd));
	  last_c_pos += (ne - nfd);
	  clear_to_eol ((oe - old) - (ne - new));
	}
    }
}

/* (PWP) tell the update routines that we have moved onto a
   new (empty) line. */
rl_on_new_line ()
{
  if (visible_line)
    visible_line[0] = '\0';

  last_c_pos = last_v_pos = 0;
  vis_botlin = last_lmargin = 0;
}

/* Actually update the display, period. */
rl_forced_update_display ()
{
  if (visible_line)
    {
      register char *temp = visible_line;

      while (*temp) *temp++ = '\0';
    }
  rl_on_new_line ();
  forced_display++;
  rl_redisplay ();
}

/* Move the cursor from last_c_pos to NEW, which are buffer indices.
   DATA is the contents of the screen line of interest; i.e., where
   the movement is being done. */
static void
move_cursor_relative (new, data)
     int new;
     char *data;
{
  register int i;

  if (last_c_pos == new) return;

  if (last_c_pos < new)
    {
      /* Move the cursor forward.  We do it by printing the command
	 to move the cursor forward if there is one, else print that
	 portion of the output buffer again.  Which is cheaper? */

      /* The above comment is left here for posterity.  It is faster
	 to print one character (non-control) than to print a control
	 sequence telling the terminal to move forward one character.
	 That kind of control is for people who don't know what the
	 data is underneath the cursor. */
#ifdef HACK_TERMCAP_MOTION
      extern char *term_forward_char;
      static void output_character_function ();

      if (term_forward_char)
	for (i = last_c_pos; i < new; i++)
	  tputs (term_forward_char, 1, output_character_function);
      else
	for (i = last_c_pos; i < new; i++)
	  putc (data[i], out_stream);
#else
      for (i = last_c_pos; i < new; i++)
	putc (data[i], out_stream);
#endif				/* HACK_TERMCAP_MOTION */
    }
  else
    backspace (last_c_pos - new);
  last_c_pos = new;
}

/* PWP: move the cursor up or down. */
move_vert (to)
     int to;
{
  void output_character_function ();
  register int delta, i;

  if (last_v_pos == to) return;

  if (to > screenheight)
    return;

  if ((delta = to - last_v_pos) > 0)
    {
      for (i = 0; i < delta; i++)
	putc ('\n', out_stream);
      tputs (term_cr, 1, output_character_function);
      last_c_pos = 0;		/* because crlf() will do \r\n */
    }
  else
    {			/* delta < 0 */
      if (term_up && *term_up)
	for (i = 0; i < -delta; i++)
	  tputs (term_up, 1, output_character_function);
    }
  last_v_pos = to;		/* now to is here */
}

/* Physically print C on out_stream.  This is for functions which know
   how to optimize the display. */
rl_show_char (c)
     int c;
{
  if (c > 127)
    {
      fprintf (out_stream, "M-");
      c -= 128;
    }

#ifdef DISPLAY_TABS
  if (c < 32 && c != '\t')
#else
  if (c < 32)
#endif
    {
      fprintf (out_stream, "C-");
      c += 64;
    }

  putc (c, out_stream);
  fflush (out_stream);
}

#ifdef DISPLAY_TABS
rl_character_len (c, pos)
     register int c, pos;
{
  if (c < ' ' || c > 126)
    {
      if (c == '\t')
	return (((pos | (int)7) + 1) - pos);
      else
	return (3);
    }
  else
    return (1);
}
#else
rl_character_len (c)
     int c;
{
  if (c < ' ' || c > 126)
    return (3);
  else
    return (1);
}
#endif  /* DISPLAY_TAB */

/* How to print things in the "echo-area".  The prompt is treated as a
   mini-modeline. */
rl_message (string, arg1, arg2)
     char *string;
{
  sprintf (msg_buf, string, arg1, arg2);
  rl_display_prompt = msg_buf;
  rl_redisplay ();
}

/* How to clear things from the "echo-area". */
rl_clear_message ()
{
  rl_display_prompt = rl_prompt;
  rl_redisplay ();
}

/* **************************************************************** */
/*								    */
/*			Terminal and Termcap			    */
/*								    */
/* **************************************************************** */

static char *term_buffer = (char *)NULL;
static char *term_string_buffer = (char *)NULL;

/* Non-zero means this terminal can't really do anything. */
int dumb_term = 0;

char PC;
char *BC, *UP;

/* Some strings to control terminal actions.  These are output by tputs (). */
char *term_goto, *term_clreol, *term_cr, *term_clrpag, *term_backspace;

int screenwidth, screenheight;

/* Non-zero if we determine that the terminal can do character insertion. */
int terminal_can_insert = 0;

/* How to insert characters. */
char *term_im, *term_ei, *term_ic, *term_ip, *term_IC;

/* How to delete characters. */
char *term_dc, *term_DC;

#ifdef HACK_TERMCAP_MOTION
char *term_forward_char;
#endif  /* HACK_TERMCAP_MOTION */

/* How to go up a line. */
char *term_up;

/* Re-initialize the terminal considering that the TERM/TERMCAP variable
   has changed. */
rl_reset_terminal (terminal_name)
     char *terminal_name;
{
  init_terminal_io (terminal_name);
}

init_terminal_io (terminal_name)
     char *terminal_name;
{
  char *term = (terminal_name? terminal_name : (char *)getenv ("TERM"));
  char *tgetstr (), *buffer;


  if (!term_string_buffer)
    term_string_buffer = (char *)xmalloc (2048);

  if (!term_buffer)
    term_buffer = (char *)xmalloc (2048);

  buffer = term_string_buffer;

  term_clrpag = term_cr = term_clreol = (char *)NULL;

  if (!term)
    term = "dumb";

  if (tgetent (term_buffer, term) < 0)
    {
      dumb_term = 1;
      return;
    }

  BC = tgetstr ("pc", &buffer);
  PC = buffer ? *buffer : 0;

  term_backspace = tgetstr ("le", &buffer);

  term_cr = tgetstr ("cr", &buffer);
  term_clreol = tgetstr ("ce", &buffer);
  term_clrpag = tgetstr ("cl", &buffer);

  if (!term_cr)
    term_cr =  "\r";

#ifdef HACK_TERMCAP_MOTION
  term_forward_char = tgetstr ("nd", &buffer);
#endif  /* HACK_TERMCAP_MOTION */

  screenwidth = tgetnum ("co");
  if (screenwidth <= 0)
    screenwidth = 80;
  screenwidth--;		/* PWP: avoid autowrap bugs */

  screenheight = tgetnum ("li");
  if (screenheight <= 0)
    screenheight = 24;

  term_im = tgetstr ("im", &buffer);
  term_ei = tgetstr ("ei", &buffer);
  term_IC = tgetstr ("IC", &buffer);
  term_ic = tgetstr ("ic", &buffer);
  term_ip = tgetstr ("ip", &buffer);
  term_IC = tgetstr ("IC", &buffer);

  /* "An application program can assume that the terminal can do
      character insertion if *any one of* the capabilities `IC',
      `im', `ic' or `ip' is provided." */
  terminal_can_insert = (term_IC || term_im || term_ic || term_ip);

  term_up = tgetstr ("up", &buffer);
  term_dc = tgetstr ("dc", &buffer);
  term_DC = tgetstr ("DC", &buffer);
}

/* A function for the use of tputs () */
static void
output_character_function (c)
     int c;
{
  putc (c, out_stream);
}

/* Write COUNT characters from STRING to the output stream. */
static void
output_some_chars (string, count)
     char *string;
     int count;
{
  fwrite (string, 1, count, out_stream);
}


/* Delete COUNT characters from the display line. */
static
delete_chars (count)
     int count;
{
  if (count > screenwidth)
    return;

  if (term_DC && *term_DC)
    {
      char *tgoto (), *buffer;
      buffer = tgoto (term_DC, 0, count);
      tputs (buffer, 1, output_character_function);
    }
  else
    {
      if (term_dc && *term_dc)
	while (count--)
	  tputs (term_dc, 1, output_character_function);
    }
}

/* Prepare to insert by inserting COUNT blank spaces. */
static
start_insert (count)
     int count;
{
  if (term_IC && *term_IC)
    {
      char *tgoto (), *buffer;
      buffer = tgoto (term_IC, 0, count);
      tputs (buffer, 1, output_character_function);
    }
  else
    {
      if (term_im && *term_im)
	tputs (term_im, 1, output_character_function);

      if (term_ic && *term_ic)
	while (count--)
	  tputs (term_ic, 1, output_character_function);
    }
}

/* We are finished doing our insertion.  Send ending string. */
static
end_insert ()
{
  if (term_ei && *term_ei)
    tputs (term_ei, 1, output_character_function);
}

/* Move the cursor back. */
backspace (count)
     int count;
{
  register int i;

  if (term_backspace)
    for (i = 0; i < count; i++)
      tputs (term_backspace, 1, output_character_function);
  else
    for (i = 0; i < count; i++)
      putc ('\b', out_stream);
}

/* Move to the start of the next line. */
crlf ()
{
  tputs (term_cr, 1, output_character_function);
  putc ('\n', out_stream);
}

/* Clear to the end of the line.  COUNT is the minimum 
   number of character spaces to clear, */
clear_to_eol (count)
     int count;
{
  if (term_clreol) {
    tputs (term_clreol, 1, output_character_function);
  } else {
    register int i;
    /* Do one more character space. */
    count++;
    for (i = 0; i < count; i++)
      putc (' ', out_stream);
    backspace (count);
  }
}

#if defined(SYSV) || defined(HPUX)
static struct termio otio;

static
rl_prep_terminal ()
{
  int tty = open ("/dev/tty", O_RDONLY);
  struct termio tio;
  ioctl (tty, TCGETA, &tio);
  ioctl (tty, TCGETA, &otio);
#ifdef HANDLE_SIGNALS
  tio.c_lflag &= ~(ICANON|ECHO);
#else
  tio.c_iflag &= ~(IXON|ISTRIP|INPCK);
  tio.c_lflag &= ~(ICANON|ISIG|ECHO);
#endif
  tio.c_cc[VEOF] = 1;		/* really: MIN */
  tio.c_cc[VEOL] = 0;		/* really: TIME */
  ioctl (tty, TCSETAW,&tio);
  close (tty);
}

static
rl_deprep_terminal ()
{
  int tty = open ("/dev/tty", O_RDONLY);
  ioctl (tty, TCSETAW, &otio);
  close (tty);
}
#else

int original_tty_flags = 0;
struct sgttyb the_ttybuff;

/* Put the terminal in CBREAK mode so that we can detect key presses. */
rl_prep_terminal ()
{
  int tty = fileno (rl_instream);

  /* We always get the latest tty values.  Maybe stty changed them. */

  ioctl (tty, TIOCGETP, &the_ttybuff);
  original_tty_flags = the_ttybuff.sg_flags;

  if (!(original_tty_flags & ECHO))
    readline_echoing_p = 0;
  else
    {
      readline_echoing_p = 1;
      the_ttybuff.sg_flags &= ~ECHO;
    }
  the_ttybuff.sg_flags |= CBREAK;
  ioctl (tty, TIOCSETN, &the_ttybuff);
}

/* Restore the terminal to its original state. */
rl_deprep_terminal ()
{
  int tty = fileno (rl_instream);

  the_ttybuff.sg_flags = original_tty_flags;
  ioctl (tty, TIOCSETN, &the_ttybuff);
  readline_echoing_p = 1;
}
#endif  /* SYSV || HPUX */


/* **************************************************************** */
/*								    */
/*			Utility Functions			    */
/*								    */
/* **************************************************************** */

/* Return 0 if C is not a member of the class of characters that belong
   in words, or 1 if it is. */

int allow_pathname_alphabetic_chars = 0;
char *pathname_alphabetic_chars = "/-_=~.#$";

alphabetic (c)
     int c;
{
  char *rindex ();
  if (pure_alphabetic (c) || (numeric (c))) 
    return (1);

  if (allow_pathname_alphabetic_chars)
    return ((int)rindex (pathname_alphabetic_chars, c));
  else
    return (0);
}

/* Return non-zero if C is a numeric character. */
numeric (c)
     int c;
{
  return (c >= '0' && c <= '9');
}

/* Ring the terminal bell. */
ding ()
{
  if (readline_echoing_p)
    {
      fprintf (stderr, "\007");
      fflush (stderr);
    }
}

/* How to abort things. */
rl_abort ()
{
  ding ();
  rl_clear_message ();
  rl_init_argument ();
  rl_pending_input = 0;
  longjmp (readline_top_level, 1);
}

/* Return a copy of the string between FROM and TO. 
   FROM is inclusive, TO is not. */
char *
rl_copy (from, to)
     int from, to;
{
  register int length;
  char *copy;

  /* Fix it if the caller is confused. */
  if (from > to) {
    int t = from;
    from = to;
    to = t;
  }

  length = to - from;
  copy = (char *)xmalloc (1 + length);
  strncpy (copy, the_line + from, length);
  copy[length] = '\0';
  return (copy);
}


/* **************************************************************** */
/*								    */
/*			Insert and Delete			    */
/*								    */
/* **************************************************************** */


/* Insert a string of text into the line at point.  This is the only
   way that you should do insertion.  rl_insert () calls this
   function. */
rl_insert_text (string)
     char *string;
{
  extern int doing_an_undo;
  register int i, l = strlen (string);
  while (rl_end + l >= rl_line_buffer_len)
    {
      rl_line_buffer =
	(char *)xrealloc (rl_line_buffer,
			  rl_line_buffer_len += DEFAULT_BUFFER_SIZE);
      the_line = rl_line_buffer;
    }

  for (i = rl_end; i >= rl_point; i--)
    the_line[i + l] = the_line[i];
  strncpy (the_line + rl_point, string, l);

  /* Remember how to undo this if we aren't undoing something. */
  if (!doing_an_undo)
    {
      /* If possible and desirable, concatenate the undos. */
      if ((strlen (string) == 1) &&
	  the_undo_list &&
	  (the_undo_list->what == UNDO_INSERT) &&
	  (the_undo_list->end == rl_point) &&
	  (the_undo_list->end - the_undo_list->start < 20))
	the_undo_list->end++;
      else
	add_undo (UNDO_INSERT, rl_point, rl_point + l, (char *)NULL);
    }
  rl_point += l;
  rl_end += l;
  the_line[rl_end] = '\0';
}

/* Delete the string between FROM and TO.  FROM is
   inclusive, TO is not. */
rl_delete_text (from, to)
     int from, to;
{
  extern int doing_an_undo;
  register char *text;
  register int i;

  /* Fix it if the caller is confused. */
  if (from > to) {
    int t = from;
    from = to;
    to = t;
  }
  text = rl_copy (from, to);
  strncpy (the_line + from, the_line + to, rl_end - to);

  /* Remember how to undo this delete. */
  if (!doing_an_undo)
    add_undo (UNDO_DELETE, from, to, text);
  else 
    free (text);
  
  rl_end -= (to - from);
  the_line[rl_end] = '\0';
}


/* **************************************************************** */
/*								    */
/*			Readline character functions		    */
/*								    */
/* **************************************************************** */

/* This is not a gap editor, just a stupid line input routine.  No hair
   is involved in writing any of the functions, and none should be. */

/* Note that:

   rl_end is the place in the string that we would place '\0';
   i.e., it is always safe to place '\0' there.

   rl_point is the place in the string where the cursor is.  Sometimes
   this is the same as rl_end.

   Any command that is called interactively receives two arguments.
   The first is a count: the numeric arg pased to this command.
   The second is the key which invoked this command.
*/


/* **************************************************************** */
/*								    */
/*			Movement Commands			    */
/*								    */
/* **************************************************************** */

/* Note that if you `optimize' the display for these functions, you cannot
   use said functions in other functions which do not do optimizing display.
   I.e., you will have to update the data base for rl_redisplay, and you
   might as well let rl_redisplay do that job. */

/* Move forward COUNT characters. */
rl_forward (count)
     int count;
{
  if (count < 0) return (rl_backward (-count));
  else
    while (count) {
      if (rl_point == rl_end) return (ding ());
      else rl_point++;
      --count;
    }
}

/* Move backward COUNT characters. */
rl_backward (count)
     int count;
{
  if (count < 0) return (rl_forward (-count));
  else
    while (count) {
      if (!rl_point) return (ding ());
      else  --rl_point;
      --count;
    }
}

/* Move to the beginning of the line. */
rl_beg_of_line ()
{
  rl_point = 0;
}

/* Move to the end of the line. */
rl_end_of_line ()
{
  rl_point = rl_end;
}

/* Move forward a word.  We do what Emacs does. */
rl_forward_word (count)
     int count;
{
  int c;

  if (count < 0) return (rl_backward_word (-count));

  while (count) {
    if (rl_point == rl_end) return;

    /* If we are not in a word, move forward until we are in one.
       Then, move forward until we hit a non-alphabetic character. */
    c = the_line[rl_point];
    if (!alphabetic (c)) {
      while (++rl_point < rl_end) {
	c = the_line[rl_point];
	if (alphabetic (c)) break;
      }
    }
    if (rl_point == rl_end) return;
    while (++rl_point < rl_end) {
      c = the_line[rl_point];
      if (!alphabetic (c)) break;
    }
    --count;
  }
}

/* Move backward a word.  We do what Emacs does. */
rl_backward_word (count)
     int count;
{
  int c;

  if (count < 0)
    return (rl_forward_word (-count));

  while (count)
    {
      if (!rl_point) return;

      /* Like rl_forward_word (), except that we look at the characters
	 just before point. */

      c = the_line[rl_point - 1];
      if (!alphabetic (c))
	{
	  while (--rl_point)
	    {
	      c = the_line[rl_point - 1];
	      if (alphabetic (c)) break;
	    }
	}

      while (rl_point)
	{
	  c = the_line[rl_point - 1];
	  if (!alphabetic (c))
	    break;
	  else --rl_point;
	}
      --count;
    }
}

/* C-l typed to a line without quoting clears the screen, and then reprints
   the prompt and the current input line. */
rl_clear_screen ()
{
  extern char *term_clrpag;
  static void output_character_function ();

  if (term_clrpag)
    tputs (term_clrpag, 1, output_character_function);
  else
    crlf ();
 
  rl_forced_update_display ();
  rl_display_fixed = 1;
}


/* **************************************************************** */
/*								    */
/*			Text commands				    */
/*								    */
/* **************************************************************** */

/* Insert the character C at the current location, moving point forward. */
rl_insert (count, c)
     int count, c;
{
  static char string[2];

  string[1] = 0;
  string[0] = c;

  if (count < 0) return;

  if (count != 1)
    {
      while (count)
	{
	  rl_insert_text (string);
	  count--;
	}
      return;
    }
  else
    {
#ifdef DISPLAY_TABS
      register int l = rl_character_len (c, last_c_pos);
#else
      register int l = rl_character_len (c);
#endif  /* DISPLAY_TABS */

      rl_insert_text (string);

      if (rl_point == rl_end && readline_echoing_p && l < screenwidth)
	{
	  rl_show_char (c);
	  last_c_pos += l;
	  rl_display_fixed = 1;
	}
    }
}

/* Insert the next typed character verbatim. */
rl_quoted_insert (count)
     int count;
{
  int c = rl_getc (in_stream);
  rl_insert (count, c);
}

/* Insert a tab character. */
rl_tab_insert (count)
     int count;
{
  rl_insert (count, '\t');
}

/* What to do when a NEWLINE is pressed.  We accept the whole line. 
   KEY is the key that invoked this command.  I guess it could have
   meaning in the future. */
rl_newline (count, key)
     int count, key;
{
  rl_done = 1;
  if (readline_echoing_p)
    {
      move_vert (vis_botlin);
      vis_botlin = 0;
      crlf ();
      fflush (out_stream);
      rl_display_fixed++;
    }
}

rl_clean_up_for_exit ()
{
  if (readline_echoing_p)
    {
      move_vert (vis_botlin);
      vis_botlin = 0;
      fflush (out_stream);
    }
}

/* Let the next key typed be metized. */
rl_prefix_meta ()
{
  int c = rl_getc (in_stream);
  rl_pending_input = META (to_upper (c));
  rl_display_fixed++;
}

/* What to do for lowercase Meta characters. */
rl_do_uppercase_version (ignore, key)
     int ignore, key;
{
  rl_pending_input = META (to_upper (UNMETA (key)));
  rl_display_fixed++;
}

/* Rubout the character behind point. */
rl_rubout (count)
     int count;
{
  if (count < 0) return (rl_delete (-count));
  if (!rl_point) return (ding ());
  if (count > 1)
    {
      int orig_point = rl_point;
      rl_backward (count);
      rl_kill_text (orig_point, rl_point);
    }
  else
    {
      int c = the_line[--rl_point];
      rl_delete_text (rl_point, rl_point + 1);

      if (rl_point == rl_end && alphabetic (c) && last_c_pos)
	{
	  backspace (1);
	  putc (' ', out_stream);
	  backspace (1);
	  last_c_pos--;
	  rl_display_fixed++;
	}
    }
}

/* Non-zero means that C-d can return EOF when typed on a blank line,
   iff it wasn't the last character typed. */
int allow_ctrl_d_for_eof = 1;

/* Delete the character under the cursor.  If INVOKING_KEY is C-d, and
   the last command was not rl_delete, then just make rl_pending_input
   be EOF.  */
rl_delete (count, invoking_key)
     int count;
{
  if (count < 0)
    return (rl_rubout (-count));

  if (rl_point == rl_end) {
    if ((!rl_point) &&
	(rl_last_func != rl_delete) &&
	(invoking_key == CTRL('D')) &&
	(allow_ctrl_d_for_eof)) {

      rl_pending_input = EOF;
      return;
    } else {
      return (ding ());
    }
  }
  if (count > 1) {
    int orig_point = rl_point;
    rl_forward (count);
    rl_kill_text (orig_point, rl_point);
    rl_point = orig_point;
  } else {
    rl_delete_text (rl_point, rl_point + 1);
  }
}


/* **************************************************************** */
/*								    */
/*			Kill commands				    */
/*								    */
/* **************************************************************** */

/* The next two functions mimic unix line editing behaviour, except they
   save the deleted text on the kill ring.  This is safer than not saving
   it, and since we have a ring, nobody should get screwed. */

/* This does what C-w does in Unix.  We can't prevent people from
   using behaviour that they expect. */
rl_unix_word_rubout ()
{
  if (!rl_point) ding ();
  else {
    int orig_point = rl_point;
    while (rl_point && whitespace (the_line[rl_point - 1]))
      rl_point--;
    while (rl_point && !whitespace (the_line[rl_point - 1]))
      rl_point--;
    rl_kill_text (rl_point, orig_point);
  }
}

/* Here is C-u doing what Unix does.  You don't *have* to use these
   key-bindings.  We have a choice of killing the entire line, or
   killing from where we are to the start of the line.  We choose the
   latter, because if you are a Unix weenie, then you haven't backspaced
   into the line at all, and if you aren't, then you know what you are
   doing. */
rl_unix_line_discard ()
{
  if (!rl_point) ding ();
  else {
    rl_kill_text (rl_point, 0);
    rl_point = 0;
  }
}



/* **************************************************************** */
/*								    */
/*			Commands For Typos			    */
/*								    */
/* **************************************************************** */

/* Random and interesting things in here.  */


/* **************************************************************** */
/*								    */
/*			Changing Case				    */
/*								    */
/* **************************************************************** */

/* The three kinds of things that we know how to do. */
#define UpCase 1
#define DownCase 2
#define CapCase 3

/* Uppercase the word at point. */
rl_upcase_word (count)
     int count;
{
  rl_change_case (count, UpCase);
}

/* Lowercase the word at point. */
rl_downcase_word (count)
     int count;
{
  rl_change_case (count, DownCase);
}

/* Upcase the first letter, downcase the rest. */
rl_capitalize_word (count)
     int count;
{
  rl_change_case (count, CapCase);
}

/* The meaty function. 
   Change the case of COUNT words, performing OP on them.
   OP is one of UpCase, DownCase, or CapCase.
   If a negative argument is given, leave point where it started,
   otherwise, leave it where it moves to. */
rl_change_case (count, op)
     int count, op;
{
  register int start = rl_point, end;
  int state = 0;

  rl_forward_word (count);
  end = rl_point;

  if (count < 0) {
    int temp = start;
    start = end;
    end = temp;
  }

  for (; start < end; start++) {
    switch (op) {
      
    case UpCase:
      the_line[start] = to_upper (the_line[start]);
      break;

    case DownCase:
      the_line[start] = to_lower (the_line[start]);
      break;
    
    case CapCase:
      if (state == 0) {
	the_line[start] = to_upper (the_line[start]);
	state = 1;
      } else {
	the_line[start] = to_lower (the_line[start]);
      }
      if (!pure_alphabetic (the_line[start]))
	state = 0;
      break;

    default:
      abort ();
    }
  }
  rl_point = end;
}

/* **************************************************************** */
/*								    */
/*			Transposition				    */
/*								    */
/* **************************************************************** */

/* Transpose the words at point. */
rl_transpose_words (count)
     int count;
{
  char *word1, *word2;
  int w1_beg, w1_end, w2_beg, w2_end;
  int orig_point = rl_point;

  if (!count) return;

  /* Find the two words. */
  rl_forward_word (count);
  w2_end = rl_point;
  rl_backward_word (1);
  w2_beg = rl_point;
  rl_backward_word (count);
  w1_beg = rl_point;
  rl_forward_word (1);
  w1_end = rl_point;

  /* Do some check to make sure that there really are two words. */
  if ((w1_beg == w2_beg) || (w2_beg < w1_end))
    {
      ding ();
      rl_point = orig_point;
      return;
    }

  /* Get the text of the words. */
  word1 = rl_copy (w1_beg, w1_end);
  word2 = rl_copy (w2_beg, w2_end);

  /* We are about to do many insertions and deletions.  Remember them
     as one operation. */
  add_undo (UNDO_BEGIN, 0, 0, 0);

  /* Do the stuff at word2 first, so that we don't have to worry
     about word1 moving. */
  rl_point = w2_beg;
  rl_delete_text (w2_beg, w2_end);
  rl_insert_text (word1);

  rl_point = w1_beg;
  rl_delete_text (w1_beg, w1_end);
  rl_insert_text (word2);

  /* This is exactly correct since the text before this point has not
     changed in length. */
  rl_point = w2_end;

  /* I think that does it. */
  add_undo (UNDO_END, 0, 0, 0);
  free (word1); free (word2);
}

/* Transpose the characters at point.  If point is at the end of the line,
   then transpose the characters before point. */
rl_transpose_chars (count)
     int count;
{
  if (!count)
    return;

  if (!rl_point || rl_end < 2) {
    ding ();
    return;
  }

  while (count) {
    if (rl_point == rl_end) {
      int t = the_line[rl_point - 1];
      the_line[rl_point - 1] = the_line[rl_point - 2];
      the_line[rl_point - 2] = t;
    } else {
      int t = the_line[rl_point];
      the_line[rl_point] = the_line[rl_point - 1];
      the_line[rl_point - 1] = t;
      if (count < 0 && rl_point)
	rl_point--;
      else
	rl_point++;
    }
    if (count < 0)
      count++;
    else
      count--;
  }
}


/* **************************************************************** */
/*								    */
/*	Completion matching, from readline's point of view.	    */
/*								    */
/* **************************************************************** */

/* Pointer to the generator function for completion_matches ().
   NULL means to use filename_entry_function (), the default filename
   completer. */
Function *rl_completion_entry_function = (Function *)NULL;

/* Pointer to alternative function to create matches.
   Function is called with TEXT, START, and END.
   START and END are indices in RL_LINE_BUFFER saying what the boundaries
   of TEXT are.
   If this function exists and returns NULL then call the value of
   rl_completion_entry_function to try to match, otherwise use the
   array of strings returned. */
Function *rl_attempted_completion_function = (Function *)NULL;

/* Complete the word at or before point.  You have supplied the function
   that does the initial simple matching selection algorithm (see
   completion_matches ()).  The default is to do filename completion. */
rl_complete (ignore, invoking_key)
     int ignore, invoking_key;
{
  rl_complete_internal (TAB);
  if (running_in_emacs)
    printf ("%s", the_line);
}

/* List the possible completions.  See description of rl_complete (). */
rl_possible_completions ()
{
  rl_complete_internal ('?');
}

/* The user must press "y" or "n". Non-zero return means "y" pressed. */
get_y_or_n ()
{
  int c;
 loop:
  c = rl_getc (in_stream);
  if (c == EOF || c == 4 /* c-d */) return (0);
  if (c == 'y' || c == 'Y') return (1);
  if (c == 'n' || c == 'N') return (0);
  if (c == ABORT_CHAR) rl_abort ();
  ding (); goto loop;
}

/* Up to this many items will be displayed in response to a
   possible-completions call.  After that, we ask the user if
   she is sure she wants to see them all. */
int rl_completion_query_items = 100;

/* The basic list of characters that signal a break between words for the
   completer routine.  The contents of this variable is what breaks words
   in the shell, i.e. " \t\n\"\\'`@$><=" */
char *rl_basic_word_break_characters = " \t\n\"\\'`@$><=";

/* The list of characters that signal a break between words for 
   rl_complete_internal.  The default list is the contents of
   rl_basic_word_break_characters.  */
char *rl_completer_word_break_characters = (char *)NULL;

/* List of characters that are word break characters, but should be left
   in TEXT when it is passed to the completion function.  The shell uses
   this to help determine what kind of completing to do. */
char *rl_special_prefixes = (char )NULL;

/* Complete the word at or before point.  If WHAT_TO_DO is '?', then list
   the possible completions.  Otherwise, insert the completion into the
   line. */
rl_complete_internal (what_to_do)
     int what_to_do;
{
  char *filename_completion_function ();
  char **completion_matches (), **matches;
  Function *our_func, *func_stack;
  int start, end, delimiter = 0;
  char *text;

  if (rl_completion_entry_function)
    our_func = rl_completion_entry_function;
  else
    our_func = (int (*)())filename_completion_function;

  /* We now look backwards for the start of a filename/variable word. */
  end = rl_point;
  if (rl_point)
    {
      while (--rl_point &&
	     !rindex (rl_completer_word_break_characters, the_line[rl_point]));

      /* If we are at a word break, then advance past it. */
      if (rindex (rl_completer_word_break_characters,  (the_line[rl_point])))
	{
	  /* If the character that caused the word break was a quoting
	     character, then remember it as the delimiter. */
	  if (rindex ("\"'", the_line[rl_point]) && (end - rl_point) > 1)
	    delimiter = the_line[rl_point];

	  /* If the character isn't needed to determine something special
	     about what kind of completion to perform, then advance past it. */

	  if (!rl_special_prefixes ||
	      !rindex (rl_special_prefixes, the_line[rl_point]))
	    rl_point++;
	}
    }

  start = rl_point;
  rl_point = end;
  text = rl_copy (start, end);

  /* If the user wants to TRY to complete, but then wants to give
     up and use the default completion function, they set the
     variable rl_attempted_completion_function. */
  if (rl_attempted_completion_function)
    {
      matches =
	(char **)(*rl_attempted_completion_function) (text, start, end);

      if (matches)
	goto after_usual_completion;
    }

  matches = completion_matches (text, our_func, start, end);

 after_usual_completion:
  free (text);

  if (!matches)
    ding ();
  else
    {
      register int i;

    some_matches:

      /* It seems to me that in all the cases we handle we would like
	 to ignore duplicate possiblilities.  Scan for the text to
	 insert being identical to the other completions. */
      for (i = 1; matches[i]; i++) 
	if (strcmp (matches[0], matches[i]) != 0)
	  break;

      if (!matches[i])
	{
	  for (i = 1; matches[i]; i++)
	    free (matches[i]);
	  matches[1] = (char *)NULL;
	}

      switch (what_to_do)
	{
	case TAB:
	  rl_delete_text (start, rl_point);
	  rl_point = start;
	  rl_insert_text (matches[0]);

	  /* If there are more matches, ring the bell to indicate.
	     If this was the only match, and we are hacking files,
	     check the file to see if it was a directory.  If so,
	     add a '/' to the name.  If not, and we are at the end
	     of the line, then add a space. */
	  if (matches[1])
	    {
	      ding ();		/* There are other matches remaining. */
	    }
	  else
	    {
	      char temp_string[2];
	      
	      temp_string[0] = delimiter ? delimiter : ' ';
	      temp_string[1] = '\0';

	      if (our_func == (int (*)())filename_completion_function)
		{
		  struct stat finfo;
		  char *tilde_expand ();
		  char *filename = tilde_expand (matches[0]);
	  
		  if ((stat (filename, &finfo) == 0) &&
		      (finfo.st_mode & S_IFDIR))
		    {
		      if (the_line[rl_point] != '/')
			rl_insert_text ("/");
		    }
		  else
		    {
		      if (rl_point == rl_end)
			rl_insert_text (temp_string);
		    }
		  free (filename);
		}
	      else
		{
		  if (rl_point == rl_end)
		    rl_insert_text (temp_string);
		}
	    }
	  break;

	case '?':
	  {
	    int len, count, limit, max = 0;
	    int j, k, l;

	    /* Handle simple case first.  What if there is only one answer? */
	    if (!matches[1])
	      {
		char *rindex (), *temp;

		if (our_func == (int (*)())filename_completion_function)
		  temp = rindex (matches[0], '/');
		else
		  temp = (char *)NULL;

		if (!temp)
		  temp = matches[0];
		else
		  temp++;

		crlf ();
		fprintf (out_stream, "%s", temp);
		crlf ();
		goto restart;
	      }

	    /* There is more than one answer.  Find out how many there are,
	       and find out what the maximum printed length of a single entry
	       is. */
	    for (i = 1; matches[i]; i++)
	      {
		char *rindex (), *temp = (char *)NULL;

		/* If we are hacking filenames, then only count the characters
		   after the last slash in the pathname. */
		if (our_func == (int (*)())filename_completion_function)
		  temp = rindex (matches[i], '/');
		else
		  temp = (char *)NULL;

		if (!temp)
		  temp = matches[i];
		else
		  temp++;

		if (strlen (temp) > max)
		  max = strlen (temp);
	      }

	    len = i;

	    /* If there are many items, then ask the user if she
	       really wants to see them all. */
	    if (len >= rl_completion_query_items)
	      {
		crlf ();
		fprintf (out_stream,
"There are %d possibilities.  Do you really wish to see them all? (y or n)",
			 len);
		fflush (out_stream);
		if (!get_y_or_n ())
		  {
		    crlf ();
		    goto restart;
		  }
	      }
	    /* How many items of MAX length can we fit in the screen window? */
	    max += 2;
	    limit = screenwidth / max;
	    if (limit != 1 && (limit * max == screenwidth))
	      limit--;

	    /* How many iterations of the printing loop? */
	    count = (len + (limit - 1)) / limit;

	    /* Watch out for special case.  If LEN is less than LIMIT, then
	       just do the inner printing loop. */
	    if (len < limit) count = 1;

	    /* Sort the items. */
	    {
	      int compare_strings ();
	      qsort (matches, len, sizeof (char *), compare_strings);
	    }

	    /* Print the sorted items, up-and-down alphabetically, like
	       ls might. */
	    crlf ();

	    for (i = 1; i < count + 1; i++)
	      {
		for (j = 0, l = i; j < limit; j++)
		  {
		    if (l > len || !matches[l])
		      {
			break;
		      }
		    else
		      {
			char *rindex (), *temp = (char *)NULL;

			if (our_func == (int (*)())filename_completion_function)
			  temp = rindex (matches[l], '/');
			else
			  temp = (char *)NULL;

			if (!temp)
			  temp = matches[l];
			else
			  temp++;

			fprintf (out_stream, "%s", temp);
			for (k = 0; k < max - strlen (temp); k++)
			  putc (' ', out_stream);
		      }
		    l += count;
		  }
		crlf ();
	      }
	  restart:

	    rl_on_new_line ();
	  }
	  break;

	default:
	  abort ();
	}

      for (i = 0; matches[i]; i++)
	free (matches[i]);
      free (matches);
    }
}

/* Stupid comparison routine for qsort () ing strings. */
compare_strings (s1, s2)
  char **s1, **s2;
{
  return (strcmp (*s1, *s2));
}

/* A completion function for usernames.
   TEXT contains a partial username preceded by a random
   character (usually `~').  */
char *
username_completion_function (text, state)
     int state;
     char *text;
{
  static char *username = (char *)NULL;
  static struct passwd *entry;
  static int namelen;

  if (!state)
    {
      if (username)
	free (username);
      username = savestring (&text[1]);
      namelen = strlen (username);
      setpwent ();
    }

  while (entry = getpwent ())
    {
      if (strncmp (username, entry->pw_name, namelen) == 0)
	break;
    }

  if (!entry)
    {
      endpwent ();
      return ((char *)NULL);
    }
  else
    {
      char *value = (char *)xmalloc (2 + strlen (entry->pw_name));
      *value = *text;
      strcpy (value + 1, entry->pw_name);
      return (value);
    }
}

/* Expand FILENAME if it begins with a tilde.  This always returns
   a new string. */
char *
tilde_expand (filename)
     char *filename;
{
  char *dirname = filename ? savestring (filename) : (char *)NULL;

  if (dirname && *dirname == '~')
    {
      char *temp_name;
      if (!dirname[1] || dirname[1] == '/')
	{
	  /* Prepend $HOME to the rest of the string. */
	  char *temp_home = (char *)getenv ("HOME");

	  temp_name = (char *)alloca (1 + strlen (&dirname[1])
				      + (temp_home? strlen (temp_home) : 0));
	  temp_name[0] = '\0';
	  if (temp_home)
	    strcpy (temp_name, temp_home);
	  strcat (temp_name, &dirname[1]);
	  free (dirname);
	  dirname = savestring (temp_name);
	}
      else
	{
	  struct passwd *getpwnam (), *user_entry;
	  char *username = (char *)alloca (257);
	  int i, c;

	  for (i = 1; c = dirname[i]; i++)
	    {
	      if (c == '/') break;
	      else username[i - 1] = c;
	    }
	  username[i - 1] = '\0';
	
	  if (!(user_entry = getpwnam (username)))
	    {
#ifdef SHELL
	      /* Maybe this is a special `name' for the shell. */

	      char *get_string_value ();
	      char *tp = (char *)NULL;

	      if (strcmp (username, "+") == 0)
		tp = get_string_value ("PWD");
	      else if (strcmp (username, "-") == 0)
		tp = get_string_value ("OLDPWD");

	      if (tp)
		{
		  temp_name = (char *)alloca (1 + strlen (tp)
					      + strlen (&dirname[i]));
		  strcpy (temp_name, tp);
		  strcat (temp_name, &dirname[i]);
		  goto return_name;
		}
#endif				/* SHELL */
	      /*
	       * We shouldn't report errors.
	       */
	    }
	  else
	    {
	      temp_name = (char *)alloca (1 + strlen (user_entry->pw_dir)
					  + strlen (&dirname[i]));
	      strcpy (temp_name, user_entry->pw_dir);
	      strcat (temp_name, &dirname[i]);
	    return_name:
	      free (dirname);
	      dirname = savestring (temp_name);
	    }
	}
    }
  return (dirname);
}


/* **************************************************************** */
/*								    */
/*			Undo, and Undoing			    */
/*								    */
/* **************************************************************** */

/* Non-zero tells rl_delete_text and rl_insert_text to not add to
   the undo list. */
int doing_an_undo = 0;

/* The current undo list for THE_LINE. */
UNDO_LIST *the_undo_list = (UNDO_LIST *)NULL;

/* Remember how to undo something.  Concatenate some undos if that
   seems right. */
add_undo (what, start, end, text)
     enum undo_code what;
     int start, end;
     char *text;
{
  UNDO_LIST *temp = (UNDO_LIST *)xmalloc (sizeof (UNDO_LIST));
  temp->what = what;
  temp->start = start;
  temp->end = end;
  temp->text = text;
  temp->next = the_undo_list;
  the_undo_list = temp;
}

/* Free the existing undo list. */
free_undo_list ()
{
  while (the_undo_list) {
    UNDO_LIST *release = the_undo_list;
    the_undo_list = the_undo_list->next;

    if (release->what == UNDO_DELETE)
      free (release->text);

    free (release);
  }
}

/* Undo the next thing in the list.  Return 0 if there
   is nothing to undo, or non-zero if there was. */
do_undo ()
{
  UNDO_LIST *release;
  int waiting_for_begin = 0;

undo_thing:
  if (!the_undo_list)
    return (0);

  doing_an_undo = 1;

  switch (the_undo_list->what) {

    /* Undoing deletes means inserting some text. */
  case UNDO_DELETE:
    rl_point = the_undo_list->start;
    rl_insert_text (the_undo_list->text);
    free (the_undo_list->text);
    break;

    /* Undoing inserts means deleting some text. */
  case UNDO_INSERT:
    rl_delete_text (the_undo_list->start, the_undo_list->end);
    rl_point = the_undo_list->start;
    break;

    /* Undoing an END means undoing everything 'til we get to
       a BEGIN. */
  case UNDO_END:
    waiting_for_begin++;
    break;

    /* Undoing a BEGIN means that we are done with this group. */
  case UNDO_BEGIN:
    if (waiting_for_begin)
      waiting_for_begin--;
    else 
      abort ();
    break;
  }

  doing_an_undo = 0;

  release = the_undo_list;
  the_undo_list = the_undo_list->next;
  free (release);

  if (waiting_for_begin)
    goto undo_thing;

  return (1);
}

/* Revert the current line to its previous state. */
rl_revert_line ()
{
  if (!the_undo_list) ding ();
  else {
    while (the_undo_list)
      do_undo ();
  }
}

/* Do some undoing of things that were done. */
rl_do_undo (count)
{
  if (count < 0) return;	/* Nothing to do. */

  while (count) {
    if (do_undo ()) {
      count--;
    } else {
      ding ();
      break;
    }
  }
}

/* **************************************************************** */
/*								    */
/*			History Utilities			    */
/*								    */
/* **************************************************************** */

/* We already have a history library, and that is what we use to control
   the history features of readline.  However, this is our local interface
   to the history mechanism. */

/* While we are editing the history, this is the saved
   version of the original line. */
HIST_ENTRY *saved_line_for_history = (HIST_ENTRY *)NULL;

/* Set the history pointer back to the last entry in the history. */
start_using_history ()
{
  using_history ();
  if (saved_line_for_history)
    free_history_entry (saved_line_for_history);

  saved_line_for_history = (HIST_ENTRY *)NULL;
}

/* Free the contents (and containing structure) of a HIST_ENTRY. */
free_history_entry (entry)
     HIST_ENTRY *entry;
{
  if (!entry) return;
  if (entry->line)
    free (entry->line);
  free (entry);
}

/* Perhaps put back the current line if it has changed. */
maybe_replace_line ()
{
  HIST_ENTRY *temp = current_history ();

  /* If the current line has changed, save the changes. */
  if (temp && ((UNDO_LIST *)(temp->data) != the_undo_list)) {
    temp = replace_history_entry (where_history (), the_line, the_undo_list);
    free (temp->line);
    free (temp);
  }
}

/* Put back the saved_line_for_history if there is one. */
maybe_unsave_line ()
{
  if (saved_line_for_history) {
    strcpy (the_line, saved_line_for_history->line);
    the_undo_list = (UNDO_LIST *)saved_line_for_history->data;
    free_history_entry (saved_line_for_history);
    saved_line_for_history = (HIST_ENTRY *)NULL;
    rl_end = rl_point = strlen (the_line);
  } else {
    ding ();
  }
}

/* Save the current line in saved_line_for_history. */
maybe_save_line ()
{
  if (!saved_line_for_history) {
    saved_line_for_history = (HIST_ENTRY *)xmalloc (sizeof (HIST_ENTRY));
    saved_line_for_history->line = savestring (the_line);
    saved_line_for_history->data = (char *)the_undo_list;
  }
}

  

/* **************************************************************** */
/*								    */
/*			History Commands			    */
/*								    */
/* **************************************************************** */

/* Meta-< goes to the start of the history. */
rl_beginning_of_history ()
{
  rl_get_previous_history (1 + where_history ());
}

/* Meta-> goes to the end of the history.  (The current line). */
rl_end_of_history ()
{
  HIST_ENTRY *temp;

  maybe_replace_line ();
  using_history ();
  maybe_unsave_line ();
}

/* Move down to the next history line. */
rl_get_next_history (count)
     int count;
{
  HIST_ENTRY *temp = (HIST_ENTRY *)NULL;

  if (count < 0) 
    return (rl_get_previous_history (-count));

  if (!count) return;

  maybe_replace_line ();

  while (count) {
    temp = next_history ();
    if (!temp)
      break;
    --count;
  }

  if (!temp)
    maybe_unsave_line ();
  else {
    strcpy (the_line, temp->line);
    the_undo_list = (UNDO_LIST *)temp->data;
    rl_end = rl_point = strlen (the_line);
  }
}

/* Get the previous item out of our interactive history, making it the current
   line.  If there is no previous history, just ding. */
rl_get_previous_history (count)
     int count;
{
  HIST_ENTRY *old_temp = (HIST_ENTRY *)NULL;
  HIST_ENTRY *temp;

  if (count < 0)
    return (rl_get_next_history (-count));

  if (!count)
    return;

  /* If we don't have a line saved, then save this one. */
  maybe_save_line ();
  
  /* If the current line has changed, save the changes. */
  maybe_replace_line ();

  while (count) {
    temp = previous_history ();
    if (!temp)
      break;
    else
      old_temp = temp;
    --count;
  }

  /* If there was a large argument, and we moved back to the start of the
     history, that is not an error.  So use the last value found. */
  if (!temp && old_temp)
    temp = old_temp;

  if (!temp)
    ding ();
  else {
    strcpy (the_line, temp->line);
    the_undo_list = (UNDO_LIST *)temp->data;
    rl_end = rl_point = strlen (the_line);
  }
}

/* There is a command in ksh which yanks into this line, the last word
   of the previous line.  Here it is.  We left it on M-. */
rl_yank_previous_last_arg (ignore)
     int ignore;
{
}



/* **************************************************************** */
/*								    */
/*			I-Search and Searching			    */
/*								    */
/* **************************************************************** */

/* Search backwards through the history looking for a string which is typed
   interactively.  Start with the current line. */
rl_reverse_search_history (sign, key)
     int sign;
     int key;
{
  rl_search_history (-sign, key);
}

/* Search forwards through the history looking for a string which is typed
   interactively.  Start with the current line. */
rl_forward_search_history (sign, key)
     int sign;
     int key;
{
  rl_search_history (sign, key);
}

/* Search through the history looking for an interactively typed string.
   This is analogous to i-search.  We start the search in the current line.
   DIRECTION is which direction to search; > 0 means forward, < 0 means
   backwards. */
rl_search_history (direction, invoking_key)
     int direction;
     int invoking_key;
{
  /* The string that the user types in to search for. */
  char *search_string = (char *)alloca (128);

  /* The current length of SEARCH_STRING. */
  int search_string_index;

  /* The list of lines to search through. */
  char **lines;

  /* The length of LINES. */
  int hlen;

  /* Where we get LINES from. */
  HIST_ENTRY **hlist = history_list ();
  
  /* Buffer for the search message. */
  char message[200];

  int orig_point = rl_point;
  int orig_line = where_history ();
  int last_found_line = orig_line;
  int c, done = 0;
  register int i = 0;


  /* The line currently being searched. */
  char *sline;

  /* Offset in that line. */
  int index;

  /* Non-zero if we are doing a reverse search. */
  int reverse = (direction < 0);

  /* Create an arrary of pointers to the lines that we want to search. */

  maybe_replace_line ();
  if (hlist)
    for (i = 0; hlist[i]; i++);

  /* Allocate space for this many lines, +1 for the current input line,
     and remember those lines. */
  lines = (char **)alloca ((1 + (hlen = i)) * sizeof (char *));
  for (i = 0; i < hlen; i++)
    lines[i] = hlist[i]->line;

  if (saved_line_for_history)
    lines[i] = saved_line_for_history->line;
  else {
    /* Funny alloca doesn't let me do this.
    lines[i] =
       (char *)strcpy ((char *)alloca (1 + strlen (the_line)), &the_line[0]);
     */

    /* So I have to type it in this way instead. */
    lines[i] = (char *)alloca (1 + strlen (the_line));
    strcpy (lines[i], &the_line[0]);
  }

  hlen++;

  /* The line where we start the search. */
  i = orig_line;

  /* Initialize search parameters. */
  *search_string = '\0';
  search_string_index = 0;

  strcpy (message, "(");
  if (reverse)
    strcat (message, "reverse-");

  strcat (message, "i-search): ");

  rl_message (message);

  sline = the_line;
  index = rl_point;

  while (!done) {
    c = rl_getc (in_stream);

    /* Hack C to Do What I Mean. */
    {
      Function *f = keymap[c];
 
      if (f == rl_reverse_search_history)
	c = reverse ? -1 : -2;
      else if (f == rl_forward_search_history)
	c =  !reverse ? -1 : -2;
    }

    switch (c) {

    case ESC:
      done = 1;
      continue;
    
      /* case invoking_key: */
    case -1:
      goto search_again;

      /* switch directions */
    case -2:
      direction = -direction;
      reverse = (direction < 0);

      strcpy (message, "(");
      if (reverse)
	strcat (message, "reverse-");
      strcat (message, "i-search)`");
      if (search_string)
	strcat (message, search_string);
      strcat (message, "': ");
      rl_message (message);
      rl_redisplay ();
      goto do_search;

    case CTRL ('G'):
      strcpy (the_line, lines[orig_line]);
      rl_point = orig_point;
      rl_end = strlen (the_line);
      rl_clear_message ();
      return;

    default:
      if (c < 32 || c > 126) {
	rl_execute_next (c);
	done = 1;
	continue;
      } else {
	search_string[search_string_index++] = c;
	search_string[search_string_index] = '\0';
	goto do_search;

      search_again:

	if (!search_string_index)
	  continue;
	else {
	  if (reverse)
	    --index;
	  else 
	    if (index != strlen (sline))
	      ++index;
	    else 
	      ding ();
	}
      do_search:

	while (1) {

	  if (reverse) {
	    while (index >= 0)
	      if (strncmp
		  (search_string, sline + index, search_string_index) == 0)
		goto string_found;
	      else
		index--;
	  } else {
	    register int limit = (strlen (sline) - search_string_index) + 1;

	    while (index < limit) {
	      if (strncmp (search_string, sline + index, search_string_index) == 0)
		goto string_found;
	      index++;
	    }
	  }

	next_line:
	  i += direction;

	  /* At limit for direction? */
	  if ((reverse && i < 0) ||
	      (!reverse && i == hlen))
	    goto search_failed;

	  sline = lines[i];
	  if (reverse)
	    index = strlen (sline);
	  else
	    index = 0;

	  /* If the search string is longer than the current line, no match. */
	  if (search_string_index > strlen (sline))
	    goto next_line;
	  
	  /* Start actually searching. */
	  if (reverse)
	    index -= search_string_index;
	}

      search_failed:
	/* We cannot find the search string.  Ding the bell. */
	ding ();
	i = last_found_line;
	break;

      string_found:
	/* We have found the search string.  Just display it.  But don't
	   actually move there in the history list until the user accepts
	   the location. */
	strcpy (the_line, lines[i]);
	rl_point = index;
	rl_end = strlen (the_line);
	last_found_line = i;
	rl_redisplay ();
      }
    }
    continue;
  }
  /* The user has won.  They found the string that they wanted.  Now all
     we have to do is place them there. */
  {
    int now = last_found_line;

    /* First put back the original state. */
    strcpy (the_line, lines[orig_line]);

    if (now < orig_line)
      rl_get_previous_history (orig_line - now);
    else
      rl_get_next_history (now - orig_line);

    rl_point = index;
    rl_clear_message ();
  }
}

/* Make C be the next command to be executed. */
rl_execute_next (c)
     int c;
{
  rl_pending_input = c;
}

/* **************************************************************** */
/*								    */
/*			Killing Mechanism			    */
/*								    */
/* **************************************************************** */


/* What we assume for a max number of kills. */
#define DEFAULT_MAX_KILLS 10

/* The real variable to look at to find out when to flush kills. */
int rl_max_kills = DEFAULT_MAX_KILLS;

/* Where to store killed text. */
char **rl_kill_ring = (char **)NULL;

/* Where we are in the kill ring. */
int rl_kill_index = 0;

/* How many slots we have in the kill ring. */
int rl_kill_ring_length = 0;

/* How to say that you only want to save a certain amount
   of kill material. */
rl_set_retained_kills (num)
     int num;
{}

/* The way to kill something.  This appends or prepends to the last
   kill, if the last command was a kill command.  if FROM is less
   than TO, then the text is appended, otherwise prepended.  If the
   last command was not a kill command, then a new slot is made for
   this kill. */
rl_kill_text (from, to)
     int from, to;
{
  int slot;
  char *text = rl_copy (from, to);

  /* Is there anything to kill? */
  if (from == to) {
    free (text);
    last_command_was_kill++;
    return;
  }

  /* Delete the copied text from the line. */
  rl_delete_text (from, to);

  /* First, find the slot to work with. */
  if (!last_command_was_kill) {

    /* Get a new slot.  */
    if (!rl_kill_ring) {

      /* If we don't have any defined, then make one. */
      rl_kill_ring =
	(char **)xmalloc (((rl_kill_ring_length = 1) + 1) * sizeof (char *));
      slot = 1;      

    } else {

      /* We have to add a new slot on the end, unless we have exceeded
	 the max limit for remembering kills. */ 
      slot = rl_kill_ring_length;
      if (slot == rl_max_kills) {
	register int i;
	free (rl_kill_ring[0]);
	for (i = 0; i < slot; i++)
	  rl_kill_ring[i] = rl_kill_ring[i + 1];
      } else {
	rl_kill_ring =
	  (char **)xrealloc (rl_kill_ring,
			     ((slot = (rl_kill_ring_length += 1)) + 1)
			     * sizeof (char *));
      }
    }
    slot--;
  } else {
    slot = rl_kill_ring_length - 1;
  }

  /* If the last command was a kill, prepend or append. */
  if (last_command_was_kill) {
    char *old = rl_kill_ring[slot];
    char *new = (char *)xmalloc (1 + strlen (old) + strlen (text));

    if (from < to) {
      strcpy (new, old);
      strcat (new, text);
    } else {
      strcpy (new, text);
      strcat (new, old);
    }
    free (old);
    free (text);
    rl_kill_ring[slot] = new;
  } else {
    rl_kill_ring[slot] = text;
  }
  rl_kill_index = slot;
  last_command_was_kill++;
}

/* Now REMEMBER!  In order to do prepending or appending correctly, kill
   commands always make rl_point's original position be the FROM argument,
   and rl_point's extent be the TO argument. */


/* **************************************************************** */
/*								    */
/*			Killing Commands			    */
/*								    */
/* **************************************************************** */

/* Delete the word at point, saving the text in the kill ring. */
rl_kill_word (count)
     int count;
{
  int orig_point = rl_point;

  if (count < 0) return (rl_backward_kill_word (-count));

  rl_forward_word (count);

  if (rl_point != orig_point)
    rl_kill_text (orig_point, rl_point);
  rl_point = orig_point;
}

/* Rubout the word before point, placing it on the kill ring. */
rl_backward_kill_word (count)
     int count;
{
  int orig_point = rl_point;

  if (count < 0) return (rl_backward_kill_word (-count));
  rl_backward_word (count);

  if (rl_point != orig_point)
    rl_kill_text (orig_point, rl_point);
}

/* Kill from here to the end of the line.  If DIRECTION is negative, kill
   back to the line start instead. */
rl_kill_line (direction)
     int direction;
{
  int orig_point = rl_point;

  if (direction < 0) return (rl_backward_kill_line (1));
  rl_end_of_line ();
  if (orig_point != rl_point)
    rl_kill_text (orig_point, rl_point);
  rl_point = orig_point;
}

/* Kill backwards to the start of the line.  If DIRECTION is negative, kill
   forwards to the line end instead. */
rl_backward_kill_line (direction)
     int direction;
{
  int orig_point = rl_point;

  if (direction < 0) return (rl_kill_line (1));
  if (!rl_point) ding ();
  else {
    rl_beg_of_line ();
    rl_kill_text (orig_point, rl_point);
  }
}

/* Yank back the last killed text.  This ignores arguments. */
rl_yank ()
{
  if (!rl_kill_ring) rl_abort ();
  rl_insert_text (rl_kill_ring[rl_kill_index]);
}

/* If the last command was yank, or yank_pop, and the text just
   before point is identical to the current kill item, then
   delete that text from the line, rotate the index down, and
   yank back some other text. */
rl_yank_pop ()
{
  int l;

  if (((rl_last_func != rl_yank_pop) && (rl_last_func != rl_yank)) ||
      !rl_kill_ring) {
    rl_abort ();
  }

  l = strlen (rl_kill_ring[rl_kill_index]);
  if (((rl_point - l) >= 0) &&
      (strncmp (the_line + (rl_point - l),
		rl_kill_ring[rl_kill_index], l) == 0)) {
    rl_delete_text ((rl_point - l), rl_point);
    rl_point -= l;
    rl_kill_index--;
    if (rl_kill_index < 0)
      rl_kill_index = rl_kill_ring_length - 1;
    rl_yank ();
  } else {
    rl_abort ();
  }
}


/* **************************************************************** */
/*								    */
/*			VI Emulation Mode			    */
/*								    */
/* **************************************************************** */

/* Ugh. */

/* Word movement, from the point of view of vi.  What does vi stand for,
   anyway? Visual Idiocy? */

/* Previous word in vi mode. */
rl_vi_prev_word (count)
     int count;
{
  if (count < 0)
    return (rl_vi_next_word (-count));

  return (rl_backward_word (count));
}

/* Next word in vi mode. */
rl_vi_next_word (count)
     int count;
{
  if (count < 0)
    return (rl_vi_prev_word (-count));

  return (rl_forward_word (count));
}

/* What to do in the case of C-d. */
rl_vi_eof_maybe (count, c)
     int count, c;
{
  if (c == CTRL('D') && rl_point == 0 && allow_ctrl_d_for_eof) {
    rl_pending_input = EOF;
    return;
  }

  rl_newline ();
}

/* Insertion mode stuff. */

/* Switching from one mode to the other really just involve
   switching keymaps. */
rl_vi_insertion_mode ()
{
  keymap = vi_insertion_keymap;
}

rl_vi_movement_mode ()
{
  keymap = vi_movement_keymap;
}

rl_vi_arg_digit (count, c)
     int count, c;
{
  if (c == '0' && rl_numeric_arg == 1 && !rl_explicit_arg)
    rl_beg_of_line ();
  else
    rl_digit_argument (count, c);
}

rl_vi_change (count)
     int count;
{
}

rl_vi_char_search (count)
     int count;
{
}

/* How to toggle back and forth between editing modes. */
rl_vi_editing_mode ()
{
  rl_editing_mode = vi_mode;
  rl_vi_insertion_mode ();
}

rl_emacs_editing_mode ()
{
  rl_editing_mode = emacs_mode;
  keymap = emacs_standard_keymap;
}


/* **************************************************************** */
/*								    */
/*			     Completion				    */
/*								    */
/* **************************************************************** */

/* Return an array of (char *) which is a list of completions for TEXT.
   If there are no completions, return a NULL pointer.
   The first entry in the returned array is the substitution for TEXT.
    The remaining entries are the possible completions.
   The array is terminated with a NULL pointer.

   ENTRY_FUNCTION is a function of two args, and returns a (char *).
   The first argument is TEXT.
   The second is a state argument; it should be zero on the first call, and
   non-zero on subsequent calls.  It returns a NULL pointer to the caller
   when there are no more matches. */
char **
completion_matches (text, entry_function)
     char *text;
     char *(*entry_function) ();
{
  /* Number of slots in match_list. */
  int match_list_size;

  /* The list of matches. */
  char **match_list =
    (char **)xmalloc (((match_list_size = 10) + 1) * sizeof (char *));

  /* Number of matches actually found. */
  int matches = 0;

  /* Temporary string binder. */
  char *string;

  match_list[1] = (char *)NULL;

  while (string = (*entry_function) (text, matches)) {
    if (matches + 1 == match_list_size)
      match_list =
	(char **)xrealloc (match_list,
			   ((match_list_size += 10) + 1) * sizeof (char *));

    match_list[++matches] = string;
    match_list[matches + 1] = (char *)NULL;
  }

  /* If there were any matches, then look through them finding out the
     lowest common denominator.  That then becomes match_list[0]. */
  if (matches) {
    register int i = 1;
    int low = 100000;		/* Count of max-matched characters. */

    /* If only one match, just use that. */
    if (matches == 1) {
      match_list[0] = match_list[1];
      match_list[1] = (char *)NULL;
    } else {
      /* Otherwise, compare each member of the list with the next, finding out
	 where they stop matching. */

      while (i < matches) {
	register int c1, c2, si;

#ifdef CASE_INSENSITIVE
	for (si = 0;
	     (c1 = to_lower(match_list[i][si])) &&
	     (c2 = to_lower(match_list[i + 1][si]));
	     si++)
	  if (c1 != c2) break;
#else
	for (si = 0;
	     (c1 = match_list[i][si]) &&
	     (c2 = match_list[i + 1][si]);
	     si++)
	  if (c1 != c2) break;
#endif  /* CASE_INSENSITIVE */

	if (low > si) low = si;
	i++;
      }
      match_list[0] = (char *)xmalloc (low + 1);
      strncpy (match_list[0], match_list[1], low);
      match_list[0][low] = '\0';
    }
  } else {			/* There were no matches. */
    free (match_list);
    match_list = (char **)NULL;
  }
  return (match_list);
}

/* Okay, now we write the entry_function for filename completion.  In the
   general case.  Note that completion in the shell is a little different
   because of all the pathnames that must be followed when looking up the
   completion for a command. */
char *
filename_completion_function (text, state)
     int state;
     char *text;
{
  static DIR *directory;
  static char *filename = (char *)NULL;
  static char *dirname = (char *)NULL;
  static char *users_dirname = (char *)NULL;
  static int filename_len;

  struct direct *entry = (struct direct *)NULL;

  /* If we don't have any state, then do some initialization. */
  if (!state) {
    char *rindex (), *temp;

    if (dirname) free (dirname);
    if (filename) free (filename);
    if (users_dirname) free (users_dirname);

    filename = savestring (text);
    if (!*text) text = ".";
    dirname = savestring (text);

    temp = rindex (dirname, '/');

    if (temp) {
      strcpy (filename, ++temp);
      *temp = '\0';
    } else {
      strcpy (dirname, ".");
    }

    /* We aren't done yet.  We also support the "~user" syntax. */

    /* Save the version of the directory that the user typed. */
    users_dirname = savestring (dirname);
    {
      char *tilde_expand (), *temp_dirname = tilde_expand (dirname);
      free (dirname);
      dirname = temp_dirname;
#ifdef SHELL
      {
	extern int follow_symbolic_links;
	char *make_absolute ();

	if (follow_symbolic_links && (strcmp (dirname, ".") != 0))
	  {
	    temp_dirname = make_absolute (dirname, get_working_directory (""));

	    if (temp_dirname)
	      {
		free (dirname);
		dirname = temp_dirname;
	      }
	  }
      }
#endif  /* SHELL */
    }
    directory = opendir (dirname);
    filename_len = strlen (filename);
  }

  /* At this point we should entertain the possibility of hacking wildcarded
     filenames, like /usr/man*\/te<TAB>.  If the directory name contains
     globbing characters, then build an array of directories to glob on, and
     glob on the first one. */

  /* Now that we have some state, we can read the directory. */

  while (directory && (entry = readdir (directory))) {

    /* Special case for no filename.  All entries except "." and ".." match. */
    if (!filename_len) {
      if ((strcmp (entry->d_name, ".") != 0) &&
	  (strcmp (entry->d_name, "..") != 0))
	break;
    } else {
      /* Otherwise, if these match upto the length of filename, then
	 it is a match. */
#ifdef TMB_SYSV
      if ((strlen (entry->d_name) >= filename_len) &&
	  (strncmp (filename, entry->d_name, filename_len) == 0))
#else
      if ((entry->d_namlen >= filename_len) &&
	  (strncmp (filename, entry->d_name, filename_len) == 0))
#endif  /* TMB_SYSV */
	{
	  break;
	}
    }
  }

  if (!entry) {
    if (directory) {
      closedir (directory);
      directory = (DIR *)NULL;
    }
    return (char *)NULL;
  } else {
    char *temp;

    if (dirname && (strcmp (dirname, ".") != 0)) {
#ifdef TMB_SYSV
      temp = (char *)xmalloc (1 + strlen (users_dirname) + strlen (entry->d_name));
#else
      temp = (char *)xmalloc (1 + strlen (users_dirname) + entry->d_namlen);
#endif  /* TMB_SYSV */
      strcpy (temp, users_dirname);
      strcat (temp, entry->d_name);
    } else {
      temp = (savestring (entry->d_name));
    }
    return (temp);
  }
}


/* **************************************************************** */
/*								    */
/*			Binding keys				    */
/*								    */
/* **************************************************************** */

/* Bind KEY to FUNCTION.  Returns non-zero if KEY is out of range. */
rl_bind_key (key, function)
     int key;
     Function *function;
{
  if (key < 0 || key > 255)
    return (key);
  keymap[key] = function;
 return (0);
}


/* Make KEY do nothing. */
rl_unbind_key (key)
     int key;
{
  return ( rl_bind_key (key, (Function *)NULL));
}

/* Return a pointer to the function that STRING represents. 
   If STRING doesn't have a matching function, then a NULL pointer
   is returned. */
Function *
rl_named_function (string)
     char *string;
{
  register int i;

  for (i = 0; funmap[i]->name; i++)
    if (stricmp (funmap[i]->name, string) == 0)
      return (funmap[i]->function);
  return ((Function *)NULL);
}

/* Do key bindings from a file.  If FILENAME is NULL it defaults
   to `~/.inputrc'.  If the file existed and could be opened and
   read, 0 is returned, otherwise errno is returned. */
rl_read_init_file (filename)
     char *filename;
{
  extern int errno;
  int line_size, line_index;
  char *line = (char *)xmalloc (line_size = 100);

  FILE *file;

  int c;

  /* Default the filename. */
  if (!filename)
    filename = "~/.inputrc";

  filename = tilde_expand (filename);

  /* Open the file. */
  file = fopen (filename, "r");
  if (!file)
    return (errno);
  free (filename);

  /* Loop reading lines from the file.  Lines that start with `#' are
     comments, all other lines are commands for readline initialization. */
  while ((c = rl_getc (file)) != EOF)
    {
      /* If comment, flush to EOL. */
      if (c == '#')
	{
	  while ((c = rl_getc (file)) != EOF && c != '\n');
	  if (c == EOF)
	    goto function_exit;
	  continue;
	}

      /* Otherwise, this is the start of a line.  Read the
	 line from the file. */
      line_index = 0;
      while (c != EOF && c != '\n')
	{
	  line[line_index++] = c;
	  if (line_index == line_size)
	    line = (char *)xrealloc (line, line_size += 100);
	  c = rl_getc (file);
	}
      line[line_index] = '\0';

      /* Parse the line. */
      rl_parse_and_bind (line);
    }

function_exit:

  free (line);
  /* Close up the file and exit. */
  fclose (file);
  return (0);
}

/* Read the binding command from STRING and perform it.
   A key binding command looks like: Keyname: function-name\0,
   a variable binding command looks like: set variable value. */
rl_parse_and_bind (string)
     char *string;
{
  char *rindex (), *funname, *kname;
  int key, i;

  if (!string || !*string || *string == '#')
    return;

  /* Advance to the colon (:) or whitespace which separates the two objects. */
  for (i = 0; string[i] && string[i] != ':' && string[i] != ' '; i++);

  /* Mark the end of the command (or keyname). */
  if (string[i])
    string[i++] = '\0';

  /* If this is a command to set a variable, then do that. */
  if (stricmp (string, "set") == 0) {
    char *var = string + i;
    char *value;

    /* Make VAR point to start of variable name. */
    while (*var && whitespace (*var)) var++;

    /* Make value point to start of value string. */
    value = var;
    while (*value && !whitespace (*value)) value++;
    if (*value)
      *value++ = '\0';
    while (*value && whitespace (*value)) value++;

    rl_variable_bind (var, value);
    return;
  }

  /* Skip any whitespace between keyname and funname. */
  for (; string[i] && whitespace (string[i]); i++);
  funname = &string[i];

  /* No extra whitespace at the end of the string. */
  for (; string[i] && !whitespace (string[i]); i++);
  string[i] = '\0';

  /* Get the actual character we want to deal with. */
  kname = rindex (string, '-');
  if (!kname)
    kname = string;
  else
    kname++;

  key = glean_key_from_name (kname);

  /* Add in control and meta bits. */
  if (substring_member_of_array (string, possible_control_prefixes))
    key = CTRL (to_upper (key));

  if (substring_member_of_array (string, possible_meta_prefixes))
    key = META (key);

  rl_bind_key (key, rl_named_function (funname));
}

rl_variable_bind (name, value)
     char *name, *value;
{
  if (stricmp (name, "editing-mode") == 0)
    {
      if (strnicmp (value, "vi", 2) == 0)
	{
	  keymap = vi_insertion_keymap;
	  rl_editing_mode = vi_mode;
	}
      else if (strnicmp (value, "emacs", 5) == 0)
	{
	  keymap = emacs_standard_keymap;
	  rl_editing_mode = emacs_mode;
	}
    }
  else if (stricmp (name, "horizontal-scroll-mode") == 0)
    {
      if (!*value || stricmp (value, "On") == 0)
	horizontal_scroll_mode = 1;
      else
	horizontal_scroll_mode = 0;
    }
}

/* Return the character which matches NAME.
   For example, `Space' returns ' '. */

typedef struct {
  char *name;
  int value;
} assoc_list;

assoc_list name_key_alist[] = {
  { "Space", ' ' },
  { "SPC", ' ' },
  { "Rubout", 0x7f },
  { "DEL", 0x7f },
  { "Tab", 0x09 },
  { "Newline", '\n' },
  { "Return", '\n' },
  { "LFD", '\n' },
  { "Escape", '\033' },
  { "ESC", '\033' },

  { (char *)0x0, 0 }
};

glean_key_from_name (name)
     char *name;
{
  register int i;

  for (i = 0; name_key_alist[i].name; i++) 
    if (stricmp (name, name_key_alist[i].name) == 0)
      return (name_key_alist[i].value);

  return (*name);
}


/* **************************************************************** */
/*								    */
/*			String Utlity Functions			    */
/*								    */
/* **************************************************************** */

/* Return non-zero if any members of ARRAY are a substring in STRING. */
substring_member_of_array (string, array)
     char *string, **array;
{
  char *strindex ();
  register int i, l;
  int len = strlen (string);

  while (*array) {
    if (strindex (string, *array))
	return (1);
    array++;
  }
  return (0);
}

/* Whoops, Unix doesn't have strnicmp. */

/* Compare at most COUNT characters from string1 to string2.  Case
   doesn't matter. */
strnicmp (string1, string2, count)
     char *string1, *string2;
{
  register char ch1, ch2;

  while (count) {
    ch1 = *string1++;
    ch2 = *string2++;
    if (to_upper(ch1) == to_upper(ch2))
      count--;
    else break;
  }
  return (count);
}

/* strcmp (), but caseless. */
stricmp (string1, string2)
     char *string1, *string2;
{
  register char ch1, ch2;

  while (*string1 && *string2) {
    ch1 = *string1++;
    ch2 = *string2++;
    if (to_upper(ch1) != to_upper(ch2))
      return (1);
  }
  return (*string1 | *string2);
}

/* Determine if s2 occurs in s1.  If so, return a pointer to the
   match in s1.  The compare is case insensitive. */
char *
strindex (s1, s2)
     register char *s1, *s2;
{
  register int i, l = strlen (s2);
  register int len = strlen (s1);

  for (i = 0; (len - i) >= l; i++)
    if (strnicmp (&s1[i], s2, l) == 0)
      return (s1 + i);
  return ((char *)NULL);
}


/* **************************************************************** */
/*								    */
/*			SYSV Support				    */
/*								    */
/* **************************************************************** */

/* Since system V reads input differently than we do, I have to
   make a special version of getc for that. */

#ifdef SYSV

extern int errno;
#include <sys/errno.h>

rl_getc (stream)
     FILE *stream;
{
  int result;
  unsigned char c;

  while (1)
    {
      result = read (fileno (stream), &c, sizeof (char));
      if (result == sizeof (char))
	return (c);

      if (errno != EINTR)
	return (EOF);
    }
}
#else
rl_getc (stream)
     FILE *stream;
{
  return (getc (stream));
}
#endif


/* **************************************************************** */
/*								    */
/*			xmalloc and xrealloc ()		     	    */
/*								    */
/* **************************************************************** */

static char *
xmalloc (bytes)
     int bytes;
{
  static memory_error_and_abort ();
  char *temp = (char *)malloc (bytes);

  if (!temp)
    memory_error_and_abort ();
  return (temp);
}

static char *
xrealloc (pointer, bytes)
     char *pointer;
     int bytes;
{
  static memory_error_and_abort ();
  char *temp = (char *)realloc (pointer, bytes);

  if (!temp)
    memory_error_and_abort ();
  return (temp);
}

static
memory_error_and_abort ()
{
  fprintf (stderr, "readline: Out of virtual memory!\n");
  abort ();
}


/* **************************************************************** */
/*								    */
/*			Testing Readline			    */
/*								    */
/* **************************************************************** */

#ifdef TEST

main ()
{
  HIST_ENTRY **history_list ();
  char *temp = (char *)NULL;
  char *prompt = "readline% ";
  int done = 0;

  while (!done) {
    temp = readline (prompt);

    /* Test for EOF. */
    if ((long)temp == (long)-1)
      exit (1);

    /* If there is anything on the line, print it and remember it. */
    if (*temp) {
      fprintf (stderr, "%s\r\n", temp);
      add_history (temp);
    }

    /* Check for `command' that we handle. */
    if (strcmp (temp, "quit") == 0)
      done = 1;

    if (strcmp (temp, "list") == 0) {
      HIST_ENTRY **list = history_list ();
      register int i;
      if (list) {
	for (i = 0; list[i]; i++) {
	  fprintf (stderr, "%d: %s\r\n", i, list[i]->line);
	  free (list[i]->line);
	}
	free (list);
      }
    }
    free (temp);
  }
}

#endif


/*
 * Local variables:
 * compile-command: "gcc -g -traditional -DTEST -o readline readline.c history.o -ltermcap"
 * end:
 */
