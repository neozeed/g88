/* Print and select stack frames for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/stack.c,v 1.18 90/04/30 14:08:47 robertb Exp $
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

#include <stdio.h>

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "ui.h"


/* Thie "selected" stack frame is used by default for local and arg access.
   May be zero, for no selected frame.  */

FRAME selected_frame;

/* Level of the selected frame:
   0 for innermost, 1 for its caller, ...
   or -1 for frame specified by address with no defined level.  */

int selected_frame_level;

/* Nonzero means print the full filename and linenumber
   when a frame is printed, and do so in a format programs can parse.  */

int frame_file_full_name = 0;


void print_frame_info ();

/* Print a stack frame briefly.  FRAME should be the frame id
   and LEVEL should be its level in the stack (or -1 for level not defined).
   This prints the level, the function executing, the arguments,
   and the file name and line number.
   If the pc is not at the beginning of the source line,
   the actual pc is printed at the beginning.

   If SOURCE is 1, print the source line as well.
   If SOURCE is -1, print ONLY the source line.  */

static void
print_stack_frame (frame, level, source)
     FRAME frame;
     int level;
     int source;
{
  struct frame_info *fi;

  fi = get_frame_info (frame);

  print_frame_info (fi, level, source, 1);
}

/* This displays the effect of the last instruction that we single
   stepped and then the instruction that we are about to execute. */
static void
print_effect_and_instruction(pc)
  CORE_ADDR pc;
{
  char buffer[1000];

  print_effect();
  sprint_address(pc, buffer, 0);
  ui_fprintf(stdout, "0x%x: %-22s ", pc, buffer);
  print_insn(pc, stdout, M_NORMAL);
  ui_fprintf(stdout, "\n");
}

/* Flag which will indicate when the frame has been changed
   by and "up" or "down" command.  */
static int frame_changed;

void
print_frame_info (fi, level, source, args)
     struct frame_info *fi;
     register int level;
     int source;
     int args;
{
  struct symtab_and_line sal;
  struct symbol *func;
  register char *funname = 0;
  int numargs;
  struct partial_symtab *pst;

  /* Don't give very much information if we haven't readin the
     symbol table yet.  */
  pst = find_pc_psymtab (fi->pc);
  if (pst && !pst->readin)
    {
      /* Abbreviated information.  */
      char *fname;

      if (!find_pc_partial_function (fi->pc, &fname, 0))
	fname = "??";
	
      printf_filtered ("#%-2d ", level);
      printf_filtered ("0x%x in ", fi->pc);

      printf_filtered ("%s (...) (...)\n", fname);
      
      return;
    }

  sal = find_pc_line (fi->pc, fi->next_frame);
  func = find_pc_function (fi->pc);
  if (func)
    funname = SYMBOL_NAME (func);
  else
    {
      register int misc_index = find_pc_misc_function (fi->pc);
      if (misc_index >= 0)
	funname = misc_function_vector[misc_index].name;
    }

#ifdef	TEK_HACK	/* -rcb 4/24/90 */
  /* If we do not have source line numbers (as when we are in assembly
     code) and if we are supposed to print only the line (as when we
     are single-stepping by instructions), then print our fancy
     instruction-stepping information. */
  if (!sal.symtab && source != 0) {
    print_effect_and_instruction(fi->pc);
  } else
#endif

  if (frame_changed || source >= 0 || !sal.symtab)
    {
      if (level >= 0)
	printf_filtered ("#%-2d ", level);
      else if (frame_changed)
	ui_fprintf(stdout, "#%-2d ", 0);
      if (fi->pc != sal.pc || !sal.symtab)
	printf_filtered ("0x%x in ", fi->pc);
      printf_filtered ("%s (", funname ? funname : "??");
      if (args)
	{
	  FRAME_NUM_ARGS (numargs, fi);
	  print_frame_args (func, fi, numargs, stdout);
	}
      printf_filtered (")");
      if (sal.symtab)
	printf_filtered (" (%s line %d)", sal.symtab->filename, sal.line);
      printf_filtered ("\n");
    }

  if ((frame_changed || source != 0) && sal.symtab)
    {
      int done = 0;
      int mid_statement = source < 0 && fi->pc != sal.pc;
      if (frame_file_full_name)
	done = identify_source_line (sal.symtab, sal.line, mid_statement);
      if (!done) {
        extern last_iword;	/* Last instruction, or zero, in remote.c */
        if (last_iword != 0) {

          /* last_iword is non-zero if we are stepping by instructions.
             In this case, only print the source line if we are at the
             first instruction of the line.  Then print the effect of
             the last instruction executed and the instruction that
             we will execute next.  -rcb 4/23/90 */

          if (!mid_statement) {
#ifdef TEK_HACK
            print_source_lines (sal.symtab, sal.line, sal.line + 1, 1, 0);
#else
            print_source_lines (sal.symtab, sal.line, sal.line + 1, 1);
#endif /* TEK_HACK */
          }
          print_effect_and_instruction(fi->pc);
        } else {
	  if (mid_statement)
	    printf_filtered ("0x%x\t", fi->pc);
#ifdef TEK_HACK
	  print_source_lines (sal.symtab, sal.line, sal.line + 1, 1, 0);
#else
	  print_source_lines (sal.symtab, sal.line, sal.line + 1, 1);
#endif /* TEK_HACK */
	}
      }
      current_source_line = max (sal.line - 5, 1);
    }
  frame_changed = 0;
  if (source != 0)
    set_default_breakpoint (1, fi->pc, sal.symtab, sal.line);

  ui_fflush (stdout);
}

/* Call here to print info on selected frame, after a trap.  */

void
print_sel_frame (just_source)
     int just_source;
{
  print_stack_frame (selected_frame, -1, just_source ? -1 : 1);
}

/* Call here to print info on selected frame and on watchpoint hit location. */

void
print_watchpoint_frame(sxip)
     int sxip;
{
  (get_current_frame ())->pc = sxip & ~3;;
  ui_fprintf(stdout, "--- Watchpoint hit at:\n");
  print_sel_frame(0);
  (get_current_frame ())->pc = read_pc ();
  ui_fprintf(stdout, "--- Program stopped at:\n");
  print_sel_frame(0);
}

/* Print info on the selected frame, including level number
   but not source.  */

void
print_selected_frame ()
{
  print_stack_frame (selected_frame, selected_frame_level, 0);
}

void flush_cached_frames ();

#ifdef FRAME_SPECIFICATION_DYADIC
extern FRAME setup_arbitrary_frame ();
#endif

/*
 * Read a frame specification in whatever the appropriate format is.
 */
static FRAME
parse_frame_specification (frame_exp)
     char *frame_exp;
{
  int numargs = 0;
  int arg1, arg2;
  
  if (frame_exp)
    {
      char *addr_string, *p;
      struct cleanup *tmp_cleanup;

      while (*frame_exp == ' ') frame_exp++;
      for (p = frame_exp; *p && *p != ' '; p++)
	;

      if (*frame_exp)
	{
	  numargs = 1;
	  addr_string = savestring(frame_exp, p - frame_exp);

	  {
	    tmp_cleanup = make_cleanup (free, addr_string);
	    arg1 = parse_and_eval_address (addr_string);
	    do_cleanups (tmp_cleanup);
	  }

	  while (*p == ' ') p++;
	  
	  if (*p)
	    {
	      numargs = 2;
	      arg2 = parse_and_eval_address (p);
	    }
	}
    }

  switch (numargs)
    {
    case 0:
      return selected_frame;
      /* NOTREACHED */
    case 1:
      {
	int level = arg1;
	FRAME fid = find_relative_frame (get_current_frame (), &level);
	FRAME tfid;

	if (level == 0)
	  /* find_relative_frame was successful */
	  return fid;

	/* If (s)he specifies the frame with an address, he deserves what
	   (s)he gets.  Still, give the highest one that matches.  */

	for (fid = get_current_frame ();
	     fid && FRAME_FP (fid) != arg1;
	     fid = get_prev_frame (fid))
	  ;

	if (fid)
	  while ((tfid = get_prev_frame (fid)) &&
		 (FRAME_FP (tfid) == arg1))
	    fid = tfid;
	  
#ifdef FRAME_SPECIFICATION_DYADIC
	if (!fid)
	  ui_badnews(-1,"Incorrect number of args in frame specification");

	return fid;
#else
#ifdef TEK_HACK
/* Why create a new frame not linked in with frame list? fid is perfectly
 * good here.
 *   davidl 11/1/89
 */
	return fid;
#else
	return create_new_frame (arg1, 0);
#endif /* TEK_HACK */
#endif
      }
      /* NOTREACHED */
    case 2:
      /* Must be addresses */
#ifndef FRAME_SPECIFICATION_DYADIC
      ui_badnews(-1,"Incorrect number of args in frame specification");
#else
      return setup_arbitrary_frame (arg1, arg2);
#endif
      /* NOTREACHED */
    }
  ui_badnews(1,"Internal: Error in parsing in parse_frame_specification");
  /* NOTREACHED */
}

/* Print verbosely the selected frame or the frame at address ADDR.
   This means absolutely all information in the frame is printed.  */

static void
frame_info (addr_exp)
     char *addr_exp;
{
  FRAME frame;
  struct frame_info *fi;
  struct frame_saved_regs fsr;
  struct symtab_and_line sal;
  struct symbol *func;
  FRAME calling_frame;
  int i, count;
  char *funname = 0;
  int numargs;

  if (!(have_inferior_p () || have_core_file_p ()))
    ui_badnews(-1,"No inferior or core file.");

  frame = parse_frame_specification (addr_exp);
  if (!frame)
    ui_badnews(-1,"Invalid frame specified.");

  fi = get_frame_info (frame);
  get_frame_saved_regs (fi, &fsr);
  sal = find_pc_line (fi->pc, fi->next_frame);
  func = get_frame_function (frame);
  if (func)
    funname = SYMBOL_NAME (func);
  else
    {
      register int misc_index = find_pc_misc_function (fi->pc);
      if (misc_index >= 0)
	funname = misc_function_vector[misc_index].name;
    }
  calling_frame = get_prev_frame (frame);

  if (!addr_exp && selected_frame_level >= 0)
    ui_fprintf(stdout, "Stack level %d, frame at 0x%x:\n pc = 0x%x",
	    selected_frame_level, FRAME_FP(frame), fi->pc);
  else
    ui_fprintf(stdout, "Stack frame at 0x%x:\n pc = 0x%x",
	    FRAME_FP(frame), fi->pc);

  if (funname)
    ui_fprintf(stdout, " in %s", funname);
  if (sal.symtab)
    ui_fprintf(stdout, " (%s line %d)", sal.symtab->filename, sal.line);
  ui_fprintf(stdout, "; saved pc 0x%x\n", FRAME_SAVED_PC (frame));
  if (calling_frame)
    ui_fprintf(stdout, " called by frame at 0x%x", FRAME_FP (calling_frame));
  if (fi->next_frame && calling_frame)
    ui_fprintf(stdout, ",");
  if (fi->next_frame)
    ui_fprintf(stdout, " caller of frame at 0x%x", fi->next_frame);
  if (fi->next_frame || calling_frame)
    ui_fprintf(stdout, "\n");
  ui_fprintf(stdout, " Arglist at 0x%x,", FRAME_ARGS_ADDRESS (fi));
  FRAME_NUM_ARGS (i, fi);
  if (i < 0)
    ui_fprintf(stdout, " args: ");
  else if (i == 0)
    ui_fprintf(stdout, " no args.");
  else if (i == 1)
    ui_fprintf(stdout, " 1 arg: ");
  else
    ui_fprintf(stdout, " %d args: ", i);

  FRAME_NUM_ARGS (numargs, fi);
  print_frame_args (func, fi, numargs, stdout);
  ui_fprintf(stdout, "\n");
  /* The sp is special; what's returned isn't the save address, but
     actually the value of the previous frame's sp.  */
  ui_fprintf(stdout, " Previous frame's sp is 0x%x\n", fsr.regs[SP_REGNUM]);
  count = 0;
  for (i = 0; i < NUM_GENERAL_REGS; i++)
#ifdef TEK_HACK
    if (fsr.regs[i] != i && i != SP_REGNUM)
#else /* not TEK_HACK */
    if (fsr.regs[i] && i != SP_REGNUM)
#endif /* not TEK_HACK */
      {
	if (count % 4 != 0)
	  ui_fprintf(stdout, ", ");
	else
	  {
	    if (count == 0)
	      ui_fprintf(stdout, " Saved registers:");
	    ui_fprintf(stdout, "\n  ");
	  }
#ifdef TEK_HACK
        if (fsr.regs[i] == INVALID_CORE_ADDR)
          ui_fprintf(stdout, "#%s:%s#", reg_names[i], THE_UNKNOWN);
        else if (fsr.regs[i] == i)
          ui_fprintf(stdout, "%s not saved", reg_names[i]);
        else if (fsr.regs[i] < NUM_GENERAL_REGS)
          ui_fprintf(stdout, "%s in %s", reg_names[i], reg_names[fsr.regs[i]]);
        else
#endif /* TEK_HACK */
	ui_fprintf(stdout, "%s at 0x%x", reg_names[i], fsr.regs[i]);
	count++;
      }
  if (count)
    ui_fprintf(stdout, "\n");
}

#if 0
/* Set a limit on the number of frames printed by default in a
   backtrace.  */

static int backtrace_limit;

static void
set_backtrace_limit_command (count_exp, from_tty)
     char *count_exp;
     int from_tty;
{
  int count = parse_and_eval_address (count_exp);

  if (count < 0)
    ui_badnews(-1,"Negative argument not meaningful as backtrace limit.");

  backtrace_limit = count;
}

static void
backtrace_limit_info (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (arg)
    ui_badnews(-1,"\"Info backtrace-limit\" takes no arguments.");

  ui_fprintf(stdout, "Backtrace limit: %d.\n", backtrace_limit);
}
#endif

/* Print briefly all stack frames or just the innermost COUNT frames.  */

static void
backtrace_command (count_exp)
     char *count_exp;
{
  struct frame_info *fi;
  register int count;
  register FRAME frame;
  register int i;
  register FRAME trailing;
  register int trailing_level;

  /* The following code must do two things.  First, it must
     set the variable TRAILING to the frame from which we should start
     printing.  Second, it must set the variable count to the number
     of frames which we should print, or -1 if all of them.  */
  trailing = get_current_frame ();
  trailing_level = 0;
  if (count_exp)
    {
      count = parse_and_eval_address (count_exp);
      if (count < 0)
	{
	  FRAME current;

	  count = -count;

	  current = trailing;
	  while (current && count--)
	    current = get_prev_frame (current);
	  
	  /* Will stop when CURRENT reaches the top of the stack.  TRAILING
	     will be COUNT below it.  */
	  while (current)
	    {
	      trailing = get_prev_frame (trailing);
	      current = get_prev_frame (current);
	      trailing_level++;
	    }
	  
	  count = -1;
	}
    }
  else
    count = -1;

  for (i = 0, frame = trailing;
       frame && count--;
       i++, frame = get_prev_frame (frame))
    {
      QUIT;
      fi = get_frame_info (frame);
      print_frame_info (fi, trailing_level + i, 0, 1);
    }

  /* If we've stopped before the end, mention that.  */
  if (frame)
    printf_filtered ("(More stack frames follow...)\n");
}

/* Print the local variables of a block B active in FRAME.
   Return 1 if any variables were printed; 0 otherwise.  */

static int
print_block_frame_locals (b, frame, stream)
     struct block *b;
     register FRAME frame;
     register FILE *stream;
{
  int nsyms;
  register int i;
  register struct symbol *sym;
  register int values_printed = 0;

  nsyms = BLOCK_NSYMS (b);

  for (i = 0; i < nsyms; i++)
    {
      sym = BLOCK_SYM (b, i);
      if (SYMBOL_CLASS (sym) == LOC_LOCAL
	  || SYMBOL_CLASS (sym) == LOC_REGISTER
	  || SYMBOL_CLASS (sym) == LOC_STATIC)
	{
	  values_printed = 1;
	  fputs_filtered (SYMBOL_NAME (sym), stream);
	  fputs_filtered (" = ", stream);
	  print_variable_value (sym, frame, stream);
	  fprintf_filtered (stream, "\n");
	  ui_fflush (stream);
	}
    }
  return values_printed;
}

/* Print on STREAM all the local variables in frame FRAME,
   including all the blocks active in that frame
   at its current pc.

   Returns 1 if the job was done,
   or 0 if nothing was printed because we have no info
   on the function running in FRAME.  */

static int
print_frame_local_vars (frame, stream)
     register FRAME frame;
     register FILE *stream;
{
  register struct block *block = get_frame_block (frame);
  register int values_printed = 0;

  if (block == 0)
    {
      fprintf_filtered (stream, "No symbol table info available.\n");
      ui_fflush (stream);
      return 0;
    }
  
  while (block != 0)
    {
      if (print_block_frame_locals (block, frame, stream))
	values_printed = 1;
      /* After handling the function's top-level block, stop.
	 Don't continue to its superblock, the block of
	 per-file symbols.  */
      if (BLOCK_FUNCTION (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  if (!values_printed)
    {
      fprintf_filtered (stream, "No locals.\n");
      ui_fflush (stream);
    }
  
  return 1;
}

static void
locals_info ()
{
  if (!have_inferior_p () && !have_core_file_p ())
    ui_badnews(-1,"No inferior or core file.");

  print_frame_local_vars (selected_frame, stdout);
}

static int
print_frame_arg_vars (frame, stream)
     register FRAME frame;
     register FILE *stream;
{
  struct symbol *func = get_frame_function (frame);
  register struct block *b;
  int nsyms;
  register int i;
  register struct symbol *sym;
  register int values_printed = 0;

  if (func == 0)
    {
      fprintf_filtered (stream, "No symbol table info available.\n");
      ui_fflush (stream);
      return 0;
    }

  b = SYMBOL_BLOCK_VALUE (func);
  nsyms = BLOCK_NSYMS (b);

  for (i = 0; i < nsyms; i++)
    {
      sym = BLOCK_SYM (b, i);
      if (SYMBOL_CLASS (sym) == LOC_ARG
	  || SYMBOL_CLASS (sym) == LOC_REF_ARG
	  || SYMBOL_CLASS (sym) == LOC_REGPARM)
	{
	  values_printed = 1;
	  fputs_filtered (SYMBOL_NAME (sym), stream);
	  fputs_filtered (" = ", stream);
	  print_variable_value (sym, frame, stream);
	  fprintf_filtered (stream, "\n");
	  ui_fflush (stream);
	}
    }

  if (!values_printed)
    {
      fprintf_filtered (stream, "No arguments.\n");
      ui_fflush (stream);
    }

  return 1;
}

static void
args_info ()
{
  if (!have_inferior_p () && !have_core_file_p ())
    ui_badnews(-1,"No inferior or core file.");
  print_frame_arg_vars (selected_frame, stdout);
}

/* Select frame FRAME, and note that its stack level is LEVEL.
   LEVEL may be -1 if an actual level number is not known.  */

void
select_frame (frame, level)
     FRAME frame;
     int level;
{
  selected_frame = frame;
  selected_frame_level = level;
  /* Ensure that symbols for this frame are readin.  */
  if (frame)
    find_pc_symtab (get_frame_info (frame)->pc);
}

/* Store the selected frame and its level into *FRAMEP and *LEVELP.  */

void
record_selected_frame (frameaddrp, levelp)
     FRAME_ADDR *frameaddrp;
     int *levelp;
{
  *frameaddrp = FRAME_FP (selected_frame);
  *levelp = selected_frame_level;
}

/* Return the symbol-block in which the selected frame is executing.
   Can return zero under various legitimate circumstances.  */

struct block *
get_selected_block ()
{
  if (!have_inferior_p () && !have_core_file_p ())
    return 0;

  if (!selected_frame)
    return get_current_block ();
  return get_frame_block (selected_frame);
}

/* Find a frame a certain number of levels away from FRAME.
   LEVEL_OFFSET_PTR points to an int containing the number of levels.
   Positive means go to earlier frames (up); negative, the reverse.
   The int that contains the number of levels is counted toward
   zero as the frames for those levels are found.
   If the top or bottom frame is reached, that frame is returned,
   but the final value of *LEVEL_OFFSET_PTR is nonzero and indicates
   how much farther the original request asked to go.  */

FRAME
find_relative_frame (frame, level_offset_ptr)
     register FRAME frame;
     register int* level_offset_ptr;
{
  register FRAME prev;
  register FRAME frame1, frame2;

  /* Going up is simple: just do get_prev_frame enough times
     or until initial frame is reached.  */
  while (*level_offset_ptr > 0)
    {
      prev = get_prev_frame (frame);
      if (prev == 0)
	break;
      (*level_offset_ptr)--;
      frame = prev;
    }
  /* Going down could be done by iterating get_frame_info to
     find the next frame, but that would be quadratic
     since get_frame_info must scan all the way from the current frame.
     The following algorithm is linear.  */
  if (*level_offset_ptr < 0)
    {
      /* First put frame1 at innermost frame
	 and frame2 N levels up from there.  */
      frame1 = get_current_frame ();
      frame2 = frame1;
      while (*level_offset_ptr < 0 && frame2 != frame)
	{
	  frame2 = get_prev_frame (frame2);
	  (*level_offset_ptr) ++;
	}
      /* Then slide frame1 and frame2 up in synchrony
	 and when frame2 reaches our starting point
	 frame1 must be N levels down from there.  */
      while (frame2 != frame)
	{
	  frame1 = get_prev_frame (frame1);
	  frame2 = get_prev_frame (frame2);
	}
      return frame1;
    }
  return frame;
}

/* The "frame" command.  With no arg, print selected frame briefly.
   With arg LEVEL_EXP, select the frame at level LEVEL if it is a
   valid level.  Otherwise, treat level_exp as an address expression
   and print it.  See parse_frame_specification for more info on proper
   frame expressions. */

static void
frame_command (level_exp, from_tty)
     char *level_exp;
     int from_tty;
{
  register FRAME frame, frame1;
  unsigned int level = 0;

  if (!have_inferior_p () && ! have_core_file_p ())
    ui_badnews(-1,"No inferior or core file.");

  frame = parse_frame_specification (level_exp);

  for (frame1 = get_prev_frame (0);
       frame1 && frame1 != frame;
       frame1 = get_prev_frame (frame1))
    level++;

  if (!frame1)
    level = 0;

  frame_changed = level;
  select_frame (frame, level);

#ifdef TEK_HACK
  if (!from_tty) {
 /* We need to update current_source_line and current_source_symtab, or the 
  * next 'list' command references an out-of-date frame.
  */
    if (level) {
      struct symtab_and_line sal;
      register FRAME fi = get_frame_info(selected_frame);

      sal = find_pc_line (fi->pc, fi->next_frame);
      current_source_line = max (sal.line - 5, 1);
      current_source_symtab = sal.symtab;
    }
  }
#else
  if (!from_tty)
    return;
#endif /* TEK_HACK */

  print_stack_frame (selected_frame, selected_frame_level, 1);
}

/* Select the frame up one or COUNT stack levels
   from the previously selected frame, and print it briefly.  */

static void
up_command (count_exp)
     char *count_exp;
{
  register FRAME frame;
  int count = 1, count1;
  if (count_exp)
    count = parse_and_eval_address (count_exp);
  count1 = count;
  
  if (!have_inferior_p () && !have_core_file_p ())
    ui_badnews(-1,"No inferior or core file.");

  frame = find_relative_frame (selected_frame, &count1);
  if (count1 != 0 && count_exp == 0)
    ui_badnews(-1,"Initial frame selected; you cannot go up.");
  select_frame (frame, selected_frame_level + count - count1);

  print_stack_frame (selected_frame, selected_frame_level, 1);
  frame_changed++;
}

/* Select the frame down one or COUNT stack levels
   from the previously selected frame, and print it briefly.  */

static void
down_command (count_exp)
     char *count_exp;
{
  register FRAME frame;
  int count = -1, count1;
  if (count_exp)
    count = - parse_and_eval_address (count_exp);
  count1 = count;
  
  frame = find_relative_frame (selected_frame, &count1);
  if (count1 != 0 && count_exp == 0)
    ui_badnews(-1,"Bottom (i.e., innermost) frame selected; you cannot go down.");
  select_frame (frame, selected_frame_level + count - count1);

  print_stack_frame (selected_frame, selected_frame_level, 1);
  frame_changed--;
}

static void
return_command (retval_exp, from_tty)
     char *retval_exp;
     int from_tty;
{
  struct symbol *thisfun = get_frame_function (selected_frame);
  FRAME_ADDR selected_frame_addr = FRAME_FP (selected_frame);

  /* If interactive, require confirmation.  */

  if (from_tty)
    {
      if (thisfun != 0)
	{
	  if (!query ("Make %s return now? ", SYMBOL_NAME (thisfun)))
	    ui_badnews(-1,"Not confirmed.");
	}
      else
	if (!query ("Make selected stack frame return now? "))
	  ui_badnews(-1,"Not confirmed.");
    }

  /* Do the real work.  Pop until the specified frame is current.  We
     use this method because the selected_frame is not valid after
     a POP_FRAME.  Note that this will not work if the selected frame
     shares it's fp with another frame.  */

  while (selected_frame_addr != FRAME_FP (get_current_frame()))
    POP_FRAME;

  /* Then pop that frame.  */

  POP_FRAME;

  /* Compute the return value (if any) and store in the place
     for return values.  */

  if (retval_exp)
    set_return_value (parse_and_eval (retval_exp));

  /* If interactive, print the frame that is now current.  */

  if (from_tty)
    frame_command ("0", 1);
}

extern struct cmd_list_element *setlist;
#ifdef TEK_PROG_HACK
extern struct cmd_list_element *set_cmd;
#endif /* TEK_PROG_HACK */

void
_initialize_stack ()
{
#if 0  
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;
#endif /* TEK_PROG_HACK */
  backtrace_limit = 30;
#endif

  add_com ("return", class_stack, return_command,
	   "Make selected stack frame return to its caller.\n\
Control remains in the debugger, but when you continue\n\
execution will resume in the frame above the one now selected.\n\
If an argument is given, it is an expression for the value to return.");

  add_com ("up", class_stack, up_command,
	   "Select and print stack frame that called this one.\n\
An argument says how many frames up to go.");

  add_com ("down", class_stack, down_command,
	   "Select and print stack frame called by this one.\n\
An argument says how many frames down to go.");
  add_com_alias ("do", "down", class_stack, 1);

  add_com ("frame", class_stack, frame_command,
	   "Select and print a stack frame.\n\
With no argument, print the selected stack frame.  (See also \"info frame\").\n\
An argument specifies the frame to select.\n\
It can be a stack frame number or the address of the frame.\n\
With argument, nothing is printed if input is coming from\n\
a command file or a user-defined command.");

  add_com_alias ("f", "frame", class_stack, 1);

  add_com ("backtrace", class_stack, backtrace_command,
	   "Print backtrace of all stack frames, or innermost COUNT frames.\n\
With a negative argument, print outermost -COUNT frames.");
  add_com_alias ("bt", "backtrace", class_stack, 0);
  add_com_alias ("where", "backtrace", class_alias, 0);
  add_info ("stack", backtrace_command,
	    "Backtrace of the stack, or innermost COUNT frames.");
  add_info_alias ("s", "stack", 1);
  add_info ("frame", frame_info,
	    "All about selected stack frame, or frame at ADDR.");
  add_info_alias ("f", "frame", 1);
  add_info ("locals", locals_info,
	    "Local variables of current stack frame.");
  add_info ("args", args_info,
	    "Argument variables of current stack frame.");

#if 0
#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("backtrace-limit", class_stack, set_backtrace_limit_command, 
	   "Specify maximum number of frames for \"backtrace\" to print by default.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */
  add_info ("backtrace-limit", backtrace_limit_info,
	    "The maximum number of frames for \"backtrace\" to print by default.");
#endif
}

