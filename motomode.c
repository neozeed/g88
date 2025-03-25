
/* This supports communication with the Motorola 188BUG
   monitor.  There is a small amount code for this support in other places
   in this file. 

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/motomode.c,v 1.12 91/01/13 23:45:29 robertb Exp $ */

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <setjmp.h>
#include <sys/types.h>
//#include <termio.h>
#include <string.h>

#include "defs.h"
#include "remote.h"
#include "ui.h"

boolean passthroughmode;
static void wait_for_remote_string();
static void send_remote_string();
static unsigned long download_or_checksum_monitor();
static boolean waiting_for_echoed_character;
static boolean verbose;			/* Print lots of info in pass mode   */
static boolean terminal_mode_altered;	/* True if we've diddled the terminal*/
#ifdef SYSV
static struct termio old_terminal_mode;
static struct termio old_remote_mode;
#else
#endif
static int terminal_fd = 0;		/* We do ioctl's on this             */
static jmp_buf int_env;                 /* longjmp on this on SIGINT sometimes*/
static jmp_buf pass_env;		/* longjmp on this to bale out of pass*/
static boolean show_target_output_mode;
static long monitor_start_addr;
static long monitor_end_addr;		/* monitor_start_addr + monitor_length*/
static long monitor_length;		/* Gotten from COFF file with monitor*/
static char *mfname1 = "tek188mon";	/* COFF file with monitor text+data  */
static char *mfname2 = "/usr/local/ashura/parunix/lib/tek188mon";
#define	BUG_RETURN	(0x63)		/* Return to 188BUG 'syscall' code   */

/* This does a BUG188 system call.  */
static unsigned long 
remote_188syscall(code, arg1, arg2, arg3, arg4, timeout_action, timeout)
    unsigned long code, arg1, arg2, arg3, arg4, timeout;
    timeout_action_t timeout_action;
{
    unsigned long return_val;
    splhi();

    invalidate_mem_cache();
    init_checksum();
    send_command(C_188SYSCALL);
    send_word(code);
    send_word(arg1);
    send_word(arg2);
    send_word(arg3);
    send_word(arg4);
    if (varvalue("traceremote")) {
        ui_fprintf(stderr, 
    "Doing 188BUG syscall code=0x%x arg1=0x%x arg2=0x%x arg3=0x%x arg4=0x%x\n", 
                         code, arg1, arg2, arg3, arg4);
    }
    send_command(C_END);
    send_checksum();
    if (wait_for_token(C_ACK, C_NONE, ta_warning, 2) !=C_ACK) {
        spllo();
        error("Didn't find C_ACK in remote_188syscall");
    }
    spllo();
    if (code == BUG_RETURN) {
      do_pass_command("", true, true);
    }
    init_checksum();
    switch (wait_for_token(C_DATASTART, C_DATAERROR, timeout_action, timeout)) {
        case C_DATASTART:
            return_val = get_word();
            if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
                error("Didn't find C_DATAEND in remote_188syscall");
            }
            verify_checksum();
            break;

        case C_DATAERROR:
            error("DACC while doing 188BUG syscall");
            break;

        default:
            error("Didn't get C_DATASTART from target");
            break;
    }

    return return_val; 
}

/* Turn off echoing of characters typed at the terminal. */
void init_terminal_mode()
{
#ifdef SYSV
  struct termio t;
 
  if (ioctl(terminal_fd, TCGETA, &t) < 0) {
    ui_fprintf(stderr, "init_terminal_mode: Error in TCGETA ioctl");
  }
  t.c_iflag &= ~(INLCR|IGNCR|ICRNL);

  /* Turn off echoing of characters on the terminal.  The target will
     echo the characters and we display everyting that the target
     sends to us. */

  t.c_lflag &= ~(ECHO|ECHOE|ICANON|ECHOK|ECHONL|XCASE);
  t.c_lflag |= ISIG;

  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 1;	/* Tenth's of seconds to wait for chars */
  
  if (ioctl(terminal_fd, TCSETAW, &t) < 0) {
    ui_badnews(-1, "init_terminal_mode: Error in TCSETAW ioctl with terminal");
  }
  terminal_mode_altered = true;
#else
  terminal_mode_altered = false;
#endif
}

/* Make a copy of the current terminal mode so that we can restore it
   later. */
void save_terminal_mode()
{
#ifdef SYSV
  if (ioctl(terminal_fd, TCGETA, &old_terminal_mode) < 0) {
    ui_badnews(-1, "do_pass_command: Error in TCGETA ioctl with terminal");
  }
#endif
}

/* Restore the terminal mode to be the way it was
   when we entered do_pass_command().  */
void restore_terminal_mode()
{
  if (!terminal_mode_altered) {
    return;
  }
#ifdef SYSV
 if (ioctl(terminal_fd, TCSETAW, &old_terminal_mode) < 0) {
    fatal("restore_terminal_mode: Error in TCSETAW ioctl");
  }
#else
#endif
  terminal_mode_altered = false;
}

/* Restore the remote tty mode to be the way it was
   when we entered do_pass_command().  */
static void restore_remote_mode()
{
#ifdef SYSV
  if (ioctl(remote_fd, TCSETAW, &old_remote_mode) < 0) {
    fatal("restore_remote_mode: Error in TCSETAW ioctl");
  }
#endif
}

/* Return true if the checksum of the text segment of the file mfname1 
   or mfname2 matches the checksum of the string of words of the same length at
   address 'monitor_start_addr' in the target's memory. */
static boolean monitor_checksum_matches()
{
  unsigned cs;
  char c, buf[20];
  int i;

  cs = download_or_checksum_monitor(mfname1, 
                                    mfname2, 
                                    false, 
                                    &monitor_length, 
                                    &monitor_start_addr);
  sprintf(buf, "%0X", cs);
  if (verbose) {
    ui_fprintf(stdout, "Host checksum =%s\n", buf);
  }
  monitor_end_addr = monitor_start_addr + monitor_length;
  send_remote_string("cs %x %x\r", monitor_start_addr, monitor_end_addr);
  wait_for_remote_string("Effective address: %0X\r\n", monitor_start_addr);
  wait_for_remote_string("Effective address: %0X\r\n", monitor_end_addr - 1);
  wait_for_remote_string("Checksum: ");
  for (i = 0 ; i < 8 ;i++) {
    c = get_remote_char(ta_none, 1);
    if (verbose)
      ui_putc(c, stdout);
    if (c != buf[i]) {
      return false;
    }
  }
  return true;
}

/* Do the "*w" command (of pass-through mode).  Warm-start the Tektronix
   debug monitor.  We send a control-X to clear 188BUG's command line
   buffer in case there are already some characters in it. */
static void pass_warm_start_cmd()
{
  if (!monitor_checksum_matches()) {
    ui_fprintf(stdout, "Monitor checksum doesn't match, can't warm-start\n");
    return;
  }
  passthroughmode = false;
  send_remote_string("g %0x\r", monitor_start_addr + 4);
  wait_for_remote_string("Effective address: %0X\r\n", monitor_start_addr + 4);
  ui_fprintf(stdout, "All general register values except r9's copied to\n");
  ui_fprintf(stdout, "Tektronix monitor register image.\n");
}

/* Handle SIGINT while in do_pass_command().  */
void pass_siginthandler()
{
  if (resetting_remote) {
    passthroughmode = false;
    spllo();
    if (reset_env == (int *)0) {
      ui_badnews(-1, "SIGINT handler in motomode.c finds nil pointer");
    }
    longjmp(reset_env, 1);
  }
  spllo();
  if (waiting_for_echoed_character) {
    waiting_for_echoed_character = false;
    longjmp(int_env, 1);
  } else {
    if (passthroughmode) {
      char c, buf[2];

      passthroughmode = false;
      restore_terminal_mode();
      c = '\0';
      while (c != 'y' && c != 'Y' && c != 'n' && c != 'N') {
        ui_fprintf(stderr, "Attempt to warm start Tektronix debug monitor ?");
        ui_fgets(buf, sizeof buf, stdin);
        c = buf[0];
        ui_putc('\n', stdout);
      }
      if (c == 'y' || c == 'Y') {
        pass_warm_start_cmd();
        longjmp(pass_env, 1);
      }
    }
  }
  init_terminal_mode();
}

/* Handle SIGTSTP while in do_pass_command().  We change the action
   on SIGTSTP to be default action, i.e., process suspension.  Then
   we propogate the signal by sending it to ourselves.  This will cause
   the process to be suspended, and if we are running under the C
   shell, the user will get a C shell prompt.  When and if we are resumed
   control will be just after the call to kill() below.  We change
   the action for SIGTSTP back to be calling this function, we
   reset the terminal mode to raw, and we send a <RET> to 188BUG to
   get a fresh prompt.  */
static void pass_sigstphandler()
{
  passthroughmode = false;
  restore_terminal_mode();
#ifndef _WIN32
  signal(SIGTSTP, SIG_DFL);
#ifdef SYSV
  sigsetmask(sigblock() & ~sigmask(SIGTSTP));
#else
//	motomode.c:269: too few arguments to function `sigblock'
#endif
  kill(getpid(), SIGTSTP);
  signal(SIGTSTP, pass_sigstphandler);
#endif
  passthroughmode = true;
  init_terminal_mode();
  put_remote_char('\r');
}

/* Wait for the passed character to return from the remote tty line.
   Don't expect a control-X to be echoed as itself. */
static void wait_for_echoed_character(char_to_match)
  char char_to_match;
{
  jmp_buf timeout_env;
  char char_just_read;
  if (char_to_match == ('x' & 0x1f)) {
    return;
  }
  while (1) {
    if (resetting_remote) {
      char_just_read = (char)get_remote_char(reset_env, 
                                             varvalue("resettimeout"));
      if (show_target_output_mode) {
        ui_putc(char_just_read, stdout);
        ui_fflush(stdout);
      }

      if (char_to_match == char_just_read) {
        break;
      }
    } else {
      if (setjmp(timeout_env) == 0) {
        char_just_read = (char)get_remote_char(timeout_env, 1);
        if (show_target_output_mode) {
          ui_putc(char_just_read, stdout);
          ui_fflush(stdout);
        }
  
        if (char_to_match == char_just_read) {
          break;
        }
      } else {
        if (isprint(char_to_match)) {
          ui_fprintf(stderr, "<Timeout waiting for %c to echo>\n", 
                                                   char_to_match);
        } else {
          ui_fprintf(stderr, "<Timeout waiting for 0x%x to echo>\n", 
                                             (unsigned char)char_to_match);
        }
        break;
      }
    }
  }
}

/* Wait for the passed string to be emitted by the target. */
/*VARARGS1*/
static void wait_for_remote_string(fmt, a, b, c, d, e, f)
  char *fmt;
{
  char buf[1000], *p;

  sprintf(buf, fmt, a, b, c, d, e, f);
  p = buf;

  while (*p) {
    wait_for_echoed_character(*p++);
  }
}

/* Send the passed string to the target. */
/*VARARGS1*/
static void send_remote_string(fmt, a, b, c, d, e, f)
  char *fmt;
{
  char buf[1000], *s;

  sprintf(buf, fmt, a, b, c, d, e, f);
  s = buf;
  while (*s) {
    put_remote_char(*s);
    wait_for_echoed_character(*s);
    s++;
  }
}

/* Download an 80 byte program that will read bytes from the serial line
 * and stuff them into memory.  We send the Tektronix debug monitor to
 * the target this way, it is much faster. */
static void download_monitor_boot()
{
  static char *s[] = { 
    "c622007\r",  "d003ffff\r", "c82200f\r", 
    "64c40080\r", "e846eff9\r", "64c40083\r", "e9a60005\r", "c622007\r", 
    "d003ffff\r", "c82200f\r",  "60840080\r", "2c850000\r", "60a50001\r", 
    "c3fffff3\r", ".\r",        "" };

  int i;
  unsigned long monitor_boot_addr = monitor_start_addr + 0x4000;

  send_remote_string("mm %x;n\r", monitor_boot_addr);
  send_remote_string("5c40fff8\r");

  /* This is "or.u r5,r0,hi16(monitor_start_addr)" */

  send_remote_string("5ca0%04x\r", 0xffff & (monitor_start_addr >> 16));

  /* This is "or r5,r5,lo16(monitor_start_addr)" */

  send_remote_string("58a5%04x\r", monitor_start_addr & 0xffff);

  for (i = 0 ; *s[i] ; i++) {
    send_remote_string(s[i]);
  }
  wait_for_remote_string("%s", "188-Bug>");
  send_remote_string("g %x\r", monitor_boot_addr);
  wait_for_remote_string("Effective address: %X\r\n",  monitor_boot_addr);
}

/*
 * This converts the debug monitor into a form that can be
 * fed to 188BUG.  This returns the checksum of the monitor loaded from the
 * COFF file whose file name is passed in the first argument.
 * If the second argument is true, the monitor is sent to the target.
 * The return value is the checksum of the monitor.  We use the
 * same algorithm as the 188BUG.  The word pointed to by the third
 * argument is set to the length of the monitor in bytes.
 */

#ifdef SYSV
#include <a.out.h>
#else
#include "a.out.gnu.h"
#endif

#define TERMINAL_CHAR (0x80)  /* Tells monitor boot to jump to monitor */
static unsigned long 
download_or_checksum_monitor(filename1, 
                             filename2, 
                             download, 
                             monitor_length_p,
                             monitor_start_p)

  char *filename1, *filename2;  /* Two file names to find monitor in         */
  boolean download;		/* True => send monitor text to target       */
  int *monitor_length_p;	/* Var parameter, length of monitor in bytes */
  unsigned long *monitor_start_p; /* Var parameter, starting address of mon  */
{
  int fd;			/* File descriptor for monitor COFF file */
  unsigned char c;
  int i, not_at_text_yet;
#ifdef SYSV
  struct filehdr filehdr;	/* COFF file header structure for monitor */
  struct scnhdr scnhdr;		/* COFF section header structure for mon  */
#else
  struct exec aout_hdr;
#endif

  unsigned long checksum = 0;	/* Checksum of monitor text section       */
  unsigned long w, lower_half, lower_carry, upper_half, upper_carry;
  char *filename = filename1;	/* Name of monitor COFF file              */

  fd = open(filename1, O_RDONLY);
  if (fd <= 0) {
    fd = open(filename2, O_RDONLY);
    filename = filename2;
  }
  if (fd > 0) {
#ifdef SYSV
    if(read(fd, &filehdr, sizeof filehdr) != sizeof filehdr) {
      ui_badnews(-1, "Unable to read %s", filename);
    }
    if (filehdr.f_magic != MC88MAGIC) {
      ui_badnews(-1, "%s is not an 88000 coff file", filename);
    }
    if (!(filehdr.f_flags & F_EXEC)) {
      ui_badnews(-1, "%s is not executable, check for unresolved symbols",
                       filename);
    }
    lseek(fd, filehdr.f_opthdr, 1); /* Skip the optional header */
    not_at_text_yet = 1;
    while (not_at_text_yet) {
      if (read(fd, &scnhdr, sizeof scnhdr) != sizeof scnhdr) {
        ui_badnews(-1, "Error reading section header");
      }
      not_at_text_yet = !(scnhdr.s_flags & STYP_TEXT);
    }
    lseek(fd, scnhdr.s_scnptr, 0); /* Seek to start of raw section */
    if (monitor_length_p) {
      *monitor_length_p = scnhdr.s_size;
    }
    if (monitor_start_p) {
      *monitor_start_p = scnhdr.s_paddr;
    }
    for (i = 0 ; i < scnhdr.s_size ; i += sizeof w) {
      if (read(fd, &w, 4) != 4) {
        ui_badnews(-1, "Error reading 4 bytes for section '%s' of %s",
                                    scnhdr.s_name, filename);
      }
#else
    if(read(fd, &aout_hdr, sizeof aout_hdr) != sizeof aout_hdr) {
      ui_badnews(-1, "Unable to read %s", filename);
    }
    if (aout_hdr.a_trsize > 0 || aout_hdr.a_drsize > 0) {
      ui_badnews(-1, "%s is not executable, check for unresolved symbols",
                       filename);
    }
    lseek(fd, N_TXTOFF(aout_hdr), 0);
    if (monitor_length_p) {
      *monitor_length_p = aout_hdr.a_text;
    }
    if (monitor_start_p) {
      *monitor_start_p = aout_hdr.a_entry;
    }
    for (i = 0 ; i < aout_hdr.a_text ; i += sizeof w) {
      if (read(fd, &w, 4) != 4) {
        ui_badnews(-1, "Error reading 4 bytes of %s", filename);
      }
#endif
      if (download) {
        int j;

        for (j = (sizeof w) - 1 ; j >= 0 ; j--){
          c = (w >> (j*8)) & 0xff;
          if (TERMINAL_CHAR <= c && c <= COMMAND_PREFIX1) {
            put_remote_char(COMMAND_PREFIX1);
            c &= 0x7f;
          }
          put_remote_char(c);
        }
      }
      lower_half = (checksum & 0xffff) + (w & 0xffff);
      lower_carry = lower_half >> 16;
      lower_half &= 0xffff;

      upper_half = ((checksum >> 16) & 0xffff) + ((w >> 16) & 0xffff);
      upper_half += lower_carry;
      upper_carry = upper_half >> 16;
      upper_half &= 0xffff;

      checksum = (upper_half << 16) + lower_half + upper_carry;
    }
    if (download)
      put_remote_char(TERMINAL_CHAR);
  } else {
    ui_badnews(-1, "Counldn't open %s for reading", filename);
  }
  return checksum;
}

/* Tell the serial driver to do flow control.  We do this to the remote
   tty when we are talking to 188BUG. */
static void turn_on_xon_mode(fd)
  int fd;
{
#ifdef SYSV
  struct termio t;

  if (ioctl(fd, TCGETA, &t) < 0) {
    fatal("turn_on_xon_mode: Error in doing TCGETA ioctl");
  }

  t.c_iflag |= IXON | IXOFF | IXANY;
  t.c_cc[VSTART] = 'Q' & 0x1f;
  t.c_cc[VSTOP] = 'S' & 0x1f;

  if (ioctl(fd, TCSETAW, &t) < 0) {
    fatal("turn_on_xon_mode: Error in doing TCSETAW ioctl");
  }
#else
#endif
}
 
/* Command to pass ascii between gdb user and 188BUG */
static void pass_command(str, from_tty)
{
  do_pass_command(str, from_tty, from_tty);
}

/* Command to return to 188BUG and pass ascii between gdb user and 188BUG */
static void bug_command(str, from_tty)
{
  ui_fprintf(stdout, "All register values except r9's copied to\n");
  ui_fprintf(stdout, "188BUG register image (copy r9 manually if you care).\n");
  remote_188syscall(BUG_RETURN, 0, 0, 0, 0, ta_warning, 1);
}

/* Send characters received from target to the gdb user and send
   characters typed by the user to the target.
  
   There are several commands that the user can do by typing an
   asterick at the start of a line followed by a command character.
   They are described in the help message (see code for case 'h' below).

   We put the terminal into raw mode and read characters with read()
   and write them with ui_putc(.., stdout) and ui_fprintf(stdout, ..).
   This may seem odd, but the formatted output of ui_fprintf is
   handy and I have to read characters asychronously (thus the need
   to call read()).

   We handle both SIGINT and SIGTSTP while executing in this function.
   We just propogate SIGTSTP after resetting the terminal and the 
   handler for SIGTSTP.  SIGINT does different things depending on
   what we are doing at the time. */

void do_pass_command(str, from_tty, verbose_parameter)
  char *str;
  boolean from_tty;
  boolean verbose_parameter;
{
#define BUFSIZE (100)
  jmp_buf timeout_env;
  boolean next_char_is_command = false;
  boolean this_line_is_a_command = false;
  boolean wait_for_echo_mode = false;
  boolean file_open = false;	/* True => we are taking input from file   */
  int i;
  int arg_char_index;		/* Index into command_argument             */
  int column =0;			/* Used to figure if '*' is start of command*/
  char c, command_char;
  char *b, *p;
  char command_argument[BUFSIZE]; /* Argument buffer for '*s', '*W', '*b'   */
  int redirect_count;		/* # of characters read w/ last '*s' command*/
  char *fname;			/* Name of file that we are sourcing        */
  FILE *file;			/* File pointer for file we are sourcing    */

  if (!valid_baud(varvalue("resetbaud"))) {
    setvar("resetbaud", 9600);
  }
  if (setjmp(pass_env)) {
    passthroughmode = false;
    spllo();	/* This will reset handler to be request_quit() */
#ifndef _WIN32
    signal(SIGTSTP, SIG_DFL);
#endif
    restore_terminal_mode();
    restore_remote_mode();
    return;
  }
  verbose = verbose_parameter;
  save_terminal_mode();
#ifdef SYSV
  if (ioctl(remote_fd, TCGETA, &old_remote_mode) < 0) {
    ui_badnews(-1, "do_pass_command: Error in TCGETA ioctl with remote");
  }
#endif
  passthroughmode = true;
  spllo();
  if (from_tty) {
    verbose = true;
#ifndef _WIN32
    signal(SIGTSTP, pass_sigstphandler);
#endif
    init_terminal_mode(); 
  }
 
  turn_on_xon_mode(remote_fd);
  if (verbose) {
    ui_fprintf(stdout, "Connected to target on %s\n", remote_tty_name);
  }
  show_target_output_mode = verbose;
  while (passthroughmode) {

    /* Read as many characters as we can from the target and display
       them on the terminal.  Don't echo <RET> characters from the
       target, the terminal serial driver will do it for us. */

    while (1) {
      if (!resetting_remote && setjmp(timeout_env) == 0) {
        c = get_remote_char(timeout_env, 0);
        if (c != '\r' && show_target_output_mode) {
          ui_putc(c, stdout);
        }
      } else {
        break;
      }
    }
    ui_fflush(stdout);

    /* Read a character from the current source file.  If we can't get a
       character there, look for one from the terminal.  */

    if (file_open) {
      c = fgetc(file);
      if (c == EOF) {
        file_open = false;
        if (verbose) {
          ui_fprintf(stdout, 
               "Finished redirecting input from %s (read %d characters).\n", 
                                                fname, redirect_count);
        }
        close(file);
      } else {
        redirect_count++;
      }
    }
    if (!file_open) {
      if (str && (c = *str) != '\0') {
        str++;
      } else {
        char buf[1];
        if (from_tty && read(terminal_fd, &buf[0], sizeof buf) > 0) {
          c = buf[0];
        } else {
          c = 0;
        }
      }
    }
    if (c != 0) {

      /* Keep track of what column this is so that we know whether an
         asterick is a command character or not. */

      if (c == '\r' || c == '\n') {
        column = 0;
      } else {
        column++;
      }

      /* If we've seen an asterik, interpret the command, otherwise send  
         the character that we just got from the terminal to the target. */
  
      if (this_line_is_a_command) {
        if (c != '\r' && c != '\n') {
          if (arg_char_index < (BUFSIZE - 1)) {
            command_argument[arg_char_index++] = c;
            command_argument[arg_char_index] = '\0';
          } else {
            ui_fprintf(stderr, "Argument buffer overflow\n");
          }
          if (verbose)
            ui_putc(c, stdout);
        } else {
          if (verbose)
            ui_fprintf(stdout, "\n");
          switch (command_char) {
            case 'b':
              b = &command_argument[0];
              while (*b && isspace(*b)) b++;
              i = atoi(b);
              if (!valid_baud(i)) {
                ui_fprintf(stderr, "%d is not a valid baud.\n", i);
              } else if (i == current_baud) {
                if (verbose)
                  ui_fprintf(stderr, 
                         "Target tty line data rate is already at %d baud.\n", 
                                                    current_baud);
              } else {
                setbaud(i);
                if (verbose)
                  ui_fprintf(stdout, "Target tty line now set at %d baud\n", 
                                 current_baud);
              }
              break;

            case 'B':
#ifdef SYSV
              if (ioctl(remote_fd, TCSBRK, 0) < 0) {
                ui_fprintf(stderr, 
             "Error doing TCSBRK ioctl, trying to send break to target on %s\n",
                                                            remote_tty_name);
              }
#else
#endif
              break;
             

            case 'c':
              if (!monitor_checksum_matches()) {
                download_monitor_boot();
                (void)download_or_checksum_monitor(mfname1, mfname2, true, 0, 0);
                goto eat_it_Dijkstra;
              }
              send_remote_string("g %0x\r", monitor_start_addr);
              wait_for_remote_string("Effective address: %0X\r\n", 
                                                          monitor_start_addr);
eat_it_Dijkstra:
              passthroughmode = false;
              break;

            case 'e':
              wait_for_echo_mode = true;
              if (verbose) {
                  ui_fprintf(stdout, "Wait-for-echo on\n");
              }
              break;
  
            case 'i':
              send_interrupt_signal();
              break;

            case 'p':
              /* Loop here waiting for characters from the target until
                 none has been recieved for a whole second. */
              while (1) {
                if (setjmp(timeout_env) == 0) {
                  c = get_remote_char(timeout_env, 1);
                  if (c != '\r' && show_target_output_mode) {
                    ui_putc(c, stdout);
                  }
                } else {
                  break;
                }
              }
              break;

            case 'q':
              passthroughmode = false;
              break;

            case 'r':
              send_reset_signal();
              setbaud(varvalue("resetbaud"));
              if (verbose) 
                ui_fprintf(stdout, "Target tty line now set at %d baud\n", 
                                 current_baud);
              break;
  
            case 'h':
              ui_fprintf(stdout, "\n");
              ui_fprintf(stdout, 
              " *b rate  - make rate (currently %d) the new baud setting\n", 
                                              current_baud);
              ui_fprintf(stdout,
              " *B       - send a .25 second break to target\n");

              ui_fprintf(stdout, 
              " *c       - cold-start the Tektronix debug monitor\n");

              ui_fprintf(stdout, 
              " *e       - turn on wait-for-echo mode (currently %s)\n", 
                                            wait_for_echo_mode ? "on" : "off");
  
              ui_fprintf(stdout, 
              " *h       - print this command list\n");

              ui_fprintf(stdout, 
              " *i       - send an interrupt signal to the target\n");

              ui_fprintf(stdout, 
              " *p       - pause for 1 second while waiting for target\n");

              ui_fprintf(stdout, 
              " *q       - leave pass-through-mode, return to gdb mode\n");

              ui_fprintf(stdout, 
              " *r       - send a reset signal to the target\n");

              ui_fprintf(stdout,
              "            and then switch to %d baud\n", 
                                                      varvalue("resetbaud"));
              ui_fprintf(stdout, 
              " *s fname - send contents of file 'fname' to the target\n");

              ui_fprintf(stdout, 
              " *t       - toggle show-target-output mode (currently %s)\n",
                                        show_target_output_mode ? "on" : "off");

              ui_fprintf(stdout, 
              " *w       - warm-start Tek monitor, return to gdb mode\n");

              ui_fprintf(stdout, 
              " *Wstring - wait for 'string' from target\n");

              ui_fprintf(stdout, 
              " *z       - turn off wait-for-echo mode\n");
              break;

            case 's':
              fname = &command_argument[0];
              while (*fname && isspace(*fname)) fname++;
              file = fopen(fname, "r");
              if (file == 0) {
                ui_fprintf(stderr, "Couldn't open %s for reading\n", fname);
              } else {
                if (verbose)
                  ui_fprintf(stdout, "Redirecting input from %s\n", fname);
                file_open = true;
                redirect_count = 0;
              }
              break;

            case 't':
              show_target_output_mode = !show_target_output_mode;
              if (verbose)
                ui_fprintf(stdout, "Show-target-output-mode is %s\n",
                                  show_target_output_mode ? "on" : "off");
              break;

            case 'w':
              pass_warm_start_cmd();
              break;

            case 'W':
              wait_for_remote_string("%s", command_argument);
              break;

            case 'z':
              if (wait_for_echo_mode) {
                wait_for_echo_mode = false;
                ui_fprintf(stdout, "Wait-for-echo mode turned off\n");
              }
              break;

            default:
              ui_fprintf(stderr, 
                "Unknown command character '%c', type '*h' for help.\n", 
                                                                 command_char);
              break;
          }
          this_line_is_a_command = false;
        }
      } else {

        /* If the last character was an asterik in column 1, this character
           is the command character.  Save it and echo it to the terminal */

        if (next_char_is_command) {
          command_char = c;
          if (verbose)
            ui_fprintf(stdout, "%c", c);
          ui_fflush(stdout);
          arg_char_index = 0;
          next_char_is_command = false;
          this_line_is_a_command = true;
          continue;
        }

        /* If this is a command character, echo it here and don't send it
           to the target. */

        if (c == '*' && column == 1) {
          next_char_is_command = true;
          if (verbose)
            ui_fprintf(stdout, "%c", c);
        } else {
          put_remote_char(c);
          if (wait_for_echo_mode) {
            if (setjmp(int_env)) {
              if (file_open) {
                ui_fprintf(stderr, 
               "Terminating redirection of %s, (%d characters read so far).\n", 
                                           fname, redirect_count);
                ui_fprintf(stderr, "Was waiting for the echo of %c (0x%x)\n", 
                                           isprint(c) ? c : ' ', c);
                file_open = false;
                fclose(file);
              } else {
                ui_fprintf(stderr, 
         "<gdb ascii mode interrupted (was waiting for echo of %c (0x%x))>\n",
                                  isprint(c) ? c : ' ', c);
              }
              column = 0;
              next_char_is_command = false;
              this_line_is_a_command = false;
            } else {
              waiting_for_echoed_character = true;
              wait_for_echoed_character(c);
              waiting_for_echoed_character = false;
            }
          }
        }
      }
    }
    ui_fflush(stdout);
  }
  spllo();	/* This will reset handler to be request_quit() */
#ifndef _WIN32
  signal(SIGTSTP, SIG_DFL);
#endif
  restore_terminal_mode();
  restore_remote_mode();
}

void
_initialize_motomode()
{
  add_com("bug", class_run, bug_command,
     "Return to 188BUG, the ROM monitor on the Motorola 188 board.");
  add_com("pass", class_run, pass_command, 
     "Pass ascii between user and target (for talking to 188BUG).");
}
