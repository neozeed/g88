/* Low level interface to ptrace, for GDB when running on the Motorola 88000.
   Copyright (C) 1988 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/m88k-dep.c,v 1.48 90/12/10 21:18:29 robertb Exp $
   $Locker:  $

   Tektronix programming extensions to GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc. */


#include <sys/types.h>
#include <stdio.h>
#include <sys/dir.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <fcntl.h>

#ifdef USEDGCOFF	/* -rcb 6/90 */
#include "dghdr/a.out.h"
#else
#ifdef NON_NATIVE
#include "a.out.gnu.h"  /* -rcb 8/20/90 */
#else
#include <a.out.h>
#endif
#endif

#include <sys/file.h>
#include <sys/stat.h>

#include "defs.h"
#include "param.h"
#include "m88k-opcode.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "ui.h"

/* Offsets to interesting registers in struct ptrace_user. */
#define R0_OFFSET   ((int)&((struct ptrace_user *)0)->pt_r0)
#define PSR_OFFSET  ((int)&((struct ptrace_user *)0)->pt_psr)
#define SXIP_OFFSET ((int)&((struct ptrace_user *)0)->pt_sigframe.sig_sxip)
#define SNIP_OFFSET ((int)&((struct ptrace_user *)0)->pt_sigframe.sig_snip)
#define SFIP_OFFSET ((int)&((struct ptrace_user *)0)->pt_sigframe.sig_sfip)
#define FPSR_OFFSET ((int)&((struct ptrace_user *)0)->pt_fpsr)
#define FPCR_OFFSET ((int)&((struct ptrace_user *)0)->pt_fpcr)

extern struct block *block_for_pc();

extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];
extern int simulator;

REGISTER_TYPE read_register();
static CORE_ADDR m88k_find_saved_fp();
CORE_ADDR m88k_find_return_address();
REGISTER_TYPE read_hard_register();
static unsigned long find_reg_val();
CORE_ADDR synthesize_previous_sp();
static CORE_ADDR synthesize_previous_fp();
static CORE_ADDR find_pc_function_addr();
CORE_ADDR m88k_frame_find_single_saved_reg();

#ifdef GHSFORTRAN
#ifdef READ_DBX_FORMAT
fix_fortran_lengths() { ui_badnews(-1, "fix_fortran_lengths called"); }
fortran_length() { ui_badnews(-1, "fortran_length() called"); }
#endif
#endif

/*
 * serr -- return the system-defined string for the current system error code.
 * If the error code is out of bounds, make up a string.
 */
char *
serr()
{
  static char errbuf[80];

  if (0<errno && errno<sys_nerr)
    {
      return sys_errlist[errno];
    }
  else
    {
      (void) sprintf(errbuf, "unknown system error (%d)", errno);
      return errbuf;
    }
}

/*
 * call_ptrace -- do a ptrace; if it fails, print an error message.
 */
int
call_ptrace (request, pid, arg3, arg4)
  int request, pid, arg3, arg4;
{
  int val;

  if (remote_debugging) {
    fatal("ptrace(%d, %d, %d, %d) called when cross-debugging",
	    request, pid, arg3, arg4);
  }
  errno = 0;
  val = ptrace (request, pid, arg3, arg4);
  if (errno) {
    error("ptrace(%d,%d,%d,%d) failure: %s.", request, pid, arg3, arg4, serr());
  }
  return val;
}

/*
 * kill_inferior -- if there is an inferior process, kill it and wait for it.
 */
kill_inferior ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  call_ptrace (8, inferior_pid, 0, 0);

  /*
   * The following wait is sloppy; error checking should be done.
   */
  wait (0);

  inferior_died ();
  ui_endSubProc();
}

/*
 * kill_inferior_fast -- kill inferior as gdb exits.
 * Like kill_inferior but doesn't call inferior_died().
 */
kill_inferior_fast ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  ptrace (8, inferior_pid, 0, 0);	/* not call_ptrace! */
  wait (0);
  ui_endSubProc();
}

/*
 * fetch_inferior_registers -- get the register state.
 */
fetch_inferior_registers ()
{
  forget_registers();	/* clear register cache */
}

/*
 * read_inferior_memory -- copy from inferior memory to our memory.
 * Copy "len" bytes from inferior's memory starting at "memaddr" to debugger
 * memory starting at "myaddr".
 * On failure (cannot read from inferior, usually because address is out
 * of bounds) returns the value of errno, else returns 0.
 */
int
read_inferior_memory (memaddr, myaddr, len, usmode)
  CORE_ADDR memaddr;
  char *myaddr;
  int len;
{
  register int i;
  register CORE_ADDR addr;
  register int count;
  register int *buffer;

  /* Round starting address down to longword boundary. */
  addr = memaddr & -sizeof(int);

  /* Round ending address up; get number of longwords that makes. */
  count = (memaddr + len - addr + sizeof(int) - 1) / sizeof(int);

  /* Allocate buffer of that many longwords.  */
  buffer = (int *) alloca (count * sizeof(int));

  /* Read all the longwords */
  for (i = 0; i < count; i++, addr += sizeof(int))
    {
      errno = 0;
      if (remote_debugging)
	buffer[i] = remote_fetch_word (addr, usmode);
      else
	buffer[i] = ptrace (1, inferior_pid, addr, 0);
      if (errno)
	return errno;
    }

  /* Copy appropriate bytes out of the buffer.  */
  bcopy ((char *) buffer + (memaddr & (sizeof(int) - 1)), myaddr, len);
  return 0;
}

/*
 * write_inferior_memory -- copy from our memory to inferior memory.
 * Copy "len" bytes of data from debugger memory at "myaddr" to inferior's
 * memory at "memaddr".
 * On failure (cannot write the inferior) returns the value of errno, else
 * returns 0.
 */
int
write_inferior_memory (memaddr, myaddr, len)
  CORE_ADDR memaddr;
  char *myaddr;
  int len;
{
  register int i;
  register CORE_ADDR addr;
  register int count;
  register int *buffer;

  /* Round starting address down to longword boundary.  */
  addr = memaddr & -sizeof(int);

  /* Round ending address up; get number of longwords that makes.  */
  count = (memaddr + len - addr + sizeof(int) - 1) / sizeof(int);

  /* Allocate buffer of that many longwords.  */
  buffer = (int *) alloca (count * sizeof(int));

  /* If the start address in inferior memory is not word-aligned, then fetch
     the word that's there now and insert it in our buffer before doing the
     bcopy, so that, when we write fullwords into the inferior, this partial
     word will not be destroyed. */
  if (addr & (sizeof(int)-1))
    {
      if (remote_debugging)
	buffer[0] = remote_fetch_word (addr, M_NORMAL);
      else
	{
	  errno = 0;
	  buffer[0] = ptrace (1, inferior_pid, addr, 0);
	  if (errno)
	    return errno;
	}
    }

  /* Likewise, prefill the last word if necessary. */
  if ((memaddr+len)&(sizeof(int)-1) && count>1)
    {
      if (remote_debugging)
	buffer[count-1] = remote_fetch_word (addr + (count-1)*sizeof(int), M_NORMAL);
      else
	{
	  errno = 0;
	  buffer[count-1] = ptrace (1, inferior_pid,
				    addr + (count-1)*sizeof(int), 0);
	  if (errno)
	    return errno;
	}
    }

  /* Copy data to be written over corresponding part of our buffer. */
  bcopy (myaddr, (char *) buffer + (memaddr & (sizeof(int) - 1)), len);

  /* Write the entire buffer to the inferior. */
  for (i = 0; i < count; i++, addr += sizeof(int))
    {
      errno = 0;
      if (remote_debugging)
	remote_store_word (addr, buffer[i]);
      else
	ptrace (4, inferior_pid, addr, buffer[i]);
      if (errno)
	return errno;
    }

  return 0;
}

/* Work with core dump and executable files, for GDB.
   This code would be in core.c if it weren't machine-dependent. */

#ifndef N_TXTADDR
#define N_TXTADDR(hdr) 0
#endif

#ifndef N_DATADDR
#define N_DATADDR(hdr) hdr.a_text
#endif

/* Hook for `exec_file_command' command to call.  */

extern void (*exec_file_display_hook) ();

/* File names of core file and executable file.  */

extern char *corefile;
extern char *execfile;

/* Descriptors on which core file and executable file are open.
   Note that the execchan is closed when an inferior is created
   and reopened if the inferior dies or is killed.  */

extern int corechan;
extern int execchan;

/* Last modification time of executable file.
   Also used in source.c to compare against mtime of a source file.  */

extern int exec_mtime;

/* Virtual addresses of bounds of the two areas of memory in the core file.  */

extern CORE_ADDR data_start;
extern CORE_ADDR data_end;
extern CORE_ADDR stack_start;
extern CORE_ADDR stack_end;

/* Virtual addresses of bounds of two areas of memory in the exec file.
   Note that the data area in the exec file is used only when there is no
   core file. */

extern CORE_ADDR text_start;
extern CORE_ADDR text_end;
extern CORE_ADDR bss_start;	/* For cross-debugging  -rcb 12/89 */
extern CORE_ADDR entry_point;	/* For cross-debugging  -rcb 12/89 */
extern u_long bss_size;		/* For cross-debugging  -rcb 12/89 */


extern CORE_ADDR exec_data_start;
extern CORE_ADDR exec_data_end;

/* Address in executable file of start of text area data.  */

extern int text_offset;

/* Address in executable file of start of data area data.  */

extern int exec_data_offset;

/* Address in core file of start of data area data.  */

extern int data_offset;

/* Address in core file of start of stack area data.  */

extern int stack_offset;

/* various coff data structures */

#ifdef COFF_FORMAT
extern FILHDR file_hdr;
extern SCNHDR text_hdr;
extern SCNHDR data_hdr;
#endif

#ifndef COFF_FORMAT
#ifndef AOUTHDR
#define AOUTHDR struct exec
#endif
#endif

extern AOUTHDR exec_aouthdr;

extern char objname[];		/* For cross-debugging  -rcb 12/89 */

/* a.out header of exec file.  */


/*
 * core_file_command -- establish a core dump file.
 */
core_file_command (filename, from_tty)
  char *filename;
  int from_tty;
{
#ifdef NOTDEF
  register int regno;
  struct ptrace_user u;
#define U_WORD(offset) (*(int *)((char *)(&u)+(offset)))

  if (remote_debugging)
    ui_badnews(-1, "Can't look at a core file when attached.");

  /* Discard all vestiges of any previous core file,
     and mark data and stack spaces as empty.  */

  if (corefile)
    free (corefile);
  corefile = 0;

  if (corechan >= 0)
    close (corechan);
  corechan = -1;

  data_start = 0;
  data_end = 0;
  stack_start = STACK_END_ADDR;
  stack_end = STACK_END_ADDR;

  forget_registers();

  /* If there is no new core file to establish, that's all. */
  if (!filename)
    {
      if (from_tty)
	printf ("No core file now.\n");
      return;
    }

  if (have_inferior_p ())
    ui_badnews (-1,
  "To look at a core file, you must first kill the inferior with \"kill\"."
		);
  corechan = open (filename, O_RDONLY, 0);
  if (corechan < 0)
    perror_with_name (filename);

  /* Make the filename into a pathname and store at corefile. */
  if (filename[0] == '/')
    corefile = savestring (filename, strlen (filename));
  else
    corefile = concat (current_directory, "/", filename);

  /* Read the ptrace_user structure. */
  if (myread (corechan, &u, sizeof u) < 0)
    perror_with_name (filename);

  data_start = u.pt_o_data_start;
  data_end = data_start +  u.pt_dsize;
  stack_start = stack_end -  u.pt_ssize;
  data_offset = (int) u.pt_dataptr;
  stack_offset = data_offset + u.pt_dsize;

  /* Read the register values out of the core file and store them where
     `read_register' will find them.
     Start with the 32 general purpose (integer unit) registers. */
  for (regno = 0; regno < NUM_GENERAL_REGS; ++regno)
    write_hard_register(regno, U_WORD(R0_OFFSET + regno*sizeof(REGISTER_TYPE)));

  /* Read the other user-accessible registers. */
  write_hard_register(EPSR_REGNUM, U_WORD(PSR_OFFSET));
  write_hard_register(SXIP_REGNUM, U_WORD(SXIP_OFFSET));
  write_hard_register(SNIP_REGNUM, U_WORD(SNIP_OFFSET));
  write_hard_register(SFIP_REGNUM, U_WORD(SFIP_OFFSET));
  write_hard_register(FPSR_REGNUM, U_WORD(FPSR_OFFSET));
  write_hard_register(FPCR_REGNUM, U_WORD(FPCR_OFFSET));

  /* Set up the current frame based on the frame pointer and PC. */
  set_current_frame (create_new_frame (read_register (SYNTH_FP_REGNUM),
				       read_register(SYNTH_PC_REGNUM)));
  select_frame (get_current_frame (), 0);
  validate_files ();
#undef U_WORD
#endif
}

#ifdef COFF_FORMAT
/*
 * exec_file_command -- do an exec command.
 */
exec_file_command (filename, from_tty)
  char *filename;
  int from_tty;
{
  int val;
  int aout_hdrsize;
  int num_sections;

  /* Eliminate all traces of old exec file.
     Mark text segment as empty.  */

  if (execfile)
    free (execfile);
  execfile = 0;
  data_start = 0;
  data_end -= exec_data_start;
  text_start = 0;
  text_end = 0;
  exec_data_start = 0;
  exec_data_end = 0;
  if (execchan >= 0)
    close (execchan);
  execchan = -1;

  /* Now open and digest the file the user requested, if any.  */
  if (filename)
    {
      /* Open the executable file. */
      execchan = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0, &execfile);
      if (execchan < 0)
	perror_with_name (filename);

      if (read_file_hdr (execchan, &file_hdr) < 0)
	ui_badnews(-1, "\"%s\": not in executable format.", execfile);

      aout_hdrsize = file_hdr.f_opthdr;
      num_sections = file_hdr.f_nscns;

      if (read_aout_hdr (execchan, &exec_aouthdr, aout_hdrsize) < 0)
	ui_badnews(-1, "\"%s\": can't read optional aouthdr", execfile);

      if (read_section_hdr (execchan, _TEXT, &text_hdr, num_sections) < 0)
	ui_badnews(-1, "\"%s\": can't read text section header", execfile);

      if (read_section_hdr (execchan, _DATA, &data_hdr, num_sections) < 0)
	ui_badnews(-1, "\"%s\": can't read data section header", execfile);

      text_start = exec_aouthdr.text_start;
#ifdef SWALLOW_WHOLE
      initialize_text(execchan, text_hdr.s_scnptr, text_hdr.s_size);
#else
      initialize_text(text_hdr.s_scnptr, text_hdr.s_size);
#endif
      text_end = text_start + exec_aouthdr.tsize;
      text_offset = text_hdr.s_scnptr;
      exec_data_start = exec_aouthdr.data_start;
      exec_data_end = exec_data_start + exec_aouthdr.dsize;
      exec_data_offset = data_hdr.s_scnptr;
      data_start = exec_data_start;
      data_end += exec_data_start;
      bss_size = exec_aouthdr.bsize;	/* For cross-debugging  -rcb 12/89 */
      entry_point = exec_aouthdr.entry;	/* For cross-debugging  -rcb 12/89 */
      bss_start = exec_data_end;	/* For cross-debugging  -rcb 12/89 */
      exec_mtime = file_hdr.f_timdat;

      validate_files ();
    }
  else if (from_tty)
    printf ("No exec file now.\n");

  /* Tell display code (if any) about the changed file name.  */
  if (exec_file_display_hook)
    (*exec_file_display_hook) (filename);

  /* Tell cross-debugger about the changed file name. */
  strcpy(objname, filename);
}
#else
void
exec_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;

  /* Eliminate all traces of old exec file.
     Mark text segment as empty.  */

  if (execfile)
    free (execfile);
  execfile = 0;
  data_start = 0;
  data_end -= exec_data_start;
  text_start = 0;
  text_end = 0;
  exec_data_start = 0;
  exec_data_end = 0;
  if (execchan >= 0)
    close (execchan);
  execchan = -1;

  /* Now open and digest the file the user requested, if any.  */

  if (filename)
    {
      execchan = openp (getenv ("PATH"), 1, filename, O_RDONLY, 0,
			&execfile);
      if (execchan < 0)
	perror_with_name (filename);

      {
	struct stat st_exec;
	val = myread (execchan, &exec_aouthdr, sizeof (AOUTHDR));

	if (val < 0)
	  perror_with_name (filename);

        text_start = N_TXTADDR (exec_aouthdr);
#ifdef SWALLOW_WHOLE
      initialize_text(execchan, N_TXTOFF(exec_aouthdr), exec_aouthdr.a_text);
#else
      initialize_text(N_TXTOFF(exec_aouthdr), exec_aouthdr.a_text);
#endif
        exec_data_start = N_DATADDR (exec_aouthdr);
	text_offset = N_TXTOFF (exec_aouthdr);
	exec_data_offset = N_TXTOFF (exec_aouthdr) + exec_aouthdr.a_text;

	text_end = text_start + exec_aouthdr.a_text;
        exec_data_end = exec_data_start + exec_aouthdr.a_data;
	data_start = exec_data_start;
	data_end += exec_data_start;

      bss_size = exec_aouthdr.a_bss;	/* For cross-debugging  -rcb 12/89 */
      entry_point = exec_aouthdr.a_entry;/* For cross-debugging  -rcb 12/89 */
      bss_start = exec_data_end;	/* For cross-debugging  -rcb 12/89 */

	fstat (execchan, &st_exec);
	exec_mtime = st_exec.st_mtime;
      }

      validate_files ();
    }
  else if (from_tty)
    printf ("No exec file now.\n");

  /* Tell display code (if any) about the changed file name.  */
  if (exec_file_display_hook)
    (*exec_file_display_hook) (filename);

  /* Tell cross-debugger about the changed file name. */
  strcpy(objname, filename);
}
#endif

/*
 * IEEE_isNAN -- return 1 if argument is Not A Number, else 0.
 * fp points to a single precision OR double precision floating point value;
 * len is the number of bytes, either 4 or 8.
 * Returns 1 iff fp points to a valid IEEE floating point number.
 * Returns 0 if fp points to a denormalized number or a NaN
 */
int
IEEE_isNAN(fp, len)
  int *fp;
  int len;
{
  register int first_word;
  register int exponent;

  first_word = *fp;
  switch (len)
    {
      case 4:
	exponent = first_word & 0x7f800000;
	return exponent==0x7f800000 || exponent==0 && first_word!=0;

      case 8:
	exponent = first_word & 0x7ff00000;
	return exponent==0x7ff00000 ||
	       exponent==0 && (first_word!=0 || fp[1]!=0);

      default:
	return 1;
    }
}

/*
 * bad_text_addr -- return 1 if address if not a valid instruction location.
 * If we are cross-debugging, it is ok to have the stack below the text, but
 * make sure that the address is word-aligned.
 */
static int
bad_text_addr(addr)
  CORE_ADDR addr;
{
  return
    addr == 0 ||
    addr > 0xfff00000 ||
    addr & 3 ||
    (!remote_debugging && addr > read_register (SP_REGNUM));
}

#define GROWS_STACK_IMM(inst)	  (((inst) & 0xffff0000) == 0x67ff0000)
#define GROWS_STACK_REG2(inst)	  (((inst) & 0xffffffe0) == 0xf7ff6400)
#define SHRINKS_STACK_IMM(inst)	  (((inst) & 0xffff0000) == 0x63ff0000)
#define SHRINKS_STACK_REG2(inst)  (((inst) & 0xffffffe0) == 0xf7ff6000)

#define	SETS_FRAME_POINTER_IMM(inst)  (((inst) & 0xffff0000) == 0x63df0000)
#define	SETS_FRAME_POINTER_REG2(inst) (((inst) & 0xffffffe0) == 0xf7df6000)
#define	MOVE_R31_TO_R30(inst) ((inst) == 0xf7c0581f)

#define IS_BPT(inst)		  (remote_debugging ?  \
				((inst) == 0xf000d0fe) : ((inst) == 0xf000d1ff))
#define IS_BR(inst)		  m88k_is_br(inst)
#define LOADS_REG32_BOT(reg,inst) (((inst)>>16) == (reg|(reg<<5)|0x5800))
#define LOADS_REG32_TOP(reg,inst) (((inst) >> 16) == ((reg << 5) | 0x5c00))
#define LOADS_REG16(reg,inst)	  (((inst) >> 16) == ((reg << 5) | 0x5800))
#define STORES_REG_TO_STACK(reg, inst) \
				  (((inst) >> 16) == ((reg << 5) | 0x241f))
#define GET_IMM16(inst)		  ((short)(inst))
#define GET_U_IMM16(inst)	  ((inst) & 0xffff)
#define GET_DEST(inst)		  (((inst) >> 21) & 0x1f)
#define GET_S1(inst)		  (((inst) >> 16) & 0x1f)
#define GET_S2(inst)		  ((inst) & 0x1f)
/*
 * Test to see if the instruction is a bsr, bsr.n, jsr, or jsr.n.;
 * i.e., a function call.
 */
#define ISCALL(i)		  (((i) & 0xf8000000) == 0xc8000000 || \
				   ((i) & 0xfc00fbe0) == 0xf400c800)

/*
 * Test to see if the instruction is a bsr.n or jsr.n.;
 * i.e., a function call.
 */
#define ISCALL_N(i)		  (((i) & 0xfc000000) == 0xcc000000 || \
				   ((i) & 0xfc00fce0) == 0xf400cc00)

/*
 * m88k_is_br -- return 1 if instruction is a branch (transfers control).
 * Does not consider traps to be branches.
 */
static int
m88k_is_br(inst)
  int inst;
{
  /* Here are all the 88k instructions that can transfer control (change the
     instruction pointers), together with tests for them:
	br:	((inst)>>27)==0x18   11000
	bsr:	((inst)>>27)==0x19   11001
	bb0:	((inst)>>27)==0x1a   11010
	bb1:	((inst)>>27)==0x1b   11011
	bcnd:	((inst)>>27)==0x1d   11101
	jmp:	((inst)&0xfffffbe0)==0xf400c000
	jsr:	((inst)&0xfffffbe0)==0xf400c800
	rte:	(inst)==0xf400fc00
	tb0:	((inst)&0xfc00fe00)==0xf000d000
	tb1:	(((inst)&0xfc00fe00)==0xf000d800 && (inst)&0x1f00)
	tbnd:	((inst)>>21)==0x7c0 ||
		((inst)&0xffe0ffe0)==0xf400f800
	tcnd:	((inst)&0xfc00fe00)==0xf000e800
   */
  register int i;

  /* Test for br, bsr, bb0, bb1, and bcnd. */
  i = inst>>27;
  if ((i & 0x1c) == 0x18 || i==0x1d)
    return 1;

  /* Test for jmp and jsr. */
  if ((inst & 0xfffff3e0) == 0xf400c000)
    return 1;

#ifdef NOTDEF
  /* Don't test for traps.
     This routine is used to see if an instruction in a function prolog
     is a transfer of control.  Under UTek V, system calls return to the
     following instructions without modifying r1 or r30, the registers we're
     interested in. */

  /* Test for tb0 or tb1.
     But mask out the instruction "tb1 <b>,r0,<vec>", which will never transfer.
     */
  if ((inst & 0xfc00f600) == 0xf000d000 &&
      (inst & 0xfc1ffe00) != 0xf000d800)
    return 1;

  /* Test for tbnd and tcnd. */
  if ((inst>>21) == 0x7c0 || (inst&0xffe0ffe0) == 0xf400f800 ||
      (inst&0xfc00fe00) == 0xf000e800)
    return 1;
#endif

  /* Test for rte. */
  if (inst == 0xf400fc00)
    return 1;

  return 0;
}

/* True if the block was compiled with gcc. */
int block_gcc_compiled_p(b)
  struct block *b;
{
  if (BLOCK_COMPILED(b) == gcc_compiled) return 1;
  if (b->language == language_c && varvalue("gcc_compiled")) return 1;
  return 0;
}

/*
 * m88k_frame_chain -- get a frame's chain pointer (link to next older frame).
 * Given the address of a frame, return the "chain-pointer".  For a GnuStyle
 * frame or for GreenHills with -ga, just return the saved frame pointer.
 * Otherwise the previous frame pointer must be synthesized.
 */
FRAME_ADDR
m88k_frame_chain (thisframe)
  FRAME thisframe; /* address of a frame stucture */
{
  CORE_ADDR called_at;
  struct block *b;
  CORE_ADDR assumed_sp;
  CORE_ADDR parent;
  CORE_ADDR addr, newfp;

  /* First get the address of the instruction that called this subroutine.
     (This might be 4 bytes off due to a delayed branch, but it doesn't
     matter for our purposes here.) */

  called_at = m88k_find_return_address (thisframe);
  if (called_at==0 || bad_text_addr (called_at)) {
    if (remote_debugging) {
      return NULL_CORE_ADDR;
    }
    ui_badnews (-1, "m88k_frame_chain: can't get back to parent");
  }

  /* If the calling routine was compiled with gcc or with GHS cc -ga, then it
     left a frame pointer in FP_REGNUM.  In this case, if the called routine
     (the one into which called_at points) was also compiled in this way, then
     it stored that frame pointer in its frame.  If both of these are true,
     pick up that frame pointer.

     Otherwise either the caller didn't maintain a frame pointer or the called
     routine didn't save it, so try to synthesize a frame pointer. */

  b = block_for_pc (called_at);

  newfp = NULL_CORE_ADDR;
  if (b == (struct block *)0 || block_gcc_compiled_p(b) || 
      BLOCK_COMPILED(b) == ghs_ga_compiled) {
    newfp = m88k_find_saved_fp (thisframe);
  }
  if (newfp == NULL_CORE_ADDR || newfp == INVALID_CORE_ADDR) {
    newfp = synthesize_previous_fp(synthesize_previous_sp(thisframe->sp, 
                                                          thisframe->subr_entry,
                                                          thisframe->pc), 
                                   called_at);
  }

  if (newfp == thisframe->frame) {
    if (remote_debugging) {
      return NULL_CORE_ADDR;
    }
    ui_fprintf(stderr, "m88k_frame_chain: prev fp == newfp, sp=0x%x pc=0x%x\n",
                                           thisframe->sp, thisframe->pc);
    ui_badnews(-1,     "                  prev sp=0x%x fp=0x%x, prev pc=0x%x", 
                                  synthesize_previous_sp(thisframe->sp, 
                                                         thisframe->subr_entry,
                                                         thisframe->pc), 
                                  newfp, 
                                  called_at);
  }
  return newfp;
}

/*
 * m88k_frame_find_saved_regs -- fill in the frame_save_register structure.
 */
m88k_frame_find_saved_regs (frame_id, saved_regs)
  FRAME frame_id;	/* address of a frame stucture */
  struct frame_saved_regs *saved_regs;
{
  register int i;

  saved_regs->regs[0] = 0;	/* r0 = 0 on 88k */
  for (i = 1; i < NUM_GENERAL_REGS; i++)
    saved_regs->regs[i] = m88k_frame_find_single_saved_reg (frame_id, i);
}

/*
 * m88k_frame_find_single_saved_reg -- find the address of a saved register.
 * Find the address, or saved register number, of a single register.  If the
 * register was altered and not saved, return INVALID_CORE_ADDR.  If "regnum"
 * is the SP, return its actual value. If a register was not saved and not
 * altered, just return its number.
 */
CORE_ADDR
m88k_frame_find_single_saved_reg (frame_id, regnum)
  FRAME frame_id;		/* Points to frame structure               */
  int regnum;			/* Register to track the value of          */
{
  CORE_ADDR reg_addr_so_far;	/* Location of regnum when pc is at addr    */
  CORE_ADDR assumed_sp;		/* Stack pointer value when pc is at addr   */
  CORE_ADDR addr;		/* Address of current instruction           */
  CORE_ADDR end_addr;		/* Address of last instruction to examine   */
  CORE_ADDR my_entry;		/* Entry point of function that create frame*/
  unsigned long inst;		/* Current instruction                      */
  INSTAB *instab_entry;		/* Points to info on current instruction    */
  int dest;			/* Destination register number of curr inst */
  int src1;			/* Source 1 register number of current inst */
  int src2;			/* Source 2 register number of current inst */
  int cnt = 5000;		/* Limit on # of instructions to examine    */

  if (regnum >= NUM_GENERAL_REGS) {
    return INVALID_CORE_ADDR;
  }
  if (regnum == 0) {
    return 0;
  }

  /* I don't know whether this is correct, to return the register number
     when we are passed a nil frame.  I used to have an error message
     here.  It was displayed after I tried to print a register after
     downloading a target.  I haven't been able to reproduce the bug,
     but if it was calling this routine to find out where the current
     value of a register is, this should fix it.  -rcb 4/90 

     And now I've changed it back.  find_saved_register(), the main
     caller, needed to check for nil frames anyway, so we should
     never see one here. */

  if (frame_id == 0) {
    ui_badnews(-1, "m88k_frame_find_single_saved_reg: nil frame"); 
  }

  /* The stack pointer is special, return its value, not where it is stored */
  if (regnum == SP_REGNUM) {
    if (frame_id->prev) {
      return frame_id->prev->sp;
    } else {
      return synthesize_previous_sp (frame_id->sp, 
                                     frame_id->subr_entry, 
                                     frame_id->pc);
    }
  }

  /* If this is the current frame, look at every instruction right
     up to the one we just executed.  Otherwise look at every instruction
     up to and including the one just before the bsr or jsr that
     caused us to enter a new function.  If we looked at the
     bsr or jsr, we would decide that all of the parameter registers
     are junk and sometimes incorrectly return INVALID_CORE_ADDR. */

  if (frame_id->pc == read_register(SYNTH_PC_REGNUM)) {
    end_addr = frame_id->pc - 4;
  } else {
    end_addr = frame_id->pc - 8;
  }

  reg_addr_so_far = regnum;
  my_entry = frame_id->subr_entry;
  assumed_sp = synthesize_previous_sp(frame_id->sp, my_entry, end_addr);

  for (addr = my_entry ; addr <= end_addr ; addr += 4) {
    if (cnt-- == 0) {
      ui_badnews(-1, "Giving up in m88k_frame_find_single_saved_reg");
    }

    inst = fetch_instruction(addr);

    /* Non-preserved registers are assumed to be trashed by subroutine
       calls.   -rcb 3/90 */

    if (ISCALL(inst)) {

      /* If this function suspended with a bsr.n or jsr.n, look at the
         instruction in the branch delay slot instead of at the bsr.n or
         jsr.n */

      if (ISCALL_N(inst) && addr == end_addr) {
        inst = fetch_instruction(addr + 4);
      } else {
        if (reg_addr_so_far < 14 || reg_addr_so_far > 25) {
          return INVALID_CORE_ADDR;
        }
        continue;
      }
    }

    instab_entry = (INSTAB *) m88k_inst_find (inst);
    dest = GET_DEST (inst);
    src1 = GET_S1 (inst);
    src2 = GET_S2 (inst);

    /* Adjust our notion of the stack pointer's value if the current
       instruction manipulates the stack pointer register. */

    if ((dest == SP_REGNUM) && (src1 == SP_REGNUM)) {
      if (instab_entry->effect & GROWS) {
        if (instab_entry->op3.type == REG) {
          assumed_sp -= find_reg_val (GET_S2 (inst), addr - 4, my_entry);
        } else {
          assumed_sp -= GET_IMM16 (inst);
          continue;
        }
      } else if (instab_entry->effect & SHRINKS) {
        if (instab_entry->op3.type == REG) {
          assumed_sp += find_reg_val (GET_S2 (inst), addr - 4, my_entry);
        } else {
          assumed_sp += GET_IMM16 (inst);
          continue;
	}
      }
    }

    /* See if the contents of "reg_addr_so_far" is being saved in 
       another register.  If it is moved into a preserved register, assume
       that the value stays there and return the number of this
       preserved register.  If is moved into another register, keep 
       tracking it. */

    if ((instab_entry->effect & MOVES) && 
        (src1 == 0) && 
        (src2 == reg_addr_so_far)) {

      reg_addr_so_far = dest;
      if (14 <= reg_addr_so_far && reg_addr_so_far <= 25) {
        return reg_addr_so_far;
      }
      continue;
    }

    /* If the destination register isn't "regnum", skip this instruction.  If
       the instruction is marked as a double, then also check the destination
       register plus one (modulo 32) */

    if (dest != reg_addr_so_far) {
      if (! (instab_entry->effect & DBL)) {
        continue;
      }
      dest = (dest + 1) % 32;
      if (dest != reg_addr_so_far) {
        continue;
      }
    }

    /* See if the instruction saves the value that we are tracking
       on the stack */

    if (instab_entry->effect & SAVES) {
      if (src1 == SP_REGNUM) {
        reg_addr_so_far = assumed_sp;
        if (instab_entry->op3.type == REG) {
          reg_addr_so_far += find_reg_val (GET_S2 (inst), addr - 4, my_entry);
        } else {
          reg_addr_so_far += GET_IMM16 (inst);
        }

        if (dest == (GET_DEST(inst) + 1)) {
          reg_addr_so_far += 4;
        }
        return reg_addr_so_far;

      } else if (src1 == FP_REGNUM) {
        reg_addr_so_far = frame_id->frame;
        if (instab_entry->op3.type == REG) {
          reg_addr_so_far += find_reg_val (GET_S2 (inst), addr - 4, my_entry);
        } else {
          reg_addr_so_far += GET_IMM16 (inst);
        }

        if (dest == (GET_DEST(inst) + 1)) {
          reg_addr_so_far += 4;
        }
        return reg_addr_so_far;
      }
    }

    /* See if the instruction alters "reg_addr_so_far". */

    if (instab_entry->effect & ALTERS) {
      return INVALID_CORE_ADDR;
    }
  }
  return reg_addr_so_far;
}

/*
 * m88k_init_extra_frame_info -- fill in the extra frame info.
 */
m88k_init_extra_frame_info (frame_id)
  FRAME frame_id;	/* address of a frame structure */
{
  frame_id->subr_entry = find_pc_function_addr (frame_id->pc);
  if (frame_id->next) {
    frame_id->sp = synthesize_previous_sp(frame_id->next->sp, 
                                          frame_id->next->subr_entry,
                                          frame_id->next->pc);
  } else {
    frame_id->sp = read_register(SP_REGNUM);
  }
}

/* synthesize_previous_fp -- synthesize the previous frame pointer.
   Synthesize the previous frame pointer given the "current" stack pointer
   and the "current" pc.  */
static CORE_ADDR
synthesize_previous_fp(current_sp, current_pc)
  CORE_ADDR current_sp;
  CORE_ADDR current_pc;
{
  CORE_ADDR subr_entry;
  CORE_ADDR addr;
  unsigned long inst;
  int cnt = 1000;

  subr_entry = find_pc_function_addr (current_pc);
  if (bad_text_addr (subr_entry)) {
    return NULL_CORE_ADDR;
  }

  /* Step backward through the subroutine, adjusting our idea of the frame
     pointer.  The tacit assumptions being made here boggle the mind. */

  for (addr = subr_entry; addr < current_pc - 4; addr += sizeof inst) {
    if (remote_debugging) {
      if (cnt-- == 0) {
        error("Giving up in synthesize_previous_fp");
      }
    }
    /* By calling 'fetch_intruction()' we get instructions from the text
       section of the file.  This is faster than doing a ptrace() and won't
       show us the breakpoint instructions that we've put in the text
       that is in memory being executed. */

    inst = fetch_instruction(addr);

    if (SETS_FRAME_POINTER_IMM(inst)) {
      return synthesize_previous_sp(current_sp, addr, current_pc) + 
             GET_IMM16(inst);
    }
    if (SETS_FRAME_POINTER_REG2(inst)) {
      return synthesize_previous_sp(current_sp, addr, current_pc) + 
             find_reg_val(GET_S2(inst), addr - 4, subr_entry);
    }
    /* Added 10/24/90 by rcs to handle DG generated code */
    if (MOVE_R31_TO_R30(inst)) {
      return current_sp;
    }
  }
  /* We couldn't find an assignment to the frame pointer, r30, so
     it is probably a Green Hill's-generated frame.  Assume that
     the frame pointer is the caller's stack pointer minus 8. */
  return synthesize_previous_sp(current_sp, subr_entry, current_pc) - 8;
}

/*
 * synthesize_previous_sp -- synthesize the previous stack pointer.
 * Synthesize the previous stack pointer given the "current" stack pointer
 * and the "current" pc.
 */
CORE_ADDR
synthesize_previous_sp(current_sp, previous_pc, current_pc)
  CORE_ADDR current_sp;
  CORE_ADDR previous_pc;	/* Usually the subroutine entry point */
  CORE_ADDR current_pc;
{
  CORE_ADDR subr_entry, previous_sp;
  CORE_ADDR addr;
  unsigned long inst;
  int cnt = 5000;


  /* Step backward through the subroutine, adjusting our idea of the stack
     pointer.  The tacit assumptions being made here boggle the mind. */
  previous_sp = current_sp;
  for (addr = current_pc-4; addr >= previous_pc; addr -= 4) {
    if (remote_debugging) {
      if (cnt-- == 0) {
        error("Giving up in synthesize_previous_sp");
      }
    }
    /* By calling 'fetch_intruction()' we get instructions from the text
       section of the file.  This is faster than doing a ptrace() and won't
       show us the breakpoint instructions that we've put in the text
       that is in memory being executed. */

    inst = fetch_instruction(addr);

    if (GROWS_STACK_IMM (inst)) {
      previous_sp += GET_IMM16 (inst);
    } else if (GROWS_STACK_REG2 (inst)) {
      previous_sp += find_reg_val (GET_S2 (inst), addr - 4, subr_entry);
    } else if (SHRINKS_STACK_IMM (inst)) {
      previous_sp -= GET_IMM16 (inst);
    } else if (SHRINKS_STACK_REG2 (inst)) {
      previous_sp -= find_reg_val (GET_S2 (inst), addr - 4, subr_entry);
    }
  }

  /* We now have the value of the stack pointer when the subroutine was
     entered. */
  return previous_sp;
}

struct symbol *find_pc_function();

/* given a pc, find the address of the function containing that pc */

static CORE_ADDR
find_pc_function_addr (pc)
     CORE_ADDR pc;
{
  struct symbol *func;
  int misc_func;

  func = find_pc_function (pc);

  if (!func)
    {
      misc_func = find_pc_misc_function (pc);
      if (misc_func == -1)
	return -1;

      return misc_function_vector[misc_func].address;
    }
  return BLOCK_START (SYMBOL_BLOCK_VALUE (func));
}

/* find the value in register "reg" by working backward in the code, from
   "start" to "end", looking for where "reg" gets loaded with an immediat
   value.  We assume that this is done in one of two ways: (1) an
   "or.u <reg>,R0,Imm16" for the top half (if any) and an
   "or <reg>,<reg>,Imm16" for the bottom half or (2) an "or <reg>,R0,Imm16".
   The first way is for loading constants larger then 16 bits. */

static unsigned long
find_reg_val (reg, start, end)
     int reg;
     CORE_ADDR start;
     CORE_ADDR end;
{
  int value = 0; /* zero meens "I don't know" */
  unsigned long inst;

  /* A little error checking ...  We can take this out later */

  if (start < end) {
    ui_badnews (-1, "internal: find_reg_val: start(%x) < end(%x)", start, end);
  }
  if (reg & ~0x1f) {
    ui_badnews (-1, "internal: find_reg_val: bad reg = %d", reg);
  }

  for (; (start >= end); start -= 4) {
    inst = fetch_instruction(start);
    if (LOADS_REG32_BOT (reg, inst)) {
      value |= GET_U_IMM16 (inst);
      break;
    }
    if (LOADS_REG16 (reg, inst)) {
      return GET_U_IMM16 (inst);
    }
  }
  for (start -= 4; (start >= end); start -= 4) {
    inst = fetch_instruction(start);
    if (LOADS_REG32_TOP (reg, inst)) {
       value |= GET_U_IMM16 (inst) << 16;
       break;
    }
  }
  return value;
}

#ifdef ATTACH_DETACH

/*
 * This attach/detach code is compliant with the BCS, except that the question
 * of whether the attached process continues to execute after ptrace() is up
 * in the air.
 * Most of this code was taken from sparc-dep.c.
 *   -=- andrew@frip.wv.tek.com
 */

#define PTRACE_ATTACH 128
#define PTRACE_DETACH 129

extern int attach_flag;

/* Start debugging the process whose number is PID.  */

int
attach (pid)
     int pid;
{
  errno = 0;
  call_ptrace (PTRACE_ATTACH, pid, 0, 0);
  attach_flag = 1;

  /*
   * The BCS says that the kernel does not stop the process,
   * so the debugger must.
   */
  if (kill(pid, SIGTRAP) == -1) {
    perror_with_name("kill");
  }

  return pid;
}

/* Stop debugging the process whose number is PID
   and continue it with signal number SIGNAL.
   SIGNAL = 0 means just continue it.  */

detach (signal)
     int signal;
{
  errno = 0;
  call_ptrace (PTRACE_DETACH, inferior_pid, 1, 0);
  attach_flag = 0;
  if (signal && kill(inferior_pid, signal) == -1) {
    perror_with_name("kill");
  }
  ui_endSubProc();
}
#endif /* ATTACH_DETACH */

/* The following is used to save and restore registers during function
   calls.  A stack is necessary since the functions themselves may contain
   break points allowing the user to nest these calls.  */

static struct d_stack
{
	struct d_stack *next;
	int sxip, snip, sfip;
	int reg[FP_REGNUM];
	int sp;
} *d_top = NULL;

m88k_push_dummy_frame ()
{
  register struct d_stack *new_top;
  register int i;

  new_top = (struct d_stack *) malloc(sizeof(*new_top));

  if (new_top == NULL)
    ui_badnews (-1, "Cannot create dummy frame for procedure call.");
  new_top->next = d_top;
  d_top = new_top;
  fetch_inferior_registers();
  d_top->sxip = read_register(SXIP_REGNUM);
  d_top->snip = read_register(SNIP_REGNUM);
  d_top->sfip = read_register(SFIP_REGNUM);

  for (i = 1; i <= FP_REGNUM; i++)
    d_top->reg[i-1] = read_register(i);
  d_top->sp = read_register(SP_REGNUM);
}

m88k_pop_frame ()
{
  register struct d_stack *tmp;
  register int i, sp;

  sp = read_register(SP_REGNUM);

  while (d_top)
    {
      /* In case of a long jump we have to peel back the stack */

      if (d_top->sp >= sp)
	break;
      tmp = d_top;
      d_top = d_top->next;
      free(tmp);
    }
  if (d_top)
    {
      write_register(SNIP_REGNUM, d_top->snip);
      write_register(SFIP_REGNUM, d_top->sfip);
      for (i = 1; i <= FP_REGNUM; i++)
	write_register(i, d_top->reg[i-1]);
      write_register(SP_REGNUM, d_top->sp);
      tmp = d_top;
      d_top = d_top->next;
      free(tmp);
    }
  else
    ui_badnews (-1, "Cannot pop empty dummy stack.");
}

m88k_clear_dummy_stack ()
{
  register struct d_stack *tmp;

  while (d_top)
    {
      tmp = d_top;
      d_top = d_top->next;
      free(tmp);
    }
}

int m88k_is_dummy_active()
{
  return d_top != 0;
}

#ifdef SWALLOW_WHOLE
static int *instruction_buf;	/* array of executable instructions */
static int max_inst_idx;	/* index (+ 1) of last instruction */

/*
 * Read in entire text segment into buffer pointed to by instruction_buf.
 */
initialize_text (chan, start, size)
  int chan;
  int start;
  int size;
{
  /* Make sure we are on a word boundary. */
  size &= ~(sizeof(int)-1);
  size += sizeof(int);
  instruction_buf = (int *)xmalloc(size);

  if (lseek (chan, (long)start, 0) < 0)
    ui_badnews(-1, "Could not read in text");
  if (myread (chan, (char *)instruction_buf, size) != size)
    ui_badnews(-1, "Could not read in text");
  max_inst_idx = size / sizeof(int);
}

/*
 * Return instruction at given address, 0 if address is out of range.
 */
int
fetch_instruction(addr)
CORE_ADDR addr;
{
  int idx = (addr - text_start) / sizeof(int);
  return idx >= 0 && idx < max_inst_idx ? instruction_buf[idx] : 0;
}
#else	/* not SWALLOW_WHOLE */
#define CHUNKSIZE	4*0x1000 /* must be a one followed by CHUNKBITS of
				    zeroes */
#define CHUNKBITS	14	/* the number of bits to the right of the one
				   in CHUNKSIZE */
static struct text_chunk
{
	int buf[CHUNKSIZE];
} **instruction_buf;	/* an array of buffers whose size is CHUNKSIZE */
static int number_of_chunks = 0;
static CORE_ADDR text_loc;

/*
 * Allocate an array of pointers to text_chunks, set number_of_chunks.
 */
initialize_text (start, size)
  int start;
  int size;
{
  clear_instruction_buf();
  text_loc = start;
  size &= ~(CHUNKSIZE-1);
  size >>= CHUNKBITS;
  number_of_chunks = size + 1;	/* might be 1 more than needed,
				   but who cares? */
  instruction_buf = (struct text_chunk **)
		    xmalloc(number_of_chunks * (sizeof(struct text_chunk *)));
  bzero(instruction_buf, number_of_chunks*(sizeof(struct text_chunk *)));
}

/*
 * Return the instruction at the given address.  If the text has not been
 * read into corresponding chunk, do so now.
 */
int
fetch_instruction(addr)
  CORE_ADDR addr;
{
  register int offset = addr - text_start;
  register int chunk_idx = offset >> CHUNKBITS;
  register int instr_idx = (offset & (CHUNKSIZE - 1)) >> 2;

  if (offset < 0 || chunk_idx >= number_of_chunks) {
    return 0;
  }

  if (instruction_buf[chunk_idx] == NULL) {
    /* Read in chunk */
    reopen_exec_file(); /* just in case it has been closed */
    instruction_buf[chunk_idx] = (struct text_chunk *)xmalloc(CHUNKSIZE);

    if (lseek (execchan, (long)text_loc+chunk_idx*CHUNKSIZE, 0) < 0) {
      ui_badnews(-1, "seek failed reading instr at 0x%x", addr);
    }

    if (myread (execchan, (char *)(instruction_buf[chunk_idx]->buf),
                    CHUNKSIZE) < 0) {
      ui_badnews(-1, "read failed reading instr at 0x%x", addr);
    }
  }
  return instruction_buf[chunk_idx]->buf[instr_idx];
}

/*
 * Free up all space allocated for text chunks, reset instruction_buf back to
 * NULL and number_of_chunks to 0.
 */
static
clear_instruction_buf()
{
  register int i;

  if (instruction_buf)
    {
      for (i = 0; i < number_of_chunks; i++)
	if (instruction_buf[i])
	  free(instruction_buf[i]);
      number_of_chunks = 0;
      free(instruction_buf);
      instruction_buf = NULL;
    }
}
#endif /* not SWALLOW_WHOLE */

/*
 * Low-level register read/write interface to Unix user processes.
 * This replaces a tumor of machine semi-independent routines.
 */

/* Register access table.
   One byte per register, indexed by the various XXX_REGNUM values.
   Within each byte:
     USER_REGACC is set if that register can be fetched for a user process;
     REMOTE_REGACC is set if that register can be fetched from a remote target;
     SIM_REGACC is set if that register can be fetched from a simulated machine.
   */
char reg_access[NUM_REGS] = {
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r0 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r1 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r2 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r3 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r4 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r5 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r6 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r7 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r8 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r9 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r10 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r11 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r12 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r13 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r14 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r15 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r16 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r17 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r18 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r19 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r20 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r21 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r22 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r23 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r24 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r25 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r26 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r27 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r28 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r29 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r30 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* r31 */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* synth_pc */
		      REMOTE_REGACC | SIM_REGACC,	/* pid */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* psr */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* epsr */
		      REMOTE_REGACC | SIM_REGACC,	/* ssbr */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* sxip */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* snip */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* sfip */
		      REMOTE_REGACC | SIM_REGACC,	/* vbr */
		      REMOTE_REGACC | SIM_REGACC,	/* dmt0 */
		      REMOTE_REGACC | SIM_REGACC,	/* dmd0 */
		      REMOTE_REGACC | SIM_REGACC,	/* dma0 */
		      REMOTE_REGACC | SIM_REGACC,	/* dmt1 */
		      REMOTE_REGACC | SIM_REGACC,	/* dmd1 */
		      REMOTE_REGACC | SIM_REGACC,	/* dma1 */
		      REMOTE_REGACC | SIM_REGACC,	/* dmt2 */
		      REMOTE_REGACC | SIM_REGACC,	/* dmd2 */
		      REMOTE_REGACC | SIM_REGACC,	/* dma2 */
		      REMOTE_REGACC | SIM_REGACC,	/* sr0 */
		      REMOTE_REGACC | SIM_REGACC,	/* sr1 */
		      REMOTE_REGACC | SIM_REGACC,	/* sr2 */
		      REMOTE_REGACC | SIM_REGACC,	/* sr3 */
		      REMOTE_REGACC | SIM_REGACC,	/* fpecr */
		      REMOTE_REGACC | SIM_REGACC,	/* fphs1 */
		      REMOTE_REGACC | SIM_REGACC,	/* fpls1 */
		      REMOTE_REGACC | SIM_REGACC,	/* fphs2 */
		      REMOTE_REGACC | SIM_REGACC,	/* fpls2 */
		      REMOTE_REGACC | SIM_REGACC,	/* fppt */
		      REMOTE_REGACC | SIM_REGACC,	/* fprh */
		      REMOTE_REGACC | SIM_REGACC,	/* fprl */
		      REMOTE_REGACC | SIM_REGACC,	/* fpit */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* fpsr */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* fpcr */
		      REMOTE_REGACC | SIM_REGACC,	/* ceimr */
				      SIM_REGACC,	/* comefrom */
				      SIM_REGACC,	/* membrk */
				      SIM_REGACC,	/* stackbase */
				      SIM_REGACC,	/* ramsize */
	USER_REGACC | REMOTE_REGACC | SIM_REGACC,	/* synthfp */
};

/* Names of registers.
   This table must correspond to the various XXX_REGNUM values defined in
   m-m88k.h. */
char *reg_names[NUM_REGS+8+5] = {
 	"r0",
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",
	"r16",
	"r17",
	"r18",
	"r19",
	"r20",
	"r21",
	"r22",
	"r23",
	"r24",
	"r25",
	"r26",
	"r27",
	"r28",
	"r29",
	"fp",
	"sp",
	"pc",
	"pid",
	"psr",
	"epsr",
	"ssbr",
	"sxip",
	"snip",
	"sfip",
	"vbr",
	"dmt0",
	"dmd0",
	"dma0",
	"dmt1",
	"dmd1",
	"dma1",
	"dmt2",
	"dmd2",
	"dma2",
	"sr0",
	"sr1",
	"sr2",
	"sr3",
	"fpecr",
	"fphs1",
	"fpls1",
	"fphs2",
	"fpls2",
	"fppt",
	"fprh",
	"fprl",
	"fpit",
	"fpsr",
	"fpcr",
	"ceimr",
	"comefrom",
	"membrk",
	"stackbase",
	"ramsize",
	"synthfp",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"<err0>",	/* Only internal errors in gdb or errors in debug */
	"<err1>",	/* information from the executable will use these */
	"<err2>",	/* It beats dumping core -rcb.                    */
	"<err3>",
	"<err4>",
};

/* Offsets from the beginning of the ptrace_user structure to each register
   that is accessible from a Unix process. */
short reg_offset[NUM_REGS] = {
#ifdef NOTDEF
	R0_OFFSET + 0*sizeof(REGISTER_TYPE),	/* r0 */
	R0_OFFSET + 1*sizeof(REGISTER_TYPE),	/* r1 */
	R0_OFFSET + 2*sizeof(REGISTER_TYPE),	/* r2 */
	R0_OFFSET + 3*sizeof(REGISTER_TYPE),	/* r3 */
	R0_OFFSET + 4*sizeof(REGISTER_TYPE),	/* r4 */
	R0_OFFSET + 5*sizeof(REGISTER_TYPE),	/* r5 */
	R0_OFFSET + 6*sizeof(REGISTER_TYPE),	/* r6 */
	R0_OFFSET + 7*sizeof(REGISTER_TYPE),	/* r6 */
	R0_OFFSET + 8*sizeof(REGISTER_TYPE),	/* r8 */
	R0_OFFSET + 9*sizeof(REGISTER_TYPE),	/* r9 */
	R0_OFFSET + 10*sizeof(REGISTER_TYPE),	/* r10 */
	R0_OFFSET + 11*sizeof(REGISTER_TYPE),	/* r11 */
	R0_OFFSET + 12*sizeof(REGISTER_TYPE),	/* r12 */
	R0_OFFSET + 13*sizeof(REGISTER_TYPE),	/* r13 */
	R0_OFFSET + 14*sizeof(REGISTER_TYPE),	/* r14 */
	R0_OFFSET + 15*sizeof(REGISTER_TYPE),	/* r15 */
	R0_OFFSET + 16*sizeof(REGISTER_TYPE),	/* r16 */
	R0_OFFSET + 17*sizeof(REGISTER_TYPE),	/* r16 */
	R0_OFFSET + 18*sizeof(REGISTER_TYPE),	/* r18 */
	R0_OFFSET + 19*sizeof(REGISTER_TYPE),	/* r19 */
	R0_OFFSET + 20*sizeof(REGISTER_TYPE),	/* r20 */
	R0_OFFSET + 21*sizeof(REGISTER_TYPE),	/* r21 */
	R0_OFFSET + 22*sizeof(REGISTER_TYPE),	/* r22 */
	R0_OFFSET + 23*sizeof(REGISTER_TYPE),	/* r23 */
	R0_OFFSET + 24*sizeof(REGISTER_TYPE),	/* r24 */
	R0_OFFSET + 25*sizeof(REGISTER_TYPE),	/* r25 */
	R0_OFFSET + 26*sizeof(REGISTER_TYPE),	/* r26 */
	R0_OFFSET + 27*sizeof(REGISTER_TYPE),	/* r26 */
	R0_OFFSET + 28*sizeof(REGISTER_TYPE),	/* r28 */
	R0_OFFSET + 29*sizeof(REGISTER_TYPE),	/* r29 */
	R0_OFFSET + 30*sizeof(REGISTER_TYPE),	/* r30 */
	R0_OFFSET + 31*sizeof(REGISTER_TYPE),	/* r31 */
	-1,					/* synth_pc */
	-1,					/* pid */
	-1,					/* psr */
	PSR_OFFSET,				/* epsr */
	-1,					/* ssbr */
	SXIP_OFFSET,				/* sxip */
	SNIP_OFFSET,				/* snip */
	SFIP_OFFSET,				/* sfip */
	-1,					/* vbr */
	-1,					/* dmt0 */
	-1,					/* dmd0 */
	-1,					/* dma0 */
	-1,					/* dmt1 */
	-1,					/* dmd1 */
	-1,					/* dma1 */
	-1,					/* dmt2 */
	-1,					/* dmd2 */
	-1,					/* dma2 */
	-1,					/* sr0 */
	-1,					/* sr1 */
	-1,					/* sr2 */
	-1,					/* sr3 */
	-1,					/* fpecr */
	-1,					/* fphs1 */
	-1,					/* fpls1 */
	-1,					/* fphs2 */
	-1,					/* fpls2 */
	-1,					/* fppt */
	-1,					/* fprh */
	-1,					/* fprl */
	-1,					/* fpit */
	FPSR_OFFSET,				/* fpsr */
	FPCR_OFFSET,				/* fpcr */
#endif
	-1,					/* ceimr */
	-1,					/* comefrom */
	-1,					/* membrk */
	-1,					/* stackbase */
	-1,					/* ramsize */
	-1					/* synthfp */
};

/*
 * reg_contents[] is a write-through cache of registers.
 * If there's a core file, then it's pre-loaded,
 * otherwise it's loaded on demand.
 * reg_known[regno] is nonzero if reg_contents[regno] is valid.
 */
static REGISTER_TYPE reg_contents[NUM_REGS];
static char reg_known[NUM_REGS];

/*
 * forget_registers -- discard the cache of register contents.
 * Called when a new inferior is forked, for example.
 */
forget_registers()
{
  bzero(reg_known, sizeof reg_known);
}

/*
 * read_hard_register -- get the contents of a real register.
 */
REGISTER_TYPE
read_hard_register(regno)
  unsigned regno;
{
  if (remote_debugging)
    return remote_read_register(regno);

  if (!CAN_REGACC_USER(regno))
    ui_badnews(-1,
	"read_register(%d): register %s not accessible from user process.",
		regno, reg_names[regno]);

  /* If we haven't already fetched (or stored) this register, go get it.
     (If we're dealing with a core file, all registers have been fetched.) */
  if (!reg_known[regno])
    {
      if (!inferior_pid)
	ui_badnews(-1, "read_hard_register(%d): no inferior.", regno);
      errno = 0;
      reg_contents[regno] = ptrace (3, inferior_pid, reg_offset[regno], 0);
      if (errno)
	ui_badnews(-1, "read_hard_register(%d): ptrace failure: %s.",
		   regno, serr());
      reg_known[regno] = 1;
    }
  return reg_contents[regno];
}

/*
 * write_hard_register -- store new real register contents.
 * This routine is also called when reading a core file, to supply
 * registers from the core dump.
 */
static
write_hard_register(regno, value)
  unsigned regno;
  REGISTER_TYPE value;
{
  if (remote_debugging)
    {
      remote_write_register(regno, value);
      return;
    }

  if (!CAN_REGACC_USER(regno))
    ui_badnews(-1,
	"write_register(%d): register %s not accessible from user process.",
		regno, reg_names[regno]);

  /* Store this register value in our cache, then write it through to the
     inferior process if there is one.  (There won't be one if we're
     examining a core file.) */
  reg_contents[regno] = value;
  reg_known[regno] = 1;
  if (inferior_pid)
    {
      errno = 0;
      ptrace (6, inferior_pid, reg_offset[regno], value);
      if (errno)
	ui_badnews(-1, "write_hard_register(%d): ptrace failure: %s.",
		   regno, serr());
    }
}

/*
 * read_register -- get the contents of a real or phony register.
 */
REGISTER_TYPE
read_register(regno)
  unsigned regno;
{
  if (NUM_REGS <= regno)
    ui_badnews(-1, "read_register(%d): bad register number.", regno);

  switch (regno)
    {
      case R0_REGNUM:
	/* There's no need to fetch r0. */
	return 0;

      case SYNTH_PC_REGNUM:
	/* Reading $pc gives the address of the next instruction to execute. */
	{
	  /* Select the next instruction address in the same way as the CPU:
	     -- if SNIP has the valid bit set, that's the address;
	     -- otherwise, if SFIP has the valid bit set, that's the address;
	     -- otherwise the address is SFIP+4. */
	  register int pc;
          if (simulator) {
             return read_hard_register(SYNTH_PC_REGNUM);
          }
	  pc = read_hard_register(SNIP_REGNUM);
	  if (pc & IP_VALID)
	    return pc & ~(IP_VALID|IP_EXCEPTION);
	  pc = read_hard_register(SFIP_REGNUM);
	  if (pc & IP_VALID)
	    return pc & ~(IP_VALID|IP_EXCEPTION);
	  return (pc & ~(IP_VALID|IP_EXCEPTION)) + 4;
	}

      case PSR_REGNUM:
	if (simulator) {
          return read_hard_register(PSR_REGNUM);
        } else {
          return read_hard_register(EPSR_REGNUM);
        }

      case EPSR_REGNUM:
	/* Reading from either $psr or $epsr returns $epsr. */
	return read_hard_register(EPSR_REGNUM);

      case SYNTH_FP_REGNUM:
	{
	  register int synthfp;
	  register struct block *b;
	  register int pc;

	  /* Synthesizing a frame pointer is an expensive operation.
	     Keep it in cache. */
	  if (reg_known[SYNTH_FP_REGNUM])
	    return reg_contents[SYNTH_FP_REGNUM];

	  /* If the currently executing routine was generated by the Green
	     Hills compiler without the -ga flag, or by some compiler that
	     we don't know about, then we have to synthesize a frame pointer.
	     Otherwise $synthfp is just $r30. */
	  pc = read_register(SYNTH_PC_REGNUM);
          b = block_for_pc(pc);
          if (b == (struct block *)0 || block_gcc_compiled_p(b) ||
              BLOCK_COMPILED(b) == ghs_ga_compiled) {
            synthfp = read_hard_register(FP_REGNUM);
          } else {
            synthfp = synthesize_previous_fp(read_register(SP_REGNUM), pc);
          }
	  reg_contents[SYNTH_FP_REGNUM] = synthfp;
	  reg_known[SYNTH_FP_REGNUM] = 1;
	  return synthfp;
	}

      default:
	return read_hard_register(regno);
    }
}

/*
 * write_register -- store new real or phony register contents.
 */
write_register(regno, value)
  unsigned regno;
  REGISTER_TYPE value;
{
  if (NUM_REGS <= regno)
    ui_badnews(-1, "write_register(%d): bad register number.", regno);

  switch (regno)
    {
      case R0_REGNUM:
	/* We mustn't change the value of r0. */
	break;

      case SYNTH_PC_REGNUM:
	/* If writing $pc to something other than its current value,
	   changes $snip and $sxip so that the process will resume at
	   that address. */
        if (simulator) {
          write_hard_register(SYNTH_PC_REGNUM, value);
          break;
        }
	value &= ~(IP_VALID|IP_EXCEPTION);
	if (value != read_register(SYNTH_PC_REGNUM))
	  {
	    register int new_snip = value | IP_VALID;
	    register int new_sfip = new_snip + 4;
	    ui_fprintf(stdout, "[Setting $SNIP to 0x%x, $SFIP to 0x%x]\n",
		       new_snip, new_sfip);
	    write_hard_register(SNIP_REGNUM, new_snip);
	    write_hard_register(SFIP_REGNUM, new_sfip);
	    reg_known[SYNTH_FP_REGNUM] = 0;
	  }
	break;

      case PSR_REGNUM:
	if (simulator) {
          return write_hard_register(PSR_REGNUM, value);
        } else {
          return write_hard_register(EPSR_REGNUM, value);
        }
      case EPSR_REGNUM:
	/* Writing to either $psr or $epsr changes $epsr. */
	write_hard_register(EPSR_REGNUM, value);
	break;

      case SYNTH_FP_REGNUM:
	/* Forbid writing $synthfp.
	   (I'm not sure just what we would change.) */
	ui_badnews(-1, "Can't change $synthfp.");

      case SP_REGNUM:
      case FP_REGNUM:
      case SNIP_REGNUM:
      case SFIP_REGNUM:
	/* Changing these registers can change the synthetic frame pointer,
	   so forget the cache value for SYNTH_FP. */
	reg_known[SYNTH_FP_REGNUM] = 0;

	/* fall through ... */

      default:
	write_hard_register(regno, value);
    }
}

/*
 * m88k_find_return_address -- try to discover the return address for a frame.
 * The return value is the address in the inferior's virtual space to which
 * the frame will return if we can figure it out, or 0 if we can't.
 */
CORE_ADDR
m88k_find_return_address (thisframe)
  FRAME thisframe;
{
  CORE_ADDR addr, return_address, find_saved_register_on_entry();

  if (thisframe == 0) {
    ui_badnews(-1, "m88k_find_return_address passed a nil frame id");
  }
  addr = find_saved_register_on_entry(thisframe, R1_REGNUM);
  if (addr == INVALID_CORE_ADDR) {
    if (remote_debugging) {
      return INVALID_CORE_ADDR;
    }
    ui_badnews(-1, 
      "m88k_find_return_address couldn't find address of saved return address");
  }
  if (addr < NUM_GENERAL_REGS) {
    read_register_bytes(REGISTER_BYTE(addr), 
                        &return_address, 
                        sizeof(return_address));
  } else {
    read_memory(addr, &return_address, sizeof(return_address), M_NORMAL);
  }
  return return_address;
}

/*
 * m88k_find_saved_fp -- try to discover the saved frame pointer for a frame.
 * The saved frame pointer is the caller's fp.
 * The return value is the that value if we can figure it out, or 0 if we can't.
 */
static CORE_ADDR
m88k_find_saved_fp (thisframe)
  FRAME thisframe;
{
  CORE_ADDR addr, frame_pointer, find_saved_register_on_entry();

  addr = find_saved_register_on_entry(thisframe, FP_REGNUM);
  if (addr == INVALID_CORE_ADDR) {
    return INVALID_CORE_ADDR;
  }
  if (addr < NUM_GENERAL_REGS) {
    read_register_bytes(REGISTER_BYTE (addr), 
                        &frame_pointer, 
                        sizeof(frame_pointer));
  } else {
    read_memory(addr, &frame_pointer, sizeof(frame_pointer), M_NORMAL);
  }
  return frame_pointer;
}
