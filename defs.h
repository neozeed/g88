/* Basic definitions for GDB, the GNU debugger.
   Copyright (C) 1986, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/defs.h,v 1.11 90/06/30 17:29:20 robertb Exp $
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

#define CORE_ADDR unsigned int
#ifdef TEK_HACK
#define NULL_CORE_ADDR ((CORE_ADDR)0)
#define INVALID_CORE_ADDR ((CORE_ADDR)~0)
#endif /* TEK_HACK */

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

extern char *savestring ();
extern char *concat ();
extern char *xmalloc (), *xrealloc ();
//extern char *alloca ();
extern int parse_escape ();
extern char *reg_names[];

extern int quit_flag;

extern int immediate_quit;

#define QUIT { if (quit_flag) quit (); }

#ifdef TEK_HACK
/* What to say if you can't find a registers value. */
#define THE_UNKNOWN "???"
#endif /* TEK_HACK */

/* Notes on classes: class_alias is for alias commands which are not
   abbreviations of the original command.  */

enum command_class
{
  no_class = -1, class_run = 0, class_vars, class_stack,
  class_files, class_support, class_info, class_breakpoint,
  class_alias, class_obscure, class_user,
#ifdef TEK_PROG_HACK
  class_conditional, class_nested,
#endif /* TEK_PROG_HACK */
};

/* the cleanup list records things that have to be undone
   if an error happens (descriptors to be closed, memory to be freed, etc.)
   Each link in the chain records a function to call and an
   argument to give it.

   Use make_cleanup to add an element to the cleanup chain.
   Use do_cleanups to do all cleanup actions back to a given
   point in the chain.  Use discard_cleanups to remove cleanups
   from the chain back to a given point, not doing them.  */

struct cleanup
{
  struct cleanup *next;
  void (*function) ();
  int arg;
};

extern void do_cleanups ();
extern void discard_cleanups ();
extern struct cleanup *make_cleanup ();
extern struct cleanup *save_cleanups ();
extern void restore_cleanups ();
extern void free_current_contents ();
extern void reinitialize_more_filter ();
extern void fputs_filtered ();
extern void fprintf_filtered ();
//extern void printf_filtered ();
//extern void print_spaces_filtered ();

/* Structure for saved commands lines
   (for breakpoints, defined commands, etc).  */

struct command_line
{
  struct command_line *next;
#ifdef TEK_PROG_HACK
  struct cmd_list_element *cmd;
#endif /* TEK_PROG_HACK */
  char *line;
};
#ifdef TEK_PROG_HACK
/* Normaly, "cmd" will point the cmd_list_element for the command associated
   with this command line and "line" will point to the text of the rest of
   the line.  If the cmd_list_element pointed to is of type "nested", then
   "line" points to another "command_line" list.  This new list is the body
   of the nested command.  If the nested command takes a "condition" or a
   "match", then "line" of the first "command_line" in the new list will be
   the "condition" or "match".

   If "cmd" is zero, then no look-up has been done for this command line and
   "line" points to the complete text.

   The first "command_line" in a list may have a "cmd" with the value -1.
   This means that the rest of the "command_line" list is the body of a user
   defined command with delimiters spedified by the string list pointed to by
   "line".  */
#endif /* TEK_PROG_HACK */

struct command_line *read_command_lines ();
#ifdef TEK_PROG_HACK
void print_command_lines ();
#endif /* TEK_PROG_HACK */

#ifdef TEK_HACK
/* I added these to support the examine-user-space command for the
   cross-debugging mode of gdb  -rcb 3/90 */
  
#define	M_NORMAL	1
#define	M_USERMODE	0
#endif

/* String containing the current directory (what getwd would return).  */

char *current_directory;

/* -rcb 6/90 */
#ifdef sun-4
#include <alloca.h>
#endif

extern char *index(); /* -rcb 6/90 */
extern char *rindex(); /* -rcb 6/90 */

#ifdef _WIN32
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
#define __sys_types_h
#define SIGTRAP 33
#define SIGINT 32
#define index rindex
#endif
