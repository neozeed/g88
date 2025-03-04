/*
 * Header file for the UTek 88000 simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/sim_interface.h,v 1.30 90/08/07 10:20:15 robertb Exp $
 */

#ifndef _TYPE_H
#ifndef _TYPES_
#ifndef __sys_types_h
typedef unsigned long u_long;
typedef unsigned short u_short;
typedef unsigned char u_char;
#endif
#endif
#endif

/*
 * Number of bytes per 88000 page.
 */
#define PAGESIZE   (4096)

/*
 * The general purpose registers of the 88000.  regs[0] is r0,
 * regs[1] is r1, etc.
 */
extern u_long regs[];

/*
 * The Integer Unit's control and status registers.  sfu0_regs[0] is
 * the pid, etc.  Storage is allocated for 64 registers to make life
 * easier for the execution and decoding logic even though
 * there are only 13 such registers in the 88000.
 */
extern u_long sfu0_regs[];

/*
 * The Floating Point Unit's control and status registers.
 */
extern u_long sfu1_regs[];

/*
 * The simulated 88000 instruction pointer.
 */
extern u_long ip;

/*
 * Executes 88000 instructions starting at ip 
 * until a breakpoint is encountered.  Return code
 * indicates reason for returning.
 */
extern int sim();

/*
 * sim() return codes.
 */

#define NONE        0    /* shouldn't happen,there is no reason for return*/
#define BREAKPOINT  1    /* tb0 0,r0,131 was executed.            */
#define INTERRUPT   2    /* User hit control-C or program wanted to stop. */
#define HOSED       3    /* Too many ERR exceptions, bailing out.         */
#define CAUGHT_EXCEPTION 4   /* An exception was caught.                      */
#define	MPSWITCH	5 /* Time to switch to next processor */

/*
 * This array controls the behavior of the simulator when an exception
 * occurs.  If exception i happens, sim_catch_exception[i] is tested.  If
 * it is non-zero, the simulator returns control to the front end with
 * the code "CAUGHT_EXCEPTION".
 */
extern char sim_catch_exception[];

/*
 * The last exception that happened.
 */
extern int sim_exception;

/*
 * Initializes all of simulated memory and the registers.
 */
extern void sim_init();

/*
 * These are the memory access routines.  They read or write a byte,
 * a half word (16 bits), or a word.
 *
 * The write routines return -1 if not successful, 0 otherwise.
 * The first parameter of each routine is the 88000 physical address
 * to be read or written.  The write routines take the value to
 * be written in the second parameter.
 */
extern u_char read_sim_b();
extern u_short read_sim_h();
extern u_long read_sim_w();

extern int write_sim_b();
extern int write_sim_h();
extern int write_sim_w();

/*
 * This takes a 88k physical address and a length in bytes.  It
 * returns a pointer to the simulation memory for the physical
 * address.  If a buffer starting at this address and extending for
 * for the length specified by the second parameter doesn't fit
 * in continguous host memory a 0 is returned.
 */
extern u_char *sim_ptr();

/*
 * These read and write files directly with simulation memory.
 */
extern int sim_readfile();
extern int sim_writefile();

/*
 * This is like 'bzero', except that it takes a physical 88000
 * address and zero's simulation memory.
 */
extern int sim_zeromem();

/*
 * This is set to one if there was an error in the last call to
 * one of the routines above, zero otherwise.
 */
extern int sim_errno;

extern void sim_io_print();

extern void sim_io_print_devices();

/*
 * When this is set to a non-zero value the simulator
 * will stop after the next branch instruction where the 
 * branch is taken.
 */
extern int sim_interrupt_flag;

#define INT_FRONT_END    1
#define INT_DEVICE       2
#define INT_INTERNAL     4
#define INT_RESET        8
#define INT_SIGNAL       16
#define INT_SINGLESTEP   32
#define	INT_MPSWITCH	 64

/*
 * These must be provided by the simulator front-end.
 */
extern sim_printf();

extern sim_putchar();

extern sim_getchar();

/*
 * This will print a trace of IO operations.
 */
extern void sim_io_trace();

extern int *sim_verbose_ptr;

/*
 * Translates kernel logical addresses to physical addresses.
 */
extern int sim_v_to_p();

/*
 * Prints a map of simulated physical memory use.
 */
extern void sim_printmap();

/*
 * This is true (non-zero) when the simulator is running in
 * continuous mode (not single-stepping), false when it is not.
 */
extern int sim_in_execution;

/*
 * 88000 exception codes.
 */
#define E_NONE      (-1)
#define E_RST       (0)
#define E_INT       (1)
#define E_CACC      (2)
#define E_DACC      (3)
#define E_MA        (4)
#define E_OPC       (5)
#define E_PRV       (6)
#define E_BND       (7)
#define E_IDE       (8)
#define E_IOV       (9)
#define E_ERR       (10)

#define E_FP        (114)
#define E_FPIM      (115)

#ifndef M_USER
#define M_USER       (0)
#endif

#ifndef M_SUPERVISOR
#define M_SUPERVISOR (1)
#endif

#ifndef M_CURMODE
#define M_CURMODE    (2)
#endif

/*
 * This is the record that is written to the execution trace file.
 *
 * The 'lowdata' field is the low 6 bits of the instruction, if
 * the record is of an instruction fetch.  Its the low 6 bits
 * of the register contents before a store if a store is being 
 * traced.  If a load is being traced it is the low 6 bits of
 * the register contents after the load is executed.
 *
 * The global, cacheinhibit, and writethru bits are the or of
 * bits in the area, segment, and page descriptors.
 */
struct etrace {
    u_long physical_address;
    u_short      size : 3,   /* 1 - byte, 2 - half, 4 - word                 */
                fetch : 1,   /* True if this is an instruction fetch.        */
               usmode : 1,   /* True if a supervisor space access, 0 for user*/
                 lock : 1,   /* True if this is an xmem, 0 otherwise         */
                write : 1,   /* True if this is a st or xmem, 0 otherwise    */
               global : 1,   /* or of global bits in area, segment, page desc*/
         cacheinhibit : 1,   /* or of cacheinhibit bits                      */
            writethru : 1,   /* or of writethru bits                         */
              lowdata : 6;   /* Bottom 6 bits of data in transaction         */

};

/*
 * Gdb-front end routine to get value of a convenience variable.
 */
extern int varvalue();

/* From defs.h, we don't want to include that whole file here.  If 
   the user types a control-C, he wants the command to stop. */

#define QUIT	{ if (quit_flag) request_quit(); }
extern quit_flag;

/* The number of the currently selected processor */
int selected_processor;

/* Changes the selected processor */
extern void select_processor();
