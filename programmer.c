/* Tektronix programming extensions to GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/programmer.c,v 1.48 91/01/13 23:44:36 robertb Exp $
   $Locker:  $

 */


#ifdef TEK_PROG_HACK  /* this covers the whole file! */



/*     +-----------------+     */
/*  -=#|  Include Files  |#=-  */
/*     +-----------------+     */



#include "defs.h"
#include "param.h"
#include "ui.h"
#include "obstack.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "regex.h"
#include "symtab.h"
#include "frame.h"
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>



/*     +-------------+     */
/*  -=#|  #Define's  |#=-  */
/*     +-------------+     */



#define BYTEWIDTH            8
#define obstack_chunk_alloc  xmalloc
#define obstack_chunk_free   free
#define EAT_WHITESP_FRD(p)    while (*(p)&&((*(p)==' ')||(*(p)=='\t'))) ++(p);
#define EAT_WHITESP_BWD(p)    while ((*(p) == ' ') || (*(p) == '\t')) --(p);
#define iscmdchar(c)          (isalpha(c)||((c)=='-')||isdigit(c))
#define DFLTSHIFTTOLOWER      1
#define DFLTSCANLIMIT         1
#define DFLTTICKCHR           '`'
#define OPENBRACE             '{'
#define CLOSEBRACE            '}'
#define STARCHR               '*'
#define MATCHCHR              'm'
#define COUNTCHR              'c'
#define PRINTCHR              'p'
#define EXAMINECHR            'x'
#define DFLTSUCCEEDEDMSG      "succeeded"
#define DFLTFAILEDMSG         "failed"
#define MAXPROMPTLEN          128
#define MACROSTACK            "macro-stack"



/*     +------------------------+     */
/*  -=#|  External Subroutines  |#=-  */
/*     +------------------------+     */



extern void free ();
extern char *index ();
extern void execute_command ();
extern void add_com ();
extern char *validate_comname ();
extern void dont_repeat ();
extern void nested_commands_command ();
extern void free_command_lines ();
extern void source_cleanup ();
extern char *command_line_input ();
extern struct format_data decode_format ();
extern void validate_format ();
extern struct expression *parse_c_expression ();
extern struct expression *parse_c_1 ();
extern char *last_break_macro ();



/*     +-----------------------------------------------------------+     */
/*  -=#|  Data Global To Entire Program, Not Defined In This File  |#=-  */
/*     +-----------------------------------------------------------+     */



extern int autorepeat;
extern struct cmd_list_element *setlist;
extern struct cmd_list_element *cmdlist;
extern struct cmd_list_element *set_cmd;
extern char *gbl_prompt;
extern FILE *instream;
extern int usingX;
extern char last_format;
extern char _rcsid[];
extern char *version;
extern char *source_path;
extern int selected_frame_level;



/*     +--------------------+     */
/*  -=#|  Type Definitions  |#=-  */
/*     +--------------------+     */



/* This structure is used for one-way linked lists.  The data item stored in
   the elements of these lists is a character string pointer.  The two main
   uses of these lists are to hold the arguments to a user defined function
   and to hold the results of a match statment. */

struct string_list_el {
  char *string;
  struct string_list_el *next;
};
typedef struct string_list_el *string_list_el;

/* This next structure is used to impliment a stack that keeps track of the
   conditional command state.  Each IF command pushes a new element on the
   stack which will be removed from the stack by the matching ENDIF command.
   Each such "execution level" can be thought of as being divided into blocks
   by ELIF and ELSE commands.  (There can be only one block beginning with an
   ELSE command and it must be the last block in the level.)  Only one block
   of commands in a level will be executed.  While looking for which block to
   execute (by evaluating IF and ELIF expressions) the execute state for the
   level will be "exec_disabled".  When the block to execute (if any) is found,
   the execution state is set to "exec_enabled" while that block is executed.
   Once a block is executed, the execution state for this level is set to
   "exec_skiping" and nothing gets executed while this level is active. */

enum exec_state_type {
  exec_enabled, exec_disabled, exec_skipping,
};

struct cond_level
{
  enum exec_state_type exec_state;
  int else_ok;
  struct cond_level *prev;
};
typedef struct cond_level *cond_level;

/* This structure is used to impliment the stack behind "scanlimit".
   "Scanlimit" specifies how many times an input line is scanned for macros
   durring a single call to "macro_expand()".
   
    This is also used to impliment the stacks behind "shift-to-lower" and
    "macroescape". */

struct intlist_element {
  int value;
  struct intlist_element *next;
};

/* This data struct is from printcmd.c: it should be in a common include.  It
   is used here as part of the interface to "print_formatted()".  */

struct format_data
{
  int count;
  char format;
  char size;
};



/*     +----------------------+     */
/*  -=#|  Forward References  |#=-  */
/*     +----------------------+     */



static char		       *skip_quoted_string ();
static void		       error_bad_arg ();
static int		       count_strings ();
static char		       *get_a_string ();
static void		       print_string_list ();
static string_list_el	       new_string_list_el ();
static void		       arg_expansions_cleanup ();
static void		       match_expansions_cleanup ();
static void		       pushed_int_cleanup ();
static struct cleanup	       *init_fake_pr_routines ();
static void		       restore_print_routines ();
static int		       fake_fprintf ();
static int		       fake_fputs ();
static int		       fake_fputc ();
static int		       fake_fflush (); 
static char		       *date_macro ();
static char		       *line_macro ();
static char		       *cur_function_macro ();
static char		       *cur_srcfile_macro ();
static char		       *level_macro ();
static char		       *is_outer_macro ();
static char		       *expand_print_or_examine ();
static void		       perform_star_expansion ();
static char		       *expand_single_macro ();
static char		       *do_macro_scan_right ();
       void		       set_either_prompt_command ();
static void		       eval_and_set_state ();
static void		       match_routine ();
       void		       free_macro_expansion ();
       void		       free_string_list ();
       struct cmd_list_element *my_lookup_cmd ();
       string_list_el	       parse_string_list ();
       struct command_line     *copy_command_lines ();
       void		       update_prompt ();
       void		       clear_cond_stack ();
       char		       *macro_expand ();
       void		       print_command_lines ();
       struct command_line     *read_command_lines ();
       int		       skip_execute ();
       struct cleanup	       *push_def_expansions ();
       struct cleanup	       *push_match_expansions ();
static void		       nested_while_command ();
static void		       nested_define_command ();
static void		       nested_document_command ();
static void		       macro_command ();
static void		       match_command ();
static void		       while_command ();
static void		       if_command ();
static void		       elif_command ();
static void		       else_command ();
static void		       endif_command ();
static void		       set_autorepeat ();
static void		       set_prompt_command ();
static void		       set_prompt2_command ();
static void		       set_macro_escape ();
static void		       set_macro_scan_limit ();
static void		       set_shift_to_lower ();
static void		       autorepeat_info ();
static void		       user_info ();
static void		       macro_info ();
static void		       shift_to_lower_info ();
       void		       _initialize_programmer ();
       void		       init_cmd_pointers ();

			       

/*     +-------------------------------------+     */
/*  -=#|  Data Global Within This File Only  |#=-  */
/*     +-------------------------------------+     */



static struct obstack macro_expansion_obstack;
static struct obstack *meo = &macro_expansion_obstack;

static string_list_el arg_expansions = 0;
static string_list_el match_expansions = 0;  

static int tickchr = DFLTTICKCHR;
static int macro_scan_limit = DFLTSCANLIMIT;
static struct intlist_element *scanlimit_stack = 0;
static struct intlist_element *shift_to_lower_stack = 0;
static struct intlist_element *tickchr_stack = 0;
static struct cmd_list_element *macrolist;
static struct string_list_el *macro_stack = 0;

static cond_level cond_stack = 0;
static int nesting_level = 0;

static char *prompt[2];

/* Strings used by all the "match" commands. */

static char *match_string, *match_pattern;
struct re_pattern_buffer buf;

/* The following "commands" are internal only: they are not on any list.  They
   are used when a command that causes nesting is executed.  These are
   initialized at start up.  */

static struct cmd_list_element nested_define;
static struct cmd_list_element nested_commands;
static struct cmd_list_element nested_document;
static struct cmd_list_element nested_while;

/* These variables are used to hold pointers to particular commands.  They are
   initialized when the commands are "created" durring start up.  They are
   used in comparisons with more general command pointers. */

static struct cmd_list_element *define_p;
static struct cmd_list_element *commands_p;
static struct cmd_list_element *document_p;
static struct cmd_list_element *while_p;
static struct cmd_list_element *elif_p;
static struct cmd_list_element *if_p;
static struct cmd_list_element *endif_p;

/* The following variables are used in faking out the print routines durring
   the expansion of the `p and `x macros.  Basicly what happens is that some
   entries in the user interface vector are temporarily replaced so that the
   the internals of the "print" or "execute" commands can be executed and
   their output "captured".  This captured text then becomes the expantion
   macro in question.  */

static int (*save_fprintf)();
static int (*save_fputs)();
static int (*save_fputc)();
static int (*save_fflush)();
static int save_usingX;
static int fake_pr_buffer_size = 0;
static char *fake_pr_buffer = 0;
static char *fake_pr_ptr;



/*     +------------------------------------------------+     */
/*  -=#|  Utility Routines Available In This File Only  |#=-  */
/*     +------------------------------------------------+     */



/* Skip a quoted string: return pointer to matching quote ("). */

static char *
skip_quoted_string (c)
     register char *c;
{
  if (*c != '"')
    ui_badnews (-1, "internal: skip_quoted_string(), no quote: %s ", c);
  for (;;)
    {
      ++c;
      if (*c == '"')
	return c;
      if (*c == '\\')
        ++c;
    }
#ifdef lint
  return c;
#endif
}


/* Print error message for unexpected argument. */

static void
error_bad_arg (want, got)
     char *want;
     char *got;
{
  ui_badnews (-1, "This command wants %s, you typed \"%s\"", want, got);
}


/*
 *  ---  Routines to deal with string lists.  ---
 */


/* Count the number of entries in string list "l". */

static
count_strings (l)
     string_list_el l;
{
  int count = 0;

  for (; l; l = l->next) ++count;
  return count;
}


/* Return the "n"th element in string list "l". */

static char *
get_a_string (l, n)
     string_list_el l;
     int n;
{
  int count = 0;

  for (; l; l = l->next)
    if (count++ == n)
      return l->string;

  return 0;
}


/* Print a string list using a given format for each element. */

static void
print_string_list (fmt, l)
     char *fmt;
     string_list_el l;
{
  for (; l; l=l->next)
    printf_filtered (fmt, l->string);
}


/* Allocate memory for a new string list element. */

static string_list_el
 new_string_list_el ()
{
  string_list_el new;
  
  new = (string_list_el) xmalloc (sizeof (struct string_list_el));
  new->next = 0;
  new->string = 0;
  return new;
}


/*
 *  ---  Various cleanup routines.  ---
 */


/* Free argument expansions. */

static void
arg_expansions_cleanup (prev)
     string_list_el prev;
{
  free_string_list (arg_expansions);
  arg_expansions = prev;
}


/* Free match expansions. */

static void
match_expansions_cleanup (prev)
     string_list_el prev;
{
  free_string_list (match_expansions);
  match_expansions = prev;
}


/* Free a pushed int after some error. */

static void
pushed_int_cleanup (thing)
     int *thing;
{
  struct intlist_element **badstack;
  struct intlist_element *badelement;

  if (thing == &tickchr)
    badstack = &tickchr_stack;
  else if (thing == &macro_scan_limit)
    badstack = &scanlimit_stack;
  else if (thing == &shift_to_lower)
    badstack = &shift_to_lower_stack;
  else
    ui_badnews (1, "pushed_int_cleanup called with unknown argument");

  badelement = *badstack;
  *thing = badelement->value;
  *badstack = badelement -> next;
  free (badelement);
}



/*
 *  ---  Routines that termporarily replace User Interface routines so  ---
 *  ---  that the output of gdb commands can be "captured".             ---
 */


/* Activate "capture output" mode by temporarily replacing some UI routines. */

static struct cleanup *
init_fake_pr_routines ()
{
  save_fprintf = uiVector->uiv_fprintf;
  save_fputs = uiVector->uiv_fputs;
  save_fputc = uiVector->uiv_fputc;
  save_fflush = uiVector->uiv_fflush;
  save_usingX = usingX;
  if (fake_pr_buffer == 0)
    {
      fake_pr_buffer = xmalloc (1024);
      fake_pr_buffer_size = 1024;
    }
  fake_pr_ptr = fake_pr_buffer;
  uiVector->uiv_fprintf = fake_fprintf;
  uiVector->uiv_fputs = fake_fputs;
  uiVector->uiv_fputc = fake_fputc;
  uiVector->uiv_fflush = fake_fflush;
  usingX = 1;
  return make_cleanup (restore_print_routines, 0);
}


/* Deactiveate "capture output" mode by resoreing some UI routines. */

static void
restore_print_routines ()
{
  uiVector->uiv_fprintf = save_fprintf;
  uiVector->uiv_fputs = save_fputs;
  uiVector->uiv_fputc = save_fputc;
  uiVector->uiv_fflush = save_fflush;
  usingX = save_usingX;
}


/* Temporary replacement for "ui_fprintf". */

static
fake_fprintf (stream, format, arg1, arg2, arg3, arg4, arg5, arg6)
     FILE *stream;
     char *format;
{
  if (stream != stdout)
    {
      (*save_fprintf) (stream, format, arg1, arg2, arg3, arg4, arg5, arg6);
      return;
    }

  if (fake_pr_buffer_size - (fake_pr_ptr - fake_pr_buffer) < 1024)
    {
      int delta = fake_pr_ptr - fake_pr_buffer;

      fake_pr_buffer_size += 1024;
      fake_pr_buffer = xrealloc (fake_pr_buffer, fake_pr_buffer_size);
      fake_pr_ptr = fake_pr_buffer + delta;
    }

  sprintf (fake_pr_ptr, format, arg1, arg2, arg3, arg4, arg5, arg6);
  fake_pr_ptr += strlen (fake_pr_ptr);
}


/* Temporary replacement for "ui_fputs". */

static
fake_fputs (buf, stream)
     char *buf;
     FILE *stream;
{
  int req;

  if (stream != stdout)
    {
      return (*save_fputs) (buf, stream);
    }

  req = strlen (buf);
  if (fake_pr_buffer_size - (fake_pr_ptr - fake_pr_buffer) < req)
    {
      int delta = fake_pr_ptr - fake_pr_buffer;

      fake_pr_buffer_size += 1024;
      fake_pr_buffer = xrealloc (fake_pr_buffer, fake_pr_buffer_size);
      fake_pr_ptr = fake_pr_buffer + delta;
    }

  *fake_pr_ptr = 0;
  (void) strcat (fake_pr_ptr, buf);
  fake_pr_ptr += strlen (fake_pr_ptr);
}


/* Temporary replacement for "ui_fputc". */

static
fake_fputc (ch, stream)
     FILE *stream;
{
  if (stream != stdout)
    {
      (*save_fputc) (ch, stream);
      return;
    }

  if (ch == 0)
    return;

  if (fake_pr_buffer_size - (fake_pr_ptr - fake_pr_buffer) < 2)
    {
      int delta = fake_pr_ptr - fake_pr_buffer;

      fake_pr_buffer_size += 1024;
      fake_pr_buffer = xrealloc (fake_pr_buffer, fake_pr_buffer_size);
      fake_pr_ptr = fake_pr_buffer + delta;
    }

  *fake_pr_ptr++ = ch;
}


/* Temporary replacement for "ui_fflush". */

static 
fake_fflush (stream)
     FILE *stream;
{
  if (stream != stdout)
    (*save_fflush) (stream);
}


/*
 *  ---  Builtin macros: support routines.  ---
 */


/* `date - The current date, formatted as specified by "fmt". */

static char *
date_macro (fmt)
     char *fmt;
{
#ifdef SYSV
  char date_string[256];
  char *eol, *ret;
  time_t machine_time;

  date_string[0] = 0;
  ret = 0;
  time (&machine_time);
  cftime (date_string, fmt, &machine_time);
  if (*date_string)
    {
      eol = date_string + strlen(date_string) - 1;
      if (*eol == '\n')
	*eol = 0;
      ret = &date_string[0];
    }
  return ret;
#else
  return "date_macro routine is broken";
#endif
}


/* `line - The source line currently being executed. */

static char *
line_macro (arg)
  char *arg;
{
  register int c;
  register int desc, n;
  register FILE *stream;
  char *thisline = (char *) xmalloc(256);
  register int line = current_source_line + 5;

  make_cleanup(free, thisline);
  thisline[0] = 0;
  if (symtab_list == 0 && partial_symtab_list == 0)
	return thisline;

  /* Pull in a current source symtab if necessary */
  if (current_source_symtab == 0)
      select_source_symtab(0);
  if (current_source_symtab == 0)
      ui_badnews(-1,"No default source file yet.");
  desc = openp (source_path, 0, current_source_symtab->filename, 
	O_RDONLY, 0, &current_source_symtab->fullname);
  if (desc < 0)
  {
    extern int errno;

    perror_with_name (current_source_symtab->filename);
    print_sys_errmsg (current_source_symtab->filename, errno);
    return;
  }
  if (current_source_symtab->line_charpos == 0)
      find_source_lines (current_source_symtab, desc);

  if (line < 1 || line > 
    current_source_symtab->nlines) {
        close (desc);
        ui_badnews(-1,"Line number out of range; %s has %d lines.",
           current_source_symtab->filename, current_source_symtab->nlines);
  }

  if (lseek (desc, 
	current_source_symtab->line_charpos[line - 1], 0) < 0)
  {
    close (desc);
    perror_with_name (current_source_symtab->filename);
  }

  stream = fdopen (desc, "r");
  clearerr (stream);

  c = fgetc (stream);
  if (c == EOF) return thisline;
  sprintf(thisline, "%d", line);
  n = strlen(thisline);
  thisline[n++] = '\t';
  do {
      if (c < 040 && c != '\t' && c != '\n') {
          thisline[n++] = '^';
          thisline[n++] = (char) (c + 0100);
      }
      else if (c == 0177) {
          thisline[n++] = '^';
          thisline[n++] = '?';
      }
      else if (c == '\\') {
	  thisline[n++] = c; 
          thisline[n++] = c;
      }
      else
          thisline[n++] = c;
  } while (c != '\n' && (c = fgetc (stream)) >= 0);
  fclose(stream);
  thisline[n - 1] = 0;
  return thisline;
}


/* `func - The name of the function currently being executed. */

static char *
cur_function_macro(arg)
    char *arg;
{
  extern FRAME selected_frame;
  struct symbol *func;
  char *funname = 0;

  if (selected_frame == 0) return 0;

  func = get_frame_function (selected_frame);
  if (func)
    funname = SYMBOL_NAME (func);

  return funname;
}


/* `file - Name of current source file. */

static char *
cur_srcfile_macro (arg)
    char *arg;
{
  /* Pull in a current source symtab if necessary */
  if (current_source_symtab == 0)
      select_source_symtab(0);
  if (current_source_symtab == 0)
      return 0;
  return current_source_symtab->filename;
}


/* `level - Level number of current stack frame. */

static char *
level_macro ()
{
  char tmp[8];

  sprintf(tmp, "%d", selected_frame_level);
  return tmp;
}


/* `isouter - 1 if current frame is outer-most, 0 otherwise. */

static char *
is_outer_macro ()
{
  char tmp[2];
  register FRAME frame;
  int count = 1;

  if (! have_inferior_p() && ! have_core_file_p())
     count = 1;
  else {
     frame = find_relative_frame(selected_frame, &count);
 /* find_relative_frame zeros count if it's able to go up one frame, in
  * which case we must not be in the outermost frame.
  */
  }
  sprintf(tmp, "%d", count);
  return tmp;
}


/*
 *  ---  Support routines for macro expansion.  ---
 */


/* Take care of the `x or `p macros. */

static char *
expand_print_or_examine (is_print, arg, brace)
     char *arg;
     char *brace;
{
  struct expression *expr;
  register struct cleanup *old_chain = 0;
  register char format = 0;
  register value val;
  struct format_data fmt;
  char *tmpstr;
  CORE_ADDR examine_address;

  if (brace)
    {
      tmpstr = savestring (arg, brace - arg);
      old_chain = make_cleanup (free, tmpstr);
    }
  else
    tmpstr = arg;

  if (tmpstr && *tmpstr == '/')
    {
      tmpstr++;
      fmt = decode_format (&tmpstr, last_format, 0);
      if (is_print)
	validate_format (fmt, "print");
      last_format = format = fmt.format;
    }

  if (tmpstr && *tmpstr)
    {
      expr = parse_c_1 (&tmpstr, 0, 0);
      if (brace && *tmpstr)
	ui_badnews (-1, "Junk after end of expression in macro.");
      if (old_chain)
	(void) make_cleanup (free_current_contents, &expr);
      else
	old_chain = make_cleanup (free_current_contents, &expr);
      val = evaluate_expression (expr);
    }
  else
    val = access_value_history (0);

  if (old_chain)
    (void) init_fake_pr_routines ();
  else
    old_chain = init_fake_pr_routines ();

  if (is_print)
    print_formatted (val, format, fmt.size, M_NORMAL);
  else
    {
      if (last_format == 'i'
	  && TYPE_CODE (VALUE_TYPE (val)) != TYPE_CODE_PTR
	  && VALUE_LVAL (val) == lval_memory)
	examine_address = VALUE_ADDRESS (val);
      else
	examine_address = (CORE_ADDR) value_as_long (val);
      do_examine (format, examine_address, M_NORMAL);
    }

  obstack_grow (meo, fake_pr_buffer, fake_pr_ptr - fake_pr_buffer);
  do_cleanups (old_chain);

  if (brace)
    return brace + 1;
  return tmpstr;
}


/* Take care of `*<n> or `m*<n> macros. */

static void
perform_star_expansion (l, n)
     string_list_el l;
     int n;
{
  int count = 0;
  int need_blank = 0;

  for (; l; l = l->next)
    if (count++ >= n)
      {
        if (need_blank)
          obstack_1grow (meo, ' ');
        need_blank = 1;
        obstack_grow (meo, l->string, strlen(l->string));
      }
}


/* Expand the macro pointed to by the variable pointed to by "pp" and incriment
   the variable pointed to by "pp" to point the the first character after the
   macro. */

static char *
expand_single_macro (pp)
     char **pp;
{
  char *p = *pp;
  char *p2;
  char *matching_brace = 0;
  int nestbrace;
  int is_star = 0;
  int have_number = 0;
  int number = 0;
  string_list_el which_list = arg_expansions;
  struct cmd_list_element *c;
  static char sml_buffer[16];

  /* Make sure all is well. */
  
  if (*p != tickchr)
    ui_badnews (1, "Internal: expand_single_macro called wrong.");
  ++p;
  
  /* See if the macro starts with an open brace. */
  
  if (*p == OPENBRACE)
    {
      for (nestbrace = 0, p2 = ++p; *p2; ++p2)
	{
	  if (*p2 == OPENBRACE)
	    ++nestbrace;
	  else if ((*p2 == CLOSEBRACE) && (nestbrace-- == 0))
	    {
	      matching_brace = p2;
	      break;
	    }
	}
      if (!matching_brace)
        goto failure_label;
    }

  /* If the form is m..., then what follows references the match stuff. */
  
  if (*p == MATCHCHR)
    {
      ++p;
      if (isdigit (*p))
        /* `m<n>  */
        which_list = match_expansions;
      else if (! iscmdchar (*p))
        /* `m  */
        which_list = match_expansions;
      else if (*p=='c' && (! iscmdchar (p[1])))
        /* `mc */
        which_list = match_expansions;
      else if (*p == STARCHR && isdigit (p[1]))
        /* `m*<n> */
        which_list = match_expansions;

      if (which_list != match_expansions)
        --p;
    }
  
  /* See if a count is being requested: `c `{c} `mc or `{mc} */
  
  if (*p == COUNTCHR && (strlen(p) == 1 || *(p + 1) == '}' ||
	*(p + 1) == ' ' || *(p + 1) == '\t' || *(p + 1) == '\\'))
    {
      ++p;
      if (matching_brace && (matching_brace != p))
        goto failure_label;
      sprintf (sml_buffer, "%d", count_strings (which_list));
      *pp = p;
      return sml_buffer;
    }
  
  else 
    {

  /* Handle the "special" macros first */

  	p2 = p;
  	c = lookup_cmd (&p2, macrolist, "*macro*", -1, 0);
  	if ((c) && (! matching_brace || (p2 == matching_brace)))
    	{
      	    if (matching_brace)
		++p2;
            while (isspace (p2[-1]))  --p2;
      	    *pp = p2;
      	    if (c->function)
		p2 = (*((char *(*)())(c->function)))(c->aux);
              else
	    p2 = c->aux;
          return p2;
        }
    }

  /* Handle the form p[/<fmt> ]<expr>. */
  
  if (matching_brace && *p == PRINTCHR)
    {
      *pp = expand_print_or_examine (1, p+1, matching_brace);
      return 0;
    }
  
  /* Handle the form x[/<fmt> ]<expr>. */
  
  if (matching_brace && *p == EXAMINECHR)
    {
      *pp = expand_print_or_examine (0, p+1, matching_brace);
      return 0;
    }
  
  /* See if this is a "skip" form: `*<n> `{*<n>} `m*<n> or `{m*<n>} */
  
  if (*p == STARCHR)
    {
      ++p;
      ++is_star;
    }
  
  /* Grab the <n> part, if any. */
  
  if (isdigit (*p))
    {
      have_number = 1;
      number = atoi (p);
      while (isdigit (*++p)) /*null*/;
    }
  
  /* At this point, we should have everything for all types of macros except
     "special" ones like `func, `file, and so on. */
  
  /* Make sure we match any open brace. */
  
  if (matching_brace && (p != matching_brace))
    if ((which_list == match_expansions) || is_star || have_number)
      goto failure_label;
  
  /* See if we want all the expansions except the first <n>.  If <n> is
     ommited, don't skip any.  Each expansion will be surrounded by a single
     blank. */
  
  if (is_star)
    {
      if (matching_brace)
        *pp = matching_brace + 1;
      else
        *pp = p;
      perform_star_expansion (which_list, number);
      return 0;
    }
  
  /* See if we want the <n>'th expansion. */
  
  if (have_number)
    {
      *pp = p;
      return get_a_string (which_list, number);
    }
  
  /* See if it is the form `m or `{m} */
  
  if (which_list && (which_list == match_expansions))
    {
      *pp = p;
      p2 = (count_strings (match_expansions) ? "success" : "failure");
      c = lookup_cmd (&p2, macrolist, "*macro*", -1, 0);
      if (c)
	return  c->aux;
    }
  
 failure_label:
  ui_badnews (-1, "Ill formed macro");
}


/* Scan the text pointed to by "in", expanding any macros found in it. */

static char *
do_macro_scan_right (in)
     char *in;
{
  char *p;
  int cnt;
  
  p = index (in, tickchr);
  if (p == 0)
    return 0;
  
  while (p)
    {
      obstack_grow (meo, in, p - in);
      in = p;
      while ((*in == tickchr) && (in[1] == tickchr))
        {
          obstack_1grow (meo, tickchr);
          in += 2;
        }
     /* if (in != p) break;  pjg  */
      if (*in == tickchr)
        {
          p = expand_single_macro (&in);
          if (p)
            obstack_grow (meo, p, strlen(p));
        }
      p = index (in, tickchr);
    }
  return obstack_copy0 (meo, in, strlen (in));
}


/*
 *  ---  Support routines for GDB commands defined below  ---
 */


/* Set either prompt: promp[0] is the major promt, prompt[1] is secondary. */

void
set_either_prompt_command (which, text)
     int which;
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
  free (prompt[which]);
  prompt[which] = new;
  update_prompt (0);
}


/* Called from if_command and elif_command */

static void
eval_and_set_state (arg, level)
     char *arg;
     cond_level level;
{
  struct expression *expr;
  value val;
  int cleanup = 0;
  struct cleanup *old_chain = 0;
  char *cp = arg ? arg + 5 : 0;
  char *tem;
  int is_not = 0;

  /* See if this is a "not". */

  if (arg && strncmp (arg, "not", 3) == 0 && isspace (arg[3]))
    {
      is_not = 1;
      arg += 3;
      EAT_WHITESP_FRD (arg);
    }

  /* Either evaluate the argument or get a value from history. */
  
  if (arg && strncmp (arg, "match", 5) == 0 && isspace (arg[5]))
    {
      
      /* Doing 'if match' or 'elif match' */

      cp = arg + 5;
      EAT_WHITESP_FRD (cp);
      match_command(cp);
      /* 'if' succeeds only if we get a total match */
      if (match_string && strlen(match_string) &&
	  strncmp (match_string, match_expansions->string,
		  strlen(match_string)) == 0) {
	/*
	 * we have a total match.
	 */
        level->exec_state = is_not ? exec_disabled : exec_enabled;
      }
      else
        level->exec_state = is_not ? exec_enabled : exec_disabled;
      return;
    }
  else if (arg && strncmp (arg, "defined", 7) == 0 && isspace (arg[7]))
    {
      tem = arg + 8;
      if (lookup_cmd (&tem, macrolist, "", -1, 0))
        level->exec_state = is_not ? exec_disabled : exec_enabled;
      else
        level->exec_state = is_not ? exec_enabled : exec_disabled;
      return;
    }
  else if (arg && *arg)
    {
      expr = parse_c_expression (arg);
      old_chain = make_cleanup (free_current_contents, &expr);
      cleanup = 1;
      val = evaluate_expression (expr);
    }
  else
    val = access_value_history (0);
  
  /* Set the execution state depending on the value obtained above. */
  
  if (val->lval == lval_reg_invalid)
    ui_badnews(-1, "condition is #%s=%s#", reg_names[VALUE_REGNO (val)],
	       THE_UNKNOWN);
  level->exec_state = val->contents[0] ?
    (is_not ? exec_disabled : exec_enabled) :
      (is_not ? exec_enabled : exec_disabled);
  
  /* Clean up after ourself */
  
  if (cleanup)
    do_cleanups (old_chain);
}


/* This stores matches in a list, the head of which is pointed to by
 * match_expansions Notice that we don't free this list in this module.
 * Caller is responsible for that.
 * We store at most RE_NREGS - 1 matches (currently 9), even though we will
 * march through as many matches as are supplied in the pattern.  Thus matches
 * 10 and subsequent are lost.
 * The first element on the list is the entire string matched by pattern; if
 * no matches are specified in pattern by \(..\), that's all the list will
 * contain.  Thus
 *	"hiya"	hiya	generates a list containing one element, "hiya"
 *	"\(hiya\)" hiya generates a list containing two elements, "hiya"
 *		(entire match) and "hiya" (exact match returned)
 *	"hiya" woops    generates a zero list (match_expansions set to 0)
 */

static void
match_routine (matchbuf, mstring)
     struct re_pattern_buffer *matchbuf;
     char *mstring;
{
  int n = 1;
  struct re_registers myregs;
  string_list_el match_node;
  string_list_el last_node;
  int num_matched;
  
  num_matched = re_match(matchbuf, mstring, strlen(mstring),  0, &myregs);
  if (num_matched >= 1) {
    /* Save entire matched pattern */
    match_node = new_string_list_el ();
    match_node->string = savestring(mstring, num_matched);
    match_expansions = match_node;
    last_node = match_node;
    /* Save matches */
    while (n < RE_NREGS) {
      match_node->next = new_string_list_el ();
      match_node = match_node->next;
      if ((myregs.end[n] <= myregs.start[n])
	  || (myregs.end[n] > myregs.end[0]))
	{
	  match_node->string = savestring("", 0);
	}
      else
	{              
	  match_node->string =
	    savestring(mstring + myregs.start[n], 
		       myregs.end[n] - myregs.start[n]);
          last_node = match_node;
	}
      n++;
    }
    match_node = last_node->next;
    last_node->next = 0;
    while (match_node)
    {
      last_node = match_node;
      match_node = last_node -> next;
      free (last_node->string);
      free (last_node);
    }
  }
  else
    match_expansions = 0;
}



/*     +--------------------------------------+     */
/*  -=#|  Utility Routines Available Globaly  |#=-  */
/*     +--------------------------------------+     */


/* This is to allow us to expand the grammar set for command names without
 * munging either the expression parser or command.c.  We look at the incoming
 * command name and examine it for a certain set of known attributes, and
 * make the right substitutions accordingly, then call lookup_cmd() with the
 * result.
 */
struct cmd_list_element *
my_lookup_cmd (line, list, cmdtype, allow_unknown, ignore_help_classes)
    char **line;
    struct cmd_list_element *list;
    char *cmdtype;
    int allow_unknown;
    int ignore_help_classes;
{
/* Currently these are the 'special characters' we know about; if the command
 * starts with one of these, we substitute known values for the special
 * character and call lookup_cmd.
 *       >   redirect-output
 *  	 /   forward-search
 *       ?   reverse-search
 */
      char *p = *line;
      char *newcmd;

      while (*p == ' ' || *p == '\t') p++;
      if (*p != '/' && *p != '?' && *p != '>')
  	return (lookup_cmd(line, list, cmdtype, allow_unknown, 
		ignore_help_classes));

      if (*p == '>') {
	newcmd = (char *) alloca(strlen(p) + strlen("redirect-output "));
        sprintf(newcmd, "redirect-output %s", p + 1);
      }
      else if (*p == '/') {
	newcmd = (char *) alloca(strlen(p) + strlen("forward-search "));
        sprintf(newcmd, "forward-search %s", p + 1);
        p = newcmd + strlen(newcmd) - 1;
        while (*p == ' ' || *p == '\t') p--;
        if (*p == '/') *p = '\0';
      }
      else {  /* *p == '?' */
	newcmd = (char *) alloca(strlen(p) + strlen("reverse-search "));
        sprintf(newcmd, "reverse-search %s", p + 1);
        p = newcmd + strlen(newcmd) - 1;
        while (*p == ' ' || *p == '\t') p--;
        if (*p == '?') *p = '\0';
      }
      /*  free (*line); */
      *line = newcmd;
      return (lookup_cmd(line, list, cmdtype, allow_unknown, 
		ignore_help_classes));
}

/* Free old, macro expanded, line. */

void
free_macro_expansion (old)
     char *old;
{
  obstack_free (meo, old);
}


/* Free all the memory associated with a string list.  This includes the
   the memory for the string list elements and for the actual strings. */

void
free_string_list (l)
     string_list_el l;
{
  string_list_el next_l;

  for (; l; l = next_l)
    {
      next_l = l->next;
      if (l->string)
        free (l->string);
      free (l);
    }
}


/* Parse a string into a list of strings.  Strings deliminated by white
   space or by quotes (") if the string is quoted.  If a string is to
   contain white space, it should be quoted.
   Example:
     input:  this is "one test"and another
    output:  [this|->] [is|->] [one test|->] [and|->] [another|0] */

string_list_el
parse_string_list (text)
     char *text;
{
  char *first, *last;
  string_list_el ret_val, new, tail;
  
  if (text == 0)
    return 0;
  
  first = text;
  EAT_WHITESP_FRD (first);
  if (*first == '\0')
    return 0;
  
  ret_val = 0;
  last = first;
  while (*last)
    {
      if ((*last == '"') && (first == last))
	{
	  last = skip_quoted_string (first);
	  new = (string_list_el) xmalloc (sizeof (struct string_list_el));
	  new->string = savestring (first + 1, last - first - 1);
	  new->next = 0;
	  if (ret_val) 
	    tail->next = new;
	  else 
	    ret_val = new;
          tail = new;
          ++last;
	  EAT_WHITESP_FRD (last);
	  first = last;
	}
      else if ((*last == ' ') || (*last == '"'))
	{
	  new = (string_list_el) xmalloc (sizeof (struct string_list_el));
	  new->string = savestring (first, last - first);
	  new->next = 0;
	  if (ret_val)
	    tail->next = new;
	  else
	    ret_val = new;
	  tail = new;
	  EAT_WHITESP_FRD (last);
	  first = last;	  
	}
      if (*last && *last != '"')
	++last;
    }
  new = (string_list_el) xmalloc (sizeof (struct string_list_el));
  new->string = savestring (first, last - first);
  new->next = 0;
  if (ret_val)
    tail->next = new;
  else
    ret_val = new;
  return ret_val;
}


/* Update the prompt with respect to the level.  If "subcmd" is -1, the level
   is decreased by 1.  If "subcmd" is 0, leave the level alone.  If "subcmd"
   is is 1, increase the level by 1.  If "subcmd" is 2, set the level to 0.
   Once the level is updated,  generate a new prompt.  If the level is 0,
   the new prompt is the primary prompt (prompt[0]).  Otherwise the
   secondary prompt (prompt[1]) serves as a format for generating the new
   prompt (as in "sprintf (new_prompt, prompt[1], level)") */

void
update_prompt (subcmd)
{
  switch (subcmd) {
  case -1:
    --nesting_level;
    break;
  case 0:
    break;
  case 1:
    ++nesting_level;
    break;
  case 2:
    nesting_level = 0;
    break;
  default:
    ui_badnews (1, "Internal: update_prompt called with arg %d\n", subcmd);
  }
  
  if (gbl_prompt && *gbl_prompt && gbl_prompt != prompt[0])
    free (gbl_prompt);
  
  if (nesting_level == 0)
    gbl_prompt = prompt[0];
  else
    {
      gbl_prompt = (char *) xmalloc (MAXPROMPTLEN);
      sprintf (gbl_prompt, prompt[1], nesting_level);
    }
}


/* Clear the condition stack, freeing all its memeory. */  

void
clear_cond_stack ()
{
  cond_level level_to_free;
  
  while (cond_stack)
    {
      level_to_free = cond_stack;
      cond_stack = cond_stack->prev;
      free (level_to_free);
    }
  
  /* Fix up the nesting level too. */

  update_prompt (2);
}


/* Make one pass of macro expansion.  "in" points to the text to be scanned
   for macros and expanded.  "c" points to the command to which the text is
   attached.  If the command will not be executed due to the condition
   stack,  then expansion is skipped.  Durring one pass of macro expansion,
   the text my be scanned for macros severial times, depending on 
   "macro_scan_limit". */

char *
macro_expand (in, c)
     char *in;
     struct cmd_list_element *c;
{
  int scans_done;
  char *old, *new;

/* First we check the current execution state.  If this is
 *    exec_enabled	we expand
 *    exec_skipping     we don't expand
 *    exec_disabled     we expand ONLY if the current command is 'elif' and
 *	the previous level's state was not exec_disabled.
 */

  if (in == 0)
    return obstack_copy (meo, "", 1);
  
  old =  obstack_copy (meo, in, strlen (in) + 1);
  /* -rcb 6/90 added "cond_stack &&" to front of this if to eliminate
     segmentation fault here on the sparc. 

     -rcb 12/90  This broke macro expansion.  The code had depended on
     not only being able to dereference a nil pointer, but also on
     finding a zero there and 'exec_enabled' having an ordinal value of zero.
     yech.  */

  if (!cond_stack || ((cond_stack->exec_state == exec_enabled) || (cond_stack->exec_state ==
    exec_disabled && c == elif_p && cond_stack->prev->exec_state !=
    exec_disabled))) {
      for (scans_done = 0; scans_done < macro_scan_limit; ++scans_done)
        {
          new = do_macro_scan_right (old);
          if (new == 0)
            return old;
          free_macro_expansion (old);
          old = new;
        }
  }
  return old;
}


/* Decide if execution should be skipped based on the condition stack and on
   the particular command.  Return 1 if execution should be skipped, zero
   otherwise. */

skip_execute (c)
     struct cmd_list_element *c;
{
  if (cond_stack == 0)
    return 0;
  if (c->class == (int) class_conditional)
    return 0;
  return (cond_stack->exec_state != exec_enabled);
}


/* Push the old user defined command arguments and parse the new ones.  The
   cleanup list is used to maintain the stack of arguments via the cleanup
   routine arg_expansions_cleanup(). */

struct cleanup *
push_def_expansions (name, text, delims)
     char *name;
     char *text;
     string_list_el delims;
{
  struct cleanup *old_chain;
  string_list_el ret_val, new, tail;
  string_list_el cd;
  char *first, *last;
  char delim_char;
  char *p = text;
  int len;
  
  /* Push the old set of expansions via the cleanup list. */

  old_chain = make_cleanup (arg_expansions_cleanup, arg_expansions);

  /* If there is no text to parse, we are done. */

  if (p == 0)
    {
      arg_expansions = 0;
      return old_chain;
    }

  /* The first thing on the string list will be the name of the user
     defined command.  This corrasponds to `0. */

  ret_val = (string_list_el) xmalloc (sizeof (struct string_list_el));
  ret_val->string = savestring (name, strlen (name));
  ret_val->next = 0;
  tail = ret_val;

  /* If "text" is the empty string or blank, we are done. */

  EAT_WHITESP_FRD (p);
  if (p == 0)
    {
      arg_expansions = ret_val;
      return old_chain;
    }

  /* See if there is a single, one-character delimiter, like a comma. */

  delim_char = 0;
  if (delims == 0)
    delim_char = ',';
  else if ((count_strings (delims) == 1) &&
	   (strlen (delims->string) == 1))
    delim_char = *(delims->string);

  first = p;
  while (*p)
    {
      if (*p == '"')
	p = skip_quoted_string (p);
      else if (delim_char)
	{
	  /* There is a simple delimiter, so find the next one. */

	  if (*p == delim_char)
	    {
	      last = p++ - 1;
	      EAT_WHITESP_BWD (last);
	      new = (string_list_el) xmalloc (sizeof (struct string_list_el));
	      len = (last >= first) ? last - first + 1 : 0;
	      new->string = savestring (first, len);
	      new->next = 0;
	      tail->next = new;
	      tail = new;
	      EAT_WHITESP_FRD (p);
	      first = p;
	      continue;
	    }
	}
      else
	{
	  /* Check through the a list of delimiter strings. */

	  for (cd=delims; cd; cd=cd->next)
	    {
	      len = strlen(cd->string);
	      if ((strncmp (p, cd->string, len) == 0) &&
		  ((p == text) ? 1 : (p[-1] == ' ')) &&
		  ((p[len] ? (p[len] == ' ') : 1)))
		{
		  last = p - 1;
		  EAT_WHITESP_BWD (last);
		  if (last >= first)
		    {
		      new = (string_list_el)
			xmalloc (sizeof (struct string_list_el));
		      new->string = savestring (first, last - first + 1);
		      new->next = 0;
		      tail->next = new;
		      tail = new;
		    }
		  new = (string_list_el) xmalloc
		    (sizeof (struct string_list_el));
		  new->string = savestring (cd->string,
					       strlen (cd->string));
		  new->next = 0;
		  tail->next = new;
		  tail = new;
		  p += strlen (cd->string);
		  EAT_WHITESP_FRD (p);
		  first = p--;
		  break;
		}
	    }
	}
      if (*p)
	++p;
    }

  /* Handle the last argument. */

  last = p;
  EAT_WHITESP_BWD (last);
  if (last > first)
    {
      new = (string_list_el) xmalloc (sizeof (struct string_list_el));
      new->string = savestring (first, last - first);
      new->next = 0;
      tail->next = new;
    }
  arg_expansions = ret_val;
  return old_chain;
}


/* Push the old match returns.  The cleanup list is used to maintain the
   stack of match returns via the cleanup routine
   match_expansions_cleanup(). */

struct cleanup *
push_match_expansions ()
{
  struct cleanup *old_chain;
  
  old_chain = make_cleanup (match_expansions_cleanup, match_expansions);
  match_expansions = 0;
  return old_chain;
}


/*
 *  ---  Support routines for "command_line" lists.  ---
 */


/* Make a deep copy of a command list. The text of all the new copies will
   have been processed by macro_expand exactly once durring the copy.  For
   a complete description of how "command_line"s are used, see the comment
   after the decliration for "command_line" in the defs.h include file. */

struct command_line *
copy_command_lines (in)
     register struct command_line *in;
{
  register struct command_line *out;
  extern char *macro_expand();
  
  if (in == 0)
    return 0;
  
  out = (struct command_line *) xmalloc (sizeof (struct command_line));
  
  out->cmd = in->cmd;
  if (in->cmd && (in->cmd->class == class_nested))
    out->line = (char *) copy_command_lines ((struct command_lines *)
					     in->line);
  else {
    char *tmp;

    tmp = macro_expand(in->line, in->cmd);
    out->line = savestring(tmp, strlen (tmp));
    free_macro_expansion(tmp);
  }
  out->next = copy_command_lines (in->next);
  
  return out;
}


/* Print a "command_line" list at a given nesting/condition level.  This 
   does not handle the case where "cmd" is -1,  that must be check for
   before this is called.  The major "wrinkle" here is for dealing with the
   case of sub-commands.  If "cmd"->aux is non-zero, it points to the
   cmd_list_element of the command that is the "parent" of this command.  In
   this way, we can reconstruct the "fully qualified" command.  */

void
print_command_lines (lvl, cmdlines)
     int lvl;
register struct command_line *cmdlines;
{
  int i;
  
  for (; cmdlines; cmdlines = cmdlines->next)
    {
      printf_filtered ("\t");
      if (cmdlines->cmd)
        {
          if (cmdlines->cmd->class == class_nested)
            {
              for (i = 0; i < lvl; ++i)
                printf_filtered ("  ");
              printf_filtered ("%s %s\n", cmdlines->cmd->name,
                               ((struct command_line *)
                                (cmdlines->line))->line);
              print_command_lines (lvl + 1,
                                   ((struct command_line *)
                                    (cmdlines->line))->next);
              printf_filtered ("\t");
              for (i = 0; i < lvl; ++i)
                printf_filtered ("  ");
              printf_filtered ("end\n");
              continue;
            }
          if ((cmdlines->cmd->class == class_conditional)
	      && (cmdlines->cmd != if_p))
            --lvl;
          for (i = 0; i < lvl; ++i)
            printf_filtered ("  ");
	  if (cmdlines->cmd->aux)
	    printf_filtered ("%s",
              ((struct cmd_list_element *)(cmdlines->cmd->aux))->prefixname);
          printf_filtered ("%s %s\n", cmdlines->cmd->name, cmdlines->line);
          if ((cmdlines->cmd->class == class_conditional)
	      && (cmdlines->cmd != endif_p))
            ++lvl;
        }
      else
        {
          for (i = 0; i < lvl; ++i)
            printf_filtered ("  ");
          printf_filtered ("%s\n", cmdlines->line);
        }
    }
}


/* Read command lines forming a "command_line" list. */

struct command_line *
read_command_lines (kind)
     int kind; /* 0  'document' command
                  1  'commands' command
		  2  'while'  command
		  3  'define' command
		  4  nested 'define' command
		  != 0 && != 4, check commands for validity.
		  == 1, a 'silent' command is a legal initial command. */
{
  struct command_line *first = 0;
  register struct command_line *next, *tail = 0;
  register char *p, *p1;
  struct cleanup *old_chain = 0;
  register struct cmd_list_element *c;
  char *psav;
  struct cmd_list_element *nested_cmd;
  cond_level starting_level = cond_stack;
  int new_kind;
  
  update_prompt (1);
  
  while (1)
    {
      dont_repeat ();
      p = command_line_input ((instream == stdin) ? gbl_prompt : 0,
                              instream == stdin);
      if (p == (char *)EOF)
	break;
      /* Remove leading and trailing blanks.  */
      while (*p == ' ' || *p == '\t') p++;
      /*if (kind == 1 || kind == 2) p = macro_expand(p);*/
      p1 = p + strlen (p);
      while (p1 != p && (p1[-1] == ' ' || p1[-1] == '\t')) p1--;
      
      /* Is this "end"?  */
      if (p1 - p == 3 && !strncmp (p, "end", 3))
	break;
      /* See if the command is a valid one. */
      switch (kind) {
      case 0:
        c = 0;
        break;
      case 4:  
       /* We could have a 'while' inside a define here, in which case
        * we'll have to change new_kind */
        psav = p;
        c = my_lookup_cmd (&psav, cmdlist, "", 1, 1);
        p = psav;
        p1 = p + strlen (p);
        while (p1 != p && (p1[-1] == ' ' || p1[-1] == '\t')) p1--;
        break;
      case 1:
        if ((tail == 0) && (p1 - p == 6) && !strncmp (p, "silent", 6))
          {
            c = 0;
            break;
          }
        /* Fall through if not first line or silent "command". */
      default:
        psav = p;
        c = my_lookup_cmd (&psav, cmdlist, "", 0, 1);
        if (c == NULL)
          ui_badnews (-1, "bad command: %s", p);
        if (c->function == 0)
          ui_badnews (-1,"%s: this is not a command, just a help topic.",p);
        p = psav;
        p1 = p + strlen (p);
        while (p1 != p && (p1[-1] == ' ' || p1[-1] == '\t')) p1--;
      }
      
      /* See if we should nest. It would be nice if the cmd_list_element
         could tell us this, but that would be more intrusive. */
      nested_cmd = 0;
      if (c == define_p)
        {
          nested_cmd = &nested_define;
          new_kind = 4;
        }
      else if (c == commands_p)
        {
          nested_cmd = &nested_commands;
          new_kind = 1;
        }
      else if (c == document_p)
        {
          nested_cmd = &nested_document;
          new_kind = 0;
        }
      else if (c == while_p)
        {
          nested_cmd = &nested_while;
          new_kind = 2;
        }
      else if (c == if_p)
        {
          update_prompt (1);
        }
      else if (c == endif_p)
        {
          update_prompt (-1);
        }
      
      if (nested_cmd)
        {
          next = (struct command_line *)
	    xmalloc (sizeof (struct command_line));
          next->cmd = nested_cmd;
          next->line = (char *) xmalloc (sizeof (struct command_line));
          ((struct command_line *)(next->line))->cmd = 0;
          ((struct command_line *)(next->line))->line =
	    savestring (p, p1 - p);
          ((struct command_line *)(next->line))->next =
            read_command_lines (new_kind);
          next->next = 0;
        }
      else
        {
	  
	  /* No => add this line to the chain of command lines.  */
	  next = (struct command_line *)
	    xmalloc (sizeof (struct command_line));
	  next->cmd = c;
	  next->line = savestring (p, p1 - p);
	  next->next = 0;
        }
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
  
  if (starting_level != cond_stack)
    ui_badnews (-1, "Missing \"endif\" command(s)");
  update_prompt (-1);
  
  /* Now we are about to return the chain to our caller,
     so freeing it becomes his responsibility.  */
  if (first)
    discard_cleanups (old_chain);
  return first;
}



/*     +-----------------------+     */
/*  -=#|  Nested GDB Commands  |#=-  */
/*     +-----------------------+     */



/* Nested "while" command. */

static void
nested_while_command (cmds, from_tty)
     char *cmds;
     int from_tty;
{
  register struct command_line *cmdlines, *l;
  register struct cmd_list_element *c;
  register char *cond;
  struct expression *expr;
  value val;
  struct cleanup *old_chain, *chain_2;
  char *cp;
  enum {cond_expression, do_match, do_defined} cond_type;
  int is_not = 0;
  char *tem;

  for (;;)
    {
      cond_type = cond_expression;
      cmdlines = (struct command_line *)cmds;
      cond = macro_expand (cmdlines->line, cmdlines->cmd);
      old_chain = make_cleanup (free_macro_expansion, cond);

      if (strncmp (cond, "not", 3) == 0 && isspace (cond[3]))
        {
          is_not = 1;
          cond += 3;
          EAT_WHITESP_FRD (cond);
        }
      
      if (strncmp (cond, "match", 5) == 0 && isspace (cond[5]))
        cond_type = do_match;
      else if (strncmp (cond, "defined", 7) == 0 && isspace (cond[7]))
        cond_type = do_defined;
      
      /* Test the condition. */
      
      if (cond_type == do_match)
        {
          cp = cond + 5;
          /* Doing 'while match' ' */
          EAT_WHITESP_FRD (cp);
          match_command(cp);
          /* 'while' succeeds only if we get a total match */
          if (match_string && strlen(match_string) &&
              strncmp (match_string, match_expansions->string,
                      strlen(match_string)) == 0) {
            /*
             * we have a total match.
             */
            if (is_not)
              break;
            else
              do_cleanups(old_chain);
          }
          else if (is_not)
            do_cleanups (old_chain);
          else
            break;
        }
      else if (cond_type == do_defined)
        {
          tem = cond + 8;
          if (lookup_cmd (&tem, macrolist, "", -1, 0))
            {
              if (is_not)
                break;
              else
                do_cleanups(old_chain);
            }
          else if (is_not)
            do_cleanups (old_chain);
          else
            break; 
        }
      else
        {
          expr = parse_c_expression (cond);
          (void) make_cleanup (free_current_contents, &expr);
          val = evaluate_expression (expr);
          if (val->lval == lval_reg_invalid)
            ui_badnews(-1, "condition is #%s=%s#", reg_names[VALUE_REGNO (val)],
                       THE_UNKNOWN);
          if (val->contents[0] != 0)
            {
              if (is_not)
                break;
              else
                do_cleanups(old_chain);
            }
          else if (is_not)
            do_cleanups (old_chain);
          else
            break; 
        }
      
      /* Do the loop */
      
      /* Check for a null loop. */
      cmdlines = cmdlines->next;
      if (cmdlines == 0)
        continue;
      
      /* Set the instream to 0, indicating execution of a
         user-defined function.  */
      old_chain =  make_cleanup (source_cleanup, instream);
      instream = (FILE *) 0;
      while (cmdlines)
        {
          execute_command (cmdlines->cmd, cmdlines->line, 0);
          cmdlines = cmdlines->next;
        }
      do_cleanups (old_chain);
    }
  do_cleanups (old_chain);
}


/* Nested "define" command. */

static void
nested_define_command (cmds, from_tty)
     char *cmds;
     int from_tty;
{
  register struct command_line *cmdlines;
  register struct cmd_list_element *c;
  register char *comname;
  char *tem;
  char *extra;
  struct command_line *real_cmds;
  
  cmdlines = (struct command_line *)cmds;
  tem = macro_expand (cmdlines->line, cmdlines->cmd);
  extra = validate_comname (tem);
  comname = savestring (tem, extra - tem);
  extra = savestring (extra, strlen (extra));
  free_macro_expansion (tem);
  
  tem = comname;
  c = lookup_cmd (&tem, cmdlist, "", -1, 1);
  
  if (c && c->class == (int) class_user)
    free_command_lines (&c->function);
  
  if (*extra)
    {
      real_cmds = (struct command_line *)
	xmalloc (sizeof (struct command_line));
      real_cmds->cmd = (struct cmd_list_element *) INVALID_CORE_ADDR;
      real_cmds->line = (char *) parse_string_list (extra);
      free (extra);
      real_cmds->next = copy_command_lines (cmdlines->next);
    }
  else
    real_cmds = copy_command_lines (cmdlines->next);
  
  add_com (comname, class_user, real_cmds,
	   (c && c->class == (int) class_user)
	    ? c->doc : savestring ("User-defined.", 13));
}


/* Nested "document" command. */

static void
nested_document_command (cmds)
     char *cmds;
{
  register struct command_line *doclines;
  register struct cmd_list_element *c;
  char *comname;
  char *extra;
  
  doclines = (struct command_line *)cmds;
  comname= doclines->line;
  
  extra = validate_comname (comname);
  if (*extra)
    ui_badnews(-1,"Junk in argument list: \"%s\"", extra);
  
  c = lookup_cmd (&comname, cmdlist, "", 0, 1);
  
  if (c->class != (int) class_user)
    ui_badnews(-1,"Command \"%s\" is built-in.", doclines->line);
  
  doclines = doclines->next;
  
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
}



/*     +----------------+     */
/*  -=#|  GDB Commands  |#=-  */
/*     +----------------+     */


/* Assigns a value to the 'fileoffuncname' macro. */

static void
assign_funcname_command(arg)
     char *arg;
{
   char cmdstring[1024];
   int desc;

   /* If 'arg' is the name of a file we can find, just use it.  
    * If not, try to find a source file containing a function named 'arg'.
    * If that fails, set the `fileoffuncname macro to NOT_FOUND.
    */
   desc = openp (source_path, 0, arg, O_RDONLY, 0, 0);
   if (desc != -1) {
	sprintf(cmdstring, "fileoffuncname \"%s\"", arg);
        close(desc);
   }
   else { 
        struct symtabs_and_lines sals;
        struct symtab_and_line sal;

	if (lookup_symbol(arg, 0, VAR_NAMESPACE, 0)) {
            sals = decode_line_spec(arg, 0);
            sal = sals.sals[0];
            free (sals.sals);
            sprintf(cmdstring, "fileoffuncname \"%s\"", sal.symtab->filename);
        }
        else {
	    sprintf(cmdstring, "fileoffuncname \"NOT_FOUND\"");
        }
   }
   macro_command(cmdstring);
}

/* Assign a value to the 'typevar' command. Possible values are for present
 * VAR for variable name (usable with watchpoint commands) and OTHER for
 * functions or line numbers (usable with break commands) */
static void
assign_typevar_command(arg)
     char *arg;
{
   char cmdstring[1024];
   struct symbol *sym;

   sym = lookup_symbol(arg, get_selected_block(), VAR_NAMESPACE, 0);
/* May have to munge arg to deal with scope delimiters */
   if (sym) {
	switch (sym->class) {
	    case LOC_STATIC:
	    case LOC_REGISTER:
	    case LOC_ARG:
	    case LOC_REF_ARG:
	    case LOC_REGPARM:
	    case LOC_LOCAL:
		sprintf(cmdstring, "typevar \"VAR\"");
		break;
	    default:
		sprintf(cmdstring, "typevar \"OTHER\"");
        }
    }
    else {
	sprintf(cmdstring, "typevar \"OTHER\"");
    }
   macro_command(cmdstring);
}


/* Remember (or forget) a user-defined macro. */

static void
macro_command (args)
     char *args;
{
  char *tem;
  char *p, *q, ch;
  char *body;
  char *macname;
  struct cmd_list_element *c;
  struct string_list_el *new;

  if ((args == 0) || (*args == 0))
    error_no_arg ("name of user macro");
  body = validate_comname (args);
  macname = savestring (args, body - args);
  (void) make_cleanup (free, macname);
 
  if (body)
    {
      if (*body == 0)
        body = 0;
      else
        {
          EAT_WHITESP_FRD (body);
          if (*body != '"')
            ui_badnews (-1, "Ill formed macro body.");
          tem = skip_quoted_string (body++);
          p = body;
          body = xmalloc (tem - body + 1);
          q = body;
          while (p != tem)
            {
              ch = *p++;
              if (ch == '\\')
                {
                  ch = parse_escape (&p);
                  if (ch == 0)
                    break; /* C loses */
                  else if (ch > 0)
                    *q++ = ch;
                }
              else
                *q++ = ch;
            }
          *q = 0;
          (void) make_cleanup (free, body);
        }
    }

  tem = args;
  c = lookup_cmd (&tem, macrolist, "", -1, 0);
  if (c)
    {
      if (c->class == (int) class_user)
        if (body)
          tem = "Redefine user macro \"%s\"? ";
        else
          tem = "Delete user macro \"%s\"? ";
      else
        if (body)
          tem = "Really redefine built-in macro \"%s\"? ";
        else
          tem = "Really delete built-in macro \"%s\"? ";
      if (!query (tem, macname))
	ui_badnews (-1, "Macro \"%s\" not redefined.", macname);
      if (strcmp(macname, MACROSTACK) == 0)
        {
          if (body)
            {
              new = new_string_list_el ();
              new->string = c->aux;
              new->next = macro_stack;
              macro_stack = new;
              c->aux = savestring (body, strlen (body));
              return;
            }
          else if (macro_stack)
            {
              if (c->aux)
                free (c->aux);
              c->aux = macro_stack->string;
              new = macro_stack->next;
              free (macro_stack);
              macro_stack = new;
              return;
            }
        }
      else if (c->aux)
	free (c->aux);
      c->function = 0;
      c->class = (int) class_user;
    }

  if (body)
    {
      c = add_cmd (macname, class_user, 0, "User Macro", &macrolist);
      c->aux = savestring (body, strlen(body));
    }
  else
    delete_cmd (macname, &macrolist);
}


/* This is passed a string which should contain "....." ......
 * where the "..." part is the pattern to match, and the ..... is the
 * string to match against it.  
 */

static void
match_command (line)
     char *line;
{
  char *mark;
  char fastmap[(1 << BYTEWIDTH)];
  int len;
  
  if (line == 0)
    error_no_arg ("pattern to match against");
  if (*line != '"')
    ui_badnews(-1, "must begin pattern with \" character");
  
  /* Make sure we skip over " chars which are escaped thus \"  */
  mark = skip_quoted_string (line++);
  match_pattern = savestring(line, mark++ - line);
  
  /* Now skip white space, everything else is the string to match against
   * the pattern
   */
  while (*mark && (*mark == ' ' || *mark == '\t'))
    mark++;
  if (*mark == '\0') {
    free(match_pattern);
    error_no_arg ("string to match against pattern");
  }
  match_string = savestring(mark, strlen(mark));
  
  /* Now we know we have a good pattern and string.  Compile the
   * pattern and do the match.
   */
  buf.allocated = 40;
  buf.buffer = (char *) xmalloc (buf.allocated);
  
  /* Currently re_match makes no use of fastmaps; no use supplying one */
  /*buf.fastmap = fastmap;*/
  re_compile_pattern(match_pattern, strlen(match_pattern), &buf);
  /*re_compile_fastmap(&buf);*/
  match_routine(&buf, match_string);
}


/*
 *  ---  Conditional commands  ---
 */


/* The "while" command.  Used like:

   while <condition/match/defined>
     ...
     end

   The command between the "while" and its matching "end" will be executed
   repeatedly while <condition> evaluates to 1, while the match is successfull,
   or while the macro is deifned. */

static void
while_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  struct command_line *cmdlines, *newone;
  struct cleanup *old_chain;
  char *cond;
  
  /* We don't want conditional commands to auto repeat. */
  dont_repeat ();
  
  /* Make sure there is a condition. */
  if ((arg == 0) || (*arg == 0))
    error_no_arg ("while condition");
  cond = savestring (arg, strlen (arg));
  
  /* Gather up the body of the while loop */
  
  if (from_tty)
    {
      ui_fprintf(stdout, "Type commands for body of while loop.\n\
End with a line saying just \"end\".\n");
      ui_fflush (stdout);
    }
  cmdlines = read_command_lines (2);
  old_chain = make_cleanup (free_command_lines, &cmdlines);
  
  /* Try to execute the loop only if we are currently enabled */
  if (cond_stack == 0 || cond_stack->exec_state == exec_enabled)
    {
      /* Construct a command list for "nested_while_command" and call it. */
      newone = (struct command_line *) xmalloc (sizeof (struct command_line));
      newone->cmd = 0;
      newone->line = cond;
      newone->next = cmdlines;
      cmdlines = newone;
      nested_while_command ((char *) cmdlines, from_tty);
    }
  
  /* All done, clean up. */
  do_cleanups (old_chain);
}


/* The "if" command.  Used like:

   if <condition/match>
     ...
   [elif <condition/match>
     ...] ...
   [else
     ...]
   endif

   If the "if" condition/match is true, then the command lines following
   the "if" are executed.  Otherwise, the commands following the first
   "elif" command with a true condition/match are executed.  If the "if"
   condition/match was false, and there were no "elif" commands or none
   with a true condition/match, then the command followin the "else" command,
   if any, are executed.  The "endif" command signals the end of this
   "condition level". */

static void
if_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  cond_level new_level;
  
  /* We don't want conditional commands to auto repeat. */
  
  dont_repeat ();
  
  /* Create a new level */
  
  update_prompt (1);
  new_level = (cond_level) malloc (sizeof(struct cond_level));
  new_level->prev = cond_stack;
  new_level->else_ok = 1;
  cond_stack = new_level;
  
  /* Handle the case where we are not currently enabled */
  
  if (new_level->prev->exec_state != exec_enabled)
    {
      new_level->exec_state = exec_skipping;
      return;
    }
  eval_and_set_state(arg, new_level);
}


/* The "elif" command.  (See above) */

static void
elif_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  cond_level cur_level = cond_stack;
  
  /* We don't want conditional commands to auto repeat. */
  
  dont_repeat ();
  
  /* Make sure we are have seen an "if" at this level and not an "else". */
  
  if (! (cur_level && cur_level->else_ok))
    ui_badnews(-1, "not expecting an \"elif\" command");
  
  /* If we have been executing commands, then start skipping them. */
  
  if (cur_level->exec_state == exec_enabled)
    cur_level->exec_state = exec_skipping;
  
  /* If we are skipping commands until the next "endif", then bugout without
     evaluating our argument. */
  
  if (cur_level->exec_state == exec_skipping)
    return;
  
  eval_and_set_state(arg, cur_level);
}


/* The "else" command.  (See above) */

static void
else_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  cond_level cur_level = cond_stack;
  
  /* We don't want conditional commands to auto repeat. */
  
  dont_repeat ();
  
  /* Make sure we are have seen an "if" at this level and not an "else". */
  
  if (! (cur_level && cur_level->else_ok))
    ui_badnews(-1, "not expecting an \"else\" command");
  
  /* Make sure this is the only else we'll take at this level */
  
  cur_level->else_ok = 0;
  
  /* "Toggle" the execution sate. */
  
  if (cur_level->exec_state == exec_enabled)
    cur_level->exec_state = exec_disabled;
  else if (cur_level->exec_state == exec_disabled)
    cur_level->exec_state = exec_enabled;
}


/* The "endif" command.  (See above) */

static void
endif_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  cond_level cur_level = cond_stack;
  
  /* We don't want conditional commands to auto repeat. */
  
  dont_repeat ();
  
  /* Make sure we have already seen an "if" */
  
  if (! cur_level)
    ui_badnews(-1, "not expecting an \"endif\" command");
  
  /* Get rid of this level */
  
  update_prompt (-1);
  cond_stack = cur_level->prev;
  free (cur_level);
}


/*
 *  ---  Set sub-commands.  ---
 */


/* Set autorepeat on or off. */

static void
set_autorepeat (arg)
     char *arg;
{
  if (arg == 0)
    {
      autorepeat = ! autorepeat;
      return;
    }
  EAT_WHITESP_FRD (arg);
  if (*arg == 0)
    autorepeat = ! autorepeat;
  else if (strcmp (arg, "on") == 0)
    autorepeat = 1;
  else if (strcmp (arg, "off") == 0)
    autorepeat = 0;
  else
    error_bad_arg ("\"on\" or \"off\"", arg);
}


/* Set the primary prompt.  (default is "(gdb)") */

static void
set_prompt_command (arg)
     char *arg;
{
  set_either_prompt_command (0, arg);
}


/* Set the secondary prompt.  (defaulg is "<%d\>") */

static void
set_prompt2_command (arg)
     char *arg;
{
  set_either_prompt_command (1, arg);
}


/* Set the macro escape character.  (default is '`')  The format is:
   set macroescape {push | {pop | <char>}}
   If "push" is used, then the current macro escape character is pushed on a
   stack.  If "pop" is used, the top of that stack becomes the new macro
   escape character and the stack is poped. */

static void
set_macro_escape (arg)
     char *arg;
{
  struct intlist_element *newone;
  char newtick;
  register struct cleanup *old_chain = 0;

  if (arg)
    EAT_WHITESP_FRD (arg);
  if ((arg == 0) || (*arg == 0))
    error_no_arg ("macro escape character");
  if ((strncmp (arg, "push", 4) == 0) && isspace (arg[4]))
    {
      newone = (struct intlist_element *)
	xmalloc (sizeof (struct intlist_element));
      newone->value = tickchr;
      newone->next = tickchr_stack;
      tickchr_stack = newone;
      old_chain = make_cleanup (pushed_int_cleanup, &tickchr);
      arg += 4;
      EAT_WHITESP_FRD (arg);
      if (*arg == 0)
        ui_badnews (-1, "Nothing following push.\n");
    }
  if (strncmp (arg, "pop", 3) == 0)
    {
      arg += 3;
      EAT_WHITESP_FRD (arg);
      if (*arg)
        ui_badnews (-1,"Junk following pop: \"%s\"", arg);
      if (tickchr_stack)
	{
	  tickchr = tickchr_stack->value;
	  newone = tickchr_stack->next;
	  free (tickchr_stack);
	  tickchr_stack = newone;
	}
      else
	tickchr = DFLTTICKCHR;
    }
  else
    {
      newtick = *arg++;
      EAT_WHITESP_FRD (arg);
      if (*arg != 0)
        ui_badnews (-1,"Junk following macro escape character: \"%s\"", arg);
      tickchr = newtick;
    }

  if (old_chain)
    discard_cleanups (old_chain);
}


/* Set the macro scan limit.  (default is 1)  The format is:
   set scanlimit {push | {pop | <char>}}
   If "pop" is used, then the scan limit is set to a previous value.  If
   "push" is used, then the current value is saved before the new value is
   set. */

static void
set_macro_scan_limit (arg)
     char *arg;
{
  char *p;
  struct intlist_element *newone;
  register struct cleanup *old_chain = 0;
  
  if (arg == 0)
    error_no_arg ("macro scan limit");
  EAT_WHITESP_FRD (arg);
  if ((strncmp (arg, "push", 4) == 0) && isspace (arg[4]))
    {
      newone = (struct intlist_element *)
	xmalloc (sizeof (struct intlist_element));
      newone->value = macro_scan_limit;
      newone->next = scanlimit_stack;
      scanlimit_stack = newone;
      old_chain = make_cleanup (pushed_int_cleanup, &macro_scan_limit);
      arg += 4;
      EAT_WHITESP_FRD (arg);
    }
  if (strncmp (arg, "pop", 3) == 0)
    {
      arg += 3;
      EAT_WHITESP_FRD (arg);
      if (*arg)
        ui_badnews(-1,"Junk following pop: \"%s\"", arg);
      if (scanlimit_stack)
	{
	  macro_scan_limit = scanlimit_stack->value;
	  newone = scanlimit_stack->next;
	  free (scanlimit_stack);
	  scanlimit_stack = newone;
	}
      else
	macro_scan_limit = DFLTSCANLIMIT;
    }
  else
    {
      for (p = arg; *p; ++p)
	if ((*p < '0') && (*p > '9'))
	  ui_badnews (-1, "Argument must be numeric.");
      macro_scan_limit = atoi (arg);
    }

  if (old_chain)
    discard_cleanups (old_chain);
}

/* Set shift-to-lower.  (default is ON).  This format is:
   set shift-to-lower pop | [push] { ON | OFF }
   If "pop" is used, then shift-to-lower is set to a previous value.  If
   "push" is used, then the current value is saved before the new value is
   set. */

static void
set_shift_to_lower (arg)
     char *arg;
{
  char *p;
  struct intlist_element *newone;
  register struct cleanup *old_chain = 0;

  if (arg == 0)
    error_no_arg ("shift-to-lower setting");
  EAT_WHITESP_FRD (arg);
  if ((strncmp (arg, "push", 4) == 0) && isspace (arg[4]))
    {
      newone = (struct intlist_element *)
	xmalloc (sizeof (struct intlist_element));
      newone->value = shift_to_lower;
      newone->next = shift_to_lower_stack;
      shift_to_lower_stack = newone;
      old_chain = make_cleanup (pushed_int_cleanup, &shift_to_lower);
      arg += 4;
      EAT_WHITESP_FRD (arg);
    }
  if (strncmp (arg, "pop", 3) == 0)
    {
      arg += 3;
      EAT_WHITESP_FRD (arg);
      if (*arg)
        ui_badnews(-1,"Junk following pop: \"%s\"", arg);
      if (shift_to_lower_stack)
	{
	  shift_to_lower = shift_to_lower_stack->value;
	  newone = shift_to_lower_stack->next;
	  free (shift_to_lower_stack);
	  shift_to_lower_stack = newone;
	}
      else
	shift_to_lower = DFLTSHIFTTOLOWER;
    }
  else
    shift_to_lower = parse_binary_operation ("set shift-to-lower", arg);

  if (old_chain)
    discard_cleanups (old_chain);
}


/*
 *  ---  Info sub-commands. ---
 */


/* Print out the current state of "autorepeat". */

static void
autorepeat_info ()
{
  printf_filtered ("The autorepeat feature is %s.\n",
		   (autorepeat ? "on" : "off"));
}


/* Either print out a single user defined command, or all of them. */

static void
user_info (cmd_name)
     char *cmd_name;
{
  struct cmd_list_element *c;
  register struct command_line *cmdlines;
  
  if (cmd_name && *cmd_name)
    {
      c = lookup_cmd (&cmd_name, cmdlist, "", 0, 1);
      if (c == 0) return; /* Just to be sure. */
      if (c->function == 0)
	ui_badnews (-1,"That is not a command, just a help topic.");
      if (c->class != (int) class_user)
        ui_badnews (-1, "That is not a user command.");
      printf_filtered ("define %s ", c->name);
      cmdlines = (struct command_line *) c->function;
      if (cmdlines->cmd == (struct cmd_list_element *) INVALID_CORE_ADDR)
	{
	  print_string_list ("%s ", cmdlines->line);
	  cmdlines = cmdlines->next;
	}
      printf_filtered ("\n");
      print_command_lines (0, cmdlines);
      printf_filtered ("end\n");
    }
  else
    for (c = cmdlist; c; c = c->next)
      {
        cmdlines = (struct command_line *) c->function;
        if (cmdlines == 0)
          continue;
        if (c->class != (int) class_user)
          continue;
        printf_filtered ("define %s ", c->name);
	if (cmdlines->cmd == (struct cmd_list_element *) INVALID_CORE_ADDR)
	  {
	    print_string_list ("%s ", cmdlines->line);
	    cmdlines = cmdlines->next;
	  }
	printf_filtered ("\n");
	print_command_lines (0, cmdlines);
	printf_filtered ("end\n");
      }
}


/* Print out the current state of macro expansion along with expansions of
   all the macros currently defined. */

static void
macro_info ()
{
  struct cmd_list_element *c;
  struct intlist_element *slp;
  char tmp_buf[256];

  if (isprint(tickchr))
    printf_filtered ("The macro escape character is: %c\n", tickchr);
  else
    printf_filtered ("The macro escape character (0x%02x) is unprintable: no macro expansion.\n",
                     tickchr & 0xff);
  if (tickchr_stack)
    {
      printf_filtered ("Previous macro escape characters:");
      for (slp=tickchr_stack; slp; slp=slp->next)
        if (isprint (slp->value))
          printf_filtered (" %c", slp->value);
        else
          printf_filtered (" 0x%02x", slp->value & 0xff);
      printf_filtered ("\n");
    }

  if (macro_scan_limit <= 0)
    printf_filtered ("Scan limit is zero: no macro expansion.\n");

  if ((macro_scan_limit > 0) && isprint (tickchr))
    {
      printf_filtered ("A command line will be scanned at most %d time(s).\n",
			 macro_scan_limit);
      if (scanlimit_stack)
	{
	  printf_filtered ("Previous scan limits:");
	  for (slp=scanlimit_stack; slp; slp=slp->next)
	    printf_filtered (" %d", slp->value);
	  printf_filtered ("\n");
	}
      printf_filtered("\nBuiltin Macros:\n");
      for (c=macrolist; c; c=c->next)  if (c->class == no_class)
	{
	  printf_filtered("%c%s ", tickchr, c->name);
	  if (c->function)
	    {
	      char *p = (*((char *(*)())(c->function)))(c->aux);
	      strcpy (tmp_buf, p ? p : "<nil>");
	      printf_filtered("\"%s\"\n", tmp_buf);
	    }
	  else
	    printf_filtered("\"%s\"\n", c->aux);
	}
      printf_filtered("\nUser Macros:\n");
      for (c=macrolist; c; c=c->next)  if (c->class == class_user)
        {
          printf_filtered("%c%s \"%s\"\n", tickchr, c->name, c->aux);
        }
    }
}


/* Print the current state of shift-to-lower. */

static void
shift_to_lower_info ()
{
  struct intlist_element *slp;

  printf_filtered("shift-to-lower is %s: ", shift_to_lower ? "on" : "off");
  if (shift_to_lower)
    printf_filtered("command and macro names will be shifted to lower case.\n");
  else
    printf_filtered("case will be significant in command and macro names.\n");
  if (shift_to_lower_stack)
    {
      printf_filtered ("Previous settings of shift_to_lower:");
      for (slp=shift_to_lower_stack; slp; slp=slp->next)
        printf_filtered (" %s", slp->value ? "on" : "off");
      printf_filtered ("\n");
    }
}



/*     +-------------------+     */
/*  -=#|  Initializations  |#=-  */
/*     +-------------------+     */



/* Module initializations. */

void
_initialize_programmer ()
{
  struct cmd_list_element *new;

  /*
   *  Add GDB Commands.
   */

  add_com("match", class_obscure, match_command, 
          "Try to match a double-quoted pattern against a string.");
  add_com ("endif", class_conditional, endif_command, "Endif part of conditional.");
  add_com ("elif", class_conditional, elif_command, "Elif part of conditional.");
  add_com ("else", class_conditional, else_command, "Else part of conditional.");
  add_com ("if", class_conditional, if_command, "If part of conditional.");
  add_com ("while", class_conditional, while_command, "While loop.");
  add_com ("macro", class_obscure, macro_command, "Define user macro.");
  add_com ("assign-funcname", class_obscure, assign_funcname_command, 
	   "Assign value to \"fileoffuncname\" macro.");
  add_com ("assign-typevar", class_obscure, assign_typevar_command, 
	   "Assign value to \"typevar\" macro.");

  /*
   *  Add Set Sub-Commands.
   */

  new = add_cmd ("autorepeat", class_support, set_autorepeat,
		 "Change the response to a line with just a carriage return.\n\
Arguments can be \"on\" or \"off\".  No arguments will toggle 'autorepeat'.",
		 &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  new = add_cmd ("prompt", class_support, set_prompt_command,
		 "Change gdb's primary prompt from the default of \"(gdb)\"",
		 &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  new =add_cmd ("prompt2", class_support, set_prompt2_command,
		"Change gdb's secondary prompt.\n\
The default secondary prompt is \"r<%d>\".",
	   &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  new = add_cmd ("scanlimit", class_obscure, set_macro_scan_limit,
                 "Set the macro scan limit.\n\
Before a command line is executed, gdb will scan it for macros until all\n\
macros have been expanded or until the scan limit is reached.  Scanning the\n\
entire line counts as one scan.  It is possible to save the previous scan\n\
limit by including the word \"push\" in front of the new value.  Previous\n\
values can be returned to by using the word \"pop\" in place of the value.",
                 &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  new = add_cmd ("macroescape", class_obscure, set_macro_escape,
                 "Set the macro escape character.\n\
Before a command line is executed, gdb will scan it for macros.  All macros\n\
start with a special character called the macro escape character.  The\n\
default macro escape character is a back quote (`).  If you change it to an\n\
unprintable character, then scanning for macros is not done.",
                 &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  new = add_cmd ("shift-to-lower", class_obscure, set_shift_to_lower,
                 "Shift command and macro names to lower case (or don't).\n\
If shift-to-lower is set \"on\", command and macro names are shifted to\n\
lower case before they are used or defined.  If shift-to-lower is set\n\
\"off\", this is not done and character case will be significant.  It is\n\
possible to save the previous setting of shift-to-lower by including the\n\
word \"push\" in front of the new setting.  Previous settings can be\n\
returned by using the word \"pop\" in place of \"on\" or \"off\".",
                 &setlist);
  if (new)
    new->aux = (char *) set_cmd;

  /*
   *  Add Info Sub-Commands
   */

  add_info ("user", user_info,
            "Show definitions of any or all user commands.\n\
Use \"info user COMMAND\" to show the definition of COMMAND or use \"info user\"\n\
to see the definitions of all commands.");

  add_info ("macro", macro_info,
            "Show macro expansion parameters and macros with their values.\n\
Show the current macro escape character and the macro scan limit.  Also\n\
show all the currently defined macros and their values.");

  add_info ("autorepeat", autorepeat_info,
	    "Show if autorepeat is on or off.\n\
If autorepeat is on, then simply pressing the return key at the start of an\n\
empty line after gdb's prompt will cause the previous command to be repeated.\n\
It may not be a good idea for some commands to be repeated in this way, so \n\
this feature is not applied to some commands.  If autorepeat is off, this\n\
feature is never applied.");

  add_info ("shift-to-lower", shift_to_lower_info,
            "show the state of \"shift-to-lower\".\n\
If shift-to-lower is set \"on\", command and macro names are shifted to\n\
lower case before they are used or defined.  If shift-to-lower is set\n\
\"off\", this is not done and character case will be significant.");


  /*
   *  Keyword Macros
   */

  new = add_cmd ("date", no_class, date_macro,
           "Current date and time.\n\
This macro expands to the current date and time, in the same format as the\
standard utility \"date\".", &macrolist);
  new->aux = savestring ("%a %b %e %T %Z %Y", 17);
  new = add_cmd ("rcsid", no_class, 0, "Gdb's rcs id.", &macrolist);
  new->aux = savestring (_rcsid, strlen (_rcsid));
  new = add_cmd ("version", no_class, 0, "Gdb's version number.", &macrolist);
  new->aux = savestring (version, strlen (version));
  new = add_cmd ("line", no_class, line_macro, "Current source line.", &macrolist);
  new->aux = NULL;
  new = add_cmd ("level", no_class, level_macro, "Current frame level.", &macrolist);
  new->aux = NULL;
  new = add_cmd ("isouter", no_class, is_outer_macro, "Equals 1 if current frame is outermost, else 0.", &macrolist);
  new->aux = NULL;
  new = add_cmd ("func", no_class, cur_function_macro, 
	"Name of current function.", &macrolist);
  new->aux = NULL;
  new = add_cmd ("file", no_class, cur_srcfile_macro, 
	"Name of current source file.", &macrolist);
  new->aux = NULL;
  new = add_cmd ("lastbreak", no_class, last_break_macro,
        "Number of most recently defined break-point.", &macrolist);
  new->aux = savestring ("%d", 2);
  new = add_cmd ("success", no_class, 0,
        "Expansion for `m macro when the previous MATCH succeeded.",
		 &macrolist);
  new->aux = savestring (DFLTSUCCEEDEDMSG, strlen (DFLTSUCCEEDEDMSG));
  new = add_cmd ("failure", no_class, 0,
        "Expansion for `m macro when the previous MATCH failed.",
		 &macrolist);
  new->aux = savestring (DFLTFAILEDMSG, strlen (DFLTFAILEDMSG));

  /*
   *   Other Initializations
   */

  obstack_init (meo);
  prompt[0] = savestring ("(gdb) ", 6);
  prompt[1] = savestring ("<%d> ", 5);
  gbl_prompt = prompt[0];
  update_prompt (2);
}


/* Initialize intternal commands and pointers to them. */

void
init_cmd_pointers ()
{
  char *p;
  
  p = "define";
  define_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (define_p == 0)
    ui_badnews (1, "Internal: \"Define\" not defined.");
  p = "commands";
  commands_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (commands_p == 0)
    ui_badnews (1, "Internal: \"Commands\" not defined.");
  p = "document";
  document_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (document_p == 0)
    ui_badnews (1, "Internal: \"Document\" not defined.");
  p = "while";
  while_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (while_p == 0)
    ui_badnews (1, "Internal: \"While\" not defined.");
  p = "elif";
  elif_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (elif_p == 0)
    ui_badnews (1, "Internal: \"Elif\" not defined.");
  p = "if";
  if_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (if_p == 0)
    ui_badnews (1, "Internal: \"If\" not defined.");
  p = "endif";
  endif_p = lookup_cmd (&p, cmdlist, "", -1, 1);
  if (endif_p == 0)
    ui_badnews (1, "Internal: \"Endif\" not defined.");
  nested_define.function = nested_define_command;
  nested_define.name = "define";
  nested_define.class = class_nested;
  nested_commands.function = nested_commands_command;
  nested_commands.name = "commands";
  nested_commands.class = class_nested;
  nested_document.function = nested_document_command;
  nested_document.name = "document";
  nested_document.class = class_nested;
  nested_while.function = nested_while_command;
  nested_while.name = "while";
  nested_while.class = class_nested;
}

#endif /* TEK_PROG_HACK */  /* this covers the whole file! */
