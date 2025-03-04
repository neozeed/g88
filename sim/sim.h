/*
 * Header file for the UTek 88000 simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/sim.h,v 1.28 90/06/30 19:13:44 robertb Exp $
 */

#include "sim_interface.h"

/*
 * A mask that when and'd with a physical or logical 88000 address
 * gives the byte offset within the physical or logical page.
 */
#define PAGEMASK   (PAGESIZE - 1)

/*
 * Size of memory operations.  The parameter 'size' in several
 * functions has one of these values.
 */
#define	QWORD	(16)
#define DWORD   (8)
#define WORD    (4)
#define HALF    (2)
#define BYTE    (1)

/*
 * Type of memory operation.  The parameter 'mem_op_type' in several
 * functions has one of these values.
 */
#define LD      1
#define ST      2
#define XMEM    3
#define LD_U    4
#define XMEM_U  5
#define FETCH   6       /* Only used by execution trace code */

/*
 * This structure holds the decoded form of 88000 instructions.
 * They are filled in by the function in decode.c and read by
 * the execution functions in execute.c
 *
 * the norm_e_addr field points to the entry point, created with
 * a asm insert, in the main execution function for the instruction.
 *
 * dest is a 32-bit host pointer that usually points to the destination
 * register.
 *
 * s1 is a 32-bit host pointer that usually points to the source 1
 * register.
 *
 * s2 is a 32-bit host pointer that usually points to the source 2
 * register or the literal field.
 *
 * literal holds literal values that the s2 field points to in 
 * instructions that have literal operands.
 *
 * In the case of the control register manipulation instructions, the
 * literal field is a boolean that is set if the operation is not
 * permitted in user mode.
 */
struct decoded_i {
    int         (*norm_e_addr)();
    u_long    *dest;
    u_long    *s1;
    u_long    *s2;
};

/*
 * This is the data part of a memory page.  The fist element is
 * the data on the page (4096 bytes).  The second is a pointer to the
 * array of decoded instructions that corresponds to the 88000
 * instructions in the 'values' array.  If this pointer is zero
 * it means that the decoded part doesn't exist for this page yet.
 * The decoded part is allocated the first time an instruction 
 * is executed on the page.
 */
struct page {
    u_long values[PAGESIZE / 4];
    struct decoded_part *decoded_part;
};

/*
 * Has 1025 elements.  The last one is for the sim_not_decoded entry
 * that is at the end of every decoded instruction page.
 */
struct decoded_part {
    struct decoded_i decoded_i[PAGESIZE / 4 + 1];
};

/*
 * There are 2 tlb's for user space and 2 for supervisor space.
 * Of each set of 2, one is for loads and one is for stores.
 */
extern u_char **u_load_tlb[];
extern u_char **s_load_tlb[];

extern u_char **u_store_tlb[];
extern u_char **s_store_tlb[];

extern u_char *defaultpagetlb[];

/*
 * Number of entries in a 1st level tlb page (i.e., # of entries on a
 * segment page).
 */
#define SEGTLB (1024)

/*
 * Number of entries in a 2nd level tlb page.
 */
#define PAGETLB (1024)

/*
 * This is used by execute.c for implementing delayed branches.  
 * It is a global copy of the local variable 'r_delayed_p'
 */
extern struct decoded_i *delayed_p;
extern u_long delayed_ip;

/*
 * This does the actual transfer of a byte, half, word, or double
 * word in a slow memory operation.
 */
extern int l_mem_op();

/*
 * The next three functions translate addresses between different
 * spaces:
 *
 * L : logical 88000 address space
 * P : physical 88000 address space
 * D : decoded instruction pointer space
 */

extern int               l_to_p();      /* L ==> P          */

extern struct decoded_i *p_to_d();      /* P ==> D          */

/*
 * This use the code-space CMMU's mapping.
 */
extern struct decoded_i *l_to_d();      /* L ==> P ==> D    */

/*
 * Allocate the first part of a simulated 4K page.
 */
extern struct page *allocate_page();

/*
 * Allocate the decoded part of a memory page.
 */
extern struct decoded_part *allocate_decoded_part();

/*
 * Size of simulated 88000 physical memory in bytes.
 */
extern u_long memory_size;

/*
 * Number of *physical* 4K pages being simulated.  Not to be confused
 * with the logical pages.
 */
extern u_long page_table_size;

/*
 * Array of pointers to simulated physical 4K pages.
 */
extern struct page **page_table;

/*
 * Macros for referring to the Integer Unit's control and status registers.
 */
#define PID         (sfu0_regs[0])
#define PSR         (sfu0_regs[1])
#define TPSR        (sfu0_regs[2])
#define SSBR        (sfu0_regs[3])
#define SXIP        (sfu0_regs[4])
#define SNIP        (sfu0_regs[5])
#define SFIP        (sfu0_regs[6])
#define VBR         (sfu0_regs[7])
#define DMT0        (sfu0_regs[8])
#define DMD0        (sfu0_regs[9])
#define DMA0        (sfu0_regs[10])
#define DMT1        (sfu0_regs[11])
#define DMD1        (sfu0_regs[12])
#define DMA1        (sfu0_regs[13])
#define DMT2        (sfu0_regs[14])
#define DMD2        (sfu0_regs[15])
#define DMA2        (sfu0_regs[16])
#define STACKBASE   (sfu0_regs[21])
#define RAMSIZE     (sfu0_regs[22])

#define	SBR         (sfu0_regs[63])

/*
 * True if the simulated 88000 is currently executing in user mode.
 */
#define PSR_US_MODE     (PSR >> 31)

/*
 * True if the misaligned-access check is on.
 */
#define PSR_MA_CHECK_ON ((PSR & 4) == 0)

/*
 * True if the floating point unit is enabled.
 */
#define PSR_FP_UNIT_ON ((PSR & 8) == 0)

/*
 * The endian bit in the PSR (true if little endian).
 */
#define PSR_ENDIAN  (((PSR >> 30) & 1))

/*
 * True if interrupts are enabled.
 */
#define	PSR_INT_ENABLED (!(PSR & 2))

extern u_long cmmu_number_table[];
/*
 * Number of the cmmu to use for data references.
 */
#define DATA_CMMU(la)   cmmu_number_table[(0 + ((la >> 12) & 3))]

/*
 * Number of the cmmu to use for code references.
 */
#define CODE_CMMU(la)   cmmu_number_table[(4 + ((la >> 12) & 3))]

/*
 * Called after a store into the control registers to make
 * sure they still have valid values.
 */
extern void fixup_control_regs();

/*
 * Decodes 88000 instructions.
 */
extern int decode();

extern void clean_page();

/*
 * For keeping track of decoded instruction pages for IO space.
 */
struct io_list {
    struct io_list      *next_p;
    u_long              page_number;
    struct decoded_part *decoded_part;
};

extern struct io_list *io_list;

/*
 * This dumps all of the decoded instruction pages allocated for
 * the IO space.
 */
extern void flush_io_decoded_list();

/*
 * This takes the physical address of the page to be invalidated.
 */
extern void flush_io_decoded_list_entry();

/*
 * This flushes the decoded instruction cache.
 */
extern void flush_deccache();

#define READ    1
#define WRITE   2

extern void err();

#define check_pointer(p) if (!p) { err("nil pointer", __LINE__, __FILE__); }
#define assert(bexp)    if (!(bexp)) { err("assertion failed", __LINE__, __FILE__); }

/*
 * Translates the entry point of a normal instruction handler to
 * that of the single-step entry point.
 */
extern int (*normal_to_ss())();

/*
 * Set to 1 when a floating point exception occurs.
 */
extern int fp_exception;

/*
 * Returns the string that describes the passed exception.
 */
extern char *ex_name();

/*
 * Duplicates a string, returning a pointer to the new string.  
 * Returns 0 if passed a 0.
 */
extern char *sim_strdup();

/*
 * This cache's decoded instruction pointers to speed the translation
 * of logical code addresses to decoded instruction pointers.
 * The first index of this array of structures is indexed by
 * the mode (M_USER == 0, M_SUPERVISOR == 1).
 */
#define DECMAX 30

struct decitem {
    u_long pagela;
    struct decoded_i *pagep; } deccache[2][DECMAX];

/*
 * This has pointers into the deccache table, one for user mode
 * and one for supervisor.  They point to the next slot to use
 * in the deccache table.
 */
extern struct decitem *nextdecp[];

/*
 * Converts a logical address to its segment.  Used to figure out
 * which tlb segment an address is in.
 */
#define	btos(addr)	((u_long)(addr) >> 22)

/*
 * The page-within-segment of an address.
 */
#define btop(addr)	(((u_long)(addr) >> 12) & 0x3ff)

/*
 * The byte-within-page of an address.
 */
#define poff(addr)	((addr) & PAGEMASK)

/*
 * See its storage decl in decode.c
 */
extern double dummy_r0;

/*
 * True if we are tracing the exection.
 */
extern int tracing;

/*
 * Sends the passed parameters to a instruction execution trace
 * file.
 */
extern void emit_trace_record();

/* True when we are using the SIGVTALRM to switch between simulated
   processors. */
extern int waiting_for_mpswitch_interrupt;

#ifndef SIGINT
#define SIGINT 100
#define SIGTRAP 101
#endif
