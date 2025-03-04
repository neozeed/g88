/* funmap.c -- attach names to functions. */

/* $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/readline/RCS/funmap.c,v 1.4 89/11/30 14:42:10 andrew Exp $ */

/* Copyright (C) 1987,1989 Free Software Foundation, Inc.

This file is part of GNU Bash, the Bourne Again SHell.

Bash is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

Bash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with Bash; see the file LICENSE.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */



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

#include "readline.h"

typedef struct {
  char *name;
  Function *function;
} FUNMAP;

FUNMAP **funmap = (FUNMAP **)NULL;
int funmap_size = 0;
int funmap_entry = 0;

static FUNMAP default_funmap[] = {
  {"beginning-of-line", rl_beg_of_line},
  {"backward-char", rl_backward},
  {"delete-char", rl_delete},
  {"end-of-line", rl_end_of_line },
  {"forward-char", rl_forward},
  {"accept-line", rl_newline},
  {"kill-line", rl_kill_line },
  {"clear-screen", rl_clear_screen},
  {"next-history", rl_get_next_history},
  {"previous-history", rl_get_previous_history },
  {"quoted-insert", rl_quoted_insert},
  {"reverse-search-history", rl_reverse_search_history},
  {"forward-search-history", rl_forward_search_history},
  {"transpose-chars", rl_transpose_chars },
  {"unix-line-discard", rl_unix_line_discard},
  {"unix-word-rubout", rl_unix_word_rubout },
  {"yank", rl_yank},
  {"prefix-meta", rl_prefix_meta},
  {"backward-delete-char", rl_rubout},
  {"backward-word", rl_backward_word },
  {"kill-word", rl_kill_word},
  {"forward-word", rl_forward_word},
  {"tab-insert", rl_tab_insert},
  {"yank-pop", rl_yank_pop },
  {"backward-kill-word", rl_backward_kill_word},
  {"backward-kill-line", rl_backward_kill_line},
  {"transpose-words", rl_transpose_words },
  {"digit-argument", rl_digit_argument},
  {"complete", rl_complete},
  {"possible-completions", rl_possible_completions },
  {"do-uppercase-version", rl_do_uppercase_version},
  {"digit-argument", rl_digit_argument},
  {"universal-argument", rl_universal_argument },
  {"abort", rl_abort},
  {"undo", rl_do_undo},
  {"upcase-word", rl_upcase_word},
  {"downcase-word", rl_downcase_word},
  {"capitalize-word", rl_capitalize_word},
  {"revert-line", rl_revert_line},
  {"beginning-of-history", rl_beginning_of_history },
  {"end-of-history", rl_end_of_history},
  {"self-insert", rl_insert},
#ifdef TEK_HACK
  {"suspend-myself", rl_suspend_myself},
#endif /* TEK_HACK */
  {(char *)NULL, (Function *)NULL }
};

rl_add_funmap_entry (name, function)
     char *name;
     Function *function;
{
  if (funmap_entry + 2 >= funmap_size)
    if (!funmap)
      funmap = (FUNMAP **)xmalloc ((funmap_size = 80) * sizeof (FUNMAP *));
    else
      funmap =
	(FUNMAP **)xrealloc (funmap, (funmap_size += 80) * sizeof (FUNMAP *));
  
  funmap[funmap_entry] = (FUNMAP *)xmalloc (sizeof (FUNMAP));
  funmap[funmap_entry]->name = name;
  funmap[funmap_entry]->function = function;

  funmap[++funmap_entry] = (FUNMAP *)NULL;
}

static int funmap_initialized = 0;

/* Make the funmap contain all of the default entries. */
rl_initialize_funmap ()
{
  register int i;

  if (funmap_initialized)
    return;

  for (i = 0; default_funmap[i].name; i++)
    rl_add_funmap_entry (default_funmap[i].name, default_funmap[i].function);

  funmap_initialized = 1;
}

/* Things that mean `Control'. */
char *possible_control_prefixes[] = {
  "Control-", "C-", "CTRL-", (char *)NULL };

char *possible_meta_prefixes[] = {
  "Meta", "M-", (char *)NULL };
