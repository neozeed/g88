/* Start and stop the inferior process, for GDB.
   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/infrun.c,v 1.48 90/12/10 21:23:38 robertb Exp $
   $Locker:  $

 */

/* Notes on the algorithm used in wait_for_inferior to determine if we
   just did a subroutine call when stepping.  We have the following
   information at that point:

                  Current and previous (just before this step) pc.
		  Current and previous sp.
		  Current and previous start of current function.

   If the start's of the functions don't match, then

   	a) We did a subroutine call.

   In this case, the pc will be at the beginning of a function.

	b) We did a subroutine return.

   Otherwise.

	c) We did a longjmp.

   If we did a longjump, we were doing "nexti", since a next would
   have attempted to skip over the assembly language routine in which
   the longjmp is coded and would have simply been the equivalent of a
   continue.  I consider this ok behaivior.  We'd like one of two
   things to happen if we are doing a nexti through the longjmp()
   routine: 1) It behaves as a stepi, or 2) It acts like a continue as
   above.  Given that this is a special case, and that anybody who
   thinks that the concept of sub calls is meaningful in the context
   of a longjmp, I'll take either one.  Let's see what happens.  

   Acts like a subroutine return.  I can handle that with no problem
   at all.

   -->So: If the current and previous beginnings of the current
   function don't match, *and* the pc is at the start of a function,
   we've done a subroutine call.  If the pc is not at the start of a
   function, we *didn't* do a subroutine call.  

   -->If the beginnings of the current and previous function do match,
   either: 

   	a) We just did a recursive call.

	   In this case, we would be at the very beginning of a
	   function and 1) it will have a prologue (don't jump to
	   before prologue, or 2) (we assume here that it doesn't have
	   a prologue) there will have been a change in the stack
	   pointer over the last instruction.  (Ie. it's got to put
	   the saved pc somewhere.  The stack is the usual place.  In
	   a recursive call a register is only an option if there's a
	   prologue to do something with it.  This is even true on
	   register window machines; the prologue sets up the new
	   window.  It might not be true on a register window machine
	   where the call instruction moved the register window
	   itself.  Hmmm.  One would hope that the stack pointer would
	   also change.  If it doesn't, somebody send me a note, and
	   I'll work out a more general theory.
	   randy@wheaties.ai.mit.edu).  This is true (albeit slipperly
	   so) on all machines I'm aware of:

	      m68k:	Call changes stack pointer.  Regular jumps don't.

	      sparc:	Recursive calls must have frames and therefor,
	                prologues.

	      vax:	All calls have frames and hence change the
	                stack pointer.

	b) We did a return from a recursive call.  I don't see that we
	   have either the ability or the need to distinguish this
	   from an ordinary jump.  The stack frame will be printed
	   when and if the frame pointer changes; if we are in a
	   function without a frame pointer, it's the users own
	   lookout.

	c) We did a jump within a function.  We assume that this is
	   true if we didn't do a recursive call.

	d) We are in no-man's land ("I see no symbols here").  We
	   don't worry about this; it will make calls look like simple
	   jumps (and the stack frames will be printed when the frame
	   pointer moves), which is a reasonably non-violent response.

#if 0
    We skip this; it causes more problems than it's worth.
#ifdef SUN4_COMPILER_FEATURE
    We do a special ifdef for the sun 4, forcing it to single step
  into calls which don't have prologues.  This means that we can't
  nexti over leaf nodes, we can probably next over them (since they
  won't have debugging symbols, usually), and we can next out of
  functions returning structures (with a "call .stret4" at the end).
#endif
#endif
*/
   

   
   

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "wait.h"
#include "ui.h"

#include <stdio.h>
#include <signal.h>

#ifdef _WIN32
#undef TEK_HACK
#endif

#ifdef TEK_HACK
#include <sys/types.h>
#include <sys/ptrace.h>
#endif /* TEK_HACK */

/* unistd.h is needed to #define X_OK */
#ifdef USG
#include <unistd.h>
#else
#include <sys/file.h>
#endif

#ifdef UMAX_PTRACE
#include <aouthdr.h>
#include <sys/param.h>
#include <sys/ptrace.h>
#endif /* UMAX_PTRACE */

//extern char *sys_siglist[];
extern int errno;

#ifdef TEK_HACK

#define COMM_OFFSET \
	((int)((struct ptrace_user *)0)->pt_comm)
#define SIG_SOURCE_OFFSET \
	((int)&((struct ptrace_user *)0)->pt_sigframe.sig_source)
#define SIG_NUM_OFFSET \
	((int)&((struct ptrace_user *)0)->pt_sigframe.sig_num)
#define SIG_NOEXBL_OFFSET \
	((int)&((struct ptrace_user *)0)->pt_sigframe.sig_noexbl)
#define SIG_EXBL_OFFSET \
	((int)((struct ptrace_user *)0)->pt_sigframe.sig_exbl)

#endif /* TEK_HACK */

/* Tables of how to react to signals; the user sets them.  */

static char signal_stop[NSIG];
static char signal_print[NSIG];
static char signal_program[NSIG];

/* Number of watchpoint we stopped at, or 0 if none. */

extern int stop_watchpoint;

/* Nonzero if commands for watchpoint we stopped at begin "silent". */

extern int silent_watchpoint;

/* Nonzero if the last watchpoint hit was precise (the PSR serialization bit
   was on). */

extern int last_exception_was_precise;

/* Nonzero if breakpoints are now inserted in the inferior.  */

static int breakpoints_inserted;

/* Function inferior was in as of last step command.  */

static struct symbol *step_start_function;

/* This is the sequence of bytes we insert for a breakpoint.  */

static char break_insn[] = BREAKPOINT;
static char remote_break_insn[] = REMOTE_BREAKPOINT;

/* Nonzero => address for special breakpoint for resuming stepping.  */

static CORE_ADDR step_resume_break_address;

/* Original contents of the byte where the special breakpoint is.  */

static char step_resume_break_shadow[sizeof break_insn];

/* Nonzero means the special breakpoint is a duplicate
   so it has not itself been inserted.  */

static int step_resume_break_duplicate;

/* Nonzero if we are expecting a trace trap and should proceed from it.
   2 means expecting 2 trace traps and should continue both times.
   That occurs when we tell sh to exec the program: we will get
   a trap after the exec of sh and a second when the program is exec'd.  */

static int trap_expected;

/* Nonzero if the next time we try to continue the inferior, it will
   step one instruction and generate a spurious trace trap.
   This is used to compensate for a bug in HP-UX.  */

static int trap_expected_after_continue;

/* Nonzero means expecting a trace trap
   and should stop the inferior and return silently when it happens.  */

int stop_after_trap;

/* Nonzero means expecting a trace trap due to attaching to a process.  */

int stop_after_attach;

static CORE_ADDR stop_func_start;

static int stop_step_resume_break;

static int remove_breakpoints_on_following_step;

/* Nonzero if pc has been changed by the debugger
   since the inferior stopped.  */

int pc_changed;

/* Nonzero if debugging a remote machine via a serial link or ethernet.  */

int remote_debugging;

/* Save register contents here when about to pop a stack dummy frame.  */

char stop_registers[GENERAL_REGISTER_BYTES];

/* Nonzero if program stopped due to error trying to insert breakpoints.  */

static int breakpoints_failed;

/* Nonzero if inferior is in sh before our program got exec'd.  */

static int running_in_shell;

/* Nonzero after stop if current stack frame should be printed.  */

static int stop_print_frame;

#ifdef NO_SINGLE_STEP
extern int one_stepped;		/* From machine dependent code */
extern void single_step ();	/* Same. */
#endif /* NO_SINGLE_STEP */

static void insert_step_breakpoint ();
void remove_step_breakpoint ();
static void wait_for_inferior ();
static void normal_stop ();


/* Clear out all variables saying what to do when inferior is continued.
   First do this, then set the ones you want, then call `proceed'.  */

void
clear_proceed_status ()
{
  trap_expected = 0;
  step_range_start = 0;
  step_range_end = 0;
  step_frame_address = 0;
  step_over_calls = -1;
  step_resume_break_address = 0;
  stop_after_trap = 0;
  stop_after_attach = 0;

  /* Discard any remaining commands left by breakpoint we had stopped at.  */
  clear_breakpoint_commands ();
}

/* These variables used to be in wait_for_inferior(), but they're not set
   by resume(). */
static CORE_ADDR prev_sp;
static CORE_ADDR prev_func_start;
static char *prev_func_name;
static int prev_pc;

/*
 * resume -- continue execution of the inferior process.
 * If "step" is nonzero, the process is single-stepped.
 * If "signal" is nonzero, the process is continued with that signal.
 * This used to live in m88k-dep.c, but is moved here to get to the prev_*
 * variables.
 */
resume (step, signal)
  int step;
  int signal;
{
  /* This used to happen in wait_for_inferior, but that assumed that register
     values are available when the inferior is running, which is no longer
     true.  These variables will not always be set correctly, but the situation
     is no worse than in the past. */
  prev_pc = read_pc ();
  (void) find_pc_partial_function (prev_pc, &prev_func_name,
				   &prev_func_start);
  prev_func_start += FUNCTION_START_OFFSET;
  prev_sp = read_register (SP_REGNUM);

#ifdef	TEK_HACK
  if (!step) {
    extern last_iword;
    last_iword = 0;	/* For 'print_effect()' in remote.c  */
  }
#endif /* -rcb 4/90 */

  if (remote_debugging)
    remote_resume (step, signal);
  else
    /* First arg 9 means single step, 7 means just continue.
       Third arg = 1 means continue at already stored (SNIP,SFIP). */
    call_ptrace (step ? 9 : 7, inferior_pid, 1, signal);

  /* We no longer know what the inferior's registers are. */
  forget_registers();
}

/* Basic routine for continuing the program in various fashions.

   ADDR is the address to resume at, or -1 for resume where stopped.
   SIGNAL is the signal to give it, or 0 for none,
     or -1 for act according to how it stopped.
   STEP is nonzero if should trap after one instruction.
     -1 means return after that and print nothing.
     You should probably set various step_... variables
     before calling here, if you are stepping.

   You should call clear_proceed_status before calling proceed.  */

void
proceed (addr, signal, step)
     CORE_ADDR addr;
     int signal;
     int step;
{
  int oneproc = 0;
  extern int last_iword;	/* -rcb 4/23/90 */
  last_iword = 0;		/* -rcb 4/23/90 */

  if (step > 0) {
    CORE_ADDR pc = read_pc();
    if (addr != -1) {
      ui_badnews(-1, "proceed(addr=0x%x, signal=%d, step=%d), addr != -1\n", 
                              addr, signal, step);
    }
    step_start_function = find_pc_function (pc);

#ifdef TEK_HACK	/* -rcb 4/23/90 */
    if (step_range_start == step_range_end) {
      decode_instr_before_stepping(fetch_instruction(pc));  
    }
#endif

  } 
  if (step < 0)
    stop_after_trap = 1;

  if (addr == -1)
    {
      /* If there is a breakpoint at the address we will resume at,
	 step one instruction before inserting breakpoints
	 so that we do not stop right away.  */

      if (!pc_changed && breakpoint_here_p (read_pc ()))
	oneproc = 1;
    }
  else
    {
      extern simulator;
      if (!simulator) {
        write_register (SNIP_REGNUM, addr | IP_VALID);
        write_register (SFIP_REGNUM, (addr+4) | IP_VALID);
      }
    }

  if (trap_expected_after_continue)
    {
      /* If (step == 0), a trap will be automatically generated after
	 the first instruction is executed.  Force step one
	 instruction to clear this condition.  This should not occur
	 if step is nonzero, but it is harmless in that case.  */
      oneproc = 1;
      trap_expected_after_continue = 0;
    }

  if (oneproc)
    /* We will get a trace trap after one instruction.
       Continue it automatically and insert breakpoints then.  */
    trap_expected = 1;
  else
    {
      int temp = insert_breakpoints ();
      if (temp)
	{
	  print_sys_errmsg ("ptrace", temp);
	  ui_badnews(-1,"Cannot insert breakpoints.\n\
The same program may be running in another process.");
	}
      breakpoints_inserted = 1;

#ifdef TEK_HACK
      insert_watchpoints();
#endif /* TEK_HACK */

    }

  /* Install inferior's terminal modes.  */
  terminal_inferior ();

  if (signal >= 0)
    stop_signal = signal;
  /* If this signal should not be seen by program,
     give it zero.  Used for debugging signals.  */
  else if (stop_signal < NSIG && !signal_program[stop_signal])
    stop_signal= 0;

  /* Resume inferior.  */
  resume (oneproc || step, stop_signal);

  /* Wait for it to stop (if not standalone)
     and in any case decode why it stopped, and act accordingly.  */

  wait_for_inferior ();
  normal_stop ();
}

/* Start an inferior process for the first time.
   Actually it was started by the fork that created it,
   but it will have stopped one instruction after execing sh.
   Here we must get it up to actual execution of the real program.  
   When cross-debugging we don't execute any code in the target
   until the user tells us to -rcb */

void
start_inferior ()
{
  /* We will get a trace trap after one instruction.
     Continue it automatically.  Eventually (after shell does an exec)
     it will get another trace trap.  Then insert breakpoints and continue.  */

#ifdef START_INFERIOR_TRAPS_EXPECTED
  trap_expected = START_INFERIOR_TRAPS_EXPECTED;
#else
  trap_expected = 2;
#endif

  running_in_shell = 0;		/* Set to 1 at first SIGTRAP, 0 at second.  */
  trap_expected_after_continue = 0;
  breakpoints_inserted = 0;
  mark_breakpoints_out ();

#ifdef TEK_HACK
  mark_watchpoints_out();
#endif

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  terminal_init_inferior ();

  /* Install inferior's terminal modes.  */
  terminal_inferior ();

  if (remote_debugging)
    {
      trap_expected = 0;
      inferior_pid = 3;
      fetch_inferior_registers();
      set_current_frame (create_new_frame (read_register (SYNTH_FP_REGNUM),
					   read_pc ()));
      stop_frame_address = FRAME_FP (get_current_frame());

#ifdef NOT_DEF
 /* This code would single step the target when first connecting to it.
    This is undesirable in the Tektronix cross-debugging environment. */

      if (insert_breakpoints())
	ui_badnews(1,"Can't insert breakpoints");
      breakpoints_inserted = 1;
      proceed(-1, -1, 0);
#endif
    }
  else
    {
      wait_for_inferior ();
      normal_stop ();
    }
}

/* Start remote-debugging of a machine over a serial link.  */

void
start_remote ()
{
  clear_proceed_status ();
  running_in_shell = 0;
  trap_expected = 0;
  inferior_pid = 3;
  breakpoints_inserted = 0;
  mark_breakpoints_out ();
/* wait_for_inferior ();
  normal_stop();
*/
}

#ifdef ATTACH_DETACH

/* Attach to process PID, then initialize for debugging it
   and wait for the trace-trap that results from attaching.  */

void
attach_program (pid)
     int pid;
{
  attach (pid);
  inferior_pid = pid;

  mark_breakpoints_out ();

#ifdef TEK_HACK
  mark_watchpoints_out();
  ui_strtSubProc(pid);
#endif

  terminal_init_inferior ();
  clear_proceed_status ();
  stop_after_attach = 1;
  /*proceed (-1, 0, -2);*/
  terminal_inferior ();
  wait_for_inferior ();
  normal_stop ();
}
#endif /* ATTACH_DETACH */

#define BREAK 1
#define CONTINUE 2
#define FALL_THROUGH 3

/* This used to be part of wait_for_inferior().  This is called just
   after the call to wait() has returned.  We check to see if the
   process is still alive and print messages and return 'BREAK' if it isn't.
   -rcb 3/90
 */

static int check_for_exited_or_stopped_process(w)
  WAITTYPE w;
{
  /* See if the process still exists; clean up if it doesn't.  */
  if (WIFEXITED (w))
    {
      terminal_ours_for_output ();
      if (WRETCODE (w))
          ui_fprintf(stdout, "\nProgram exited with code 0%o.\n", WRETCODE (w));
        else 
          ui_fprintf(stdout, "\nProgram exited normally.\n");

      ui_endSubProc();
      ui_fflush (stdout);
      inferior_died ();
#ifdef NO_SINGLE_STEP
      one_stepped = 0;
#endif
      stop_print_frame = 0;
      return BREAK;
    }
  else if (!WIFSTOPPED (w))
    {
      kill_inferior ();
      stop_print_frame = 0;
      stop_signal = WTERMSIG (w);
      terminal_ours_for_output ();
#ifndef _WIN32
      ui_fprintf(stdout, "\nProgram terminated with signal %d, %s\n",
		  stop_signal,
		  stop_signal < NSIG
		  ? sys_siglist[stop_signal]
		  : "(undocumented)");
#endif
      ui_fprintf(stdout, "The inferior process no longer exists.\n");
      ui_fflush (stdout);
#ifdef NO_SINGLE_STEP
      one_stepped = 0;
#endif
      return BREAK;
    }
      
#ifdef NO_SINGLE_STEP
    if (one_stepped)
      single_step (0);	/* This actually cleans up the ss */
#endif /* NO_SINGLE_STEP */
    return CONTINUE;
}
      
/* This used to be part of wait_for_inferior().  What we do here is
   not easy to describe.  We do a lot of checking of signals that
   the target or process got and we handle them.
   -rcb 3/90
 */
static  int random_signal;

static int preliminary_trap_handler(stop_sp, text_end)
  CORE_ADDR stop_sp;
  CORE_ADDR text_end;
{
      /* First, distinguish signals caused by the debugger from signals
	 that have to do with the program's own actions.
	 Note that breakpoint insns may cause SIGTRAP or SIGILL
	 or SIGEMT, depending on the operating system version.
	 Here we detect when a SIGILL or SIGEMT is really a breakpoint
	 and change it to SIGTRAP.  */
      
      if (stop_signal == SIGTRAP
	  || (breakpoints_inserted &&
	      (stop_signal == SIGILL
#ifdef	SIGEMT
	       || stop_signal == SIGEMT
#endif
		))
	  || stop_after_attach)
        {
	  if (stop_signal == SIGTRAP && stop_after_trap)
	    {
	      stop_print_frame = 0;
	      return BREAK;
	    }
	  if (stop_after_attach)
	    return BREAK;
	  /* Don't even think about breakpoints
	     if still running the shell that will exec the program
	     or if just proceeded over a breakpoint.  */
	  if (stop_signal == SIGTRAP && trap_expected)
	    stop_breakpoint = 0;
	  else
	    {

#ifdef TEK_HACK
#ifdef NOTDEF /* -rcb 6/90 */
	      /* If the stop signal was SIGTRAP, ask the kernel why it stopped.
		 The BCS specifies these signal codes for SIGTRAP:
		   TRAP_FORK -- ptrace-130 child fork complete
		   TRAP_EXEC -- exec complete
		   504-511 -- that trap instruction was executed
		 In addition, Tektronix adds these signal codes:
		   TRAP_WPUSER -- user-mode watchpoint trap.
		   TRAP_WPSYS -- kernel-mode watchpoint trap. */
	      if (stop_signal == SIGTRAP && !remote_debugging)
		{
		  /* Check that the signal frame looks good.
		     When we trust the kernel, this can go away. */
		  if (call_ptrace(3, inferior_pid, SIG_SOURCE_OFFSET, 0) != 1 ||
		      call_ptrace(3, inferior_pid, SIG_NUM_OFFSET, 0) != SIGTRAP ||
		      call_ptrace(3, inferior_pid, SIG_NOEXBL_OFFSET, 0) != 1 ||
		      call_ptrace(3, inferior_pid, SIG_EXBL_OFFSET, 0) != 1)
		    {
		      ui_fprintf(stdout,
			"\nINTERNAL ERROR -- invalid signal frame.\n"
				);
		    }
		  else
		    {
		      /* Get the signal code. */
		      int code = call_ptrace(3, inferior_pid,
					SIG_EXBL_OFFSET+sizeof(long), 0);

		      /* Some versions of the kernel store this value in
			 an (unsigned short), so coerce it back to negative
			 if appropriate. */
		      if (code>0 && code & 0x8000)
			code -= 0x10000;

		      if (code == TRAP_EXEC)
			{
			  /* Inferior did an exec. */
			  register int i, w;
			  auto int comm[65];

			  /* Fetch the command. */
			  for (i=0; i<64; ++i)
			    {
			      /* Fetch the next word of the command. */
			      w = call_ptrace(3, inferior_pid,
					 COMM_OFFSET+i*sizeof(long), 0);
			      comm[i] = w;

			      /* Exit loop if we've got the null byte. */
			      if (((w-0x01010101)&~w)&0x80808080)
			        break;
			    }
			  comm[64] = 0;		/* just in case */

			  ui_fprintf(stdout,
"\nProgram executed command \"%s\".\n\
You can use the \"exec_file\" and \"symbol-file\" commands to tell gdb where\n\
to find the new executable file.\n\
(The old program is gone, so don't try to examine your variables now.)\n\
Type \"continue\" to begin execution of the new program.\n",
				     comm);
			  return BREAK;
			}
		      else if (code == TRAP_FORK)
			{
			  /* Inferior was a ptrace-130'd child.
			     We should never see this. */
			  ui_fprintf(stdout,
"\nProgram took SIGTRAP due to child fork -- this should never happen!\n"
					);
			  return BREAK;
			}
		      else if (code == TRAP_WPUSER)
			{
			  /* Inferior hit a watchpoint in user mode. */
			  if (user_watchpoint_hit())
			    {
			      return BREAK;
			    }
			  resume (0, 0);
			  return CONTINUE;
			}
		      else if (code == TRAP_WPSYS)
			{
			  /* Inferior hit a watchpoint in kernel mode. */
			  if (kernel_watchpoint_hit())
			    {
			      return BREAK;
			    }
			  resume (0, 0);
			  return CONTINUE;
			}
		    }
		}
#endif /* -rcb 6/90 */
#endif /* TEK_HACK */

	      /* See if there is a breakpoint at the current PC.  */
#if DECR_PC_AFTER_BREAK
	      /* Notice the case of stepping through a jump
		 that leads just after a breakpoint.
		 Don't confuse that with hitting the breakpoint.
		 What we check for is that 1) stepping is going on
		 and 2) the pc before the last insn does not match
		 the address of the breakpoint before the current pc.  */
	      if (!(prev_pc != stop_pc - DECR_PC_AFTER_BREAK
		    && step_range_end && !step_resume_break_address))
#endif /* DECR_PC_AFTER_BREAK not zero */
		{
		  /* See if we stopped at the special breakpoint for
		     stepping over a subroutine call.  */
		  if (stop_pc - DECR_PC_AFTER_BREAK
		      == step_resume_break_address)
		    {
		      stop_step_resume_break = 1;
		      if (DECR_PC_AFTER_BREAK)
			{
			  stop_pc -= DECR_PC_AFTER_BREAK;
#ifndef TEK_HACK
#ifndef _WIN32
			  write_register (PC_REGNUM, stop_pc);	/*** never! ***/
#endif
#endif
			  pc_changed = 0;
			}
		    }
		  else
		    {
		      stop_breakpoint =
			breakpoint_stop_status (stop_pc, stop_frame_address);
		      /* Following in case break condition called a
			 function.  */
		      stop_print_frame = 1;
#if defined (TEK_HACK) && (defined (m88k) || defined (__m88k__))
		      if (stop_breakpoint)
			{
			  pc_changed = 0;
			}
#else /* not TEK_HACK or not 88k */
		      if (stop_breakpoint && DECR_PC_AFTER_BREAK )
			{
			  stop_pc -= DECR_PC_AFTER_BREAK;
#ifndef _WIN32
			  write_register (PC_REGNUM, stop_pc);	/*** never! ***/
#endif
#ifdef NPC_REGNUM
			  write_register (NPC_REGNUM, stop_pc + 4);
#endif
			  pc_changed = 0;
			}
#endif /* not TEK_HACK or not 88k */
		    }
		}
	    }
	  
          /* Figure out whether we were stopped by a signal that we
             were expecting or not. Set 'random_signal' to 1 if were
             not expecting to be stopped the way we did. */
           
	  if (stop_signal == SIGTRAP)
	    random_signal
	      = !(stop_breakpoint || trap_expected
		  || stop_step_resume_break
#ifndef CANNOT_EXECUTE_STACK
#ifdef TEK_HACK
                  || (stop_sp INNER_THAN stop_pc && m88k_is_dummy_active() &&
		       (stop_pc INNER_THAN stop_frame_address || 
                        stop_frame_address == NULL_CORE_ADDR))
#else
		  || (stop_sp INNER_THAN stop_pc
		      && stop_pc INNER_THAN stop_frame_address)
#endif
#else
		  || stop_pc == text_end - 2
#endif
		  || (step_range_end && !step_resume_break_address));
	  else
	    {
	      random_signal
		= !(stop_breakpoint
		    || stop_step_resume_break
#ifdef news800
#ifdef TEK_HACK
                    || (stop_sp INNER_THAN stop_pc) &&
		          (stop_pc INNER_THAN stop_frame_address || 
                           stop_frame_address == NULL_CORE_ADDR))
#else
		    || (stop_sp INNER_THAN stop_pc
			&& stop_pc INNER_THAN stop_frame_address)
#endif
#endif
		    
		    );
	      if (!random_signal)
		stop_signal = SIGTRAP;
	    }
	}
      else
	random_signal = 1;

  return FALL_THROUGH;
}
     
/* This used to be a part of wait_for_inferior().  
   For the program's own signals, act according to
   the signal handling tables.  */

static int handle_random_signal()
{
  if (!(running_in_shell && stop_signal == SIGSEGV))
	{
	  /* Signal not for debugging purposes.  */
	  int printed = 0;
	  
	  stopped_by_random_signal = 1;
	  
	  if ((stop_signal >= NSIG
	      || signal_print[stop_signal]) && !remote_debugging)
	    {
	      printed = 1;
	      terminal_ours_for_output ();
#ifndef _WIN32
	      ui_fprintf(stdout, "\nProgram received signal %d, %s\n",
		      stop_signal,
		      stop_signal < NSIG
		      ? sys_siglist[stop_signal]
		      : "(undocumented)");
#endif
	      ui_fflush (stdout);
	    }
	  if (stop_signal >= NSIG
	      || signal_stop[stop_signal])
	    return BREAK;
	  /* If not going to stop, give terminal back
	     if we took it away.  */
	  else if (printed)
	    terminal_inferior ();
	}
  return FALL_THROUGH;
}


/* This used to be a part of wait_for_inferior().  Here we do most
   of the stuff for breakpoints.

   rcb 3/90 */

static int another_trap;
static int handle_breakpoint()
{
  if (stop_breakpoint || stop_step_resume_break)
	{
	  /* Does a breakpoint want us to stop?  */
	  if (stop_breakpoint && stop_breakpoint != -1
	      && stop_breakpoint != -0x1000001)
	    {
	      /* 0x1000000 is set in stop_breakpoint as returned by
		 breakpoint_stop_status to indicate a silent
		 breakpoint.  */
	      if ((stop_breakpoint > 0 ? stop_breakpoint :
		   -stop_breakpoint)
		  & 0x1000000)
		{
		  stop_print_frame = 0;
		  if (stop_breakpoint > 0)
		    stop_breakpoint -= 0x1000000;
		  else
		    stop_breakpoint += 0x1000000;
		}
	      return BREAK;
	    }
	  /* But if we have hit the step-resumption breakpoint,
	     remove it.  It has done its job getting us here.
	     The sp test is to make sure that we don't get hung
	     up in recursive calls in functions without frame
	     pointers.  If the stack pointer isn't outside of
	     where the breakpoint was set (within a routine to be
	     stepped over), we're in the middle of a recursive
	     call. Not true for reg window machines (sparc)
	     because the must change frames to call things and
	     the stack pointer doesn't have to change if it
	     the bp was set in a routine without a frame (pc can
	     be stored in some other window).
	     
	     The removal of the sp test is to allow calls to
	     alloca.  Nasty things were happening.  Oh, well,
	     gdb can only handle one level deep of lack of
	     frame pointer. */
	  if (stop_step_resume_break
	      && (step_frame_address == 0
		  || (stop_frame_address == step_frame_address)))
	    {
	      remove_step_breakpoint ();
	      step_resume_break_address = 0;
	    }
	  /* Otherwise, must remove breakpoints and single-step
	     to get us past the one we hit.  */
	  else
	    {
	      remove_breakpoints ();
	      remove_step_breakpoint ();
	      breakpoints_inserted = 0;
	      another_trap = 1;
	    }
	  
	  /* We come here if we hit a breakpoint but should not
	     stop for it.  Possibly we also were stepping
	     and should stop for that.  So fall through and
	     test for stepping.  But, if not stepping,
	     do not stop.  */
	}
  return FALL_THROUGH;
}

/* This used to be a part of wait_for_inferior().  Most of the stuff
   for stepping the processor is done here.

   -rcb 3/90 */

static int handle_stepping(stop_sp, stop_func_name)
  CORE_ADDR stop_sp;
  char *stop_func_name;
{
  static CORE_ADDR prologue_pc;

      if (step_resume_break_address)
	/* Having a step-resume breakpoint overrides anything
	   else having to do with stepping commands until
	   that breakpoint is reached.  */
	;
      /* If stepping through a line, keep going if still within it.  */
      else if (!random_signal
	       && step_range_end
	       && stop_pc >= step_range_start
	       && stop_pc < step_range_end
	       /* The step range might include the start of the
		  function, so if we are at the start of the
		  step range and either the stack or frame pointers
		  just changed, we've stepped outside */
	       && !(stop_pc == step_range_start
		    && stop_frame_address
#ifdef TEK_HACK
                    && (stop_sp INNER_THAN prev_sp)))
#else
		    && (stop_sp INNER_THAN prev_sp
			|| stop_frame_address != step_frame_address)))
#endif
	{

#ifndef TEK_HACK
	  /* I don't like this behavior -- you have to issue two "step"
	     commands to get past a function epilog (the right curly brace
              line).
	     DG didn't see this because their ABOUT_TO_RETURN macro is
	     wrong!  -=- andrew@frip.wv.tek.com */

	  /* Don't step through the return from a function
	     unless that is the first instruction stepped through.  */
	  if (ABOUT_TO_RETURN (stop_pc))
	    {
	      stop_step = 1;
	      return BREAK;
	    }
#endif /* TEK_HACK */

	}
      
      /* We stepped out of the stepping range.  See if that was due
	 to a subroutine call that we should proceed to the end of.  */
      else if (!random_signal && step_range_end)
	{
	  if (stop_func_start)
	    {
	      prologue_pc = stop_func_start;
	    }

	  /* Did we just take a signal?  */
	  if (stop_func_name && prev_func_name &&
               (!strcmp ("_sigtramp", stop_func_name)
	      && strcmp ("_sigtramp", prev_func_name)))
	    {
	      /* We've just taken a signal; go until we are back to
		 the point where we took it and one more.  */
	      step_resume_break_address = prev_pc;
	      step_resume_break_duplicate =
		breakpoint_here_p (step_resume_break_address);
	      if (breakpoints_inserted)
		insert_step_breakpoint ();
	      /* Make sure that the stepping range gets us past
		 that instruction.  */
	      if (step_range_end == 1)
		step_range_end = (step_range_start = prev_pc) + 1;
	      remove_breakpoints_on_following_step = 1;
	    }

	  /* ==> See comments at top of file on this algorithm.  <==*/
	  
	  else if (stop_pc == stop_func_start
	      && (stop_func_start != prev_func_start
		  || prologue_pc != stop_func_start
		  || stop_sp != prev_sp))
	    {
	      /* It's a subroutine call */
	      if (step_over_calls > 0 
		  || (step_over_calls &&  find_pc_function (stop_pc) == 0))
		{
		  /* A subroutine call has happened.  */
		  /* Set a special breakpoint after the return */
		  step_resume_break_address =
		    SAVED_PC_AFTER_CALL (get_current_frame ())
#ifdef	DG_HACK
#ifdef  m88k
					& 0xfffffffd
#endif
#endif /* DG_HACK */
							      ;
		  step_resume_break_duplicate
		    = breakpoint_here_p (step_resume_break_address);
		  if (breakpoints_inserted)
		    insert_step_breakpoint ();
		}
	      /* Subroutine call with source code we should not step over.
		 Do step to the first line of code in it.  */
	      else if (step_over_calls)
		{
                  struct symtab_and_line sal;

		  sal = find_pc_line (stop_func_start, 0);
		  /* Use the step_resume_break to step until
		     the end of the prologue, even if that involves jumps
		     (as it seems to on the vax under 4.2).  */
		  /* If the prologue ends in the middle of a source line,
		     continue to the end of that source line.
		     Otherwise, just go to end of prologue.  */
#ifdef PROLOGUE_FIRSTLINE_OVERLAP
		  /* no, don't either.  It skips any code that's
		     legitimately on the first line.  */
#else
		  if (sal.end && sal.pc != stop_func_start)
		    stop_func_start = sal.end;
#endif
		  
		  if (stop_func_start == stop_pc)
		    {
		      /* We are already there: stop now.  */
		      stop_step = 1;
		      return BREAK;
		    }
		  else
		    /* Put the step-breakpoint there and go until there. */
		    {
		      step_resume_break_address = stop_func_start;
		      
		      step_resume_break_duplicate
			= breakpoint_here_p (step_resume_break_address);
		      if (breakpoints_inserted)
			insert_step_breakpoint ();
		      /* Do not specify what the fp should be when we stop
			 since on some machines the prologue
			 is where the new fp value is established.  */
		      step_frame_address = 0;
		      /* And make sure stepping stops right away then.  */
		      step_range_end = step_range_start;
		    }
		}
	      else
		{
		  /* We get here only if step_over_calls is 0 and we
		     just stepped into a subroutine.  I presume
		     that step_over_calls is only 0 when we're
		     supposed to be stepping at the assembly
		     language level.*/
		  stop_step = 1;
		  return BREAK;
		}
	    }
	  /* No subroutince call; stop now.  */
	  else
	    {
	      stop_step = 1;
	      return BREAK;
	    }
	}
  return FALL_THROUGH;
}
      
/* We call this from wait_for_inferior() and from select_processor()
   to force gdb to look at the machine's pc to determine the current
   frame. -rcb 6/90 */
void set_first_frame()
{
  set_current_frame ( create_new_frame (read_register (SYNTH_FP_REGNUM),
					    read_pc ()));
}


/* Wait for control to return from inferior to debugger.
   If inferior gets a signal, we may decide to start it up again
   instead of returning.  That is why there is a loop in this function.
   When this function actually returns it means the inferior
   should be left stopped and GDB should read more commands.  */

static void
wait_for_inferior ()
{
  register int pid;
  WAITTYPE w;
  CORE_ADDR stop_sp;

  /* Give it an initial value in case find_pc_partial_function doesn't. -rcb*/
  char *stop_func_name = "";	

  extern CORE_ADDR text_end;
  int i;

  remove_breakpoints_on_following_step = 0;
  while (1)
    {
      /* Clean up saved state that will become invalid.  */
      pc_changed = 0;
      flush_cached_frames ();

      if (remote_debugging)
	remote_wait (&w);
      else
	{
	  pid = ui_wait (&w);
	  if (pid != inferior_pid)
	    continue;
	}

      /* If the process has stopped to exited, return to our caller */
      if (check_for_exited_or_stopped_process(w) == BREAK) {
          break;
      }
      
      fetch_inferior_registers ();
      stop_pc = read_pc ();
      set_first_frame();
      stop_frame_address = FRAME_FP (get_current_frame ());
      stop_sp = read_register (SP_REGNUM);
      stop_func_start = 0;
      /* Don't care about return value; stop_func_start will be 0
	 if it doesn't work.  */
      (void) find_pc_partial_function (stop_pc, &stop_func_name,
				       &stop_func_start);
      stop_func_start += FUNCTION_START_OFFSET;
      another_trap = 0;
      stop_breakpoint = 0;
      stop_watchpoint = 0;
      stop_step = 0;
      stop_stack_dummy = 0;
      stop_print_frame = 1;
      stop_step_resume_break = 0;
      random_signal = 0;
      stopped_by_random_signal = 0;
      breakpoints_failed = 0;
      
      /* Look at the cause of the stop, and decide what to do.
	 The alternatives are:
	 1) break; to really stop and return to the debugger,
	 2) drop through to start up again
	 (set another_trap to 1 to single step once)
	 3) set random_signal to 1, and the decision between 1 and 2
	 will be made according to the signal handling tables.  */
      
      stop_signal = WSTOPSIG (w);
      
      if ((i = preliminary_trap_handler(stop_sp, text_end)) == BREAK) {
          break;
      } else if (i == CONTINUE) {
          continue;
      }

      /* For the program's own signals, act according to
	 the signal handling tables.  */
      
      if (random_signal) {
        if (handle_random_signal() == BREAK) {
          break;
        }
      }

      /* Handle cases caused by hitting a breakpoint.  */
      
      if (!random_signal) {
        if (handle_breakpoint() == BREAK) {
          break;
        }
      }
      /* If this is the breakpoint at the end of a stack dummy,
	 just stop silently.  */
#ifndef CANNOT_EXECUTE_STACK
#ifdef TEK_HACK
      if (stop_sp INNER_THAN stop_pc && m88k_is_dummy_active() &&
	                (stop_pc INNER_THAN stop_frame_address || 
                         stop_frame_address == NULL_CORE_ADDR))
#else
      if (stop_sp INNER_THAN stop_pc
	  && stop_pc INNER_THAN stop_frame_address)
#endif
#else
	if (stop_pc == text_end - 2)
#endif
	  {
	    stop_print_frame = 0;
	    stop_stack_dummy = 1;
#ifdef HP_OS_BUG
	    trap_expected_after_continue = 1;
#endif
	    break;
	  }
      
      if (handle_stepping(stop_sp, stop_func_name) == BREAK) {
        break;
      }

      /* Save the pc before execution, to compare with pc after stop.  */
      prev_pc = read_pc ();	/* Might have been DECR_AFTER_BREAK */
      prev_func_start = stop_func_start; /* Ok, since if DECR_PC_AFTER
					  BREAK is defined, the
					  original pc would not have
					  been at the start of a
					  function. */
      prev_func_name = stop_func_name;
      prev_sp = stop_sp;

      /* If we did not do break;, it means we should keep
	 running the inferior and not return to debugger.  */

      /* If trap_expected is 2, it means continue once more
	 and insert breakpoints at the next trap.
	 If trap_expected is 1 and the signal was SIGSEGV, it means
	 the shell is doing some memory allocation--just resume it
	 with SIGSEGV.
	 Otherwise insert breakpoints now, and possibly single step.  */

      if (trap_expected > 1)
	{
	  trap_expected--;
	  running_in_shell = 1;
	  resume (0, 0);
	}
      else if (running_in_shell && stop_signal == SIGSEGV)
	{
	  resume (0, SIGSEGV);
	}
      else if (trap_expected && stop_signal != SIGTRAP)
	{
	  /* We took a signal which we are supposed to pass through to
	     the inferior and we haven't yet gotten our trap.  Simply
	     continue.  */
	  resume ((step_range_end && !step_resume_break_address)
		  || trap_expected,
		  stop_signal);
	}
      else
	{
	  /* Here, we are not awaiting another exec to get
	     the program we really want to debug.
	     Insert breakpoints now, unless we are trying
	     to one-proceed past a breakpoint.  */
	  running_in_shell = 0;

	  /* If we've just finished a special step resume and we don't
	     want to hit a breakpoint, pull em out.  */
	  if (!step_resume_break_address &&
	      remove_breakpoints_on_following_step)
	    {
	      remove_breakpoints_on_following_step = 0;
	      remove_breakpoints ();
	      breakpoints_inserted = 0;
	    }
	  else if (!breakpoints_inserted && !another_trap)
	    {
	      insert_step_breakpoint ();
	      breakpoints_failed = insert_breakpoints ();
	      if (breakpoints_failed)
		break;
	      breakpoints_inserted = 1;
	    }

#ifdef TEK_HACK
	  insert_watchpoints();
#endif /* TEK_HACK */

	  trap_expected = another_trap;

	  if (stop_signal == SIGTRAP)
	    stop_signal = 0;

	  resume ((step_range_end && !step_resume_break_address)
		  || trap_expected,
		  stop_signal);
	}
    }
}

/* Here to return control to GDB when the inferior stops for real.
   Print appropriate messages, remove breakpoints, give terminal our modes.

   RUNNING_IN_SHELL nonzero means the shell got a signal before
   exec'ing the program we wanted to run.
   STOP_PRINT_FRAME nonzero means print the executing frame
   (pc, function, args, file, line number and line text).
   BREAKPOINTS_FAILED nonzero means stop was due to error
   attempting to insert breakpoints.  */

static void
normal_stop ()
{
  /* Make sure that the current_frame's pc is correct.  This
     is a correction for setting up the frame info before doing
     DECR_PC_AFTER_BREAK */
  if (inferior_pid)
    (get_current_frame ())->pc = read_pc ();
  
#ifdef TEK_HACK
  remove_watchpoints();
#endif /* TEK_HACK */

  if (breakpoints_failed)
    {
      terminal_ours_for_output ();
      print_sys_errmsg ("ptrace", breakpoints_failed);
      ui_fprintf(stdout, "Stopped; cannot insert breakpoints.\n\
The same program may be running in another process.\n");
    }

  if (inferior_pid)
    remove_step_breakpoint ();

  if (inferior_pid && breakpoints_inserted)
    if (remove_breakpoints ())
      {
	terminal_ours_for_output ();
	ui_fprintf(stdout, "Cannot remove breakpoints because program is no longer writable.\n\
It must be running in another process.\n\
Further execution is probably impossible.\n");
      }

  breakpoints_inserted = 0;

  /* Delete the breakpoint we stopped at, if it wants to be deleted.
     Delete any breakpoint that is to be deleted at the next stop.  */

  breakpoint_auto_delete (stop_breakpoint);

  /* If an auto-display called a function and that got a signal,
     delete that auto-display to avoid an infinite recursion.  */

  if (stopped_by_random_signal)
    disable_current_display ();

  if (step_multi && stop_step)
    return;

  terminal_ours ();

  if (running_in_shell)
    {
      if (stop_signal == SIGSEGV)
	{
	  char *exec_file = (char *) get_exec_file (1);

#ifdef NOTDEF
	  if (access (exec_file, X_OK) != 0)
	    ui_fprintf(stdout, "The file \"%s\" is not executable.\n", exec_file);
	  else
#endif
	    ui_fprintf(stdout, "\
You have just encountered a bug in \"sh\".  GDB starts your program\n\
by running \"sh\" with a command to exec your program.\n\
This is so that \"sh\" will process wildcards and I/O redirection.\n\
This time, \"sh\" crashed.\n\
\n\
One known bug in \"sh\" bites when the environment takes up a lot of space.\n\
Try \"info env\" to see the environment; then use \"delete env\" to kill\n\
some variables whose values are large; then do \"run\" again.\n\
\n\
If that works, you might want to put those \"delete env\" commands\n\
into a \".gdbinit\" file in this directory so they will happen every time.\n");
	}
      /* Don't confuse user with his program's symbols on sh's data.  */
      stop_print_frame = 0;
    }

  if (inferior_pid == 0)
    return;

  /* Select innermost stack frame except on return from a stack dummy routine,
     or if the program has exited.  */
  if (!stop_stack_dummy)
    {
      select_frame (get_current_frame (), 0);

      if (stop_print_frame)
	{
	  if (stop_breakpoint > 0) { 
	    ui_fprintf(stdout, "\nBpt %d, ", stop_breakpoint);
          }

	  /*
	   * If we stopped for a precise watchpoint, try to display the
	   * program portion which caused the watchpoint hit.
	   * (If we stopped for an imprecise watchpoint, we don't know the
	   * location of the instruction(s) that caused the watchpoint hit(s),
	   * so there's no point in even trying.)
	   */
	  if (stop_watchpoint && last_exception_was_precise)
	    {
	      if (!silent_watchpoint)
		{
		  print_watchpoint_frame(read_register(SXIP_REGNUM));
		}
	    }
	  else
	    {
              print_sel_frame (stop_step
			   && step_frame_address == stop_frame_address
			   && step_start_function == find_pc_function (stop_pc));
	    }

	  /* Display the auto-display expressions.  */
	  do_displays ();
	}
    }

  /* Save the function value return registers
     We might be about to restore their previous contents.  */
  read_register_bytes (0, stop_registers, GENERAL_REGISTER_BYTES);

  if (stop_stack_dummy)
    {
      /* Pop the empty frame that contains the stack dummy.
         POP_FRAME ends with a setting of the current frame, so we
	 can use that next. */
      POP_FRAME;
      select_frame (get_current_frame (), 0);
    }
}

static void
insert_step_breakpoint ()
{
  if (step_resume_break_address && !step_resume_break_duplicate)
    {
      read_memory (step_resume_break_address,
		   step_resume_break_shadow, sizeof break_insn, M_NORMAL);
      write_memory (step_resume_break_address,
 		    remote_debugging ? remote_break_insn : break_insn, 
                    sizeof break_insn);
    }
}

void
remove_step_breakpoint ()
{
  if (step_resume_break_address && !step_resume_break_duplicate)
    write_memory (step_resume_break_address, step_resume_break_shadow,
		  sizeof break_insn);
}

/* Specify how various signals in the inferior should be handled.  */

static void
handle_command (args, from_tty)
     char *args;
     int from_tty;
{
  register char *p = args;
  int signum = 0;
  register int digits, wordlen;

  if (!args)
    error_no_arg ("signal to handle");

  while (*p)
    {
      /* Find the end of the next word in the args.  */
      for (wordlen = 0; p[wordlen] && p[wordlen] != ' ' && p[wordlen] != '\t';
	   wordlen++);
#ifdef TEK_PROG_HACK
      if (!signum) {
      /* Expect signal number if we don't have one */
#endif /* TEK_PROG_HACK */
          for (digits = 0; p[digits] >= '0' && p[digits] <= '9'; digits++);

      /* If it is all digits, it is signal number to operate on.  */
          if (digits == wordlen)
#ifndef TEK_PROG_HACK
	    {
#endif /* TEK_PROG_HACK */
	      signum = atoi (p);
#ifdef TEK_PROG_HACK
          else if (strncmp(p, "SIG", 3) == 0) {
      /* Look for SIG*** type signal name */
	      int n = 1;

#ifndef _WIN32
              while (n < NSIG) {
	       if (strlen(sys_siglist[n]) == wordlen &&
	        strncmp(p, sys_siglist[n], wordlen) == 0) {
	          signum = n;
		  break;
	       }
	       n++;
	      }
#endif
          }
#endif /* TEK_PROG_HACK */
	  if (signum <= 0 || signum >= NSIG)
	    {
	      p[wordlen] = '\0';
	      ui_badnews(-1,"Invalid signal %s given as argument to \"handle\".", p);
	    }
	  if (signum == SIGTRAP || signum == SIGINT)
	    {
	      if (!query ("Signal %d is used by the debugger.\nAre you sure you want to change it? ", signum))
		ui_badnews(-1,"Not confirmed.");
	    }
      } /* endif !signal */
      else if (signum == 0)
        ui_badnews(-1,"First argument is not a signal number.");

      /* Else, if already got a signal number, look for flag words
	 saying what to do for it.  */
      else if (!strncmp (p, "stop", wordlen))
	{
	  signal_stop[signum] = 1;
	  signal_print[signum] = 1;
	}
      else if (wordlen >= 2 && !strncmp (p, "print", wordlen))
	signal_print[signum] = 1;
      else if (wordlen >= 2 && !strncmp (p, "pass", wordlen))
	signal_program[signum] = 1;
      else if (!strncmp (p, "ignore", wordlen))
	signal_program[signum] = 0;
      else if (wordlen >= 3 && !strncmp (p, "nostop", wordlen))
	signal_stop[signum] = 0;
      else if (wordlen >= 4 && !strncmp (p, "noprint", wordlen))
	{
	  signal_print[signum] = 0;
	  signal_stop[signum] = 0;
	}
      else if (wordlen >= 4 && !strncmp (p, "nopass", wordlen))
	signal_program[signum] = 0;
      else if (wordlen >= 3 && !strncmp (p, "noignore", wordlen))
	signal_program[signum] = 1;
      /* Not a number and not a recognized flag word => complain.  */
      else
	{
	  p[wordlen] = 0;
	  ui_badnews(-1,"Unrecognized flag word: \"%s\".", p);
	}

      /* Find start of next word.  */
      p += wordlen;
      while (*p == ' ' || *p == '\t') p++;
    }

  if (from_tty)
    {
      /* Show the results.  */
      ui_fprintf(stdout, "Number\tStop\tPrint\tPass to program\tDescription\n");
      ui_fprintf(stdout, "%d\t", signum);
      ui_fprintf(stdout, "%s\t", signal_stop[signum] ? "Yes" : "No");
      ui_fprintf(stdout, "%s\t", signal_print[signum] ? "Yes" : "No");
      ui_fprintf(stdout, "%s\t\t", signal_program[signum] ? "Yes" : "No");
#ifndef _WIN32
      ui_fprintf(stdout, "%s\n", sys_siglist[signum]);
#endif
    }
}

/* Print current contents of the tables set by the handle command.  */

static void
signals_info (signum_exp)
     char *signum_exp;
{
  register int i;
  register char *heading =
    "Number\tStop\tPrint\tPass to program\tDescription\n";

  if (signum_exp)
    {
      i = parse_and_eval_address (signum_exp);
      if ((i < NSIG) && (i > 0))
      {
        printf_filtered (heading);
        printf_filtered ("%d\t", i);
        printf_filtered ("%s\t", signal_stop[i] ? "Yes" : "No");
        printf_filtered ("%s\t", signal_print[i] ? "Yes" : "No");
        printf_filtered ("%s\t\t", signal_program[i] ? "Yes" : "No");
#ifndef _WIN32
        printf_filtered ("%s\n", sys_siglist[i]);
#endif
        return;
      }
      else
      {
	char  bummer[128];
        sprintf(bummer,
	   "Invalid signal %d given as arguemnt to \"info signal\"", i);
        ui_badnews(-1, bummer);
      }
    }

  printf_filtered (heading);
  printf_filtered ("\n");
  for (i = 1; i < NSIG; i++)
    {
      QUIT;

      printf_filtered ("%d\t", i);
      printf_filtered ("%s\t", signal_stop[i] ? "Yes" : "No");
      printf_filtered ("%s\t", signal_print[i] ? "Yes" : "No");
      printf_filtered ("%s\t\t", signal_program[i] ? "Yes" : "No");
#ifndef _WIN32
      printf_filtered ("%s\n", sys_siglist[i]);
#endif
    }

  printf_filtered ("\nUse the \"handle\" command to change these tables.\n");
}

/* Save all of the information associated with the inferior<==>gdb
   connection.  INF_STATUS is a pointer to a "struct inferior_status"
   (defined in inferior.h).  */

struct command_line *get_breakpoint_commands ();

void
save_inferior_status (inf_status, restore_stack_info)
     struct inferior_status *inf_status;
     int restore_stack_info;
{
  inf_status->pc_changed = pc_changed;
  inf_status->stop_signal = stop_signal;
  inf_status->stop_pc = stop_pc;
  inf_status->stop_frame_address = stop_frame_address;
  inf_status->stop_breakpoint = stop_breakpoint;
  inf_status->stop_step = stop_step;
  inf_status->stop_stack_dummy = stop_stack_dummy;
  inf_status->stopped_by_random_signal = stopped_by_random_signal;
  inf_status->trap_expected = trap_expected;
  inf_status->step_range_start = step_range_start;
  inf_status->step_range_end = step_range_end;
  inf_status->step_frame_address = step_frame_address;
  inf_status->step_over_calls = step_over_calls;
  inf_status->step_resume_break_address = step_resume_break_address;
  inf_status->stop_after_trap = stop_after_trap;
  inf_status->stop_after_attach = stop_after_attach;
  inf_status->breakpoint_commands = get_breakpoint_commands ();
  inf_status->restore_stack_info = restore_stack_info;
  
  bcopy (stop_registers, inf_status->stop_registers, GENERAL_REGISTER_BYTES);
  
  record_selected_frame (&(inf_status->selected_frame_address),
			 &(inf_status->selected_level));
  return;
}

void
restore_inferior_status (inf_status)
     struct inferior_status *inf_status;
{
  FRAME fid;
  int level = inf_status->selected_level;

  pc_changed = inf_status->pc_changed;
  stop_signal = inf_status->stop_signal;
  stop_pc = inf_status->stop_pc;
  stop_frame_address = inf_status->stop_frame_address;
  stop_breakpoint = inf_status->stop_breakpoint;
  stop_step = inf_status->stop_step;
  stop_stack_dummy = inf_status->stop_stack_dummy;
  stopped_by_random_signal = inf_status->stopped_by_random_signal;
  trap_expected = inf_status->trap_expected;
  step_range_start = inf_status->step_range_start;
  step_range_end = inf_status->step_range_end;
  step_frame_address = inf_status->step_frame_address;
  step_over_calls = inf_status->step_over_calls;
  step_resume_break_address = inf_status->step_resume_break_address;
  stop_after_trap = inf_status->stop_after_trap;
  stop_after_attach = inf_status->stop_after_attach;
  set_breakpoint_commands (inf_status->breakpoint_commands);

  bcopy (inf_status->stop_registers, stop_registers, GENERAL_REGISTER_BYTES);

  if (inf_status->restore_stack_info)
    {
      fid = find_relative_frame (get_current_frame (),
				 &level);
      
      if (FRAME_FP (fid) != inf_status->selected_frame_address ||
	  level != 0)
	{
#ifndef TEK_HACK
	  ui_fprintf (stderr, "Unable to restore previously selected frame.\n");
#endif
	  select_frame (get_current_frame (), 0);
	  return;
	}
      
      select_frame (fid, inf_status->selected_level);
    }
  return;
}


void
_initialize_infrun ()
{
  register int i;

  add_info ("signals", signals_info,
	    "What debugger does when program gets various signals.\n\
Specify a signal number as argument to print info on that signal only.");

  add_com ("handle", class_run, handle_command,
	   "Specify how to handle a signal.\n\
Args are signal number followed by flags.\n\
Flags allowed are \"stop\", \"print\", \"pass\",\n\
 \"nostop\", \"noprint\" or \"nopass\".\n\
Print means print a message if this signal happens.\n\
Stop means reenter debugger if this signal happens (implies print).\n\
Pass means let program see this signal; otherwise program doesn't know.\n\
Pass and Stop may be combined.");

  for (i = 0; i < NSIG; i++)
    {
      signal_stop[i] = 1;
      signal_print[i] = 1;
      signal_program[i] = 1;
    }

  /* Signals caused by debugger's own actions
     should not be given to the program afterwards.  */
  signal_program[SIGTRAP] = 0;
  signal_program[SIGINT] = 0;

  /* Signals that are not errors should not normally enter the debugger.  */
#ifdef SIGALRM
  signal_stop[SIGALRM] = 0;
  signal_print[SIGALRM] = 0;
#endif /* SIGALRM */
#ifdef SIGVTALRM
  signal_stop[SIGVTALRM] = 0;
  signal_print[SIGVTALRM] = 0;
#endif /* SIGVTALRM */
#ifdef SIGPROF
  signal_stop[SIGPROF] = 0;
  signal_print[SIGPROF] = 0;
#endif /* SIGPROF */
#ifdef SIGCHLD
  signal_stop[SIGCHLD] = 0;
  signal_print[SIGCHLD] = 0;
#endif /* SIGCHLD */
#ifdef SIGCLD
  signal_stop[SIGCLD] = 0;
  signal_print[SIGCLD] = 0;
#endif /* SIGCLD */
#ifdef SIGIO
  signal_stop[SIGIO] = 0;
  signal_print[SIGIO] = 0;
#endif /* SIGIO */
#ifdef SIGURG
  signal_stop[SIGURG] = 0;
  signal_print[SIGURG] = 0;
#endif /* SIGURG */
}

