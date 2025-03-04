/*
 * Macros for getting various fields of 88000 instructions.
 * 
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/decode.h,v 1.2 90/04/29 19:59:59 robertb Exp $
 */

struct instr_info {
    u_long   opcode;             /* Opcode of instruction.               */
    u_long   opmask;             /* Mask to use before comparing opcode. */
    char     *mnemonic;          /* String programmers use. (comment only)*/
    u_char   format;             /* Type of instruction.                 */
};

extern struct instr_info *remote_instruction_lookup();
