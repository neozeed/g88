/*
 * Changes for m88k by David Biesack (biesack@dg-rtp.dg.com)
 * 		   and Paul Reilly   (reilly@dg-rtp.dg.com)
 * February 1989
 *
 * Changes for Green Hills compilers by Paul J. Gilliam (paulg@orca.wv.tek.com)
 * August 1989
 */

/*
   Copyright (C) 1986, 1987 Free Software Foundation, Inc.

   $Header: /home/bigbird/Usr.U6/robertb/tar/g88/RCS/m-m88k.h,v 1.24 91/01/14 01:20:44 robertb Exp $
   $Locker:  $
 */

/* Tektronix programming extensions to GDB, the GNU debugger.
   Copyright (C) 1989 Free Software Foundation, Inc.


This file is part of GDB. */

#ifndef m88k
#define m88k
#endif

/* define USG if you are using sys5 /usr/include's */
#define USG

/* USG systems need these */
#define vfork() fork()
#ifndef MAXPATHLEN
#define MAXPATHLEN 500
#endif

/* define this if you don't have the extension to coff that allows
 * file names to appear in the string table
 * (aux.x_file.x_foff)
 */
#ifndef TEK_HACK
#define COFF_NO_LONG_FILE_NAMES
#endif

/* turn this on when rest of gdb is ready */
/* #define IEEE_FLOAT */
#define NBPG NBPC		/* number of bytes per page */
#define UPAGES USIZE		/* number of pages in kernel per-user area */

#define HAVE_TERMIO

#ifdef __GNUC__
#define alloca __builtin_alloca
#endif

/* Get rid of any system-imposed stack limit if possible.  */

#define SET_STACK_LIMIT_HUGE

/* Define this if the C compiler puts an underscore at the front
   of external names before giving them to the linker.  */

#define NAMES_HAVE_UNDERSCORE

/* Specify debugger information format.  */

/* We assume that if we are not building w/ DG COFF header files
   that we are building for DBX format. */

#ifdef	USEDGCOFF
#define COFF_FORMAT
#else
#define READ_DBX_FORMAT
#endif

/* number of traps that happen between exec'ing the shell
 * to run an inferior, and when we finally get to
 * the inferior code.  This is 2 on most implementations.
 */
#define START_INFERIOR_TRAPS_EXPECTED 2

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* Code to advance PC across any function entry prologue instructions
   to reach some "real" code.  Never seems to be used. */

#define SKIP_PROLOGUE(frompc)   {}

/* Immediately after a function call, return the saved pc.
   Can't always go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame)  (read_register (SRP_REGNUM))

/* This is the amount to subtract from u.u_ar0
   to get the offset in the core file of the register values.
   Not used here; we use (struct ptrace_user) instead. */

/* #define KERNEL_U_ADDR 0 */

/* Address of end of stack space.  */

#define STACK_END_ADDR ((CORE_ADDR)0xfff00000)	/* Guess -rcb 6/90 */

/* Stack grows downward.  */

#define INNER_THAN <

#ifdef TEK_HACK
#define BAD_STACK_ADDR(addr) ((CORE_ADDR)(addr) >= STACK_END_ADDR || \
			      (CORE_ADDR)(addr) < read_register (SP_REGNUM))
#endif /* TEK_HACK */

/* Sequence of bytes for breakpoint instruction.
   Instruction 0xF000D081 is 'tb0 0,r0,129'.  This always traps. */
#define BREAKPOINT {0xF0, 0x00, 0xD0, 0x81}
#define REMOTE_BREAKPOINT {0xf0, 0, 0xd0, 0xfe }; /* rcb */

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.
   Under the Tektronix kernel, when a process executes the BREAKPOINT
   instruction above (trap 129), the kernel backs off the instruction pointers,
   so that $SNIP = trap-time $SXIP and $SFIP = trap-time $SNIP.  This sets
   things up so that a return will execute the instruction where the breakpoint
   was.  However, the "standard" (BCS defined) traps 504 through 511 do not
   cause this backup.  If gdb is ever changed to use the standard traps, it
   must be taught to back up the instruction pointers itself.  To do this,
   IT IS NOT SUFFICIENT to define DECR_PC_AFTER_BREAK! */

#define DECR_PC_AFTER_BREAK 0

/* Nonzero if instruction at PC is a return instruction.
   'jmp r1' or 'jmp.n r1' is used to return from a subroutine.
   This does not seem to be used. */

#define ABOUT_TO_RETURN(pc) \
	((read_memory_integer ((pc), 4) & ~0x400) == (0xf400c000 + SRP_REGNUM))

/* Return 1 if P points to an invalid floating point value.
   LEN is the length in bytes -- not relevant on the 386.  */

#define INVALID_FLOAT(p, len) IEEE_isNAN((p),(len))

/* Largest integer type */
#define LONGEST long

/* Name of the builtin type for the LONGEST type above. */
#define BUILTIN_TYPE_LONGEST builtin_type_long

/* Say how long (ordinary) registers are.  */

/* We run the 88k in big-endian mode */
#define BITS_BIG_ENDIAN	

#define REGISTER_TYPE long

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

/* General registers r0 through r31 have register numbers 0 through 31. */
#define R0_REGNUM	0	/* r0 */
#define R1_REGNUM	1	/* r1:    subroutine return pointer */
#define R2_REGNUM	2	/* r2 */
#define R3_REGNUM	3	/* r3 */
#define SRP_REGNUM	1	/* r1:    subroutine return pointer */
#define FP_REGNUM	30	/* r30:   frame pointer */
#define SP_REGNUM	31	/* r31:   stack pointer */

/* Number of general-purpose (integer) registers. */
#define NUM_GENERAL_REGS 32

/* Phony register: synthesized program counter.
   When debugging a Unix process, this has special semantics when read
   from (gets the $SNIP) and when written to (sets the $SNIP and $SFIP).
   On the simulator, this is the actual program counter because the simulator
   implements only a non-pipelined machine. */
#define SYNTH_PC_REGNUM 32	/* synthesized program counter */

/* Integer unit control registers.
   Some of these are unavailable when debugging a Unix process.
   Access to PSR_REGNUM is changed to EPSR_REGNUM so that they make sense. */
#define PID_REGNUM	33	/* cr0:   precessor identification register */
#define PSR_REGNUM	34	/* cr1:   process status reg (actually EPSR) */
#define EPSR_REGNUM	35	/* cr2:   exception-time process status reg */
#define SSBR_REGNUM	36	/* cr3:   shadow scoreboard register */
#define SXIP_REGNUM	37	/* cr4:   shadow execute instruction pointer */
#define SNIP_REGNUM	38	/* cr5:   shadow next instruction pointer */
#define SFIP_REGNUM	39	/* cr6:   shadow fetched intruction pointer */
#define VBR_REGNUM	40	/* cr7:   vector base register */
#define DMT0_REGNUM	41	/* cr8:   transaction register 0 */
#define DMD0_REGNUM	42	/* cr9:   data register 0 */
#define DMA0_REGNUM	43	/* cr10:  address register 0 */
#define DMT1_REGNUM	44	/* cr11:  transaction register 0 */
#define DMD1_REGNUM	45	/* cr12:  data register 0 */
#define DMA1_REGNUM	46	/* cr13:  address register 0 */
#define DMT2_REGNUM	47	/* cr14:  transaction register 0 */
#define DMD2_REGNUM	48	/* cr15:  data register 0 */
#define DMA2_REGNUM	49	/* cr16:  address register 0 */
#define SR0_REGNUM	50	/* cr17:  supervisor storage register 0 */
#define SR1_REGNUM	51	/* cr18:  supervisor storage register 1 */
#define SR2_REGNUM	52	/* cr19:  supervisor storage register 2 */
#define SR3_REGNUM	53	/* cr20:  supervisor storage register 3 */

/* Floating point unit control registers.
   Some of these are unavailable when debugging a Unix process. */
#define FPECR_REGNUM	54	/* fcr0:  FPU exception cause register */
#define FPHS1_REGNUM	55	/* fcr1:  FPU source 1 operand high register */
#define FPLS1_REGNUM	56	/* fcr2:  FPU source 1 operand low register */
#define FPHS2_REGNUM	57	/* fcr3:  FPU source 2 operand high register */
#define FPLS2_REGNUM	58	/* fcr4:  FPU source 2 operand low register */
#define FPPT_REGNUM	59	/* fcr5:  FPU precise operation type register */
#define FPRH_REGNUM	60	/* fcr6:  FPU result high register */
#define FPRL_REGNUM	61	/* fcr7:  FPU result low register */
#define FPIT_REGNUM	62	/* fcr8:  FPU imprecise operation type reg */
#define FPSR_REGNUM	63	/* fcr62: FPU status register */
#define FPCR_REGNUM	64	/* fcr63: FPU control register */

/* Registers external to the CPU. */
#define CEIMR_REGNUM	65	/* Blackbird CE interrupt mask register */

/* Simulator-only registers. */
#define COMEFROM_REGNUM	66	/* address of last jmp/jmp.n/jsr/jsr.n */
#define MEMBRK_REGNUM	67	/* stop on write to this location */
#define STACKBASE_REGNUM 68	/* stop if r31 < this value */
#define RAMSIZE_REGNUM	69	/* amount of RAM in simulated machine */

/* Phony register: synthesized frame pointer.
   If the function manipulates a real frame pointer in r30, this register
   mirrors that.  If the function does not maintain a frame pointer in r30,
   this register contains the value that the function would have maintained
   (computed by examining the stack pointer and the first few instructions
   of the function).  Can be read but not written. */
#define SYNTH_FP_REGNUM 70	/* synthesized frame pointer */

/* Number of machine registers */
#define NUM_REGS	71

/* True if regno is a register that parameters are passed in.  -rcb */
#define	IS_PARAM_REG(regno) (2<=(regno)&&(regno)<=9)

/* In the instruction pointer registers (SXIP, SNIP, SFIP), the low two bits
   are status bits: */
#define IP_VALID 2
#define IP_EXCEPTION 1

extern char reg_access[NUM_REGS];

/* Values in reg_access[] */
#define USER_REGACC 1
#define REMOTE_REGACC 2
#define SIM_REGACC 4

/* Register access macros */
#define CAN_REGACC_USER(regnum)   (reg_access[regnum]&USER_REGACC)
#define CAN_REGACC_REMOTE(regnum) (reg_access[regnum]&REMOTE_REGACC)
#define CAN_REGACC_SIM(regnum)    (reg_access[regnum]&SIM_REGACC)

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers' (and then some).  */
#define REGISTER_BYTES (NUM_REGS * sizeof(REGISTER_TYPE))

/* Total amount of space needed to store the general registers. */
#define GENERAL_REGISTER_BYTES (NUM_GENERAL_REGS * sizeof(REGISTER_TYPE))

/* Index within `registers' of the first byte of the space for
   register n.  */

#define REGISTER_BYTE(n) ((n)*sizeof(REGISTER_TYPE))

/* Number of bytes of storage in the actual machine representation
   for register n.  */

#define REGISTER_RAW_SIZE(n) (sizeof(REGISTER_TYPE))

/* Number of bytes of storage in the program's representation
   for register n. */

#define REGISTER_VIRTUAL_SIZE(n) (sizeof(REGISTER_TYPE))

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE (sizeof(REGISTER_TYPE))

/* Largest value REGISTER_VIRTUAL_SIZE can have. */

#define MAX_REGISTER_VIRTUAL_SIZE (sizeof(REGISTER_TYPE))

/* Nonzero if register N requires conversion
   from raw format to virtual format.  */

#define REGISTER_CONVERTIBLE(n) 0

/* Convert data from raw format for register regnum
   to virtual format for register regnum.  */

#define REGISTER_CONVERT_TO_VIRTUAL(regnum,from,to) \
	{bcopy ((from), (to), sizeof(REGISTER_TYPE));}

/* Convert data from virtual format for register regnum
   to raw format for register regnum.  */

#define REGISTER_CONVERT_TO_RAW(regnum,from,to) \
	{bcopy ((from), (to), sizeof(REGISTER_TYPE));}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(n) (builtin_type_int)

/* Store the address of the place in which to copy the structure that the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(addr, sp) \
  { (sp) -= sizeof (addr);		\
    write_memory ((sp), &(addr), sizeof (addr)); }

/* Extract from an array regbuf containing the (raw) register state
   a function return value of type type, and copy that, in virtual format,
   into valbuf.  */

#define EXTRACT_RETURN_VALUE(type,regbuf,valbuf) \
  {bcopy ((regbuf) + 8, (valbuf), TYPE_LENGTH (type));}

/* Extract from an array regbuf containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

/*** @@@ This is WRONG -- it's pointing to r0!  -=- andrew@frip.wv.tek.com */

#define EXTRACT_STRUCT_VALUE_ADDRESS(regbuf) (*(int *)(regbuf))


/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */

/* GreenHillsStyle: The Green Hills compiler need not produce a frame pointer.
   The VAL field in the stabs for variables allocated on the stack are
   offsets relative to the frame pointer, even if one is not used.  This is
   a 68k style frame pointer, so these offsets are negative.  A GreenHillsStyle
   frame looks like this:

                       +----------------------------+
                       |    return address (R1)     |
                       +----------------------------+
   pseudo FP ---->     |          not used          | (1)
                       +----------------------------+
                       |       local variables      |
                       |            . . .           |
                       +----------------------------+
                       |      register save area    | (2)
                       |            . . .           |
                       +----------------------------+
                       | arguments for next routine | (3)
                       |            . . .           |
                       +----------------------------+

   (1) Sometimes the Green Hills compiler generates a real frame pointer.
       This can happen when the user requests it (via flag -ga) or when the
       compiler feels it must.  If this subroutine generates a real frame
       pointer (R30), then the previous value of R30 will be saved in this
       field.

   (2) This area of the frame is used to "home" the argument registers and to
       save other registers, if needed.  The first eight words of
       arguments to a subroutine are passed in registers R2-R9.  When the
       compiler must use these registers, either for arguments to the next
       subroutine or for some other purpose, it will save these registers in
       this area of the frame.  This is called "homing."  If the compiler
       needs to save any other registers, this area may be used for that
       purpose.
       If the compiler never needs to home the argument registers, or save any
       other register, this portion of the frame is omitted, otherwise it is
       always at least 32 bytes.

   (3) Storage is allocated for all the arguments
       on the stack.  Only those arguments that could not be passed in
       registers will actually be stored here.  This portion of the frame is
       created just before the next subroutine is called.

   NOTE: Under some conditions, the compiler produces no frame at all.  Even
         so, the VAL fields of the stab entries for arguments allocated on
         the stack will be relative to the pseudo frame pointer.

   GnuStyle: The GNU compiler(s) always produce frame pointers.  The
   VAL field of the stabs for variables allocated on the stack are
   offsets relative to the frame pointer (R30) and are positive.  A
   GnuStyle frame looks like this:

                       +----------------------------+
                       |      local variables       |
                       |           . . .            |
                       +----------------------------+
                       |    return address (R1)     |
                       +----------------------------+
   real FP (R30) ----> |     previous FP (R30)      |
                       +----------------------------+
                       | arguments for next routine | (1)
                       |           . . .            |
                       +----------------------------+

   (1) When a subroutine is entered, the entire frame is created, with 36 bytes
       allocated for this area.  This is enough to home all the argument
       registers (as specified by the OCS).  It will extend this area, if
       needed, before a lower level subroutine is called.  */

/* FRAME_CHAIN takes a frame's nominal address and produces the frame's
   chain-pointer -- the address of the next older stack frame.

   FRAME_CHAIN_COMBINE takes the chain pointer and the frame's nominal address
   and produces the nominal address of the caller frame.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.
   In that case, FRAME_CHAIN_COMBINE is not used.  */

#define FRAME_CHAIN(thisframe) m88k_frame_chain(thisframe)

#define EXTRA_FRAME_INFO \
	CORE_ADDR subr_entry; \
        unsigned framesize; /* Size of this stack frame in bytes -rcb 3/90 */\
	CORE_ADDR sp; /* current stack pointer for this frame */

#define INIT_EXTRA_FRAME_INFO(fid) (m88k_init_extra_frame_info (fid))

/* If we are cross-debugging, be more open-minded about what a valid frame
   looks like.  We can't assume anything about the address, not even that
   it is in the downloaded program's text region. -rcb */
extern remote_debugging;
#define FRAME_CHAIN_VALID(chain, thisframe) \
	((chain) != 0 && \
	 (remote_debugging || \
	  (FRAME_SAVED_PC (thisframe) >= first_object_file_end)))

#define FRAME_CHAIN_COMBINE(chain, thisframe) (chain)

/* Define other aspects of the stack frame.  */

#define FRAME_SAVED_PC(frame) (m88k_find_return_address(frame))

#ifdef USEDGCOFF
#define FRAME_ARGS_ADDRESS(fi)  \
	synthesize_previous_sp((fi)->sp, (fi)->subr_entry, (fi)->pc)
#else
#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)
#endif

#ifdef USEDGCOFF
#define FRAME_LOCALS_ADDRESS(fi)  \
	synthesize_previous_sp((fi)->sp, (fi)->subr_entry, (fi)->pc)
#else
#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)
#endif

/* Return number of args passed to a frame.
   Can return -1, meaning no way to tell.  */

#define FRAME_NUM_ARGS(numargs, fi)  (numargs) = -1

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 8

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special: the address we return
   for it IS the sp for the next frame.  Look at the comments in frame.h near
   the definition of frame_saved_regs for all the details. */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs) \
  m88k_frame_find_saved_regs ((frame_info), &(frame_saved_regs))

/* This next macro is similar to the previous one, except it does the job
   for only one register. The address is just returned. */

#define FRAME_FIND_SINGLE_SAVED_REG(fi, regnum) \
  m88k_frame_find_single_saved_reg ((fi), (regnum))

/* Things needed for making the inferior call functions.  */

/* Push an empty stack frame, to record the current PC, etc.  */

#define PUSH_DUMMY_FRAME { m88k_push_dummy_frame (); }

/* Discard from the stack the innermost frame, restoring all registers.  */

#define POP_FRAME  { m88k_pop_frame (); }

/*
	or.u	r10,r0,hi16(funcaddr)
	or	r10,r10,lo16(funcaddr)
	jsr	r10
	tb0	0,r0,129	* break point *
*/

#define CALL_DUMMY {0x5d400000,0x594a0000,0xf400c80a,0xf000d081}
#define REMOTE_CALL_DUMMY {0x5d400000,0x594a0000,0xf400c80a,0xf000d0fe}

#define CALL_DUMMY_LENGTH 16

#define CALL_DUMMY_START_OFFSET 0  /* Start execution at beginning of dummy */

/* Insert the specified number of args and function address
   into a call sequence of the above form stored at DUMMYNAME.  */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, type)   \
	{ \
        int addr; \
	addr = (fun); \
	/* copy upper 16 bits of function address */ \
	bcopy(&addr,(char *)(&(dummyname)[0])+2, sizeof(short)); \
	/* copy lower 16 bits of function address */ \
	bcopy((char *)(&addr)+2,(char *)(&(dummyname)[1])+2, sizeof(short)); \
	}
