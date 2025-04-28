/* Low level interface to ptrace, for GDB when running under Unix.
   Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/inflow.c,v 1.14 90/08/19 21:23:27 robertb Exp $
   $Locker:  $

This file is part of GDB.  */

#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "ui.h"

#ifdef USG
#include <sys/types.h>
#include <fcntl.h>
#endif

#include <stdio.h>
#ifndef _WIN32
#include <sys/dir.h>
#endif
#include <signal.h>

#ifndef _WIN32
#ifdef HAVE_TERMIO
#include <termio.h>
#undef TIOCGETP
#define TIOCGETP TCGETA
#undef TIOCSETN
#define TIOCSETN TCSETA
#undef TIOCSETP
#define TIOCSETP TCSETAF
#define TERMINAL struct termio
#else
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sgtty.h>
#define TERMINAL struct sgttyb
#endif
#endif

#ifdef SET_STACK_LIMIT_HUGE
#include <sys/time.h>
#include <sys/resource.h>
extern int original_stack_limit;
#endif /* SET_STACK_LIMIT_HUGE */

extern int errno;

/* Nonzero if we are debugging an attached outside process
   rather than an inferior.  */

int attach_flag;


/* Record terminal status separately for debugger and inferior.  */

#ifndef _WIN32
static TERMINAL sg_inferior;
static TERMINAL sg_ours;
#endif

static int tflags_inferior;
static int tflags_ours;

#ifdef TIOCGETC
static struct tchars tc_inferior;
static struct tchars tc_ours;
#endif

#ifdef TIOCGLTC
static struct ltchars ltc_inferior;
static struct ltchars ltc_ours;
#endif

#ifdef TIOCLGET
static int lmode_inferior;
static int lmode_ours;
#endif

#ifdef TIOCGPGRP
static int pgrp_inferior;
static int pgrp_ours;
#else
static int (*sigint_ours) ();
static int (*sigquit_ours) ();
#endif /* TIOCGPGRP */

/* Copy of inferior_io_terminal when inferior was last started.  */
static char *inferior_thisrun_terminal;

static void terminal_ours_1 ();

/* Nonzero if our terminal settings are in effect.
   Zero if the inferior's settings are in effect.  */
static int terminal_is_ours;

/* Initialize the terminal settings we record for the inferior,
   before we actually run the inferior.  */

void
terminal_init_inferior ()
{
#ifndef _WIN32
  if (remote_debugging)
    return;

  sg_inferior = sg_ours;
  tflags_inferior = tflags_ours;

#ifdef TIOCGETC
  tc_inferior = tc_ours;
#endif

#ifdef TIOCGLTC
  ltc_inferior = ltc_ours;
#endif

#ifdef TIOCLGET
  lmode_inferior = lmode_ours;
#endif

#ifdef TIOCGPGRP
  pgrp_inferior = inferior_pid;
#endif /* TIOCGPGRP */

  terminal_is_ours = 1;
#else
return;
#endif
}

/* Put the inferior's terminal settings into effect.
   This is preparation for starting or resuming the inferior.  */

void
terminal_inferior ()
{
#ifndef _WIN32
  if (remote_debugging)
    return;

  if (terminal_is_ours)   /*  && inferior_thisrun_terminal == 0) */
    {
      fcntl (0, F_SETFL, tflags_inferior);
      fcntl (0, F_SETFL, tflags_inferior);
      ioctl (0, TIOCSETN, &sg_inferior);

#ifdef TIOCGETC
      ioctl (0, TIOCSETC, &tc_inferior);
#endif
#ifdef TIOCGLTC
      ioctl (0, TIOCSLTC, &ltc_inferior);
#endif
#ifdef TIOCLGET
      ioctl (0, TIOCLSET, &lmode_inferior);
#endif

#ifdef TIOCGPGRP
      ioctl (0, TIOCSPGRP, &pgrp_inferior);
#else
      sigint_ours = (int (*) ()) signal (SIGINT, SIG_IGN);
      sigquit_ours = (int (*) ()) signal (SIGQUIT, SIG_IGN);
#endif /* TIOCGPGRP */
    }
  terminal_is_ours = 0;
#else
return;
#endif
}

/* Put some of our terminal settings into effect,
   enough to get proper results from our output,
   but do not change into or out of RAW mode
   so that no input is discarded.

   After doing this, either terminal_ours or terminal_inferior
   should be called to get back to a normal state of affairs.  */

void
terminal_ours_for_output ()
{
  if (remote_debugging)
    return;

  terminal_ours_1 (1);
}

/* Put our terminal settings into effect.
   First record the inferior's terminal settings
   so they can be restored properly later.  */

void
terminal_ours ()
{
  if (remote_debugging)
    return;

  terminal_ours_1 (0);
}

static void
terminal_ours_1 (output_only)
     int output_only;
{
#ifndef _WIN32
#ifdef TIOCGPGRP
  /* Ignore this signal since it will happen when we try to set the pgrp.  */
  int (*osigttou) ();
#endif /* TIOCGPGRP */

  if (!terminal_is_ours)  /*   && inferior_thisrun_terminal == 0)  */
    {
      terminal_is_ours = 1;

#ifdef TIOCGPGRP
      osigttou = (int (*) ()) signal (SIGTTOU, SIG_IGN);

      ioctl (0, TIOCGPGRP, &pgrp_inferior);
      ioctl (0, TIOCSPGRP, &pgrp_ours);

      signal (SIGTTOU, osigttou);
#else
      signal (SIGINT, sigint_ours);
      signal (SIGQUIT, sigquit_ours);
#endif /* TIOCGPGRP */

      tflags_inferior = fcntl (0, F_GETFL, 0);
      ioctl (0, TIOCGETP, &sg_inferior);

#ifdef TIOCGETC
      ioctl (0, TIOCGETC, &tc_inferior);
#endif
#ifdef TIOCGLTC
      ioctl (0, TIOCGLTC, &ltc_inferior);
#endif
#ifdef TIOCLGET
      ioctl (0, TIOCLGET, &lmode_inferior);
#endif
    }

#ifdef HAVE_TERMIO
  sg_ours.c_lflag |= ICANON;
  if (output_only && !(sg_inferior.c_lflag & ICANON))
    sg_ours.c_lflag &= ~ICANON;
#else /* not HAVE_TERMIO */
  sg_ours.sg_flags &= ~RAW & ~CBREAK;
  if (output_only)
    sg_ours.sg_flags |= (RAW | CBREAK) & sg_inferior.sg_flags;
#endif /* not HAVE_TERMIO */

  fcntl (0, F_SETFL, tflags_ours);
  fcntl (0, F_SETFL, tflags_ours);
  ioctl (0, TIOCSETN, &sg_ours);

#ifdef TIOCGETC
  ioctl (0, TIOCSETC, &tc_ours);
#endif
#ifdef TIOCGLTC
  ioctl (0, TIOCSLTC, &ltc_ours);
#endif
#ifdef TIOCLGET
  ioctl (0, TIOCLSET, &lmode_ours);
#endif

#ifdef HAVE_TERMIO
  sg_ours.c_lflag |= ICANON;
#else /* not HAVE_TERMIO */
  sg_ours.sg_flags &= ~RAW & ~CBREAK;
#endif /* not HAVE_TERMIO */
#endif /* _WIN32 */
}

static void
term_status_command ()
{
#ifndef _WIN32
  register int i;

  if (remote_debugging)
    {
      printf_filtered ("No terminal status when remote debugging.\n");
      return;
    }

  printf_filtered ("Inferior's terminal status (currently saved by GDB):\n");

#ifdef HAVE_TERMIO

  printf_filtered ("fcntl flags = 0x%x, c_iflag = 0x%x, c_oflag = 0x%x,\n",
	  tflags_inferior, sg_inferior.c_iflag, sg_inferior.c_oflag);
  printf_filtered ("c_cflag = 0x%x, c_lflag = 0x%x, c_line = 0x%x.\n",
	  sg_inferior.c_cflag, sg_inferior.c_lflag, sg_inferior.c_line);
  printf_filtered ("c_cc: ");
  for (i = 0; (i < NCC); i += 1)
    printf_filtered ("0x%x ", sg_inferior.c_cc[i]);
  printf_filtered ("\n");

#else /* not HAVE_TERMIO */

  printf_filtered ("fcntl flags = 0x%x, sgttyb.sg_flags = 0x%x, owner pid = %d.\n",
	  tflags_inferior, sg_inferior.sg_flags, pgrp_inferior);

#endif /* not HAVE_TERMIO */

#ifdef TIOCGETC
  printf_filtered ("tchars: ");
  for (i = 0; i < sizeof (struct tchars); i++)
    printf_filtered ("0x%x ", ((char *)&tc_inferior)[i]);
  printf_filtered ("\n");
#endif

#ifdef TIOCGLTC
  printf_filtered ("ltchars: ");
  for (i = 0; i < sizeof (struct ltchars); i++)
    printf_filtered ("0x%x ", ((char *)&ltc_inferior)[i]);
  printf_filtered ("\n");
  ioctl (0, TIOCSLTC, &ltc_ours);
#endif
  
#ifdef TIOCLGET
  printf_filtered ("lmode:  %x\n", lmode_inferior);
#endif
#endif	/* _WIN32 */
}

static void
new_tty (ttyname)
     char *ttyname;
{
#ifndef _WIN32
  register int tty;

#ifdef TIOCNOTTY
  /* Disconnect the child process from our controlling terminal.  */
  tty = open("/dev/tty", O_RDWR);
  if (tty > 0)
    {
      ioctl(tty, TIOCNOTTY, 0);
      close(tty);
    }
#endif

  /* Now open the specified new terminal.  */

  tty = open(ttyname, O_RDWR);
  if (tty == -1)
    _exit(1);

  /* Avoid use of dup2; doesn't exist on all systems.  */
  if (tty != 0)
    { close (0); dup (tty); }
  if (tty != 1)
    { close (1); dup (tty); }
  if (tty != 2)
    { close (2); dup (tty); }
  if (tty > 2)
    close(tty);
#endif
}

/* Start an inferior process and returns its pid.
   ALLARGS is a string containing shell command to run the program.
   ENV is the environment vector to pass.  */

#ifndef SHELL_FILE
#define SHELL_FILE "/bin/sh"
#endif

int
create_inferior (allargs, env)
     char *allargs;
     char **env;
{
#ifndef _WIN32
  int pid;
  char *shell_command;
//  extern int sys_nerr;
//  extern char *sys_errlist[];
  extern int errno;

  /* If desired, concat something onto the front of ALLARGS.
     SHELL_COMMAND is the result.  */
#ifdef SHELL_COMMAND_CONCAT
  shell_command = (char *) alloca (strlen (SHELL_COMMAND_CONCAT) + strlen (allargs) + 1);
  strcpy (shell_command, SHELL_COMMAND_CONCAT);
  strcat (shell_command, allargs);
#else
  shell_command = allargs;
#endif

  /* exec is said to fail if the executable is open.  */
#ifndef TEK_HACK
  close_exec_file ();
#endif

#if defined(USG) && !defined(HAVE_VFORK)
  pid = fork ();
  if (pid < 0)
    perror_with_name ("fork");
#else
  pid = vfork ();
  if (pid < 0)
    perror_with_name ("vfork");
#endif


  ui_strtSubProc(pid);
  if (pid == 0)
    {
#ifdef TIOCGPGRP
      /* Run inferior in a separate process group.  */
      setpgrp (getpid (), getpid ());
#endif /* TIOCGPGRP */

#ifdef SET_STACK_LIMIT_HUGE
      /* Reset the stack limit back to what it was.  */
      {
	struct rlimit rlim;

	getrlimit (RLIMIT_STACK, &rlim);
	rlim.rlim_cur = original_stack_limit;
	setrlimit (RLIMIT_STACK, &rlim);
      }
#endif /* SET_STACK_LIMIT_HUGE */


      inferior_thisrun_terminal = inferior_io_terminal;
      if (inferior_io_terminal != 0)
	new_tty (inferior_io_terminal);

/* Not needed on Sun, at least, and loses there
   because it clobbers the superior.  */
/*???      signal (SIGQUIT, SIG_DFL);
      signal (SIGINT, SIG_DFL);  */

      call_ptrace (0, 0, 0, 0);
      execle (SHELL_FILE, "sh", "-c", shell_command, 0, env);

      ui_fprintf (stderr, "Cannot exec %s: %s.\n", SHELL_FILE,
	       errno < sys_nerr ? sys_errlist[errno] : "unknown error");
      ui_fflush (stderr);
      _exit (0177);
    }

#ifdef CREATE_INFERIOR_HOOK
  CREATE_INFERIOR_HOOK (pid);
#endif  
  return pid;
#else
  return -1;
#endif
}

/* Kill the inferior process.  Make us have no inferior.  */

static void
kill_command ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    ui_badnews(-1,"The program is not being run.");
  if (!query ("Kill the inferior process? "))
    ui_badnews(-1,"Not confirmed.");
  kill_inferior ();
#ifdef TEK_HACK
  ui_fprintf(stdout, "\n");
#endif /* TEK_HACK */
}

void
inferior_died ()
{
  inferior_pid = 0;
  attach_flag = 0;
  mark_breakpoints_out ();

#ifdef TEK_HACK
  mark_watchpoints_out();
#endif

  select_frame ((FRAME) 0, -1);
  reopen_exec_file ();
  if (have_core_file_p ())
    set_current_frame ( create_new_frame (read_register (SYNTH_FP_REGNUM),
					  read_pc ()));
  else
    set_current_frame (0);
}

static void
try_writing_regs_command ()
{
  register int i;
  register int value;
  extern int errno;

  if (inferior_pid == 0)
    ui_badnews(-1,"There is no inferior process now.");

  for (i = 0; ; i += 2)
    {
      QUIT;
      errno = 0;
      value = call_ptrace (3, inferior_pid, i, 0);
      call_ptrace (6, inferior_pid, i, value);
      if (errno == 0)
	{
	  ui_fprintf(stdout, " Succeeded with address 0x%x; value 0x%x (%d).\n",
		  i, value, value);
	}
      else if ((i & 0377) == 0)
	ui_fprintf(stdout, " Failed at 0x%x.\n", i);
    }
}

void
_initialize_inflow ()
{
  add_com ("term-status", class_obscure, term_status_command,
	   "Print info on inferior's saved terminal status.");

  add_com ("try-writing-regs", class_obscure, try_writing_regs_command,
	   "Try writing all locations in inferior's system block.\n\
Report which ones can be written.");

  add_com ("kill", class_run, kill_command,
	   "Kill execution of program being debugged.");

  inferior_pid = 0;

#ifndef _WIN32
  ioctl (0, TIOCGETP, &sg_ours);
  fcntl (0, F_GETFL, tflags_ours);

#ifdef TIOCGETC
  ioctl (0, TIOCGETC, &tc_ours);
#endif
#ifdef TIOCGLTC
  ioctl (0, TIOCGLTC, &ltc_ours);
#endif
#ifdef TIOCLGET
  ioctl (0, TIOCLGET, &lmode_ours);
#endif

#ifdef TIOCGPGRP
  ioctl (0, TIOCGPGRP, &pgrp_ours);
#endif /* TIOCGPGRP */
#endif

  terminal_is_ours = 1;
}

