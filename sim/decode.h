/*
 * Macros for getting various fields of 88000 instructions.
 * 
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/decode.h,v 1.9 89/07/14 08:58:44 robertb Exp $
 */

struct instr_info {
    u_long   opcode;             /* Opcode of instruction.               */
    u_long   opmask;             /* Mask to use before comparing opcode. */
    char     *mnemonic;          /* String programmers use. (comment only)*/
    u_char   format;             /* Type of instruction.                 */
    struct instr_info *link;     /* Points to next instr w/ same top bits*/
    int      (*norm_e_addr)();   /* Entry point in sim() for this instr. */
    int      (*fixup)();         /* Routine to do any extra decoding.    */
};

extern struct instr_info *sim_instruction_lookup();
extern u_long *mkliteral();
