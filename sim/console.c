/*
 * 88000 console simulator.
 * 
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/console.c,v 1.9 88/06/28 16:44:41 robertb Exp $
 */

#include "sim.h"

console_init()
{}

console_operation(address_offset, reg_ptr, size, mem_op_type, override)
    unsigned address_offset;
    unsigned *reg_ptr;
    unsigned size;
    unsigned mem_op_type;
    int      override;
{
    if (size != BYTE) {
        sim_printf("console IO must use byte operations.\n");
        return E_DACC;
    }

    switch (mem_op_type) {
        case LD:
            *reg_ptr = (sim_consgetchar() << 24) >> 24;
            break;

        case LD_U:
            *reg_ptr = sim_consgetchar() & 0xff;
            break;

        case ST:
            sim_consputchar(*reg_ptr & 0xff);
            break;

        case XMEM:
        case XMEM_U:
            sim_printf("console_operation: no support for XMEM\n");
            break;

        default:
            sim_printf("case error in console_operation\n");
            break;
    }
    return E_NONE;
}

console_print()
{
    sim_printf("console\n");
}
