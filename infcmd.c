/* Memory-access and commands for inferior process, for GDB.
   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/infcmd.c,v 1.38 91/01/13 23:43:42 robertb Exp $
   $Locker:  $ */

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "environ.h"
#include "value.h"
#include "ui.h"
#ifdef TEK_PROG_HACK
#include "command.h"
#endif /* TEK_PROG_HACK */

#include <stdio.h>
#include <signal.h>

//extern char *sys_siglist[];

#define ERROR_NO_INFERIOR \
   if (inferior_pid == 0) ui_badnews(-1,"The program is not being run.");

/* String containing arguments to give to the program,
   with a space added at the front.  Just a space means no args.  */

static char *inferior_args;

/* File name for default use for standard in/out in the inferior.  */

char *inferior_io_terminal;

/* Pid of our debugged inferior, or 0 if no inferior now.  */

int inferior_pid;

/* Last signal that the inferior received (why it stopped).  */

int stop_signal;

/* Address at which inferior stopped.  */

CORE_ADDR stop_pc;

/* Stack frame when program stopped.  */

FRAME_ADDR stop_frame_address;

/* Number of breakpoint it stopped at, or 0 if none.  */

int stop_breakpoint;

/* Number of watchpoint it stopped at, or 0 if none. */

extern int stop_watchpoint;

/* Nonzero if stopped due to a step command.  */

int stop_step;

/* Nonzero if stopped due to completion of a stack dummy routine.  */

int stop_stack_dummy;

/* Nonzero if stopped due to a random (unexpected) signal in inferior
   process.  */

int stopped_by_random_signal;

/* Range to single step within.
   If this is nonzero, respond to a single-step signal
   by continuing to step if the pc is in this range.  */

CORE_ADDR step_range_start; /* Inclusive */
CORE_ADDR step_range_end; /* Exclusive */

/* Stack frame address as of when stepping command was issued.
   This is how we know when we step into a subroutine call,
   and how to set the frame for the breakpoint used to step out.  */

FRAME_ADDR step_frame_address;

/* 1 means step over all subroutine calls.
   -1 means step over calls to undebuggable functions.  */

int step_over_calls;

/* If stepping, nonzero means step count is > 1
   so don't print frame next time inferior stops
   if it stops due to stepping.  */

int step_multi;

/* Environment to use for running inferior,
   in format described in environ.h.  */

struct environ *inferior_environ;

CORE_ADDR read_pc ();
struct command_line *get_breakpoint_commands ();
static void step_1();

int
have_inferior_p ()
{
  return inferior_pid != 0;
}

static void 
set_args_command (args)
     char *args;
{
  free (inferior_args);
  if (!args) args = "";
  inferior_args = concat (" ", args, "");
}

void
tty_command (file,from_tty)
     char *file;
     int from_tty;
{
  if (file == 0)
    error_no_arg ("terminal name for running target process");

  inferior_io_terminal = savestring (file, strlen (file));
}

static void
run_command (args, from_tty)
     char *args;
     int from_tty;
{
  extern char **environ;
  register int i;
  char *exec_file;
  char *allargs;

//  extern int sys_nerr;
//  extern char *sys_errlist[];
  extern int errno;

  dont_repeat ();

  if (inferior_pid)
    {
      if (
	  !query ("The program being debugged has been started already.\n\
Start it from the beginning? "))
	ui_badnews(-1,"Program not restarted.");
      kill_inferior ();
    }

  exec_file = (char *) get_exec_file (1);

  if (remote_debugging)
    {
      if (from_tty)
	{
	  ui_fprintf(stdout, "Starting program: %s\n", exec_file);
	  ui_fflush (stdout);
	}
    }
  else
    {
      if (args)
	set_args_command (args);

      if (from_tty)
	{
	  ui_fprintf(stdout, "Starting program: %s%s\n",
		  exec_file, inferior_args);
	  ui_fflush (stdout);
	}

      allargs = concat ("exec ", exec_file, inferior_args);
      inferior_pid = create_inferior (allargs, environ_vector (inferior_environ));
    }

  clear_proceed_status ();
#ifdef TEK_HACK
#ifdef m88k
  m88k_clear_dummy_stack();
#endif
#endif

  start_inferior ();
}

void
cont_command (proc_count_exp, from_tty)
     char *proc_count_exp;
     int from_tty;
{
  ERROR_NO_INFERIOR;

  clear_proceed_status ();

  /* If have argument, set proceed count of breakpoint or watchpoint
     we stopped at.  */
  if (stop_breakpoint > 0 && proc_count_exp)
    {
      set_ignore_count (stop_breakpoint,
			parse_and_eval_address (proc_count_exp) - 1,
			from_tty);
      if (from_tty)
	ui_fprintf(stdout, "  ");
    }
  else if (stop_watchpoint > 0 && proc_count_exp)
    {
      ignore_watchpoint_by_num (stop_watchpoint,
				parse_and_eval_address (proc_count_exp) - 1,
			        from_tty);
      if (from_tty)
	ui_fprintf(stdout, "  ");
    }

  if (from_tty)
    ui_fprintf(stdout, "Continuing.\n");

  /* Execute from the current pc, pass whatever signal stopped the
     process, continue, don't step */
  proceed (-1, -1, 0);

}

/* Step until outside of current statement.  */
static void step_1 ();

static void
step_command (count_string)
{
  step_1 (0, 0, count_string);
}

/* Likewise, but skip over subroutine calls as if single instructions.  */

static void
next_command (count_string)
{
  step_1 (1, 0, count_string);
}

/* Likewise, but step only one instruction.  */

static void
stepi_command (count_string)
{
  step_1 (0, 1, count_string);
}

static void
nexti_command (count_string)
{
  step_1 (1, 1, count_string);
}

static void
step_1 (skip_subroutines, single_inst, count_string)
     int skip_subroutines;
     int single_inst;
     char *count_string;
{
  register int count = 1;
  FRAME fr;

  ERROR_NO_INFERIOR;
  count = count_string ? parse_and_eval_address (count_string) : 1;

  for (; count > 0; count--)
    {
      clear_proceed_status ();

      fr = get_current_frame();
      if (fr != (FRAME)0) {
        step_frame_address = FRAME_FP (fr);
      }

      if (! single_inst)
	{
	  find_pc_line_pc_range (stop_pc, &step_range_start, &step_range_end);
	  if (step_range_end == 0)
	    {
	      int misc;

	      misc = find_pc_misc_function (stop_pc);
	      terminal_ours ();
	      ui_fprintf(stdout, "Current function has no line number information.\n");
	      ui_fflush (stdout);

	      /* No info or after _etext ("Can't happen") */
	      if (misc == -1 || misc == misc_function_count - 1)
		ui_badnews(-1,"No data available on pc function.");

	      ui_fprintf(stdout, "Single stepping until function exit.\n");
	      ui_fflush (stdout);

	      step_range_start = misc_function_vector[misc].address;
	      step_range_end = misc_function_vector[misc + 1].address;
	    }
	}
      else
	{
	  /* Say we are stepping, but stop after one insn whatever it does.
	     Don't step through subroutine calls even to undebuggable
	     functions.  */
	  step_range_start = step_range_end = 1;
	  if (!skip_subroutines)
	    step_over_calls = 0;
	}

      if (skip_subroutines)
	step_over_calls = 1;

      step_multi = (count > 1);
      /* Execute from the current pc, pass whatever signal stopped the
         process, stop after one instruction and display our location.  */
      proceed (-1, -1, 1);
      if (! stop_step)
	break;
    }
}

/* Continue program at specified address.  */

static void
jump_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  register CORE_ADDR addr;
  struct symtabs_and_lines sals;
  struct symtab_and_line sal;

  ERROR_NO_INFERIOR;

  if (!arg)
    error_no_arg ("starting address");

  sals = decode_line_spec_1 (arg, 1);
  if (sals.nelts != 1)
    {
      ui_badnews(-1,"Unreasonable jump request");
    }

  sal = sals.sals[0];
  free (sals.sals);

  if (sal.symtab == 0 && sal.pc == 0)
    ui_badnews(-1,"No source file has been specified.");

  if (sal.pc == 0)
    {
      sal.pc = find_line_pc (sal.symtab, sal.line);
      if (sal.pc == -1)
	ui_badnews(-1,"File \"%s\" was not compiled with the compiler -g option.",
		   sal.symtab->filename);
    }

  {
    struct symbol *fn = get_frame_function (get_current_frame ());
    struct symbol *sfn = find_pc_function (sal.pc);
    if (fn != 0 && sfn != fn
	&& ! query ("Line %d is not in `%s'.  Jump anyway? ",
		    sal.line, SYMBOL_NAME (fn)))
      ui_badnews(-1,"Not confirmed.");
  }

  if (sal.pc == 0)
    ui_badnews(-1,"No line %d in file \"%s\".", sal.line, sal.symtab->filename);

  addr = sal.pc;

  clear_proceed_status ();

  if (from_tty)
    ui_fprintf(stdout, "Continuing at 0x%x.\n", addr);

  /* Execute from addr, don't resume subprocess with any signals,
     continue, don't step */
  proceed (addr, 0, 0);
}

/* Continue program giving it specified signal.  */

static void
signal_command (signum_exp, from_tty)
     char *signum_exp;
     int from_tty;
{
  register int signum;

  dont_repeat ();		/* Too dangerous.  */
  ERROR_NO_INFERIOR;

  if (!signum_exp)
    error_no_arg ("signal number");

#ifndef _WIN32
#ifdef TEK_HACK
 /* Add support for SIGxxx type signal names */
  if (strncmp(signum_exp, "SIG", 3) == 0) {
    int n = 1;

    signum = -1;
    while (n < NSIG) {
      if (strcmp(signum_exp, sys_siglist[n]) == 0) {
         signum = n;
         break;
      }
      n++;
    }
    if (signum <= 0) 
	ui_badnews(-1, "Invalid signal %s given as argument to \"signal\".",
		signum_exp);
  }
  else {
#endif /* TEK_HACK */

  signum = parse_and_eval_address (signum_exp);
#ifdef TEK_HACK
  }
#endif /* TEK_HACK */
#endif /* _WIN32   */


  clear_proceed_status ();

  if (from_tty)
    ui_fprintf(stdout, "Continuing with signal %d.\n", signum);

  /* Execute from the current pc, resume subprocess with signal 'signum'
     continue, don't step */
  proceed (-1, signum, 0);
}

/* Execute a "stack dummy", a piece of code stored in the stack
   by the debugger to be executed in the inferior.

   To call: first, do PUSH_DUMMY_FRAME.
   Then push the contents of the dummy.  It should end with a breakpoint insn.
   Then call here, passing address at which to start the dummy.

   The contents of all registers are saved before the dummy frame is popped
   and copied into the buffer BUFFER.

   The dummy's frame is automatically popped whenever that break is hit.
   If that is the first time the program stops, run_stack_dummy
   returns to its caller with that frame already gone.
   Otherwise, the caller never gets returned to.  */

/* 4 => return instead of letting the stack dummy run.  */

static int stack_dummy_testing = 0;

void
run_stack_dummy (addr, buffer)
     CORE_ADDR addr;
     REGISTER_TYPE *buffer;
{
  /* Now proceed, having reached the desired place.  */
  clear_proceed_status ();
  if (stack_dummy_testing & 4)
    {
      POP_FRAME;
      return;
    }
  /* Execute from addr, resume subprocess with no signal,
     continue, don't step */
  proceed (addr, 0, 0);

#ifndef TEK_HACK
  if (!stop_stack_dummy)
    ui_badnews(-1,"Cannot continue previously requested operation.");
#endif

  /* On return, the stack dummy has been popped already.  */

  bcopy (stop_registers, buffer, sizeof stop_registers);
}

/* Proceed until we reach the given line as argument or exit the
   function.  When called with no argument, proceed until we reach a
   different source line with pc greater than our current one or exit
   the function.  We skip calls in both cases.

   The effect of this command with an argument is identical to setting
   a momentary breakpoint at the line specified and executing
   "finish".

   Note that eventually this command should probably be changed so
   that only source lines are printed out when we hit the breakpoint
   we set.  I'm going to postpone this until after a hopeful rewrite
   of wait_for_inferior and the proceed status code. -- randy */

void
until_next_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  FRAME frame;
  CORE_ADDR pc;
  struct symbol *func;
  struct symtab_and_line sal;
    
  clear_proceed_status ();

  frame = get_current_frame ();

  /* Step until either exited from this function or greater
     than the current line (if in symbolic section) or pc (if
     not). */

  pc = read_pc ();
  func = find_pc_function (pc);
  
  if (!func)
    {
      int misc_func = find_pc_misc_function (pc);
      
      if (misc_func != -1)
	ui_badnews(-1,"Execution is not within a known function.");
      
      step_range_start = misc_function_vector[misc_func].address;
      step_range_end = pc;
    }
  else
    {
      sal = find_pc_line (pc, 0);
      
      step_range_start = BLOCK_START (SYMBOL_BLOCK_VALUE (func));
      step_range_end = sal.end;
    }
  
  step_over_calls = 1;
  step_frame_address = FRAME_FP (frame);
  
  step_multi = 0;		/* Only one call to proceed */
  
  /* Execute from the current pc, resume subprocess with whatever signal
     stopped us, stop after 1 instruction and display our location */
  proceed (-1, -1, 1);
}

void 
until_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  if (!have_inferior_p ())
    ui_badnews(-1,"The program is not being run.");

  if (arg)
    until_break_command (arg, from_tty);
  else
    until_next_command (arg, from_tty);
}

/* "finish": Set a temporary breakpoint at the place
   the selected frame will return to, then continue.  */

static void
finish_command (arg, from_tty)
     char *arg;
     int from_tty;
{

#ifdef TEK_HACK
//  extern void finish_or_complete();

  finish_or_complete (arg, from_tty, 0, 0);
}

/* "complete": Just like finish, except ignore all breakpoints
   until we actually return from selected frame. */

void
complete_command (arg, from_tty)
     char *arg;
     int from_tty;
{
   finish_or_complete (arg, from_tty, 1, 0);
}

/* Here are the 'r' commands, to facilitate dbx-style tracing of functions.
 * We continue after a 'complete' or 'finish'.
 */
static void
rcomplete_command (arg, from_tty)
     char *arg;
     int from_tty;
{
   finish_or_complete (arg, from_tty, 1, 1);
}


static void
rfinish_command (arg, from_tty)
     char *arg;
     int from_tty;
{
   finish_or_complete (arg, from_tty, 0, 1);
}




void
finish_or_complete (arg, from_tty, ignore_breakpoints, do_continue)
     char *arg;
     int from_tty;
     int ignore_breakpoints;
     int do_continue;
{
#endif /* TEK_HACK */
  struct symtab_and_line sal;
  register FRAME frame;
  struct frame_info *fi;
  register struct symbol *function;

  if (!have_inferior_p ())
    ui_badnews(-1,"The program is not being run.");
  if (arg)
#ifdef TEK_HACK
    ui_badnews(-1,"The \"%s\" command does not take any arguments.", 
	ignore_breakpoints ? "complete" : "finish");
#else
    ui_badnews(-1,"The \"finish\" command does not take any arguments.");
#endif /* TEK_HACKL */

  frame = get_prev_frame (selected_frame);
  if (frame == 0)
#ifdef TEK_HACK
    ui_badnews(-1,"\"%s\" not meaningful in the outermost frame.",
	ignore_breakpoints ? "complete" : "finish");
#else
    ui_badnews(-1,"\"finish\" not meaningful in the outermost frame.");
#endif /* TEK_HACK */

  clear_proceed_status ();

  fi = get_frame_info (frame);
  sal = find_pc_line (fi->pc, 0);
  sal.pc = fi->pc;
  set_momentary_breakpoint (sal, frame);

  /* Find the function we will return from.  */

  fi = get_frame_info (selected_frame);
  function = find_pc_function (fi->pc);

  if (from_tty)
    {
      ui_fprintf(stdout, "Run till exit from ");
      print_selected_frame ();
    }

#ifdef TEK_HACK
 while (1) {
#endif /* TEK_HACK */
  /* Execute from the current pc, resume subprocess with whatever 
     signal stopped us, continue, don't step */
  proceed (-1, -1, 0);

  if (stop_breakpoint == -3 && function != 0)
    {
      struct type *value_type;
      register value val;
      CORE_ADDR funcaddr;

      value_type = TYPE_TARGET_TYPE (SYMBOL_TYPE (function));
      if (!value_type)
	ui_badnews(1,"internal: finish_command: function has no target type");
      
      if (TYPE_CODE (value_type) == TYPE_CODE_VOID)
	return;

      funcaddr = BLOCK_START (SYMBOL_BLOCK_VALUE (function));

      val = value_being_returned (value_type, stop_registers,
				  using_struct_return (function,
						       funcaddr,
						       value_type));

      ui_fprintf(stdout, "Value returned is $%d = ", record_latest_value (val));
      value_print (val, stdout, 0, Val_no_prettyprint, M_NORMAL);
      ui_putchar ('\n');
#ifdef TEK_HACK
      if (do_continue) {
        /* Execute from the current pc, resume subprocess with whatever 
           signal stopped us, continue, don't step */
        proceed(-1, -1, 0);
      }
      return;
#endif /* TEK_HACK */
    }
#ifdef TEK_HACK
    if (! ignore_breakpoints) {
	return;
    }
    set_momentary_breakpoint(sal, frame);
  }
#endif /* TEK_HACK */
}

static void
program_info ()
{
  if (inferior_pid == 0)
    {
      ui_fprintf(stdout, "The program being debugged is not being run.\n");
      return;
    }

  ui_fprintf(stdout, "Program being debugged is in process %d, stopped at 0x%x.\n",
	  inferior_pid, stop_pc);
  if (stop_step)
    ui_fprintf(stdout, "It stopped after being stepped.\n");
  else if (stop_breakpoint > 0)
    ui_fprintf(stdout, "It stopped at breakpoint %d.\n", stop_breakpoint);
  else if (stop_watchpoint > 0)
    ui_fprintf(stdout, "It stopped at watchpoint %d.\n", stop_watchpoint);
#ifndef _WIN32
  else if (stop_signal)
    ui_fprintf(stdout, "It stopped with signal %d (%s).\n",
	    stop_signal, sys_siglist[stop_signal]);
#endif

  ui_fprintf(stdout, "\nType \"info stack\" or \"info reg\" for more information.\n");
}

static void
environment_info (var)
     char *var;
{
  if (var)
    {
      register char *val = get_in_environ (inferior_environ, var);
      if (val)
	ui_fprintf(stdout, "%s = %s\n", var, val);
      else
	ui_fprintf(stdout, "Environment variable \"%s\" not defined.\n", var);
    }
  else
    {
      register char **vector = environ_vector (inferior_environ);
      while (*vector)
	ui_fprintf(stdout, "%s\n", *vector++);
    }
}

static void
set_environment_command (arg)
     char *arg;
{
  register char *p, *val, *var;
  int nullset = 0;

  if (arg == 0)
    error_no_arg ("environment variable and value");

  /* Find seperation between variable name and value */
  p = (char *) index (arg, '=');
  val = (char *) index (arg, ' ');

  if (p != 0 && val != 0)
    {
      /* We have both a space and an equals.  If the space is before the
	 equals and the only thing between the two is more space, use
	 the equals */
      if (p > val)
	while (*val == ' ')
	  val++;

      /* Take the smaller of the two.  If there was space before the
	 "=", they will be the same right now. */
      p = arg + min (p - arg, val - arg);
    }
  else if (val != 0 && p == 0)
    p = val;

  if (p == arg)
    error_no_arg ("environment variable to set");

  if (p == 0 || p[1] == 0)
    {
      nullset = 1;
      if (p == 0)
	p = arg + strlen (arg);	/* So that savestring below will work */
    }
  else
    {
      /* Not setting variable value to null */
      val = p + 1;
      while (*val == ' ' || *val == '\t')
	val++;
    }

  while (p != arg && (p[-1] == ' ' || p[-1] == '\t')) p--;

  var = savestring (arg, p - arg);
  if (nullset)
    {
      ui_fprintf(stdout, "Setting environment variable \"%s\" to null value.\n", var);
      set_in_environ (inferior_environ, var, "");
    }
  else
    set_in_environ (inferior_environ, var, val);
  free (var);
}

static void
unset_environment_command (var,from_tty)
     char *var;
     int from_tty;
{
  if (var == 0)
    /* If there is no argument, delete all environment variables.
       Ask for confirmation if reading from the terminal.  */
    if (!from_tty || query ("Delete all environment variables? "))
      {
        free_environ (inferior_environ);
        inferior_environ = make_environ ();
      }


  unset_in_environ (inferior_environ, var);
}

/* Read an integer from debugged memory, given address and number of bytes.  */

long
read_memory_integer (memaddr, len)
     CORE_ADDR memaddr;
     int len;
{
  char cbuf;
  short sbuf;
  int ibuf;
  long lbuf;
  int result_err;
  //extern int sys_nerr;
  //extern char *sys_errlist[];

  if (len == sizeof (char))
    {
      result_err = read_memory (memaddr, &cbuf, len, M_NORMAL);
      if (result_err)
	ui_badnews(-1,"Error reading memory address 0x%x: %s (%d).",
	       memaddr, (result_err < sys_nerr ?
			 sys_errlist[result_err] :
			 "uknown error"), result_err);
      return cbuf;
    }
  if (len == sizeof (short))
    {
      result_err = read_memory (memaddr, &sbuf, len, M_NORMAL);
      if (result_err)
	ui_badnews(-1,"Error reading memory address 0x%x: %s (%d).",
	       memaddr, (result_err < sys_nerr ?
			 sys_errlist[result_err] :
			 "uknown error"), result_err);
      return sbuf;
    }
  if (len == sizeof (int))
    {
      result_err = read_memory (memaddr, &ibuf, len, M_NORMAL);
      if (result_err)
	ui_badnews(-1,"Error reading memory address 0x%x: %s (%d).",
	       memaddr, (result_err < sys_nerr ?
			 sys_errlist[result_err] :
			 "uknown error"), result_err);
      return ibuf;
    }
  if (len == sizeof (lbuf))
    {
      result_err = read_memory (memaddr, &lbuf, len, M_NORMAL);
      if (result_err)
	ui_badnews(-1,"Error reading memory address 0x%x: %s (%d).",
	       memaddr, (result_err < sys_nerr ?
			 sys_errlist[result_err] :
			 "uknown error"), result_err);
      return lbuf;
    }
  ui_badnews(-1,"Cannot handle integers of %d bytes.", len);
}

CORE_ADDR
read_pc ()
{
  if (!have_inferior_p () && !have_core_file_p ())
    return 0;
  return read_register (SYNTH_PC_REGNUM);
}

#ifdef	TEK_HACK
/* Tektronix 88k version of `info reg`. */
static void
registers_info (addr_exp)
     char *addr_exp;
{
  register int i;
  int regnum;
  REGISTER_TYPE regval;

  if (!have_inferior_p () && !have_core_file_p ())
    ui_badnews(-1,"No inferior or core file");

  if (addr_exp)
    {
      /* The user named a specific register.
	 (Which is pretty silly; it's easier just to say "p $reg", and you get
	  format control that way.)
	 Identify the particular register and get its REGNUM. */
      if (*addr_exp >= '0' && *addr_exp <= '9')
	{
	  regnum = atoi (addr_exp);
	  if (regnum<0 || NUM_REGS<=regnum)
	    ui_badnews(-1, "There is no register number %d.", regnum);
	}
      else
	{
	  register char *p = addr_exp;
	  if (p[0] == '$')
	    p++;

	  /* We ought to do upper- to lower-case conversion here, but actually
	     I don't think anybody cares (not a lot of Fortran novices need
	     the `INFO REGS SNIP` command ... */

	  for (regnum = 0; regnum < NUM_REGS; regnum++)
	    if (!strcmp (p, reg_names[regnum]))
	      break;
	  if (regnum == NUM_REGS)
	    ui_badnews(-1,"%s: invalid register name.", addr_exp);
	}

      /* Read and display that register. */
      if (!CAN_REGACC_USER(regnum))
	ui_badnews(-1,"Can't access register %s from a Unix process.",
		   reg_names[regnum]);
      regval = read_register (regnum);
      printf_filtered ("%s\t0x%08x\t%d\n", reg_names[regnum], regval, regval);
      return;
    }

  /* No argument.  Display all registers.
     Start by displaying the 32 general-purpose registers in tabular form. */
  for (i = 0; i<NUM_GENERAL_REGS; i++)
    {
      unsigned register_contents;
/*
rnn: xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
 */
      if (!(i&7)) {
	printf_filtered ("r%-2d:", i);
      }
      if (read_relative_register_raw_bytes (i, &register_contents)) {
          printf_filtered (" %8s", THE_UNKNOWN);
      } else {
        printf_filtered(" %08x", register_contents);
      }
      if (!((i+1)&7)) {
	printf_filtered ("\n");
      }
    }

  if (selected_frame->next == (FRAME)0) {
    /* Now display the other registers.
       This code "knows" which registers are user-accessible, and formats them
       in a highly usable fashion:
  
  fp  = xxxxxxxx  sp   = xxxxxxxx  synthfp = xxxxxxxx
  pc  = xxxxxxxx  sxip = xxxxxxxx  snip    = xxxxxxxx  sfip = xxxxxxxx
  psr = xxxxxxxx  fpsr = xxxxxxxx  fpcr    = xxxxxxxx
  
       This is inadequate for remote or simulated gdb, and should be adapted
       when those products are better supported. */
  
    printf_filtered("fp  = %08x  sp   = %08x  synthfp = %08x\n",
		    read_register(FP_REGNUM), read_register(SP_REGNUM),
		    read_register(SYNTH_FP_REGNUM));
    printf_filtered("pc  = %08x  sxip = %08x  snip    = %08x  sfip = %08x\n",
		    read_register(SYNTH_PC_REGNUM), read_register(SXIP_REGNUM),
		    read_register(SNIP_REGNUM), read_register(SFIP_REGNUM));
    printf_filtered("psr = %08x  fpsr = %08x  fpcr    = %08x\n",
		    read_register(PSR_REGNUM), read_register(FPSR_REGNUM),
		    read_register(FPCR_REGNUM));
  } else {
    printf_filtered("fp = %08x  sp = %08x  pc = %08x\n",
             selected_frame->frame, selected_frame->sp, selected_frame->pc);
    printf_filtered ("Contents are relative to selected stack frame.\n");
  }
}

#else
static void
registers_info (addr_exp)
     char *addr_exp;
{
  register int i;
  int regnum;

  if (!have_inferior_p () && !have_core_file_p ())
    ui_badnews(-1,"No inferior or core file");

  if (addr_exp)
    {
      if (*addr_exp >= '0' && *addr_exp <= '9')
	regnum = atoi (addr_exp);
      else
	{
	  register char *p = addr_exp;
	  if (p[0] == '$')
	    p++;
	  for (regnum = 0; regnum < NUM_REGS; regnum++)
	    if (!strcmp (p, reg_names[regnum]))
	      break;
	  if (regnum == NUM_REGS)
	    ui_badnews(-1,"%s: invalid register name.", addr_exp);
	}
    }
  else
    printf_filtered ("Reg\tContents\n\n");

  for (i = 0; i < NUM_REGS; i++)
    {
      unsigned char raw_buffer[MAX_REGISTER_RAW_SIZE];
      unsigned char virtual_buffer[MAX_REGISTER_VIRTUAL_SIZE];
      REGISTER_TYPE val;  

      if (addr_exp != 0 && i != regnum)
	continue;

      if (!CAN_REGACC_USER(i))
	continue;

      /* Get the data in raw format, then convert also to virtual format.  */
#ifdef TEK_HACK
      if (read_relative_register_raw_bytes (i, raw_buffer))
        {
          printf_filtered ("#%s=%s#\n", reg_names[i], THE_UNKNOWN);
          continue;
        }
#else /* not TEK_HACK */
      read_relative_register_raw_bytes (i, raw_buffer);
#endif /* not TEK_HACK */
      REGISTER_CONVERT_TO_VIRTUAL (i, raw_buffer, virtual_buffer);

      printf_filtered ("%s\t", reg_names[i]);
#ifndef TEK_HACK
      print_spaces_filtered (15 - strlen (reg_names[i]), stdout);
#endif

      /* If virtual format is floating, print it that way.  */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (i)) == TYPE_CODE_FLT
	  && ! INVALID_FLOAT (virtual_buffer, REGISTER_VIRTUAL_SIZE (i)))
	val_print (REGISTER_VIRTUAL_TYPE (i), virtual_buffer, 0,
		   stdout, 0, 1, 0, Val_pretty_default, M_NORMAL);
      /* Else if virtual format is too long for printf,
	 print in hex a byte at a time.  */
      else if (REGISTER_VIRTUAL_SIZE (i) > sizeof (long))
	{
	  register int j;
	  printf_filtered ("0x");
	  for (j = 0; j < REGISTER_VIRTUAL_SIZE (i); j++)
	    printf_filtered ("%02x", virtual_buffer[j]);
	}
      /* Else print as integer in hex and in decimal.  */
      else
	{
	  long val;

	  bcopy (virtual_buffer, &val, sizeof (long));
	  if (val == 0)
	    printf_filtered ("0");
	  else
	    printf_filtered ("0x%08x  %d", val, val);
	}

      /* If register has different raw and virtual formats,
	 print the raw format in hex now.  */

      if (REGISTER_CONVERTIBLE (i))
	{
	  register int j;

	  printf_filtered ("  (raw 0x");
	  for (j = 0; j < REGISTER_RAW_SIZE (i); j++)
	    printf_filtered ("%02x", raw_buffer[j]);
	  printf_filtered (")");
	}
      printf_filtered ("\n");
    }
    printf_filtered ("Contents are relative to selected stack frame.\n");
}
#endif /* TEK_HACK */


#ifdef ATTACH_DETACH
#define PROCESS_ATTACH_ALLOWED 1
#else
#define PROCESS_ATTACH_ALLOWED 0
#endif
/*
 * TODO:
 * Should save/restore the tty state since it might be that the
 * program to be debugged was started on this tty and it wants
 * the tty in some state other than what we want.  If it's running
 * on another terminal or without a terminal, then saving and
 * restoring the tty state is a harmless no-op.
 * This only needs to be done if we are attaching to a process.
 */

/*
 * attach_command --
 * takes a program started up outside of gdb and ``attaches'' to it.
 * This stops it cold in its tracks and allows us to start tracing it.
 * For this to work, we must be able to send the process a
 * signal and we must have the same effective uid as the program.
 */

static void
attach_command (args, from_tty)
     char *args;
     int from_tty;
{
  char *exec_file;
  int pid;
  int remote = 0;

  dont_repeat();

  if (!args)
    error_no_arg ("process-id or device file to attach");

  while (*args == ' ' || *args == '\t') args++;

#ifdef TEK_HACK
  if (strncmp(args, "sim", 3) == 0) {
    extern simulator;
    simulator = 1;
    remote_debugging = 1;
    sim_init();
    start_remote ();  
    ui_fprintf(stdout, "Attached to simulator\n");
    return;
  }
#endif
  
  if (args[0] == '/')
    remote = 1;
  else
#ifndef ATTACH_DETACH
    ui_badnews(-1,"Can't attach to a process on this machine.");
#else
    pid = atoi (args);
#endif

  if (inferior_pid)
    {
      if (query ("A program is being debugged already.  Kill it? "))
	kill_inferior ();
      else
	ui_badnews(-1,"Inferior not killed.");
    }

  exec_file = (char *) get_exec_file (1);

  if (from_tty)
    {
      if (remote)
	ui_fprintf(stdout, "Attaching remote machine\n");
      else
	ui_fprintf(stdout, "Attaching program: %s pid %d\n",
		exec_file, pid);
      ui_fflush (stdout);
    }

#ifdef ATTACH_DETACH
  if (remote)
    {
#endif
      remote_open (args, from_tty);
      start_remote ();  
#ifdef ATTACH_DETACH
    }
  else
    attach_program (pid);
#endif
}

/*
 * detach_command --
 * takes a program previously attached to and detaches it.
 * The program resumes execution and will no longer stop
 * on signals, etc.  We better not have left any breakpoints
 * in the program or it'll die when it hits one.  For this
 * to work, it may be necessary for the process to have been
 * previously attached.  It *might* work if the program was
 * started via the normal ptrace (PTRACE_TRACEME).
 */

static void
detach_command (args, from_tty)
     char *args;
     int from_tty;
{
  int signal = 0;

#ifdef TEK_HACK
  extern simulator;
  if (simulator) {
    simulator = 0;
    remote_debugging = 0;
    if (from_tty) {
      ui_fprintf(stdout, "Detached from simulator\n");
    }
    return;
  } else if (remote_debugging) {
    remote_close (from_tty);
    remote_debugging = 0;
    if (from_tty) {
      ui_fprintf(stdout, "Detached from target\n");
    }
    return;
  }
#endif

#ifdef ATTACH_DETACH
  if (inferior_pid)
    {
      if (from_tty)
	{
	  char *exec_file = (char *)get_exec_file (0);
	  if (exec_file == 0)
	    exec_file = "";
	  ui_fprintf(stdout, "Detaching program: %s pid %d\n",
		  exec_file, inferior_pid);
	  ui_fflush (stdout);
	}
      if (args)
	signal = atoi (args);
      
      detach (signal);
      inferior_pid = 0;
    }
  else
#endif
    {
      if (!remote_debugging)
	ui_badnews(-1,"Not currently attached to subsidiary or remote process.");

      if (args)
	ui_badnews(-1,"Argument given to \"detach\" when remotely debugging.");
      
      remote_close (from_tty);
    }
}

/* ARGUSUED */
static void
float_info (addr_exp)
     char *addr_exp;
{
#ifdef FLOAT_INFO
	FLOAT_INFO;
#else
	ui_fprintf(stdout, "No floating point info available for this processor.\n");
#endif
}

extern struct cmd_list_element *setlist, *deletelist;
#ifdef TEK_PROG_HACK
extern struct cmd_list_element *set_cmd, *gdbdelete_cmd;
#endif /* TEK_PROG_HACK */

void
_initialize_infcmd ()
{
#ifdef TEK_PROG_HACK
  struct cmd_list_element *new;
#endif /* TEK_PROG_HACK */
  add_com ("tty", class_run, tty_command,
	   "Set terminal for future runs of program being debugged.");

#ifdef TEK_PROG_HACK
  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("args", class_run, set_args_command,
	   "Specify arguments to give program being debugged when it is started.\n\
Follow this command with any number of args, to be passed to the program.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */

  add_info ("environment", environment_info,
	    "The environment to give the program, or one variable's value.\n\
With an argument VAR, prints the value of environment variable VAR to\n\
give the program being debugged.  With no arguments, prints the entire\n\
environment to be given to the program.");

#ifdef TEK_PROG_HACK
  new = 
#endif /* TEK_PROG_HACK */
  add_cmd ("environment", class_run, unset_environment_command,
	   "Cancel environment variable VAR for the program.\n\
This does not affect the program until the next \"run\" command.",
	   &deletelist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) gdbdelete_cmd;

  new =
#endif /* TEK_PROG_HACK */
  add_cmd ("environment", class_run, set_environment_command,
	   "Set environment variable value to give the program.\n\
Arguments are VAR VALUE where VAR is variable name and VALUE is value.\n\
VALUES of environment variables are uninterpreted strings.\n\
This does not affect the program until the next \"run\" command.",
	   &setlist);
#ifdef TEK_PROG_HACK
  if (new)
    new->aux = (char *) set_cmd;
#endif /* TEK_PROG_HACK */
 
#ifdef ATTACH_DETACH
 add_com ("attach", class_run, attach_command,
 	   "Attach to a process that was started up outside of GDB.\n\
This command may take as argument a process id or a device file.\n\
For a process id, you must have permission to send the process a signal,\n\
and it must have the same effective uid as the debugger.\n\
For a device file, the file must be a connection to a remote debug server.\n\n\
Before using \"attach\", you must use the \"exec-file\" command\n\
to specify the program running in the process,\n\
and the \"symbol-file\" command to load its symbol table.");
#else
 add_com ("attach", class_run, attach_command,
 	   "Attach to a process that was started up outside of GDB.\n\
This commands takes as an argument the name of a device file.\n\
This file must be a connection to a remote debug server.\n\n\
Before using \"attach\", you must use the \"exec-file\" command\n\
to specify the program running in the process,\n\
and the \"symbol-file\" command to load its symbol table.");
#endif
  add_com ("detach", class_run, detach_command,
	   "Detach the process previously attached.\n\
The process is no longer traced and continues its execution.");

  add_com ("signal", class_run, signal_command,
	   "Continue program giving it signal number SIGNUMBER.");

  add_com ("stepi", class_run, stepi_command,
	   "Step one instruction exactly.\n\
Argument N means do this N times (or till program stops for another reason).");
  add_com_alias ("si", "stepi", class_alias, 0);

  add_com ("nexti", class_run, nexti_command,
	   "Step one instruction, but proceed through subroutine calls.\n\
Argument N means do this N times (or till program stops for another reason).");
  add_com_alias ("ni", "nexti", class_alias, 0);

#ifdef TEK_HACK
  add_com ("complete", class_run, complete_command,
	   "Execute until selected stack frame returns, ignoring breakpoints.\n\
Upon return, the value returned is printed and put in the value history.");
  add_com ("rcomplete", class_run, rcomplete_command,
	   "Execute until selected stack frame returns, ignoring breakpoints.\n\
Upon return, the value returned is printed and put in the value history.\n\
Execution then continues.");
  add_com ("rfinish", class_run, rfinish_command,
	   "Execute until selected stack frame returns or a breakpoint is hit.\n\
If the former, the value returned is printed and put in the value history\n\
and execution continues.");
  add_com ("finish", class_run, finish_command,
	   "Execute until selected stack frame returns or a breakpoint is hit.\n\
If the former, the value returned is printed and put in the value history.");
#endif /*TEK_HACK */
  add_com ("finish", class_run, finish_command,
	   "Execute until selected stack frame returns.\n\
Upon return, the value returned is printed and put in the value history.");

  add_com ("next", class_run, next_command,
	   "Step program, proceeding through subroutine calls.\n\
Like the \"step\" command as long as subroutine calls do not happen;\n\
when they do, the call is treated as one instruction.\n\
Argument N means do this N times (or till program stops for another reason).");
  add_com_alias ("n", "next", class_run, 1);

  add_com ("step", class_run, step_command,
	   "Step program until it reaches a different source line.\n\
Argument N means do this N times (or till program stops for another reason).");
  add_com_alias ("s", "step", class_run, 1);

  add_com ("until", class_run, until_command,
	   "Execute until the program reaches a source line greater than the current\n\
or a specified line or address or function (same args as break command).\n\
Execution will also stop upon exit from the current stack frame.");
  add_com_alias ("u", "until", class_run, 1);
  
  add_com ("jump", class_run, jump_command,
	   "Continue program being debugged at specified line or address.\n\
Give as argument either LINENUM or *ADDR, where ADDR is an expression\n\
for an address to start at.");

  add_com ("cont", class_run, cont_command,
"Continue program being debugged, after signal or breakpoint or watchpoint.\n\
If proceeding from breakpoint or watchpoint, a number N may be used as an\n\
argument: then the same breakpoint or watchpoint won't break until the Nth\n\
time it is reached.");
  add_com_alias ("c", "cont", class_run, 1);

  add_com ("run", class_run, run_command,
	   "Start debugged program.  You may specify arguments to give it.\n\
Args may include \"*\", or \"[...]\"; they are expanded using \"sh\".\n\
Input and output redirection with \">\", \"<\", or \">>\" are also allowed.\n\n\
With no arguments, uses arguments last specified (with \"run\" or \"set args\".\n\
To cancel previous arguments and run with no arguments,\n\
use \"set args\" without arguments.");
  add_com_alias ("r", "run", class_run, 1);

  add_info ("registers", registers_info,
	    "List of registers and their contents, for selected stack frame.\n\
Register name as argument means describe only that register.");

#ifdef TEK_HACK
  /* I keep typing 'info regs' -- this will make me happy.
     -=- andrew@frip.wv.tek.com */
  add_info_alias ("r", "registers", 1);
  add_info_alias ("re", "registers", 1);
  add_info_alias ("reg", "registers", 1);
  add_info_alias ("regs", "registers", 1);
#endif

  add_info ("program", program_info,
	    "Execution status of the program.");

  add_info ("float", float_info,
	    "Print the status of the floating point unit\n");

  inferior_args = savestring (" ", 1); /* By default, no args.  */
  inferior_environ = make_environ ();
  init_environ (inferior_environ);
}

