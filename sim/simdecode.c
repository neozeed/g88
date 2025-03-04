/*
 * This decodes 88000 instructions
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 * 
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/simdecode.c,v 1.28 90/08/02 12:19:33 robertb Exp $
 */

#include "sim.h"
#include "decode.h"
#include "format.h"
#include "fields88.h"

/*
 * These macros turn the lines of extern.h into extern declarations.
 */
#define L(x)      extern sim_##x();
#define BB0(x)    extern sim_bb0_##x();
#define BB1(x)    extern sim_bb1_##x();
#define BB0_N(x)  extern sim_bb0_n_##x();
#define BB1_N(x)  extern sim_bb1_n_##x();


#include "extern.h"

#undef L
#undef BB0
#undef BB1
#undef BB0_N
#undef BB1_N

/*
 * This is pointed to when we decode an instruction that tries
 * to modify a register that is read-only.  Like r0, or dmt3.
 *
 * We need two words in case we have a double load to modify.
 */
double dummy_r0;

#define	POOLSIZE	(1022)

struct litpool {
    struct litpool *next;
    u_long litcnt;
    u_long lits[POOLSIZE]; } *litpool = 0;

/*
 * Puts the passed value into the literal table.  Returns a pointer
 * to it.
 */
u_long *mkliteral(value)
    u_long value;
{
    struct litpool *newpool;
    u_long *ptr;

    if (!litpool) {
        litpool = (struct litpool *)malloc(sizeof(struct litpool));
        if (!litpool) {
            err("mkliteral:no space for 1st pool.\n", __LINE__, __FILE__);
        }
        litpool->litcnt = 0;
        litpool->next = 0;
    }
    if (litpool->litcnt == POOLSIZE) {
        newpool = (struct litpool *)malloc(sizeof(struct litpool));
        if (!newpool) {
            err("mkliteral:no space for new pool.\n", __LINE__, __FILE__);
        }
        newpool->litcnt = 0;
        newpool->next = litpool;
        litpool = newpool;
    }
    ptr = &litpool->lits[litpool->litcnt++];
    *ptr = value;
    return ptr;
}

/*
 * This deallocates the literal pools.
 */
void
free_literals()
{
    struct litpool *p, *np;

    p = litpool;
    litpool = (struct litpool *)0;
    while (p) {
        np = p->next;
        free(p);
        p = np;
    }
}

/*
 * This decodes a 88000 instruction into a form for quick execution.
 * The passed parameter points to the decode_i structure that will
 * receive the decoded form.  The second parameter is the instruction
 * pointer that the instruction resides at.  If there should be a
 * code access fault, a 1 is returned.  Otherwise a 0 is returned.
 */
int
decode(p, ip, usmode)
    struct decoded_i   *p;
    int ip;
{
    u_long instr;
    struct instr_info *instr_info;
    u_long physical_address;
    u_long target_addr;
    int exception;

    exception = l_to_p(usmode, ip, &physical_address, CODE_CMMU(ip), READ);
    if (exception != E_NONE) {
        return 1;
    }
    instr = read_sim_w(physical_address);
    if (sim_errno) {
        return 1;
    }

    instr_info = sim_instruction_lookup(instr);

    if (instr_info == (struct instr_info *)0) {
        p->norm_e_addr = sim_opc_exception;
        return 0;
    }

    p->norm_e_addr = instr_info->norm_e_addr;

    switch (instr_info->format) {
        /*
         * Load, store, xmem, and register-to-register instructions
         */
        case STRO:
        case STRI:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = (u_long *)(&regs[S2(instr)]);
            p->dest = (u_long *)(&regs[D(instr)]);
            break;


        case STLIT:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = mkliteral(LIT16(instr));
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        case LDRO:
        case LDRI:
        case INTRR:
        case INTRR2OP:
        case INTRR_S1_S2:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = (u_long *)(&regs[S2(instr)]);
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        case LDLIT:
        case INTRL:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = mkliteral(LIT16(instr));
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        case ROT:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = mkliteral(O5(instr));
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        case BITFIELD:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = mkliteral(instr & 0x3ff);
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        /*
         * Transfer of control instructions.
         */
        case BITBRANCH:
            p->s1 = (u_long *)(&regs[S1(instr)]);

            /*
             * See if the target address is on the same page as
             * the branch.  If so, set the s2 field to the decoded
             * instruction pointer of the target.  If not, set
             * s2 to zero and set the 'dest' field to the signed
             * offset of the target (in bytes).
             */
            target_addr = (int)ip + REL16(instr);
            if ((target_addr & ~PAGEMASK) == (ip & ~PAGEMASK)) {
                p->s2 = (u_long *)l_to_d(target_addr, usmode, 0);
            } else {
                p->dest = (u_long *)REL16(instr);
                p->s2 = (u_long *)0;
            }
            break;

        case CBRANCH:
            p->s1 = (u_long *)(&regs[S1(instr)]);

            /*
             * See if the target address is on the same page as
             * the branch.  If so, set the s2 field to the decoded
             * instruction pointer of the target.  If not, set
             * s2 to zero and set the 'dest' field to the signed
             * offset of the target (in bytes).
             */
            target_addr = (int)ip + REL16(instr);
            if ((target_addr & ~PAGEMASK) == (ip & ~PAGEMASK)) {
                p->s2 = (u_long *)l_to_d(target_addr, usmode, 0);
            } else {
                p->dest = (u_long *)REL16(instr);
                p->s2 = (u_long *)0;
            }
            break;

        case IPREL:
            /*
             * See if the target address is on the same page as
             * the branch.  If so, set the s2 field to the decoded
             * instruction pointer of the target.  If not, set
             * s2 to zero and set the 'dest' field to the signed
             * offset of the target (in bytes).
             */
            target_addr = (int)ip + REL26(instr);
            if ((target_addr & ~PAGEMASK) == (ip & ~PAGEMASK)) {
                p->s2 = (u_long *)l_to_d(target_addr, usmode, 0);
            } else {
                p->dest = (u_long *)REL26(instr);
                p->s2 = (u_long *)0;
            }
            break;

        case JMP:
            p->dest = (u_long *)(&regs[S2(instr)]);
            p->s2 = (u_long *)0;    /* For delayed jumps. */
            break;

        /*
         * Trap instructions.
         */
        case TRAP:
            p->dest = (u_long *)(1 << B5(instr));
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = (u_long *)(VEC(instr));
            break;

        case TBND:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = mkliteral(LIT16(instr));
            break;

        case TCND:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            p->s2 = (u_long *)(VEC(instr));
            p->dest = (u_long *)(M5(instr));
            break;
            
        case RTE:
            break;

        /*
         * Control register manipulation instructions.
         */
        case FLDCR:
            p->dest = (u_long *)(&regs[D(instr)]);
            p->s1 = (u_long *)(&sfu1_regs[CRS(instr)]);
            if (CRS(instr) >= 62) {
                p->norm_e_addr = sim_uldcr;
            }
            break;
            
        case LDCR:
            p->dest = (u_long *)(&regs[D(instr)]);
            p->s1 = (u_long *)(&sfu0_regs[CRS(instr)]);
            break;
            
        case FSTCR:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            /*
             * Check to see if its a real fp register.  If not, make
             * the destination point to our dummy destination.  We
             * do this so that we don't have to zero the 53 SFU1 locations
             * that do not have RAM behind them after doing a fstcr.  
             * A fine point.
             */
            if (CRS(instr) <= 8 || CRS(instr) >= 62) {
                p->dest = (u_long *)(&sfu1_regs[CRS(instr)]);
            } else {
                p->dest = (u_long *)&dummy_r0;
            }
            if (CRS(instr) >= 62) {
                p->norm_e_addr = sim_ustcr;
            }
            break;
            
        case STCR:
            p->s1 = (u_long *)(&regs[S1(instr)]);
            switch (CRS(instr)) {
                case 1: case 2: case 3: case 5: case 6: case 7:
                case 17: case 18: case 19: case 20: case 21 /* STACKBASE */:
                    p->dest = (u_long *)(&sfu0_regs[CRS(instr)]);
                    break;

                /*
                 * The rest of the registers are read only.
                 */
                default:
                    p->dest = (u_long *)&dummy_r0;
                    break;
            }
            break;

        case XCR:
            p->s2 = (u_long *)(&sfu0_regs[CRS(instr)]);
            switch (CRS(instr)) {
                case 1: case 2: case 3: case 5: case 6: case 7:
                case 17: case 18: case 19: case 20:
                    p->s1 = (u_long *)(&regs[S1(instr)]);
                    break;

                /*
                 * The rest of the registers are read only, so we
                 * point the source pointer at the control register,
                 * which makes the store into the control register
                 * not change its value.
                 */
                default:
                    p->s1 = (u_long *)(&sfu0_regs[CRS(instr)]);
                    break;
            }
            p->dest = (u_long *)(&regs[D(instr)]);
            break;

        case FXCR:
            p->s2 = (u_long *)(&sfu1_regs[CRS(instr)]);
            if (CRS(instr) < 9 || CRS(instr) > 61) {
                p->s1 = (u_long *)(&regs[S1(instr)]);
            } else {
                /*
                 * The rest of the registers are read only, so we
                 * point the source pointer at the control register,
                 * which makes the store into the control register
                 * not change its value.
                 */
                p->s1 = (u_long *)(&sfu1_regs[CRS(instr)]);
            }
            p->dest = (u_long *)(&regs[D(instr)]);
            if (CRS(instr) >= 62) {
                p->norm_e_addr = sim_uxcr;
            }
            break;
            
        default:
            p->norm_e_addr = sim_opc_exception;
            return 0;
            break;
    }
    if (instr_info->fixup) {
        instr_info->fixup(instr_info, instr, p);
    }
    return 0;
}

/*
 * We check to see if the destination is r0.  If so, we change the
 * instruction to point to a dummy copy of r0.
 */
fix_r0(instr_info, instr, p)
    struct instr_info *instr_info;
    u_long instr;
    struct decoded_i *p;
{
    if (p->dest == &regs[0]) {
        p->dest = (u_long *)&dummy_r0;
    }
}

/*
 * Put the literal in the top 16 bits and 1's into the lower 16 bits
 * and make a new literal.
 */
fix_and_u(instr_info, instr, p)
    struct instr_info *instr_info;
    u_long instr;
    struct decoded_i *p;
{
    *(p->s2) = (*(p->s2) << 16) | 0xffff;
    fix_r0(instr_info, instr, p);
}

/*
 * The literal form of the and instruction does not affect the upper
 * 16 bits, so we or in 1's to the upper 16 bits of the value and
 * make a new literal.
 */
fix_and(instr_info, instr, p)
    struct instr_info *instr_info;
    u_long instr;
    struct decoded_i *p;
{
    *(p->s2) |= 0xffff0000;
    fix_r0(instr_info, instr, p);
}

fix_mask_u(instr_info, instr, p)
    struct instr_info *instr_info;
    u_long instr;
    struct decoded_i *p;
{
    *(p->s2) <<= 16;
    fix_r0(instr_info, instr, p);
}

fix_or_u(instr_info, instr, p)
    struct instr_info *instr_info;
    u_long instr;
    struct decoded_i *p;
{
    *(p->s2) <<= 16;
    fix_r0(instr_info, instr, p);
}

#define REF(x)  (struct instr_info *)0, sim_##x
#define NOFIX   ((int (*)())0)

static
struct instr_info in_tab[] = {
0x70000000, 0xfc000000, "add",      INTRL,      REF(add),       fix_r0,
0xf4007000, 0xfc00ffe0, "add",      INTRR,      REF(add),       fix_r0,
0xf4007100, 0xfc00ffe0, "add.co",   INTRR,      REF(add_co),    fix_r0,
0xf4007200, 0xfc00ffe0, "add.ci",   INTRR,      REF(add_ci),    fix_r0,
0xf4007300, 0xfc00ffe0, "add.cio",  INTRR,      REF(add_cio),   fix_r0,

0x60000000, 0xfc000000, "addu",     INTRL,      REF(addu),      fix_r0,
0xf4006000, 0xfc00ffe0, "addu",     INTRR,      REF(addu),      fix_r0,
0xf4006100, 0xfc00ffe0, "addu.co",  INTRR,      REF(addu_co),   fix_r0,
0xf4006200, 0xfc00ffe0, "addu.ci",  INTRR,      REF(addu_ci),   fix_r0,
0xf4006300, 0xfc00ffe0, "addu.cio", INTRR,      REF(addu_cio),  fix_r0,

0x40000000, 0xfc000000, "and",      INTRL,      REF(and),       fix_and,
0x44000000, 0xfc000000, "and.u",    INTRL,      REF(and),       fix_and_u,
0xf4004000, 0xfc00ffe0, "and",      INTRR,      REF(and),       fix_r0,
0xf4004400, 0xfc00ffe0, "and.c",    INTRR,      REF(and_c),     fix_r0,

0xe8400000, 0xffe00000, "bcnd_eq0", CBRANCH,    REF(bcnd_eq0),  NOFIX,
0xe9a00000, 0xffe00000, "bcnd_ne0", CBRANCH,    REF(bcnd_ne0),  NOFIX,
0xe8200000, 0xffe00000, "bcnd_gt0", CBRANCH,    REF(bcnd_gt0),  NOFIX,
0xe9800000, 0xffe00000, "bcnd_lt0", CBRANCH,    REF(bcnd_lt0),  NOFIX,
0xe8600000, 0xffe00000, "bcnd_ge0", CBRANCH,    REF(bcnd_ge0),  NOFIX,
0xe9c00000, 0xffe00000, "bcnd_le0", CBRANCH,    REF(bcnd_le0),  NOFIX,

0xe8000000, 0xffe00000, "bcnd_0",   CBRANCH,    REF(noop),      NOFIX,
0xe8800000, 0xffe00000, "bcnd_4",   CBRANCH,    REF(bcnd_4),    NOFIX,
0xe8a00000, 0xffe00000, "bcnd_5",   CBRANCH,    REF(bcnd_5),    NOFIX,
0xe8c00000, 0xffe00000, "bcnd_6",   CBRANCH,    REF(bcnd_6),    NOFIX,
0xe8e00000, 0xffe00000, "bcnd_7",   CBRANCH,    REF(bcnd_7),    NOFIX,
0xe9000000, 0xffe00000, "bcnd_8",   CBRANCH,    REF(bcnd_8),    NOFIX,
0xe9200000, 0xffe00000, "bcnd_9",   CBRANCH,    REF(bcnd_9),    NOFIX,
0xe9400000, 0xffe00000, "bcnd_10",  CBRANCH,    REF(bcnd_10),   NOFIX,
0xe9600000, 0xffe00000, "bcnd_11",  CBRANCH,    REF(bcnd_11),   NOFIX,
0xe9e00000, 0xffe00000, "bcnd_always",CBRANCH,  REF(bcnd_always),   NOFIX,

0xec400000, 0xffe00000, "bcnd_eq0_n", CBRANCH,  REF(bcnd_eq0_n),NOFIX,
0xeda00000, 0xffe00000, "bcnd_ne0_n", CBRANCH,  REF(bcnd_ne0_n),NOFIX,
0xec200000, 0xffe00000, "bcnd_gt0_n", CBRANCH,  REF(bcnd_gt0_n),NOFIX,
0xed800000, 0xffe00000, "bcnd_lt0_n", CBRANCH,  REF(bcnd_lt0_n),NOFIX,
0xec600000, 0xffe00000, "bcnd_ge0_n", CBRANCH,  REF(bcnd_ge0_n),NOFIX,
0xedc00000, 0xffe00000, "bcnd_le0_n", CBRANCH,  REF(bcnd_le0_n),NOFIX,

0xec000000, 0xffe00000, "bcnd_0_n",   CBRANCH,    REF(noop),      NOFIX,
0xec800000, 0xffe00000, "bcnd_4_n",   CBRANCH,    REF(bcnd_4_n),    NOFIX,
0xeca00000, 0xffe00000, "bcnd_5_n",   CBRANCH,    REF(bcnd_5_n),    NOFIX,
0xecc00000, 0xffe00000, "bcnd_6_n",   CBRANCH,    REF(bcnd_6_n),    NOFIX,
0xece00000, 0xffe00000, "bcnd_7_n",   CBRANCH,    REF(bcnd_7_n),    NOFIX,
0xed000000, 0xffe00000, "bcnd_8_n",   CBRANCH,    REF(bcnd_8_n),    NOFIX,
0xed200000, 0xffe00000, "bcnd_9_n",   CBRANCH,    REF(bcnd_9_n),    NOFIX,
0xed400000, 0xffe00000, "bcnd_10_n",  CBRANCH,    REF(bcnd_10_n),   NOFIX,
0xed600000, 0xffe00000, "bcnd_11_n",  CBRANCH,    REF(bcnd_11_n),   NOFIX,
0xede00000, 0xffe00000, "bcnd_always_n",CBRANCH,  REF(bcnd_always_n),   NOFIX,

0xc0000000, 0xfc000000, "br",       IPREL,      REF(br),        NOFIX,
0xc4000000, 0xfc000000, "br.n",     IPREL,      REF(br_n),      NOFIX,

0xd0000000, 0xffe00000, "bb0_0",    BITBRANCH,  REF(bb0_0),     NOFIX,
0xd0200000, 0xffe00000, "bb0_1",    BITBRANCH,  REF(bb0_1),     NOFIX,
0xd0400000, 0xffe00000, "bb0_2",    BITBRANCH,  REF(bb0_2),     NOFIX,
0xd0600000, 0xffe00000, "bb0_3",    BITBRANCH,  REF(bb0_3),     NOFIX,
0xd0800000, 0xffe00000, "bb0_4",    BITBRANCH,  REF(bb0_4),     NOFIX,
0xd0a00000, 0xffe00000, "bb0_5",    BITBRANCH,  REF(bb0_5),     NOFIX,
0xd0c00000, 0xffe00000, "bb0_6",    BITBRANCH,  REF(bb0_6),     NOFIX,
0xd0e00000, 0xffe00000, "bb0_7",    BITBRANCH,  REF(bb0_7),     NOFIX,
0xd1000000, 0xffe00000, "bb0_8",    BITBRANCH,  REF(bb0_8),     NOFIX,
0xd1200000, 0xffe00000, "bb0_9",    BITBRANCH,  REF(bb0_9),     NOFIX,
0xd1400000, 0xffe00000, "bb0_10",   BITBRANCH,  REF(bb0_10),    NOFIX,
0xd1600000, 0xffe00000, "bb0_11",   BITBRANCH,  REF(bb0_11),    NOFIX,
0xd1800000, 0xffe00000, "bb0_12",   BITBRANCH,  REF(bb0_12),    NOFIX,
0xd1a00000, 0xffe00000, "bb0_13",   BITBRANCH,  REF(bb0_13),    NOFIX,
0xd1c00000, 0xffe00000, "bb0_14",   BITBRANCH,  REF(bb0_14),    NOFIX,
0xd1e00000, 0xffe00000, "bb0_15",   BITBRANCH,  REF(bb0_15),    NOFIX,
0xd2000000, 0xffe00000, "bb0_16",   BITBRANCH,  REF(bb0_16),    NOFIX,
0xd2200000, 0xffe00000, "bb0_17",   BITBRANCH,  REF(bb0_17),    NOFIX,
0xd2400000, 0xffe00000, "bb0_18",   BITBRANCH,  REF(bb0_18),    NOFIX,
0xd2600000, 0xffe00000, "bb0_19",   BITBRANCH,  REF(bb0_19),    NOFIX,
0xd2800000, 0xffe00000, "bb0_20",   BITBRANCH,  REF(bb0_20),    NOFIX,
0xd2a00000, 0xffe00000, "bb0_21",   BITBRANCH,  REF(bb0_21),    NOFIX,
0xd2c00000, 0xffe00000, "bb0_22",   BITBRANCH,  REF(bb0_22),    NOFIX,
0xd2e00000, 0xffe00000, "bb0_23",   BITBRANCH,  REF(bb0_23),    NOFIX,
0xd3000000, 0xffe00000, "bb0_24",   BITBRANCH,  REF(bb0_24),    NOFIX,
0xd3200000, 0xffe00000, "bb0_25",   BITBRANCH,  REF(bb0_25),    NOFIX,
0xd3400000, 0xffe00000, "bb0_26",   BITBRANCH,  REF(bb0_26),    NOFIX,
0xd3600000, 0xffe00000, "bb0_27",   BITBRANCH,  REF(bb0_27),    NOFIX,
0xd3800000, 0xffe00000, "bb0_28",   BITBRANCH,  REF(bb0_28),    NOFIX,
0xd3a00000, 0xffe00000, "bb0_29",   BITBRANCH,  REF(bb0_29),    NOFIX,
0xd3c00000, 0xffe00000, "bb0_30",   BITBRANCH,  REF(bb0_30),    NOFIX,
0xd3e00000, 0xffe00000, "bb0_31",   BITBRANCH,  REF(bb0_31),    NOFIX,

0xd8000000, 0xffe00000, "bb1_0",    BITBRANCH,  REF(bb1_0),     NOFIX,
0xd8200000, 0xffe00000, "bb1_1",    BITBRANCH,  REF(bb1_1),     NOFIX,
0xd8400000, 0xffe00000, "bb1_2",    BITBRANCH,  REF(bb1_2),     NOFIX,
0xd8600000, 0xffe00000, "bb1_3",    BITBRANCH,  REF(bb1_3),     NOFIX,
0xd8800000, 0xffe00000, "bb1_4",    BITBRANCH,  REF(bb1_4),     NOFIX,
0xd8a00000, 0xffe00000, "bb1_5",    BITBRANCH,  REF(bb1_5),     NOFIX,
0xd8c00000, 0xffe00000, "bb1_6",    BITBRANCH,  REF(bb1_6),     NOFIX,
0xd8e00000, 0xffe00000, "bb1_7",    BITBRANCH,  REF(bb1_7),     NOFIX,
0xd9000000, 0xffe00000, "bb1_8",    BITBRANCH,  REF(bb1_8),     NOFIX,
0xd9200000, 0xffe00000, "bb1_9",    BITBRANCH,  REF(bb1_9),     NOFIX,
0xd9400000, 0xffe00000, "bb1_10",   BITBRANCH,  REF(bb1_10),    NOFIX,
0xd9600000, 0xffe00000, "bb1_11",   BITBRANCH,  REF(bb1_11),    NOFIX,
0xd9800000, 0xffe00000, "bb1_12",   BITBRANCH,  REF(bb1_12),    NOFIX,
0xd9a00000, 0xffe00000, "bb1_13",   BITBRANCH,  REF(bb1_13),    NOFIX,
0xd9c00000, 0xffe00000, "bb1_14",   BITBRANCH,  REF(bb1_14),    NOFIX,
0xd9e00000, 0xffe00000, "bb1_15",   BITBRANCH,  REF(bb1_15),    NOFIX,
0xda000000, 0xffe00000, "bb1_16",   BITBRANCH,  REF(bb1_16),    NOFIX,
0xda200000, 0xffe00000, "bb1_17",   BITBRANCH,  REF(bb1_17),    NOFIX,
0xda400000, 0xffe00000, "bb1_18",   BITBRANCH,  REF(bb1_18),    NOFIX,
0xda600000, 0xffe00000, "bb1_19",   BITBRANCH,  REF(bb1_19),    NOFIX,
0xda800000, 0xffe00000, "bb1_20",   BITBRANCH,  REF(bb1_20),    NOFIX,
0xdaa00000, 0xffe00000, "bb1_21",   BITBRANCH,  REF(bb1_21),    NOFIX,
0xdac00000, 0xffe00000, "bb1_22",   BITBRANCH,  REF(bb1_22),    NOFIX,
0xdae00000, 0xffe00000, "bb1_23",   BITBRANCH,  REF(bb1_23),    NOFIX,
0xdb000000, 0xffe00000, "bb1_24",   BITBRANCH,  REF(bb1_24),    NOFIX,
0xdb200000, 0xffe00000, "bb1_25",   BITBRANCH,  REF(bb1_25),    NOFIX,
0xdb400000, 0xffe00000, "bb1_26",   BITBRANCH,  REF(bb1_26),    NOFIX,
0xdb600000, 0xffe00000, "bb1_27",   BITBRANCH,  REF(bb1_27),    NOFIX,
0xdb800000, 0xffe00000, "bb1_28",   BITBRANCH,  REF(bb1_28),    NOFIX,
0xdba00000, 0xffe00000, "bb1_29",   BITBRANCH,  REF(bb1_29),    NOFIX,
0xdbc00000, 0xffe00000, "bb1_30",   BITBRANCH,  REF(bb1_30),    NOFIX,
0xdbe00000, 0xffe00000, "bb1_31",   BITBRANCH,  REF(bb1_31),    NOFIX,

0xd4000000, 0xffe00000, "bb0_n_0",  BITBRANCH,  REF(bb0_n_0),   NOFIX,
0xd4200000, 0xffe00000, "bb0_n_1",  BITBRANCH,  REF(bb0_n_1),   NOFIX,
0xd4400000, 0xffe00000, "bb0_n_2",  BITBRANCH,  REF(bb0_n_2),   NOFIX,
0xd4600000, 0xffe00000, "bb0_n_3",  BITBRANCH,  REF(bb0_n_3),   NOFIX,
0xd4800000, 0xffe00000, "bb0_n_4",  BITBRANCH,  REF(bb0_n_4),   NOFIX,
0xd4a00000, 0xffe00000, "bb0_n_5",  BITBRANCH,  REF(bb0_n_5),   NOFIX,
0xd4c00000, 0xffe00000, "bb0_n_6",  BITBRANCH,  REF(bb0_n_6),   NOFIX,
0xd4e00000, 0xffe00000, "bb0_n_7",  BITBRANCH,  REF(bb0_n_7),   NOFIX,
0xd5000000, 0xffe00000, "bb0_n_8",  BITBRANCH,  REF(bb0_n_8),   NOFIX,
0xd5200000, 0xffe00000, "bb0_n_9",  BITBRANCH,  REF(bb0_n_9),   NOFIX,
0xd5400000, 0xffe00000, "bb0_n_10", BITBRANCH,  REF(bb0_n_10),  NOFIX,
0xd5600000, 0xffe00000, "bb0_n_11", BITBRANCH,  REF(bb0_n_11),  NOFIX,
0xd5800000, 0xffe00000, "bb0_n_12", BITBRANCH,  REF(bb0_n_12),  NOFIX,
0xd5a00000, 0xffe00000, "bb0_n_13", BITBRANCH,  REF(bb0_n_13),  NOFIX,
0xd5c00000, 0xffe00000, "bb0_n_14", BITBRANCH,  REF(bb0_n_14),  NOFIX,
0xd5e00000, 0xffe00000, "bb0_n_15", BITBRANCH,  REF(bb0_n_15),  NOFIX,
0xd6000000, 0xffe00000, "bb0_n_16", BITBRANCH,  REF(bb0_n_16),  NOFIX,
0xd6200000, 0xffe00000, "bb0_n_17", BITBRANCH,  REF(bb0_n_17),  NOFIX,
0xd6400000, 0xffe00000, "bb0_n_18", BITBRANCH,  REF(bb0_n_18),  NOFIX,
0xd6600000, 0xffe00000, "bb0_n_19", BITBRANCH,  REF(bb0_n_19),  NOFIX,
0xd6800000, 0xffe00000, "bb0_n_20", BITBRANCH,  REF(bb0_n_20),  NOFIX,
0xd6a00000, 0xffe00000, "bb0_n_21", BITBRANCH,  REF(bb0_n_21),  NOFIX,
0xd6c00000, 0xffe00000, "bb0_n_22", BITBRANCH,  REF(bb0_n_22),  NOFIX,
0xd6e00000, 0xffe00000, "bb0_n_23", BITBRANCH,  REF(bb0_n_23),  NOFIX,
0xd7000000, 0xffe00000, "bb0_n_24", BITBRANCH,  REF(bb0_n_24),  NOFIX,
0xd7200000, 0xffe00000, "bb0_n_25", BITBRANCH,  REF(bb0_n_25),  NOFIX,
0xd7400000, 0xffe00000, "bb0_n_26", BITBRANCH,  REF(bb0_n_26),  NOFIX,
0xd7600000, 0xffe00000, "bb0_n_27", BITBRANCH,  REF(bb0_n_27),  NOFIX,
0xd7800000, 0xffe00000, "bb0_n_28", BITBRANCH,  REF(bb0_n_28),  NOFIX,
0xd7a00000, 0xffe00000, "bb0_n_29", BITBRANCH,  REF(bb0_n_29),  NOFIX,
0xd7c00000, 0xffe00000, "bb0_n_30", BITBRANCH,  REF(bb0_n_30),  NOFIX,
0xd7e00000, 0xffe00000, "bb0_n_31", BITBRANCH,  REF(bb0_n_31),  NOFIX,

0xdc000000, 0xffe00000, "bb1_n_0",  BITBRANCH,  REF(bb1_n_0),   NOFIX,
0xdc200000, 0xffe00000, "bb1_n_1",  BITBRANCH,  REF(bb1_n_1),   NOFIX,
0xdc400000, 0xffe00000, "bb1_n_2",  BITBRANCH,  REF(bb1_n_2),   NOFIX,
0xdc600000, 0xffe00000, "bb1_n_3",  BITBRANCH,  REF(bb1_n_3),   NOFIX,
0xdc800000, 0xffe00000, "bb1_n_4",  BITBRANCH,  REF(bb1_n_4),   NOFIX,
0xdca00000, 0xffe00000, "bb1_n_5",  BITBRANCH,  REF(bb1_n_5),   NOFIX,
0xdcc00000, 0xffe00000, "bb1_n_6",  BITBRANCH,  REF(bb1_n_6),   NOFIX,
0xdce00000, 0xffe00000, "bb1_n_7",  BITBRANCH,  REF(bb1_n_7),   NOFIX,
0xdd000000, 0xffe00000, "bb1_n_8",  BITBRANCH,  REF(bb1_n_8),   NOFIX,
0xdd200000, 0xffe00000, "bb1_n_9",  BITBRANCH,  REF(bb1_n_9),   NOFIX,
0xdd400000, 0xffe00000, "bb1_n_10", BITBRANCH,  REF(bb1_n_10),  NOFIX,
0xdd600000, 0xffe00000, "bb1_n_11", BITBRANCH,  REF(bb1_n_11),  NOFIX,
0xdd800000, 0xffe00000, "bb1_n_12", BITBRANCH,  REF(bb1_n_12),  NOFIX,
0xdda00000, 0xffe00000, "bb1_n_13", BITBRANCH,  REF(bb1_n_13),  NOFIX,
0xddc00000, 0xffe00000, "bb1_n_14", BITBRANCH,  REF(bb1_n_14),  NOFIX,
0xdde00000, 0xffe00000, "bb1_n_15", BITBRANCH,  REF(bb1_n_15),  NOFIX,
0xde000000, 0xffe00000, "bb1_n_16", BITBRANCH,  REF(bb1_n_16),  NOFIX,
0xde200000, 0xffe00000, "bb1_n_17", BITBRANCH,  REF(bb1_n_17),  NOFIX,
0xde400000, 0xffe00000, "bb1_n_18", BITBRANCH,  REF(bb1_n_18),  NOFIX,
0xde600000, 0xffe00000, "bb1_n_19", BITBRANCH,  REF(bb1_n_19),  NOFIX,
0xde800000, 0xffe00000, "bb1_n_20", BITBRANCH,  REF(bb1_n_20),  NOFIX,
0xdea00000, 0xffe00000, "bb1_n_21", BITBRANCH,  REF(bb1_n_21),  NOFIX,
0xdec00000, 0xffe00000, "bb1_n_22", BITBRANCH,  REF(bb1_n_22),  NOFIX,
0xdee00000, 0xffe00000, "bb1_n_23", BITBRANCH,  REF(bb1_n_23),  NOFIX,
0xdf000000, 0xffe00000, "bb1_n_24", BITBRANCH,  REF(bb1_n_24),  NOFIX,
0xdf200000, 0xffe00000, "bb1_n_25", BITBRANCH,  REF(bb1_n_25),  NOFIX,
0xdf400000, 0xffe00000, "bb1_n_26", BITBRANCH,  REF(bb1_n_26),  NOFIX,
0xdf600000, 0xffe00000, "bb1_n_27", BITBRANCH,  REF(bb1_n_27),  NOFIX,
0xdf800000, 0xffe00000, "bb1_n_28", BITBRANCH,  REF(bb1_n_28),  NOFIX,
0xdfa00000, 0xffe00000, "bb1_n_29", BITBRANCH,  REF(bb1_n_29),  NOFIX,
0xdfc00000, 0xffe00000, "bb1_n_30", BITBRANCH,  REF(bb1_n_30),  NOFIX,
0xdfe00000, 0xffe00000, "bb1_n_31", BITBRANCH,  REF(bb1_n_31),  NOFIX,

0xc8000000, 0xfc000000, "bsr",      IPREL,      REF(bsr),       NOFIX,
0xcc000000, 0xfc000000, "bsr.n",    IPREL,      REF(bsr_n),     NOFIX,

0xf0008000, 0xfc00fc00, "clr",      BITFIELD,   REF(clr),       fix_r0,
0xf4008000, 0xfc00ffe0, "clr",      INTRR,      REF(clr),       fix_r0,

0x7c000000, 0xfc000000, "cmp",      INTRL,      REF(cmp),       fix_r0,
0xf4007c00, 0xfc00fee0, "cmp",      INTRR,      REF(cmp),       fix_r0,

0x78000000, 0xfc000000, "div",      INTRL,      REF(div),       fix_r0,
0xf4007800, 0xfc00fee0, "div",      INTRR,      REF(div),       fix_r0,

0x68000000, 0xfc000000, "divu",     INTRL,      REF(divu),      fix_r0,
0xf4006800, 0xfc00fee0, "divu",     INTRR,      REF(divu),      fix_r0,

0xf0009000, 0xfc00fc00, "ext",      BITFIELD,   REF(ext),       fix_r0,
0xf4009000, 0xfc00ffe0, "ext",      INTRR,      REF(ext),       fix_r0,

0xf0009800, 0xfc00fc00, "extu",     BITFIELD,   REF(extu),      fix_r0,
0xf4009800, 0xfc00ffe0, "extu",     INTRR,      REF(extu),      fix_r0,

0x84002800, 0xfc00ffe0, "fadd.sss", INTRR,      REF(fadd_sss),  fix_r0,
0x84002880, 0xfc00ffe0, "fadd.ssd", INTRR,      REF(fadd_ssd),  fix_r0,
0x84002a00, 0xfc00ffe0, "fadd.sds", INTRR,      REF(fadd_sds),  fix_r0,
0x84002a80, 0xfc00ffe0, "fadd.sdd", INTRR,      REF(fadd_sdd),  fix_r0,
0x84002820, 0xfc00ffe0, "fadd.dss", INTRR,      REF(fadd_dss),  fix_r0,
0x840028a0, 0xfc00ffe0, "fadd.dsd", INTRR,      REF(fadd_dsd),  fix_r0,
0x84002a20, 0xfc00ffe0, "fadd.dds", INTRR,      REF(fadd_dds),  fix_r0,
0x84002aa0, 0xfc00ffe0, "fadd.ddd", INTRR,      REF(fadd_ddd),  fix_r0,

0x84003800, 0xfc00ffe0, "fcmp.sss", INTRR,      REF(fcmp_sss),  fix_r0,
0x84003880, 0xfc00ffe0, "fcmp.ssd", INTRR,      REF(fcmp_ssd),  fix_r0,
0x84003a00, 0xfc00ffe0, "fcmp.sds", INTRR,      REF(fcmp_sds),  fix_r0,
0x84003a80, 0xfc00ffe0, "fcmp.sdd", INTRR,      REF(fcmp_sdd),  fix_r0,

0x84007000, 0xfc00ffe0, "fdiv.sss", INTRR,      REF(fdiv_sss),  fix_r0,
0x84007080, 0xfc00ffe0, "fdiv.ssd", INTRR,      REF(fdiv_ssd),  fix_r0,
0x84007200, 0xfc00ffe0, "fdiv.sds", INTRR,      REF(fdiv_sds),  fix_r0,
0x84007280, 0xfc00ffe0, "fdiv.sdd", INTRR,      REF(fdiv_sdd),  fix_r0,
0x84007020, 0xfc00ffe0, "fdiv.dss", INTRR,      REF(fdiv_dss),  fix_r0,
0x840070a0, 0xfc00ffe0, "fdiv.dsd", INTRR,      REF(fdiv_dsd),  fix_r0,
0x84007220, 0xfc00ffe0, "fdiv.dds", INTRR,      REF(fdiv_dds),  fix_r0,
0x840072a0, 0xfc00ffe0, "fdiv.ddd", INTRR,      REF(fdiv_ddd),  fix_r0,

0xf400ec00, 0xfc1fffe0, "ff0",      INTRR2OP,   REF(ff0),       fix_r0,
0xf400e800, 0xfc1fffe0, "ff1",      INTRR2OP,   REF(ff1),       fix_r0,

0x80004800, 0xfc1ff81f, "fldcr",    FLDCR,      REF(ldcr),      fix_r0,

0x84002000, 0xfc1fffe0, "flt.ss",   INTRR2OP,   REF(flt_ss),    fix_r0,
0x84002020, 0xfc1fffe0, "flt.ds",   INTRR2OP,   REF(flt_ds),    fix_r0,

0x84000000, 0xfc00ffe0, "fmul.sss", INTRR,      REF(fmul_sss),  fix_r0,
0x84000080, 0xfc00ffe0, "fmul.ssd", INTRR,      REF(fmul_ssd),  fix_r0,
0x84000200, 0xfc00ffe0, "fmul.sds", INTRR,      REF(fmul_sds),  fix_r0,
0x84000280, 0xfc00ffe0, "fmul.sdd", INTRR,      REF(fmul_sdd),  fix_r0,
0x84000020, 0xfc00ffe0, "fmul.dss", INTRR,      REF(fmul_dss),  fix_r0,
0x840000a0, 0xfc00ffe0, "fmul.dsd", INTRR,      REF(fmul_dsd),  fix_r0,
0x84000220, 0xfc00ffe0, "fmul.dds", INTRR,      REF(fmul_dds),  fix_r0,
0x840002a0, 0xfc00ffe0, "fmul.ddd", INTRR,      REF(fmul_ddd),  fix_r0,

0x80008800, 0xffe0f800, "fstcr",    FSTCR,      REF(stcr),      NOFIX,

0x84003000, 0xfc00ffe0, "fsub.sss", INTRR,      REF(fsub_sss),  fix_r0,
0x84003080, 0xfc00ffe0, "fsub.ssd", INTRR,      REF(fsub_ssd),  fix_r0,
0x84003200, 0xfc00ffe0, "fsub.sds", INTRR,      REF(fsub_sds),  fix_r0,
0x84003280, 0xfc00ffe0, "fsub.sdd", INTRR,      REF(fsub_sdd),  fix_r0,
0x84003020, 0xfc00ffe0, "fsub.dss", INTRR,      REF(fsub_dss),  fix_r0,
0x840030a0, 0xfc00ffe0, "fsub.dsd", INTRR,      REF(fsub_dsd),  fix_r0,
0x84003220, 0xfc00ffe0, "fsub.dds", INTRR,      REF(fsub_dds),  fix_r0,
0x840032a0, 0xfc00ffe0, "fsub.ddd", INTRR,      REF(fsub_ddd),  fix_r0,

0x8000c800, 0xfc00f800, "fxcr",     FXCR,       REF(xcr),       NOFIX,

0x84004800, 0xfc1fffe0, "int.ss",   INTRR2OP,   REF(int_ss),    fix_r0,
0x84004880, 0xfc1fffe0, "int.sd",   INTRR2OP,   REF(int_sd),    fix_r0,

0xf400c000, 0xffffffe0, "jmp",      JMP,        REF(jmp),       NOFIX,
0xf400c400, 0xffffffe0, "jmp.n",    JMP,        REF(jmp_n),     NOFIX,

0xf400c800, 0xffffffe0, "jsr",      JMP,        REF(jsr),       NOFIX,
0xf400cc00, 0xffffffe0, "jsr.n",    JMP,        REF(jsr_n),     NOFIX,

0x10000000, 0xfc000000, "ld.d",     LDLIT,      REF(ld_d),      NOFIX,
0x14000000, 0xfc000000, "ld",       LDLIT,      REF(ld),        fix_r0,
0x18000000, 0xfc000000, "ld.h",     LDLIT,      REF(ld_h),      fix_r0,
0x1c000000, 0xfc000000, "ld.b",     LDLIT,      REF(ld_b),      fix_r0,
0x00000000, 0xfc000000, "xmem.bu",  LDLIT,      REF(xmem_bu),   NOFIX,
0x04000000, 0xfc000000, "xmem",     LDLIT,      REF(xmem),      NOFIX,
0x08000000, 0xfc000000, "ld.hu",    LDLIT,      REF(ld_hu),     fix_r0,
0x0c000000, 0xfc000000, "ld.bu",    LDLIT,      REF(ld_bu),     fix_r0,

0xf4000000, 0xfc00ffe0, "xmem.bu",  LDRO,       REF(xmem_bu),   NOFIX,
0xf4000400, 0xfc00ffe0, "xmem",     LDRO,       REF(xmem),      NOFIX,
0xf4000800, 0xfc00ffe0, "ld.hu",    LDRO,       REF(ld_hu),     fix_r0,
0xf4000c00, 0xfc00ffe0, "ld.bu",    LDRO,       REF(ld_bu),     fix_r0,
0xf4001000, 0xfc00ffe0, "ld.d",     LDRO,       REF(ld_d),      fix_r0,
0xf4001400, 0xfc00ffe0, "ld",       LDRO,       REF(ld),        fix_r0,
0xf4001800, 0xfc00ffe0, "ld.h",     LDRO,       REF(ld_h),      fix_r0,
0xf4001c00, 0xfc00ffe0, "ld.b",     LDRO,       REF(ld_b),      fix_r0,

0xf4000200, 0xfc00ffe0, "xmem.bu",  LDRI,       REF(xmem_bu),   NOFIX,
0xf4000600, 0xfc00ffe0, "xmem",     LDRI,       REF(xmem_sc),   NOFIX,
0xf4000a00, 0xfc00ffe0, "ld.hu",    LDRI,       REF(ld_hu_sc),  fix_r0,
0xf4000e00, 0xfc00ffe0, "ld.bu",    LDRI,       REF(ld_bu),     fix_r0,
0xf4001200, 0xfc00ffe0, "ld.d",     LDRI,       REF(ld_d_sc),   NOFIX,
0xf4001600, 0xfc00ffe0, "ld",       LDRI,       REF(ld_sc),     fix_r0,
0xf4001a00, 0xfc00ffe0, "ld.h",     LDRI,       REF(ld_h_sc),   fix_r0,
0xf4001e00, 0xfc00ffe0, "ld.b",     LDRI,       REF(ld_b),      fix_r0,

0xf4000900, 0xfc00ffe0, "ld.hu.usr",    LDRO,   REF(ld_hu_usr), fix_r0,
0xf4000d00, 0xfc00ffe0, "ld.bu.usr",    LDRO,   REF(ld_bu_usr), fix_r0,
0xf4001100, 0xfc00ffe0, "ld.d.usr",     LDRO,   REF(ld_d_usr),  NOFIX,
0xf4001500, 0xfc00ffe0, "ld.usr",       LDRO,   REF(ld_usr),    fix_r0,
0xf4001900, 0xfc00ffe0, "ld.h.usr",     LDRO,   REF(ld_h_usr),  fix_r0,
0xf4001d00, 0xfc00ffe0, "ld.b.usr",     LDRO,   REF(ld_b_usr),  fix_r0,

0xf4000b00, 0xfc00ffe0, "ld.hu.usr",    LDRI,   REF(ld_hu_usr_sc),fix_r0,
0xf4000f00, 0xfc00ffe0, "ld.bu.usr",    LDRI,   REF(ld_bu_usr),   fix_r0,
0xf4001300, 0xfc00ffe0, "ld.d.usr",     LDRI,   REF(ld_d_usr_sc), NOFIX,
0xf4001700, 0xfc00ffe0, "ld.usr",       LDRI,   REF(ld_usr_sc),   fix_r0,
0xf4001b00, 0xfc00ffe0, "ld.h.usr",     LDRI,   REF(ld_h_usr_sc), fix_r0,
0xf4001f00, 0xfc00ffe0, "ld.b.usr",     LDRI,   REF(ld_b_usr),    fix_r0,

0xf4000100, 0xfc00ffe0, "xmem.bu.usr",  LDRO,   REF(xmem_bu_usr), NOFIX,
0xf4000500, 0xfc00ffe0, "xmem.usr",     LDRO,   REF(xmem_usr),    NOFIX,
0xf4000300, 0xfc00ffe0, "xmem.bu.usr",  LDRI,   REF(xmem_bu_usr), NOFIX,
0xf4000700, 0xfc00ffe0, "xmem.usr",     LDRI,   REF(xmem_usr_sc), NOFIX,

/*
 * This is my closest guess on the lda LDLIT and LDRO
 * modes, they aren't documented.
 */
0x30000000, 0xfc000000, "lda.d",    INTRL,      REF(addu),      fix_r0,
0x34000000, 0xfc000000, "lda",      INTRL,      REF(addu),      fix_r0,
0x38000000, 0xfc000000, "lda.h",    INTRL,      REF(addu),      fix_r0,
0x3c000000, 0xfc000000, "lda.b",    INTRL,      REF(addu),      fix_r0,

0xf4003000, 0xfc00ffe0, "lda.d",    INTRR,      REF(addu),      fix_r0,
0xf4003400, 0xfc00ffe0, "lda",      INTRR,      REF(addu),      fix_r0,
0xf4003800, 0xfc00ffe0, "lda.h",    INTRR,      REF(addu),      fix_r0,
0xf4003c00, 0xfc00ffe0, "lda.b",    INTRR,      REF(addu),      fix_r0,

0xf4003200, 0xfc00ffe0, "lda.d",    INTRR,      REF(lda_d),     fix_r0,
0xf4003600, 0xfc00ffe0, "lda",      INTRR,      REF(lda),       fix_r0,
0xf4003a00, 0xfc00ffe0, "lda.h",    INTRR,      REF(lda_h),     fix_r0,
0xf4003e00, 0xfc00ffe0, "lda.b",    INTRR,      REF(addu),      fix_r0,

0x80004000, 0xfc1ff81f, "ldcr",     LDCR,       REF(ldcr),      fix_r0,

0xf000a000, 0xfc00fc00, "mak",      BITFIELD,   REF(mak),       fix_r0,
0xf400a000, 0xfc00ffe0, "mak",      INTRR,      REF(mak),       fix_r0,

0x48000000, 0xfc000000, "mask",     INTRL,      REF(and),       fix_r0,
0x4c000000, 0xfc000000, "mask.u",   INTRL,      REF(and),       fix_mask_u,

0x6c000000, 0xfc000000, "mul",      INTRL,      REF(mul),       fix_r0,
0xf4006c00, 0xfc00fee0, "mul",      INTRR,      REF(mul),       fix_r0,

0x84005000, 0xfc1fffe0, "nint.ss",  INTRR2OP,   REF(nint_ss),   fix_r0,
0x84005080, 0xfc1fffe0, "nint.sd",  INTRR2OP,   REF(nint_sd),   fix_r0,

0x58000000, 0xfc000000, "or",       INTRL,      REF(or),        fix_r0,
0x5c000000, 0xfc000000, "or.u",     INTRL,      REF(or),        fix_or_u,
0xf4005800, 0xfc00ffe0, "or",       INTRR,      REF(or),        fix_r0,
0xf4005c00, 0xfc00ffe0, "or.c",     INTRR,      REF(or_c),      fix_r0,

0xf000a800, 0xfc00ffe0, "rot",      ROT,        REF(rot),       fix_r0,
0xf400a800, 0xfc00ffe0, "rot",      INTRR,      REF(rot),       fix_r0,

0xf400fc00, 0xffffffff, "rte",      RTE,        REF(rte),       NOFIX,

0xf0008800, 0xfc00fc00, "set",      BITFIELD,   REF(set),       fix_r0,
0xf4008800, 0xfc00ffe0, "set",      INTRR,      REF(set),       fix_r0,

0x20000000, 0xfc000000, "st.d",     STLIT,      REF(st_d),      NOFIX,
0x24000000, 0xfc000000, "st",       STLIT,      REF(st),        NOFIX,
0x28000000, 0xfc000000, "st.h",     STLIT,      REF(st_h),      NOFIX,
0x2c000000, 0xfc000000, "st.b",     STLIT,      REF(st_b),      NOFIX,

0xf4002000, 0xfc00ffe0, "st.d",     STRO,       REF(st_d),      NOFIX,
0xf4002400, 0xfc00ffe0, "st",       STRO,       REF(st),        NOFIX,
0xf4002800, 0xfc00ffe0, "st.h",     STRO,       REF(st_h),      NOFIX,
0xf4002c00, 0xfc00ffe0, "st.b",     STRO,       REF(st_b),      NOFIX,

0xf4002200, 0xfc00ffe0, "st.d",     STRI,       REF(st_d_sc),   NOFIX,
0xf4002600, 0xfc00ffe0, "st",       STRI,       REF(st_sc),     NOFIX,
0xf4002a00, 0xfc00ffe0, "st.h",     STRI,       REF(st_h_sc),   NOFIX,
0xf4002e00, 0xfc00ffe0, "st.b",     STRI,       REF(st_b),      NOFIX,

0xf4002100, 0xfc00ffe0, "st.d.usr", STRO,       REF(st_d_usr),  NOFIX,
0xf4002500, 0xfc00ffe0, "st.usr",   STRO,       REF(st_usr),    NOFIX,
0xf4002900, 0xfc00ffe0, "st.h.usr", STRO,       REF(st_h_usr),  NOFIX,
0xf4002d00, 0xfc00ffe0, "st.b.usr", STRO,       REF(st_b_usr),  NOFIX,

0xf4002300, 0xfc00ffe0, "st.d.usr", STRI,       REF(st_d_usr_sc),   NOFIX,
0xf4002700, 0xfc00ffe0, "st.usr",   STRI,       REF(st_usr_sc),     NOFIX,
0xf4002b00, 0xfc00ffe0, "st.h.usr", STRI,       REF(st_h_usr_sc),   NOFIX,
0xf4002f00, 0xfc00ffe0, "st.b.usr", STRI,       REF(st_b_usr),      NOFIX,

0x80008000, 0xffe0f800, "stcr",     STCR,       REF(stcr),      NOFIX,

0x74000000, 0xfc000000, "sub",      INTRL,      REF(sub),       fix_r0,
0xf4007400, 0xfc00ffe0, "sub",      INTRR,      REF(sub),       fix_r0,
0xf4007500, 0xfc00ffe0, "sub.bo",   INTRR,      REF(sub_bo),    fix_r0,
0xf4007600, 0xfc00ffe0, "sub.bi",   INTRR,      REF(sub_bi),    fix_r0,
0xf4007700, 0xfc00ffe0, "sub.bio",  INTRR,      REF(sub_bio),   fix_r0,

0x64000000, 0xfc000000, "subu",     INTRL,      REF(subu),      fix_r0,
0xf4006400, 0xfc00ffe0, "subu",     INTRR,      REF(subu),      fix_r0,
0xf4006500, 0xfc00ffe0, "subu.bo",  INTRR,      REF(subu_bo),   fix_r0,
0xf4006600, 0xfc00ffe0, "subu.bi",  INTRR,      REF(subu_bi),   fix_r0,
0xf4006700, 0xfc00ffe0, "subu.bio", INTRR,      REF(subu_bio),  fix_r0,

0xf000d000, 0xfc00fe00, "tb0",      TRAP,       REF(tb0),       NOFIX,
0xf000d800, 0xfc00fe00, "tb1",      TRAP,       REF(tb1),       NOFIX,

0xf8000000, 0xffe00000, "tbnd",     TBND,       REF(tbnd),      NOFIX,
0xf400f800, 0xffe0ffe0, "tbnd",     INTRR_S1_S2,REF(tbnd),      NOFIX,

0xf000e800, 0xfc00fe00, "tcnd",     TCND,       REF(tcnd),      NOFIX,

0x84005800, 0xfc1fffe0, "trnc.ss",  INTRR2OP,   REF(trnc_ss),   fix_r0,
0x84005880, 0xfc1fffe0, "trnc.sd",  INTRR2OP,   REF(trnc_sd),   fix_r0,

0x8000c000, 0xfc00c000, "xcr",      XCR,        REF(xcr),       NOFIX,

0x50000000, 0xfc000000, "xor",      INTRL,      REF(xor),       fix_r0,
0x54000000, 0xfc000000, "xor.u",    INTRL,      REF(xor),       fix_or_u,
0xf4005000, 0xfc00ffe0, "xor",      INTRR,      REF(xor),       fix_r0,
0xf4005400, 0xfc00ffe0, "xor.c",    INTRR,      REF(xor_c),     fix_r0,

0x00000000, 0x00000000, "pseudo",   0,          REF(end_of_page), NOFIX,
0x00000000, 0x00000000, "pseudo",   0,          REF(not_decoded), NOFIX,
0x00000000, 0x00000000, "pseudo",   0,          REF(opc_exception), NOFIX
};

#define INTAB_SIZE (sizeof(in_tab) / sizeof(struct instr_info))

static struct instr_info *last_p = &(in_tab[INTAB_SIZE]);

#define DECODECACHESIZE  (64)
#define DECODECACHESHIFT (32 - 6)       /* 32 - log2(DECODECACHESIZE) */
static struct instr_info decodecache[DECODECACHESIZE];

/*
 * This builds a small lookup table based on the top 6 bits of the
 * instruction.  We use it to speed up sim_instructions_lookup().
 */
builddecodetable()
{
    register struct instr_info *p;
    register struct instr_info *lp = last_p;
    register i;
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
        for (p = &in_tab[0] ; p < lp ; p++) {
            i = p->opcode >> DECODECACHESHIFT;
            p->link = decodecache[i].link;
            decodecache[i].link = p;
        }
    }
}

/*
 * This returns a pointer to the instruction table entry that matches
 * the passed instruction.
 */
struct instr_info *
sim_instruction_lookup(i)
    register u_long i;
{
    register struct instr_info *p, *prev, *head;
    register struct instr_info *lp = last_p;
    register int cnt;

    prev = head = &decodecache[i >> DECODECACHESHIFT];
    for (p = head->link ; p ; prev = p, p = p->link) {
        if ((p->opmask & i) == p->opcode) {
            /*
             * We do a little adaptive searching here: when
             * we find an instruction we put it at the head of
             * the list.
             */
            prev->link = p->link;
            p->link = head->link;
            head->link = p;
            return p;
        }
    }
    return (struct instr_info *)0;
}
