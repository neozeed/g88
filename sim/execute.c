/*
 * This executes 88000 instructions.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/execute.c,v 1.61 90/11/15 19:40:24 robertb Exp $
 */

#include "sim.h"
#include "fields88.h"
#include "montraps.h"

#define FCMP(x,y) \
{ if ((x) == (y)) i = 0x6a6; else { if ((x) < (y)) i = 0x66a; else i = 0x69a; }}            
#define CHKINT  { if (sim_interrupt_flag) goto interrupt_label; }

#define SET_US_MODE             { usmode = PSR_US_MODE; }
#define USERMODE                (usmode == M_USER)
#define GEN_EXCEPTION(vector)   { ex = (vector); goto gen_exception;}
#define EX_IF_USER              { if (USERMODE) goto prv; }

#define CARRY_BIT               ((PSR >> 28) & 1)
#define SET_CARRY               { PSR |= 0x10000000; }
#define ZERO_CARRY              { PSR &= 0xefffffff; }
#define P                       ((struct decoded_i *)p)
#define IP			(pagela+(p-pagep)/(sizeof(struct decoded_i)/4))

/*
 * Macros for treating the decoded operands as u_long integers.
 */
#define SRC1        (*P->s1)
#define SRC2        (*P->s2)
#define DST         (*P->dest)

/*
 * Macros for treating the decoded operands as single precision
 * floating point numbers.
 */
#define FSRC1       (*(float *)P->s1)
#define FSRC2       (*(float *)P->s2)
#define FDST        (*(float *)P->dest)

/*
 * Macros for treating the decoded operands as double precision
 * floating point numbers.
 */
#define DSRC1       (*(double *)P->s1)
#define DSRC2       (*(double *)P->s2)
#define DDST        (*(double *)P->dest)

#define SET_FP_CREGS(s1, s2, dreg)  { sfu1_regs[2] = s1; \
    sfu1_regs[4] = s2; sfu1_regs[5] = dreg; SBR |= 1 << dreg;}

#define REGNUM(regptr)  (regptr - &regs[0])
/*
 * This increments the decoded ip.  It is equivalent to p++;
 */
#define INC_D_IP            { p += r_size; }

//BSD vs SysV underbars
#if 0
#define L(name)                           \
    { name:;                              \
      asm(" .globl _sim_" #name);       \
      asm("_sim_" #name ":"); }
#else
#define L(name)                           \
    { name:;                              \
      asm(" .globl sim_" #name);       \
      asm("sim_" #name ":"); }
#endif


/*
 * Dispatch the decoded instruction pointed to by 'p'.
 */
#define DISPATCH                { (P->norm_e_addr)(); }

/*
 * Dispatches the next instruction after a branch.  This is where
 * we check for interrupts from the user.
 */
#define DISP_AFTER_BR           { CHKINT; DISPATCH; }

/*
 * Dispatches the next instruction after executing a delayed
 * branch.
 */
#define DISP_AFTER_DELAYED_BR   { INC_D_IP ; DISPATCH; }

/*
 * If r_delayed_p is non-zero it means that the previous instruction was a
 * a .n type branch.  
 */
#define DISPATCH_NEXT           { if (r_delayed_p) goto delayed_br; \
                                  INC_D_IP ; DISPATCH; }

/*
 * The branch instructions' decoded form has the decoded instruction
 * pointer of the target in the s2 field if the target is on the same
 * page as the branch.  If it isn't, s2 is zero and the 'literal'
 * field has the logical 88000 address of the target.
 */
#define TAKE_BRANCH                                                           \
{ if (P->s2) p = (u_long)P->s2;                                               \
  else { ip = IP + (int)P->dest;                                              \
         pagela = ip & ~PAGEMASK;                                             \
         p = (u_long)l_to_d(ip, usmode, &pagep);                              \
         if (!p) goto cacc; }                                                 \
  DISP_AFTER_BR; }

#define TAKE_DELAYED_BRANCH \
{ r_delayed_p = P; delayed_ip = IP + (int)P->dest; DISP_AFTER_DELAYED_BR; }

/*
 * This calculates the unscaled logical address.  It looks up the
 * host virtual address in the tlb's.  If it can't find the address
 * in the tlb then it calls a function 'l_mem_op'
 * to do the load, store, or exchange for it.
 */
#define CALC_ADDR(REG_PTR, SIZE, MEM_OP_TYPE, TLB, MODE)                      \
    ptr = SRC1 + SRC2;                                                        \
    if (ptr & (SIZE - 1) && PSR_MA_CHECK_ON) goto macc;                       \
    else ptr &= ~(SIZE - 1);                                                  \
    ptr = (u_long)TLB##_tlb[btos(ptr)][btop(ptr)] + poff(ptr);              \
    if (ptr < PAGESIZE) { ip = IP;                                            \
        ex = l_mem_op(SRC1 + SRC2, REG_PTR, SIZE, MEM_OP_TYPE, MODE);         \
        if (ex != E_NONE) goto gen_exception; else DISPATCH_NEXT; }

/*
 * Like the above macro except for use in scaled memory instructions.
 */
#define CALC_ADDR_SCALED(REG_PTR, SIZE, MEM_OP_TYPE, TLB, MODE)               \
    ptr = SRC1 + SRC2 * SIZE;                                                 \
    if (ptr & (SIZE - 1) && PSR_MA_CHECK_ON) goto macc;                       \
    else ptr &= ~(SIZE - 1);                                                  \
    ptr = (u_long)TLB##_tlb[btos(ptr)][btop(ptr)] + poff(ptr);              \
    if (ptr < PAGESIZE) { ip = IP;                                            \
        ex = l_mem_op(SRC1 + SRC2 * SIZE, REG_PTR, SIZE, MEM_OP_TYPE, MODE);  \
        if (ex != E_NONE) goto gen_exception; else DISPATCH_NEXT; }

/*
 * This sets our local pointers to either the user's or supervisors
 * software tlbs.  We do it when we enter the execution function and
 * whenever the PSR might have been changed.
 */
#define SET_TLB_PTRS                                                           \
   if (USERMODE) { load_tlb  = &(u_load_tlb[0]); store_tlb = &(u_store_tlb[0]);\
   } else {        load_tlb  = &(s_load_tlb[0]); store_tlb = &(s_store_tlb[0]);\
}

/*
 * This checks the stack pointer (r31) against the stack base register (cr21),
 * and halts the machine if the stack base register is nonzero, the machine is
 * in supervisor mode, and the stack pointer is less than the stack base
 * register.
 * This check is only made during those instructions that the kernel happens
 * to use to change r31: add, addu, clr, extu, ld, ldcr, or, or.u, sub, subu.
 * This is of primary use in certain kernel debugging exercises.
 * This slows the simulator way down! so normally, CHECK_STACK should be
 * defined to do nothing.
 */
#define CHECK_STACK \
    if (STACKBASE && (long)PSR<0 /* !user mode */ && regs[31]<STACKBASE) \
	{ goto stack_overflow; };

/*
 * The "come-from" register.
 * This contains the instruction pointer just before the last jmp,
 * jsr, or rte.  It is not modified by branch instructions because I don't
 * want to slow them down.  This is useful for determining "who jumped to
 * garbage".
 *  -=- /AJK
 */
long comefrom;

/*
 * The "memory breakpoint" register.
 * If nonzero, any store (or xmem) to this location causes a break.
 */
long membrk=0;

/*
 * The "single-step command" flag.
 * This is set to zero at the beginning of each command,
 * and is set to one when about to do a STEP (or STEPI or NEXT or NEXTI)
 * command.
 */
char single_step_command;

/*
 * This executes 88000 instructions.  If ss is 1, one instruction
 * will be executed.  If ss is 0, instructions will be executed
 * until a breakpoint or exceptional condition is reached.
 */
int
sim(ss)
{
    register struct decoded_i *r_delayed_p;

    register u_long p;           /* really a struct decoded_i *              */
    register u_char ***load_tlb; /* 1st level tlb for loads.                 */
    register u_char ***store_tlb;/* 1st level tlb for stores.                */
    register u_long ptr;         /* Gets both logical and pointer values.    */
    register u_long r_size  ;    /* For fast incrementing of p               */
    register i;                  /* misc temporary used exchanges and counter*/

    /*
     * Logical address of the page that we are currently executing on.
     * Always a multiple of PAGESIZE.
     */
    register u_long pagela;

    int usmode;                /* 0: user mode, 1: supervisor mode    */

    /*
     * Really a pointer to a decoded instruction.  Points to the first
     * decoded instruction slot of the page that we are currently 
     * executing on.
     */
    u_long pagep;

    /*
     * Temporaries used in arithmetic calculations.
     */
    int s1, s2;
    int s1_neg, s2_neg;

    /*
     * The exception number of the exception currently being raised
     * (if any).  Undefined if we're not processing an exception.
     */
    int ex;

    /*
     * Booleans used in the add instructions.
     */
    int both_pos, both_neg, res_neg;

    /*
     * This variable's value is returned as the value of this function.
     * It can be NONE, BREAKPOINT, INTERRUPT, or HOSED.  It is set
     * to something other than NONE just before returning.
     */
    int cause = NONE;

    /*
     * Address in our (simulator's) address space of the (simulated) memory
     * location that is the subject of the memory breakpoint.  Zero if none.
     */
    register membrkphys;
    struct page *page_ptr;

    /*
     * This decoded instruction is used by the RTE instruction.  It
     * set the pointer delayed_p to point to it and fills in some of
     * the fields.  See the code in the rte entry point.  Its harry.
     */
    static struct decoded_i rte_decoded_i;

    extern sim_not_decoded();

    /*
     * Set up the real address of the memory breakpoint.
     */
    if (membrk==0) {
	membrkphys = 0;		/* no memory breakpoint */
    } else if (membrk>=memory_size) {
	sim_printf("$membrk=0x%08x, memory size limit is 0x%08x\n",
			membrk, memory_size);
    } else {
	page_ptr = page_table[membrk>>12];
	if (page_ptr == (struct page *)0) {
	    page_ptr = allocate_page(poff(membrk));
	    page_table[membrk>>12] = page_ptr;
	}
	membrkphys = (int)((char *)&(page_ptr->values[0]) + poff(membrk));
    }

    r_delayed_p = delayed_p;

    r_size = sizeof(struct decoded_i);

    SET_US_MODE;
    SET_TLB_PTRS;

    /*
     * We do this call to make the optimizer know that all
     * of these variables are active.
     */
    sim_zero(load_tlb, store_tlb, r_delayed_p, r_size, p, usmode);

    sim_in_execution = 1;

    pagela = ip & ~PAGEMASK;

    /* If an exception was pending, recognize it first. We have to
       do this after we set 'ip', as the code that we may jump to
       depends on it being set correctly. */

    if (sim_exception != E_NONE) {
        ex = sim_exception;
        goto gen_exception_without_check;
    }

    if ((p = (u_long)l_to_d(ip, usmode, &pagep)) == 0) {
        goto cacc;
    }
    if (ss || tracing) {
        insert_ss_breakpoint(ip, usmode);
    }

    /*
     * This used to be "if (!ss) ...", but this keeps the clock from ticking
     * when running with a memory breakpoint (e.g., "stopi address") and
     * prevents me from finding a kernel bug in which a memory location is
     * trashed but only when the clock ticks.
     *   Andrew Klossner
     */
    single_step_command = ss;	/* Now have a gdb interface -rcb */
    if (!single_step_command) {
        ReleaseOscillator();
    }

     /* If an device interrupt was pending, do this first. */
    if ((sim_interrupt_flag & INT_DEVICE) && PSR_INT_ENABLED) {
        GEN_EXCEPTION(E_INT);
    }

    DISPATCH;

L(end_of_page);
    /*
     * We just tried to execute the instruction on the next page.
     * We need to set 'p' to point to the first decoded instruction
     * of the next page.
     */
    ip = IP;
    pagela = ip & ~PAGEMASK;
    if ((p = (u_long)l_to_d(ip, usmode, &pagep)) == 0) {
        goto cacc;
    }
    DISPATCH;

L(not_decoded);
    /*
     * We just tried to execute an instruction that has not yet
     * been decoded.  We decode it and then try it again.
     */
    if (decode(p, IP, usmode)) {
        goto cacc;
    }
    DISPATCH;

L(opc_exception);
    /*
     * We just executed an invalid 88000 opcode.  We raise an exception.
     */
    GEN_EXCEPTION(E_OPC);

L(ss_breakpoint);
    /*
     * This is used both by the single-step facilty and the
     * execution tracing facility.
     */
    P->norm_e_addr = sim_not_decoded;
    if (ss) {
        goto return_pt;
    } else {
        /*
         * If we are tracing instruction execution, we want to
         * call 'insert_ss_breakpoint()' for every instruction that
         * we execute.  We do this by placing a breakpoint after
         * instruction.
         */
        if (tracing) {
            insert_ss_breakpoint(IP, usmode);
        }
        DISPATCH;
    }

L(add);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if ((both_neg && !res_neg) || (both_pos && res_neg)) {
        goto iov;
    }
    CHECK_STACK;
    DISPATCH_NEXT;

L(add_co);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if (both_neg || (!res_neg && !both_pos)) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }

    if ((both_neg && !res_neg) || (both_pos && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;

L(add_ci);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2 + CARRY_BIT;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if ((both_neg && !res_neg) || (both_pos && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;

L(add_cio);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2 + CARRY_BIT;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if (both_neg || (!res_neg && !both_pos)) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }

    if ((both_neg && !res_neg) || (both_pos && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;
    
L(addu);
    DST = SRC1 + SRC2;
    CHECK_STACK;
    DISPATCH_NEXT;

L(addu_co);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if (both_neg || (!res_neg && !both_pos)) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }
    DISPATCH_NEXT;

L(addu_ci);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2 + CARRY_BIT;
    DISPATCH_NEXT;

L(addu_cio);
    s1 = SRC1;
    s2 = SRC2;
    DST = s1 + s2 + CARRY_BIT;

    both_neg = (s1 & s2) >> 31;
    both_pos = !((s1 | s2) >> 31);
    res_neg = DST >> 31;

    if (both_neg || (!res_neg && !both_pos)) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }
    DISPATCH_NEXT;
    
L(and);
    DST = SRC1 & SRC2;
    DISPATCH_NEXT;
    
L(and_c);
    DST = SRC1 & ~SRC2;
    DISPATCH_NEXT;

/***
 *** The branch instructions are at the end of this function,
 *** so that the labels that a branch targets
 *** (e.g., gen_exception_without_check) do not fall at the end, so that
 *** references to them from the beginning do not get "offset too large"
 *** errors from the stupid assembler.
 ***/

L(clr);
    if (W5(SRC2) == 0) {
        DST = SRC1 & ~(0xffffffff << O5(SRC2));
    } else {
        DST = SRC1 & ~(((1 << W5(SRC2)) - 1) << O5(SRC2));
    }
    CHECK_STACK;
    DISPATCH_NEXT;

L(cmp);
    s1 = SRC1;
    s2 = SRC2;
    if (s1 == s2) {
        DST = 0xaa4;
    } else {

        /*
         * First we figure out the u_long bits.
         */
        if ((u_long)s1 > (u_long)s2) {
            DST = 0x908;
        } else {
            DST = 0x608;
        }

        /*
         * Then we or in the signed bits.
         */
        if (s1 > s2) {
            DST |= 0x90;
        } else {
            DST |= 0x60;
        }
    }
    DISPATCH_NEXT;

L(div);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if ((int)SRC1 < 0 || (int)SRC2 <= 0) {
        SET_FP_CREGS(SRC1, SRC2, REGNUM(P->dest));
        GEN_EXCEPTION(E_IDE);
    }
    DST = (int)SRC1 / (int)SRC2;
    DISPATCH_NEXT;

L(divu);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if (SRC2 == 0) {
        SET_FP_CREGS(SRC1, SRC2, REGNUM(P->dest));
        GEN_EXCEPTION(E_IDE);
    }
    DST = SRC1 / SRC2;
    DISPATCH_NEXT;

L(ext);
    s1 = SRC1;
    s2 = SRC2;
    if (W5(s2) == 0) {
        DST = s1 >> O5(s2);
    } else {
        int left_sh_cnt = 32 - (O5(s2) + W5(s2));
        DST = (s1 << left_sh_cnt) >> (O5(s2) + left_sh_cnt);
    }
    DISPATCH_NEXT;

L(extu);
    if (W5(SRC2) == 0) {
        DST = SRC1 >> O5(SRC2);
    } else {
        DST = (SRC1 >> O5(SRC2)) & ((1 << W5(SRC2)) - 1);
    }
    CHECK_STACK;
    DISPATCH_NEXT;

L(fadd_sss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 + FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fadd_ssd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 + DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fadd_sds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 + FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fadd_sdd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 + DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fadd_dss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 + FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fadd_dsd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 + DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fadd_dds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 + FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fadd_ddd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 + DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fcmp_sss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FCMP(FSRC1, FSRC2);
    DST = i;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fcmp_ssd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FCMP(FSRC1, DSRC2);
    DST = i;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fcmp_sds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FCMP(DSRC1, FSRC2);
    DST = i;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fcmp_sdd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FCMP(DSRC1, DSRC2);
    DST = i;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fdiv_sss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if (FSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    FDST = FSRC1 / FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fdiv_ssd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if (DSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    FDST = FSRC1 / DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fdiv_sds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if (FSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    FDST = DSRC1 / FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fdiv_sdd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    if (DSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    FDST = DSRC1 / DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fdiv_dss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    if (FSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    DDST = FSRC1 / FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fdiv_dsd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    if (DSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    DDST = FSRC1 / DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fdiv_dds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    if (FSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    DDST = DSRC1 / FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fdiv_ddd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    if (DSRC2 == 0.0) {
        GEN_EXCEPTION(E_IDE);
    }
    DDST = DSRC1 / DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(ff0);
    for (i = 31 ; i >= 0 ; i--) {
        if (((SRC2 >> i) & 1) == 0) {
            DST = i;
            DISPATCH_NEXT;
        }
    }
    DST = 32;
    DISPATCH_NEXT;

L(ff1);
    for (i = 31 ; i >= 0 ; i--) {
        if (((SRC2 >> i) & 1) == 1) {
            DST = i;
            DISPATCH_NEXT;
        }
    }
    DST = 32;
    DISPATCH_NEXT;

L(flt_ss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = (float)SRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(flt_ds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = (double)SRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fmul_sss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 * FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fmul_ssd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 * DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fmul_sds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 * FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fmul_sdd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 * DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fmul_dss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 * FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fmul_dsd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 * DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fmul_dds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 * FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fmul_ddd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 * DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

/*
 * fstcr is done by stcr
 */

L(fsub_sss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 - FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fsub_ssd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = FSRC1 - DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fsub_sds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 - FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fsub_sdd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    FDST = DSRC1 - DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(fsub_dss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 - FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fsub_dsd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = FSRC1 - DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fsub_dds);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 - FSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(fsub_ddd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_double;
    DDST = DSRC1 - DSRC2;
    if (fp_exception) goto fp_imprecise_double;
    DISPATCH_NEXT;

L(int_ss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = (int)FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(int_sd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = (int)DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(jmp);
    /*
     * If the jmp target is on-page, do a quick calculation to
     * find the new decoded pointer.
     */
    {   u_long dst = DST & ~3;

	comefrom = IP;
        if ((dst & ~PAGEMASK) == pagela) {
            p = (u_long)pagep + poff(dst) * (sizeof(struct decoded_i) / 4);
        } else {
            pagela = dst & ~PAGEMASK;
            if ((p = (u_long)l_to_d(dst, usmode, &pagep)) == 0) {
                ip = dst;
                goto cacc;
            }
        }
    }
    DISP_AFTER_BR;

L(jmp_n);
    comefrom = IP;
    delayed_ip = DST & ~3;
    r_delayed_p = P;
    DISP_AFTER_DELAYED_BR;

L(jsr);
    { u_long dst = DST & ~3;
      u_long curip = IP;
      comefrom = curip;
      regs[1] = curip + 4;
      /*
       * If the jsr target is on-page, do a quick calculation to
       * find the new decoded pointer.
       */
      if ((dst & ~PAGEMASK) == pagela) {
          p += (int)(dst - curip) * (sizeof(struct decoded_i) / 4);
      } else {
          pagela = dst & ~PAGEMASK;
          if ((p = (u_long)l_to_d(dst, usmode, &pagep)) == 0) {
              ip = dst;
              goto cacc;
          }
      }
      DISP_AFTER_BR;
    }

L(jsr_n);
    comefrom = IP;
    delayed_ip = DST & ~3;
    regs[1] = IP + 8;
    r_delayed_p = P;
    DISP_AFTER_DELAYED_BR;

L(ld_d);
    CALC_ADDR(P->dest, DWORD, LD, load, usmode);
    DST = *(u_long *)ptr;
    *(P->dest + 1) = *(u_long *)(ptr + 4);
    regs[0] = 0;    /* Zer0 r0 in case this was a double load of r0 */
    regs[32] = 0;   /* Zero r32 in case this was a double load of r31 */
    DISPATCH_NEXT;

L(ld);
    CALC_ADDR(P->dest, WORD, LD, load, usmode);
    DST = *(u_long *)ptr;
    CHECK_STACK;
    DISPATCH_NEXT;

L(ld_h);
    CALC_ADDR(P->dest, HALF, LD, load, usmode);
    DST = *(short *)ptr;
    DISPATCH_NEXT;

L(ld_b);
    CALC_ADDR(P->dest, BYTE, LD, load, usmode);
    DST = *(char *)ptr;
    DISPATCH_NEXT;

L(ld_hu);
    CALC_ADDR(P->dest, HALF, LD_U, load, usmode);
    DST = *(u_short *)ptr;
    DISPATCH_NEXT;

L(ld_bu);
    CALC_ADDR(P->dest, BYTE, LD_U, load, usmode);
    DST = *(u_char *)ptr;
    DISPATCH_NEXT;

L(ld_d_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, DWORD, LD, u_load, M_USER);
    DST = *(u_long *)ptr;
    *(P->dest + 1) = *(u_long *)(ptr + 4);
    regs[0] = 0;    /* Zer0 r0 in case this was a double load of r0 */
    regs[32] = 0;   /* Zero r32 in case this was a double load of r31 */
    DISPATCH_NEXT;

L(ld_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, WORD, LD, u_load, M_USER);
    DST = *(u_long *)ptr;
    DISPATCH_NEXT;

L(ld_h_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, HALF, LD, u_load, M_USER);
    DST = *(short *)ptr;
    DISPATCH_NEXT;

L(ld_b_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, BYTE, LD, u_load, M_USER);
    DST = *(char *)ptr;
    DISPATCH_NEXT;

L(ld_hu_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, HALF, LD_U, u_load, M_USER);
    DST = *(u_short *)ptr;
    DISPATCH_NEXT;

L(ld_bu_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, BYTE, LD_U, u_load, M_USER);
    DST = *(u_char *)ptr;
    DISPATCH_NEXT;

L(ld_d_sc);
    CALC_ADDR_SCALED(P->dest, DWORD, LD, load, usmode);
    DST = *(u_long *)ptr;
    *(P->dest + 1) = *(u_long *)(ptr + 4);
    regs[0] = 0;    /* Zer0 r0 in case this was a double load of r0 */
    regs[32] = 0;   /* Zero r32 in case this was a double load of r31 */
    DISPATCH_NEXT;

L(ld_sc);
    CALC_ADDR_SCALED(P->dest, WORD, LD, load, usmode);
    DST = *(u_long *)ptr;
    DISPATCH_NEXT;

L(ld_h_sc);
    CALC_ADDR_SCALED(P->dest, HALF, LD, load, usmode);
    DST = *(short *)ptr;
    DISPATCH_NEXT;

L(ld_hu_sc);
    CALC_ADDR_SCALED(P->dest, HALF, LD_U, load, usmode);
    DST = *(u_short *)ptr;
    DISPATCH_NEXT;

L(ld_d_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, DWORD, LD, u_load, M_USER);
    DST = *(u_long *)ptr;
    *(P->dest + 1) = *(u_long *)(ptr + 4);
    regs[0] = 0;    /* Zer0 r0 in case this was a double load of r0 */
    regs[32] = 0;   /* Zero r32 in case this was a double load of r31 */
    DISPATCH_NEXT;

L(ld_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, WORD, LD, u_load, M_USER);
    DST = *(u_long *)ptr;
    DISPATCH_NEXT;

L(ld_h_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, HALF, LD, u_load, M_USER);
    DST = *(short *)ptr;
    DISPATCH_NEXT;

L(ld_hu_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, HALF, LD_U, u_load, M_USER);
    DST = *(u_short *)ptr;
    DISPATCH_NEXT;

L(lda_d)
    DST = SRC1 + (SRC2 << 3);
    DISPATCH_NEXT;

L(lda)
    DST = SRC1 + (SRC2 << 2);
    DISPATCH_NEXT;

L(lda_h)
    DST = SRC1 + (SRC2 << 1);
    DISPATCH_NEXT;

L(ldcr);
    EX_IF_USER;
    load_shadow_regs(ip, delayed_ip);
    DST = SRC1;
    CHECK_STACK;
    DISPATCH_NEXT;

L(uldcr);
    load_shadow_regs(ip, delayed_ip);
    DST = SRC1;
    DISPATCH_NEXT;

L(mak);
    if (W5(SRC2) == 0) {
        DST = SRC1 << O5(SRC2);
    } else {
        DST = (SRC1 & ((1 << W5(SRC2)) - 1)) << O5(SRC2);
    }
    DISPATCH_NEXT;

/*
 * mask and mask_u turn into and's
 */
L(mul);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = SRC1 * SRC2;
    DISPATCH_NEXT;

L(nint_ss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = (int)FSRC1;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(nint_sd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = (int)DSRC1;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(or);
    DST = SRC1 | SRC2;
    CHECK_STACK;
    DISPATCH_NEXT;

L(or_c);
    DST = SRC1 | ~SRC2;
    DISPATCH_NEXT;

L(rot);
    for (i = 0 ; i < O5(SRC2) ; i++) {
        DST = (DST >> 1) | (DST & 1) << 31;
    }
    DISPATCH_NEXT;

L(rte);
    EX_IF_USER;
    if ((PSR & 1) == 0) {
        sim_printf("ERROR: rte being executed with shadowing not frozen.\n");
        sim_printf("       Real processor would hang in this case.\n");
        cause = HOSED;
        goto return_pt;
    }
    if ((TPSR & 1) == 1 && !getenv("SIMDIAGS")) {
        sim_printf("ERROR: rte, TPSR has shadows frozen.\n");
        sim_printf("       This is almost certainly an error.\n");
        cause = HOSED;
        goto return_pt;
    }

    /* If the RTE is in a branch delay slot, cancel out the branch. */
    r_delayed_p = (struct decoded_i *)0;
    comefrom = IP;
    SBR = SSBR;
    PSR = (TPSR | 0x3f0) & 0xf800e3ff;
    /*
     * If the valid and exception bits in the shadow-next-instruction
     * pointer are both set, generate a code access fault.
     */
    if (SNIP & 3 == 3) {
        ip = SNIP & ~3;
        goto cacc;
    }

    if (SNIP & 2) {
        ip = SNIP & ~3;
        if ((SFIP & 3) == 3) {
            sim(1);
            goto cacc;
        }
        if (SFIP & 2 && (SFIP & ~3) != ((SNIP & ~3) + 4)) {
            /*
             * Both the shadow-next-instruction pointer and the
             * shadow fetched-instruction pointer are valid and
             * fetched-instruction pointer does not point to the
             * instruction following the one pointed to by the
             * shadow-next-instruction pointer.
             *
             * So we first have to execute the instruction pointed to by
             * the shadow-next-instruction pointer and then
             * jump to the one pointed to by the shadow-fetched-
             * instruction pointer.
             *
             * We use the delayed-instruction facility to do this.
             * We point r_delayed_p at a decoded instruction structure
             * with a zero s2 field.  When the delayed branch code
             * dereferences r_delayed_p and sees this zero, it will
             * requalify the decoded-instruction pointer, p, by
             * translating delayed_ip into a decoded pointer.
             */
            delayed_ip = SFIP & ~3;
            rte_decoded_i.s2 = (u_long *)0;
            r_delayed_p = &rte_decoded_i;
        }
    } else {
        if (SFIP & 2) {
            ip = SFIP & ~3;
            if (SFIP & 1) {
                goto cacc;
            }
        } else {
            ip = (SFIP & ~3) + 4;
            sim_printf("rte: SNIP and SFIP are not valid, new ip=%x\n", ip);
            cause = HOSED;
            goto return_pt;
        }
    }
    SET_US_MODE;
    SET_TLB_PTRS;
    pagela = ip & ~PAGEMASK;
    p = (u_long)l_to_d(ip, usmode, &pagep);
    if (!p) {

#ifdef NOTDEF
	/* This message doesn't connote an error an I'm tired of it.  /AJK */
        sim_printf("rte is raising code access exception.\n");
#endif

        goto cacc;
    }
    if (ss) {
        goto return_pt;
    }
    DISP_AFTER_BR;

L(set);
    if (W5(SRC2) == 0) {
        DST = SRC1 | (0xffffffff << O5(SRC2));
    } else {
        DST = SRC1 | (((1 << W5(SRC2)) - 1) << O5(SRC2));
    }
    DISPATCH_NEXT;

L(st_d);
    CALC_ADDR(P->dest, DWORD, ST, store, usmode);
    *(u_long *)ptr = DST;
    *(u_long *)(ptr + 4) = *(P->dest + 1);
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+7)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st);
    CALC_ADDR(P->dest, WORD, ST, store, usmode);
    *(u_long *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_h);
    CALC_ADDR(P->dest, HALF, ST, store, usmode);
    *(short *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+1)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_b);
    CALC_ADDR(P->dest, BYTE, ST, store, usmode);
    *(char *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_d_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, DWORD, ST, u_store, M_USER);
    *(u_long *)ptr = DST;
    *(u_long *)(ptr + 4) = *(P->dest + 1);
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+7)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, WORD, ST, u_store, M_USER);
    *(u_long *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_h_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, HALF, ST, u_store, M_USER);
    *(short *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+1)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_b_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, BYTE, ST, u_store, M_USER);
    *(char *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_d_sc);
    CALC_ADDR_SCALED(P->dest, DWORD, ST, store, usmode);
    *(u_long *)ptr = DST;
    *(u_long *)(ptr + 4) = *(P->dest + 1);
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+7)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_sc);
    CALC_ADDR_SCALED(P->dest, WORD, ST, store, usmode);
    *(u_long *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_h_sc);
    CALC_ADDR_SCALED(P->dest, HALF, ST, store, usmode);
    *(short *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+1)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_d_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, DWORD, ST, u_store, M_USER);
    *(u_long *)ptr = DST;
    *(u_long *)(ptr + 4) = *(P->dest + 1);
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+7)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, WORD, ST, u_store, M_USER);
    *(u_long *)ptr = DST;
    *(u_long *)(ptr + 4) = *(P->dest + 1);
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(st_h_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, HALF, ST, u_store, M_USER);
    *(short *)ptr = DST;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+1)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(stcr);
    EX_IF_USER;
    DST = SRC1;
    fixup_control_regs();
    check_for_interrupt();
    SET_US_MODE;
    SET_TLB_PTRS;
    DISPATCH_NEXT;

L(ustcr);
    DST = SRC1;
    fixup_control_regs();
    DISPATCH_NEXT;

L(sub);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = s1 - s2;
    res_neg = DST >> 31;

    if ((s1_neg && !s2_neg && !res_neg) ||
        (!s1_neg && s2_neg && res_neg)) {
        goto iov;
    }
    CHECK_STACK;
    DISPATCH_NEXT;

L(sub_bo);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = s1 - s2;
    res_neg = DST >> 31;

    if ((s1_neg && s1_neg) || (res_neg && (!(s1_neg) || !(s2_neg)))) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }

    if ((s1_neg && !s2_neg && !res_neg) ||
        (!s1_neg && s2_neg && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;

L(sub_bi);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = (s1 - s2) + CARRY_BIT;
    res_neg = DST >> 31;

    if ((s1_neg && !s2_neg && !res_neg) ||
        (!s1_neg && s2_neg && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;

L(sub_bio);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = (s1 - s2) + CARRY_BIT;
    res_neg = DST >> 31;

    if ((s1_neg && s1_neg) || (res_neg && (!(s1_neg) || !(s2_neg)))) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }

    if ((s1_neg && !s2_neg && !res_neg) ||
        (!s1_neg && s2_neg && res_neg)) {
        goto iov;
    }
    DISPATCH_NEXT;
    
L(subu);
    DST = SRC1 - SRC2;
    CHECK_STACK;
    DISPATCH_NEXT;

L(subu_bo);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = s1 - s2;
    res_neg = DST >> 31;

    if ((s1_neg && s1_neg) || (res_neg && (!(s1_neg) || !(s2_neg)))) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }
    DISPATCH_NEXT;

L(subu_bi);
    s1 = SRC1;
    s2 = SRC2;
    DST = (s1 - s2) + CARRY_BIT;
    DISPATCH_NEXT;

L(subu_bio);
    s1 = SRC1;
    s2 = SRC2;
    s1_neg = s1 >> 31;
    s2_neg = s2 >> 31;

    DST = (s1 - s2) + CARRY_BIT;
    res_neg = DST >> 31;

    if ((s1_neg && s1_neg) || (res_neg && (!(s1_neg) || !(s2_neg)))) {
        SET_CARRY;
    } else {
        ZERO_CARRY;
    }
    DISPATCH_NEXT;
    
L(tb0);
    if (SBR != 0) {
        sim_printf("SBR non-zero at tb0, machine hangs\n");
        cause = HOSED;
        goto return_pt;
    }
    if ((SRC1 & (int)(P->dest)) == 0) {
        if (USERMODE && (int)(P->s2) < 128) {
            GEN_EXCEPTION(E_PRV);
        }
        switch ((int)(P->s2)) {
            case TR_BPT: /* breakpoint trap */
                cause = BREAKPOINT;
                goto return_pt;

            default:
                GEN_EXCEPTION((int)(P->s2));
        }
    }
    DISPATCH_NEXT;

L(tb1);
    if (SBR != 0) {
        sim_printf("SBR non-zero at tb1, machine hangs\n");
        cause = HOSED;
        goto return_pt;
    }
    if ((SRC1 & (int)(P->dest)) == 1) {
        if (USERMODE && (int)(P->s2) < 128) {
            GEN_EXCEPTION(E_PRV);
        }
        GEN_EXCEPTION((int)(P->s2));
    }
    DISPATCH_NEXT;

L(tbnd);
    if (SRC1 > SRC2) {
        GEN_EXCEPTION(E_BND);
    }
    DISPATCH_NEXT;

L(tcnd);
    { int cc_index;
      cc_index = ((SRC1 >> 30) & 2) | ((SRC1 & 0x7fffffff) == 0);
      if (((int)(P->dest) >> cc_index) & 1) {
        if (USERMODE && (int)(P->s2) < 128) {
            GEN_EXCEPTION(E_PRV);
        }
        GEN_EXCEPTION((int)(P->s2));
      }
    }
    DISPATCH_NEXT;

L(trnc_ss);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = FSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

L(trnc_sd);
    if (!PSR_FP_UNIT_ON) goto fp_precise_single;
    DST = DSRC2;
    if (fp_exception) goto fp_imprecise_single;
    DISPATCH_NEXT;

/*
 * SRC1 - register with value to store into control register
 * SRC2 - control register to exchange with
 * DST  - destination general register
 */
L(xcr);
    EX_IF_USER;
    load_shadow_regs(ip, delayed_ip);
    i = SRC1;
    DST = SRC2;
    SRC2 = i;
    fixup_control_regs();
    check_for_interrupt();
    regs[0] = 0;        /* In case r0 got modified. */
    SET_US_MODE;
    SET_TLB_PTRS;
    DISPATCH_NEXT;

/*
 * SRC1 - register with value to store into control register
 * SRC2 - control register to exchange with
 * DST  - destination general register
 */
L(uxcr);
    i = SRC1;
    DST = SRC2;
    SRC2 = i;
    fixup_control_regs();
    regs[0] = 0;        /* In case r0 was modified. */
    DISPATCH_NEXT;

L(xmem_bu);
    CALC_ADDR(P->dest, WORD, XMEM, store, usmode);
    i = *(u_char *)ptr;
    *(u_char *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xmem);
    CALC_ADDR(P->dest, WORD, XMEM, store, usmode);
    i = *(u_long *)ptr;
    *(u_long *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xmem_sc);
    CALC_ADDR_SCALED(P->dest, WORD, XMEM, store, usmode);
    i = *(u_long *)ptr;
    *(u_long *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xmem_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, WORD, XMEM, u_store, M_USER);
    i = *(u_long *)ptr;
    *(u_long *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xmem_bu_usr);
    EX_IF_USER;
    CALC_ADDR(P->dest, WORD, XMEM, u_store, M_USER);
    i = *(u_char *)ptr;
    *(u_char *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xmem_usr_sc);
    EX_IF_USER;
    CALC_ADDR_SCALED(P->dest, WORD, XMEM, u_store, M_USER);
    i = *(u_long *)ptr;
    *(u_long *)ptr = DST;
    DST = i;
    regs[0] = 0;
    if (membrkphys && (ptr<=membrkphys+3 && membrkphys<=ptr+3)) {
	cause = BREAKPOINT;
	goto return_pt;
    }
    DISPATCH_NEXT;

L(xor);
    DST = SRC1 ^ SRC2;
    DISPATCH_NEXT;

L(xor_c);
    DST = SRC1 ^ ~SRC2;
    DISPATCH_NEXT;

/*
 * r_delayed_p points to the delayed branch instruction.
 * If the branch target is on-page we just load p with the target decoded
 * instruction pointer and go.  If not, we call l_to_d with the
 * logical address of the branch target, in delayed_ip,  and let 
 * it tell us what the target's decoded instruction pointer is.
 *
 * I use the L() macro here so that there is a global label here
 * that prof will use.
 */
L(delayed_br);
    if (!(p = (u_long)(r_delayed_p->s2))) {
        r_delayed_p = (struct decoded_i *)0;
        pagela = delayed_ip & ~PAGEMASK;
        if (!(p = (u_long)l_to_d(delayed_ip, usmode, &pagep))) {
            ip = delayed_ip;
            goto cacc;
        }
    };
    r_delayed_p = (struct decoded_i *)0;
    DISP_AFTER_BR;

prv:
    GEN_EXCEPTION(E_PRV);

cacc:
    /*
     * The code access fault exception is special in that the global
     * 'ip' is already loaded with the ip of the faulted instruction.
     * For all the other exceptions we can ask the IP macro what the
     * ip is at the time of the exception.  So we "back-compute"
     * values for pagela and p so that the IP macro will yeild the
     * current value of ip.  Note that 'p' is totally bogus as a pointer
     * to a decoded instruction because it is based on 'pagep', which
     * isn't pointing to anything relevent.
     */
    pagela = ip & ~PAGEMASK;
    p = pagep + poff(ip) * (sizeof(struct decoded_i)/4);
    GEN_EXCEPTION(E_CACC);

macc:
    SBR |= 1 << REGNUM(P->dest);
    GEN_EXCEPTION(E_MA);

iov:
    GEN_EXCEPTION(E_IOV);

fp_precise_single:
    SBR |= 1 << REGNUM(P->dest);
    GEN_EXCEPTION(E_FP);

fp_precise_double:
    SBR |= 1 << REGNUM(P->dest);
    SBR |= 1 << (REGNUM(P->dest) + 1);
    GEN_EXCEPTION(E_FP);

fp_imprecise_single:
    fp_exception = 0;
    SBR |= 1 << REGNUM(P->dest);
    GEN_EXCEPTION(E_FPIM);

/*
 * According to Mr CPU, Marvin Denmen, the first SBR bit won't
 * be set.
 */
fp_imprecise_double:
    fp_exception = 0;
    SBR |= 1 << (REGNUM(P->dest) + 1);
    GEN_EXCEPTION(E_FPIM);

/*
 * Exception processing for the 88000.
 */
L(gen_exception);
    /*
     * Copy our local representation of the instruction pointer
     * to the global that the debugger reads.
     */
    ip = IP;

    if (ex == E_INT) {
        /*
         * The interrupt flag is checked after p has been
         * updated to point to the instruction that is about
         * to execute.  The code below expects ip to point
         * to the exception-generating instruction.  Subtracting
         * 4 from the ip will make the calculation of snip and
         * sfip come out right, but sxip will be wrong.  This 
         * shouldn't matter.  I do not feel good about this kludge -rcb.
         */
        ip -= 4;
    }

    if (delayed_p) {
        SNIP = ip | 2;
        SFIP = delayed_ip | 2;
        delayed_p = 0;
    } else {
        SNIP = ip | 2;
        SFIP = (ip + 4) | 2;
    }

    if (sim_catch_exception[ex + 512*!usmode]) {
        sim_exception = ex;
        cause = CAUGHT_EXCEPTION;
        goto return_with_exception;
    }

/*
 * We enter at this point if we just entered sim() and
 * there was an outstanding exception that needed to be processed.
 */
L(gen_exception_without_check);

    if (PSR & 1) {
        ex = E_ERR;
        sim_printf("Exception with shadow registers frozen ...\n");
        if (sim_catch_exception[E_ERR + 512*!usmode]) {
	    sim_exception = E_ERR;
            cause = CAUGHT_EXCEPTION;
            goto return_with_exception;
        }
    }

    /*
     * If this isn't a data access exception and shadowing is on,
     * zero the data memory unit's transaction registers.
     */
    if (ex != E_DACC && (PSR & 1) == 0) {
        DMT0 = DMT1 = DMT2 = 0;
    }

    sim_exception = E_NONE;
    TPSR = (PSR | 0x3f0) & 0xf800e3ff;
    SSBR = SBR;
    SBR = 0;
    /*
     * Set the S/U bit to supervisor.
     * Disable FP unit and all other other SFU's.
     * Disable interrupts.
     * Set the shadow register freeze bit.
     */
    PSR |= 0x800003fb;
    SXIP = ip | 2;

    if (ex == E_CACC) {
        SXIP |= 1;
    }

    /*
     * If we faulted on the instruction following a delayed 
     * branch instruction, the SNIP will not follow the SXIP, but will 
     * instead point to the branch target.
     */
    if (r_delayed_p) {
        SNIP = delayed_ip | 2;
        SFIP = (delayed_ip + 4) | 2;
        r_delayed_p = 0;
    } else {
        SNIP = (ip + 4) | 2;
        SFIP = (ip + 8) | 2;
    }
  
    /*
     * See if the next instruction pointer and the fetched instruction
     * pointer are valid.  If not, set their exception bits.
     */
    if (!l_to_d(SNIP & ~3, usmode, 0)) {
        SNIP |= 1;
    }

    SSBR = 0;           /* No scoreboard in simulator */
    usmode = M_SUPERVISOR;
    SET_TLB_PTRS;

    ip = VBR + ex * 8;
    pagela = ip & ~PAGEMASK;
    p = (u_long)l_to_d(ip, M_SUPERVISOR, &pagep);
    if (!p) {
        sim_printf("Code fault trying to execute exception vector at\n");
        sim_printf("address %x, exception=%x", ip, ex);
        cause = HOSED;
        goto return_pt;
    }
    if (ss) {
        goto return_pt;
    }
    if (tracing) {
        insert_ss_breakpoint(ip, usmode);
    }
    DISPATCH;

L(stack_overflow);
    sim_printf("stack overflow, r31=0x%08x, cr21=0x%08x\n",
		regs[31], STACKBASE);
    cause = HOSED;
    goto return_pt;

L(interrupt_label);
    if (sim_interrupt_flag & INT_SINGLESTEP) {
        sim_interrupt_flag &= ~INT_SINGLESTEP;
        goto ss_breakpoint;
    }

    if (sim_interrupt_flag & INT_SIGNAL) {
        sim_interrupt_flag &= ~INT_SIGNAL;
        callhandlers();
        DISP_AFTER_BR;
    }

    if (sim_interrupt_flag & INT_FRONT_END) {
        sim_interrupt_flag &= ~INT_FRONT_END;
        cause = INTERRUPT;
        goto return_pt;
    }

    if (sim_interrupt_flag & INT_INTERNAL) {
        sim_interrupt_flag &= ~INT_INTERNAL;
        cause = INTERRUPT;
        goto return_pt;
    }

    if (sim_interrupt_flag & INT_DEVICE) {
        sim_interrupt_flag &= ~INT_DEVICE;
        /*
         * It is possible that between the time the sim_interrupt_flag
         * was set and the time that it was read that the PSR interrupt
         * bit has been reset.  If so, we continue with the intruction
         * stream after reseting this bit.  sim_interrupt_flag will
         * be set again when an stcr or xcr enables interrupts again.
         */
        if (!PSR_INT_ENABLED) {
            DISP_AFTER_BR;
        }
        GEN_EXCEPTION(E_INT);
    }

    if (sim_interrupt_flag & INT_RESET) {
        sim_interrupt_flag &= ~INT_RESET;
        GEN_EXCEPTION(E_RST);
    }

    if (sim_interrupt_flag & INT_MPSWITCH) {
      sim_interrupt_flag &= ~INT_MPSWITCH;
      cause = MPSWITCH;
      goto return_pt;
    }

    sim_printf("Unknown interrupt, sim_interrupt_flag=0x%x\n", 
       sim_interrupt_flag);
    sim_interrupt_flag = 0;
    cause = HOSED;


L(return_pt);
    /*
     * Copy our local representation of the instruction pointer
     * to the global that the debugger reads.
     */
    ip = IP;

    /* If the shadow-frozen bit in the PSR is off, the real chip
       continuously copies the working instruction pointers and the
       processor status register to the shadow registers.  We do 
       the same before returning to the user */

    if ((PSR & 1) == 0) {
      SNIP = ip | 2;
      TPSR = PSR;
      if (delayed_p) {
        SFIP = delayed_ip | 2;
      } else {
        SFIP = (ip + 4) | 2;
      }
    }
  
    /*
     * If we are single stepping we need to check to see if a device
     * interrupt is pending before we return.
     */
    if (ss) {
        sim_interrupt_flag &= ~INT_SINGLESTEP;
	if (cause != HOSED) {
            CHECK_STACK;
        }
    }

return_with_exception:

    callhandlers();
    sim_in_execution = 0;

    /*
     * This used to be "if (!ss) ..."  AJK
     */
    if (!single_step_command) {
        HoldOscillator();
    }

    /*
     * Copy our local variable copy of the decoded-delayed-branch
     * pointer to the global so that when we start up next we
     * will execute properly.
     */
    delayed_p = r_delayed_p;

    if (!sim_zero()) {
	return cause;
    }
    goto dummies;

L(bcnd_eq0);
    if (SRC1 == 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_ne0);
    if (SRC1 != 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_gt0);
    if ((int)SRC1 > 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_lt0);
    if ((int)SRC1 < 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_ge0);
    if ((int)SRC1 >= 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_le0);
    if ((int)SRC1 <= 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(noop);
    DISPATCH_NEXT;

L(bcnd_4);
    s1 = SRC1;
    if (s1 != 0x80000000 && s1 < 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_5);
    if ((SRC1 & 0x7fffffff) != 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_6);
    s1 = SRC1;
    if (s1 <= 0 && s1 != 0x80000000) { 
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_7);
    if (SRC1 != 0x80000000) { 
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_8);
    if (SRC1 == 0x80000000) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_9);
    s1 = SRC1;
    if (s1 > 0 || s1 == 0x80000000) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_10);
    if ((SRC1 & 0x7fffffff) == 0) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_11);
    s1 = SRC1;
    if (s1 >= 0 || s1 == 0x80000000) {
        TAKE_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_always);
    TAKE_BRANCH;

L(bcnd_eq0_n);
    if ((int)SRC1 == 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_ne0_n);
    if ((int)SRC1 != 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_gt0_n);
    if ((int)SRC1 > 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_lt0_n);
    if ((int)SRC1 < 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_ge0_n);
    if ((int)SRC1 >= 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_le0_n);
    if ((int)SRC1 <= 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_4_n);
    s1 = SRC1;
    if (s1 != 0x80000000 && s1 < 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_5_n);
    if ((SRC1 & 0x7fffffff) != 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_6_n);
    s1 = SRC1;
    if (s1 <= 0 && s1 != 0x80000000) { 
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_7_n);
    if (SRC1 != 0x80000000) { 
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_8_n);
    if (SRC1 == 0x80000000) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_9_n);
    s1 = SRC1;
    if (s1 > 0 || s1 == 0x80000000) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_10_n);
    if ((SRC1 & 0x7fffffff) == 0) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_11_n);
    s1 = SRC1;
    if (s1 >= 0 || s1 == 0x80000000) {
        TAKE_DELAYED_BRANCH;
    }
    DISPATCH_NEXT;

L(bcnd_always_n);
    TAKE_DELAYED_BRANCH;

L(br);
    TAKE_BRANCH;

L(br_n);
    TAKE_DELAYED_BRANCH;

#define BB0(bit) L(bb0_##bit); if ((SRC1 & (1 << bit)) == 0) TAKE_BRANCH; \
                                  DISPATCH_NEXT;

BB0(0);  BB0(1);  BB0(2);  BB0(3);  BB0(4);  BB0(5);  BB0(6);  BB0(7);
BB0(8);  BB0(9);  BB0(10); BB0(11); BB0(12); BB0(13); BB0(14); BB0(15);
BB0(16); BB0(17); BB0(18); BB0(19); BB0(20); BB0(21); BB0(22); BB0(23);
BB0(24); BB0(25); BB0(26); BB0(27); BB0(28); BB0(29); BB0(30); BB0(31);

#undef BB0

#define BB1(bit) L(bb1_##bit); if (SRC1 & (1 << bit)) TAKE_BRANCH; \
                                  DISPATCH_NEXT;

BB1(0);  BB1(1);  BB1(2);  BB1(3);  BB1(4);  BB1(5);  BB1(6);  BB1(7);
BB1(8);  BB1(9);  BB1(10); BB1(11); BB1(12); BB1(13); BB1(14); BB1(15);
BB1(16); BB1(17); BB1(18); BB1(19); BB1(20); BB1(21); BB1(22); BB1(23);
BB1(24); BB1(25); BB1(26); BB1(27); BB1(28); BB1(29); BB1(30); BB1(31);

#undef BB1

#define BB0_N(bit) L(bb0_n_##bit); if ((SRC1 & (1 << bit)) == 0)  \
                                        TAKE_DELAYED_BRANCH;        \
                                     DISPATCH_NEXT;

BB0_N(0);  BB0_N(1);  BB0_N(2);  BB0_N(3);
BB0_N(4);  BB0_N(5);  BB0_N(6);  BB0_N(7);
BB0_N(8);  BB0_N(9);  BB0_N(10); BB0_N(11);
BB0_N(12); BB0_N(13); BB0_N(14); BB0_N(15);
BB0_N(16); BB0_N(17); BB0_N(18); BB0_N(19);
BB0_N(20); BB0_N(21); BB0_N(22); BB0_N(23);
BB0_N(24); BB0_N(25); BB0_N(26); BB0_N(27);
BB0_N(28); BB0_N(29); BB0_N(30); BB0_N(31);

#undef BB0_N

#define BB1_N(bit) L(bb1_n_##bit); if (SRC1 & (1 << bit))        \
                                         TAKE_DELAYED_BRANCH;      \
                                     DISPATCH_NEXT;

BB1_N(0);  BB1_N(1);  BB1_N(2);  BB1_N(3);
BB1_N(4);  BB1_N(5);  BB1_N(6);  BB1_N(7);
BB1_N(8);  BB1_N(9);  BB1_N(10); BB1_N(11);
BB1_N(12); BB1_N(13); BB1_N(14); BB1_N(15);
BB1_N(16); BB1_N(17); BB1_N(18); BB1_N(19);
BB1_N(20); BB1_N(21); BB1_N(22); BB1_N(23);
BB1_N(24); BB1_N(25); BB1_N(26); BB1_N(27);
BB1_N(28); BB1_N(29); BB1_N(30); BB1_N(31);

#undef BB1_N

L(bsr);
    regs[1] = IP + 4;
    TAKE_BRANCH;

L(bsr_n);
    regs[1] = IP + 8;
    TAKE_DELAYED_BRANCH;

    /*
     * Here we fool the compiler by having a reference to each
     * instruction-entry point label.
     */

dummies:

#undef L
#define L(x) if (sim_zero()) goto x;
#define BB0(x) if (sim_zero()) goto bb0_##x;
#define BB1(x) if (sim_zero()) goto bb1_##x;
#define BB0_N(x) if (sim_zero()) goto bb0_n_##x;
#define BB1_N(x) if (sim_zero()) goto bb1_n_##x;

asm("| START DELETING HERE");
#include "extern.h"
asm("| STOP DELETING HERE");

#undef L
#undef BB0
#undef BB1
#undef BB0_N
#undef BB1_N

}
