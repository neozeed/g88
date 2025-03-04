/*
 * ce.c
 *
 * This module simulates the FBUS CSR space of the CE Board.
 * including the CE DRAM mapper hardware
 * It also simulates various ctl/stat registers on the CE board
 *
 * Copyright (C) 1987, 1988 Tektronix, Inc.
 * All Rights Reserved.
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/ce.c,v 1.12 89/08/25 14:04:16 robertb Exp $
 */

#include "sim.h"
#include "io.h"
#undef PAGE_SIZE
#include "CE_defs.h"
#include "FBUS_defs.h"
#include "IOB_defs.h"

u_char  CE_fbus_csr_space[4096];

static int     CEreset;
static int     CEparity;
static int     CEmspc;

static u_short *sr_ptr;
u_short *mask_ptr;
static u_char *msg_ptr;

/*
 * CEfbus_init()
 *
 * Initializes the CE board simulator
 *
 */
void    CEfbus_init()
{
    int    i;

    CEparity = 0;
    CEreset = 0;
    CEmspc = 0;

    /* clear out the simulated CSR space */
    for (i = 0;    i < 4096;    i++) {
        CE_fbus_csr_space[i] = 0xff;
    }

    /* Initialize the ID PROM */
    CE_fbus_csr_space[FBUS_ID_BYTE_1] = 'T';
    CE_fbus_csr_space[FBUS_ID_BYTE_2] = 'e';
    CE_fbus_csr_space[FBUS_ID_BYTE_3] = 'k';
    CE_fbus_csr_space[FBUS_ID_BYTE_4] = ' ';

    CE_fbus_csr_space[0x13] = '6';
    CE_fbus_csr_space[0x17] = '7';
    CE_fbus_csr_space[0x1b] = '1';
    CE_fbus_csr_space[0x1f] = '0';
    CE_fbus_csr_space[0x23] = '5';
    CE_fbus_csr_space[0x27] = '4';
    CE_fbus_csr_space[0x2b] = '1';

    /*
     * init FBUS_SR reg 
     */
    CE_fbus_csr_space[FBUS_SR] = 3; /* fbus_slot3 only */

    /* 
     * init FBUS_ICR reg 
     */
    CE_fbus_csr_space[FBUS_ICR] = 0x0;  /* zeroed on pwr_up */

    /* 
     * FBUS_ESR reg pwrsup randomly
     */
    CE_fbus_csr_space[FBUS_ESR] = 0xee;

    /*
     * init CE MEMCSR reg   init for MPR0/1MEG/FULL
     */
    CE_fbus_csr_space[CE_MEM_CSR_OFF] = 0xb0;

    sr_ptr = (u_short *)&CE_fbus_csr_space[CE_INTR_SR_OFF];
    *sr_ptr = 0;

    mask_ptr = (u_short *)&CE_fbus_csr_space[CE_INTR_MSK_OFF];
    *mask_ptr = 0;

    msg_ptr = (u_char *)&CE_fbus_csr_space[CE_MSG_SR_OFF];
    *msg_ptr = 0;

    /* call init routines for all CE board devices */
}

/*
 * CEfbus_print()
 *
 * Print information about the CE Board.
 */
void CEfbus_print()
{

    sim_printf("CE Board FBUS CSR Space- LOCAL slot31 access only\n");
    sim_printf("    PARITY is %s\n", CEparity ? "enabled" : "disabled");
    sim_printf("    MSPC is %s\n", CEmspc ? "enabled" : "disabled");
    sim_printf("    IRST is %s asserted\n", 
                                      CEreset ? "" : "not");

    sim_printf("    interrupt status register=0x%x\n", *sr_ptr);
    sim_printf("    interrupt mask register=0x%x\n", *mask_ptr);
    sim_printf("    message status register=0x%x\n", *msg_ptr);

    sim_printf("    CE_mapaddr_reg=0x%x\n", 
                              *(u_long *)&CE_fbus_csr_space[CE_MAP_ADDR_OFF]);
}

/*
 * Check to see if an interrupt should be requested.
 */
void
check_for_ce_interrupt()
{
    if ((*sr_ptr & *mask_ptr) != 0 && PSR_INT_ENABLED) {
        sim_interrupt_flag |= INT_DEVICE;
    }
}

/*
 * This is called by any function that simulates a device that can
 * cause a CE interrupt status register bit to be set.
 */
void
set_ce_interrupt(bit)
    u_char bit;
{
    if (bit > 13) {
        sim_printf("set_ce_interrupt gets bad bit number: %d\n", bit);
    } else {
        *sr_ptr |= 1 << bit;
	    check_for_ce_interrupt();
    }
}

/*
 */
void
reset_ce_interrupt(bit)
    u_char bit;
{
    if (bit > 13) {
        sim_printf("reset_ce_interrupt gets bad bit number: %d\n", bit);
    } else {
        *sr_ptr &= ~(1 << bit);
        sim_interrupt_flag &= INT_DEVICE;
	    check_for_ce_interrupt();
    }
}

/*
 * This is called by any function that simulates a device that can
 * cause a CE MSG interrupt status register bit to be set.
 */
void
set_ce_msgintr(bit)
    u_char bit;
{
    if (bit > 7) {
        sim_printf("set_ce_msgintr gets bad bit number: %d\n", bit);
    } else {
        *msg_ptr |= 1 << bit;
	set_ce_interrupt(11);
    }
}

/*
 * CEfbus_operation()
 *
 * This routine is the interface of the CEfbus simualtor to the m78k
 * simulator.
 */
int CEfbus_operation(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    int    size;
    int    mem_op_type;
    int    override;

{
    u_long *long_ptr1, *long_ptr2;

    switch (address_offset) {
        case FBUS_FUNC_PAR_ENABLE:
            CEparity = 1;
            CE_fbus_csr_space[FBUS_SR] |= FBUS_SR_PON;
            break;

        case FBUS_FUNC_PAR_DISABLE:
            CEparity = 0;
            CE_fbus_csr_space[FBUS_SR] &= ~FBUS_SR_PON;
            break;

        case FBUS_FUNC_MSPC_ATTACH:
            CEmspc = 1;
            CE_fbus_csr_space[FBUS_SR] |= FBUS_SR_MSPC;
            break;

        case FBUS_FUNC_MSPC_DETACH:
            CEmspc = 0;
            CE_fbus_csr_space[FBUS_SR] &= ~FBUS_SR_MSPC;
            break;

        case FBUS_FUNC_IRST_ON:
            CEfbus_init();
            CEreset = 1;
            CE_fbus_csr_space[FBUS_SR] |= FBUS_SR_IRST;
            break;

        case FBUS_FUNC_IRST_OFF:
            CEreset = 0;
            CE_fbus_csr_space[FBUS_SR] &= ~FBUS_SR_IRST;
            break;

        case (FBUS_EVENT_SPACE+0*4):
	    set_ce_msgintr(0);	/* 1st msg intr sets lowest bit */
            break;

        case (FBUS_EVENT_SPACE+1*4):
	    set_ce_msgintr(1);	
            break;

        case (FBUS_EVENT_SPACE+2*4):
	    set_ce_msgintr(2);
            break;

        case (FBUS_EVENT_SPACE+3*4):
	    set_ce_msgintr(3);	
            break;

        case (FBUS_EVENT_SPACE+4*4):
	    set_ce_msgintr(4);	
            break;

        case (FBUS_EVENT_SPACE+5*4):
	    set_ce_msgintr(5);	
            break;

        case (FBUS_EVENT_SPACE+6*4):
	    set_ce_msgintr(6);
            break;

        case (FBUS_EVENT_SPACE+7*4):
	    set_ce_msgintr(7);	
            break;

        case FBUS_FUNC_PRIOR_EVENT:	
            set_ce_interrupt(13);        
            break;

        case FBUS_ICR:
            if (CE_fbus_csr_space[FBUS_ICR] & FBUS_ICR_SRST) {
                 CE_fbus_csr_space[FBUS_ESR] = 0x0;  /* reset FBUS_ESR */
            }
            break;

        case FBUS_SR:
            if (mem_op_type == ST && !override) {
                sim_printf("   CEfbus_sr- illegal store op\n");
                return(E_DACC);
            }
            break;

        case FBUS_ESR:
            if (mem_op_type == ST && !override) {
                sim_printf("   CEfbus_esr- illegal store op\n");
                return(E_DACC);
            }
            break;

        case CE_MEM_CSR_OFF:
            if (size != BYTE) {
                sim_printf("CE_memcsr: only byte operations allowed.\n");
                return E_DACC;
            }
            break;

        case CE_MAP_ADDR_OFF:
            if (size == DWORD) {
                sim_printf("CE_mapaddr_reg doesn't allow double word access\n");
                return E_DACC;
            }
            break;

        case CE_MAP_ADDR_ALT_OFF:
            if (mem_op_type == ST && !override){
                sim_printf("   CE_mapaddr_alt_reg- illegal store op\n");
                return(E_DACC);
            }
            if (size == DWORD) {
                sim_printf("CE_mapaddr_alt_reg doesn't allow 2 word access\n");
                return E_DACC;
            }

            /*
             * ALT is a copy of mapaddr_reg- different read back port
             */
            long_ptr1 = (u_long *)&CE_fbus_csr_space[CE_MAP_ADDR_ALT_OFF];
            long_ptr2 = (u_long *)&CE_fbus_csr_space[CE_MAP_ADDR_OFF];
            *long_ptr1 = *long_ptr2;
            break;

        case CE_MAP_ACCESS_OFF:
            if (size == DWORD) {
                sim_printf("CE_mapaccess doesn't support double word access\n");
                return E_DACC;
            }
            ce_map_access(address_offset, reg, size, mem_op_type, override);
            return E_NONE;  /* do_mem_op is done by ce_map_access */

        case CE_INTR_SR_OFF:
        case CE_INTR_SR_OFF + 1:
            if (size != HALF || !(mem_op_type == LD || mem_op_type == LD_U)) {
                sim_printf("CE interrupt status reg, half/byte read-only\n");
                return E_DACC;
            }
            *reg = *sr_ptr&*mask_ptr;  /* if mask set- status is masked off */
            return E_NONE;
            break;

        case CE_INTR_MSK_OFF:
            if (size != HALF)  {
                sim_printf("CE interrupt mask reg, halfword only\n");
                return E_DACC;
            }
            break;

        /*
         * This (at 081b) overlaps with the CE_INTR_RST_OFF, at
         * 081a, but it words because the later is always accessed
         * as a short.
         */
        case CE_LCA_PGM_PORT_OFF:
            break;

        case CE_INTR_RST_OFF:
            if (size != HALF || mem_op_type != ST) {
                sim_printf("CE int. reset/set regs, halfword write-only\n");
                return E_DACC;
            }
            *sr_ptr &= ~*reg;
            break;

        case CE_INTR_SET_OFF: 
            if (size != HALF || mem_op_type != ST) {
                sim_printf("CE int. reset/set regs, halfword write-only\n");
                return E_DACC;
            }
            *sr_ptr |= *reg;
            break;

        case CE_MSG_SR_OFF:
            if (size != BYTE || !(mem_op_type == LD || mem_op_type == LD_U)) {
                sim_printf("CE message status reg, byte read-only\n");
                return E_DACC;
            }
            break;

        case CE_MSG_RST_OFF:
            if (size != BYTE || mem_op_type != ST) {
                sim_printf("CE message reset reg, byte write-only\n");
                return E_DACC;
            }
            *msg_ptr &= ~*reg;
	    *sr_ptr &= 0xf7ff;    /* clear bit 11(msg_intr's) in status reg */
            break;

        default:
            if (address_offset >= 0x400) {
                sim_printf ("CEfbus: warning: address 0x%x not simulated\n", 
                                                       address_offset);
            }
            break;
    }

    do_mem_op(reg, &CE_fbus_csr_space[address_offset], size, mem_op_type);

    check_for_ce_interrupt();
    return E_NONE;
}

/*
 * CE MAP_ACCESS simulator
 */
#define CE_RAMMAP_SIZE 512
unsigned char ce_memmap_array[CE_RAMMAP_SIZE];

int
ce_map_access(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    u_long map_index;
    u_long *port_addr;

    port_addr = (u_long *)&CE_fbus_csr_space[CE_MAP_ADDR_OFF];
    map_index = (*port_addr >> 23) & 0x1ff;
    do_mem_op(reg, &ce_memmap_array[map_index], size, mem_op_type);
}

/* CE BERR location simulator */
static u_long CEberr_location;

void
CEberr_loc_init()
{
    /* NO initialization */
}

int
CEberr_loc_operation(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    CE_fbus_csr_space[FBUS_ESR] = FBUS_ESR_AD;  /* set addr phase in fb_esr */ 
    return E_DACC;
}

void
CEberr_loc_print()
{
    sim_printf("CEberr_loc simulator\n");
}

/*
 * CE serctl simulator
 */
static u_char ce_serctl_reg;

void
ce_serctl_init()
{
    /* no init */
}

int
ce_serctl_operation(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    if (size != BYTE) {
        sim_printf("ce_serctl_operation: only byte operations allowed.\n");
        return E_DACC;
    }

    do_mem_op(reg, &ce_serctl_reg, BYTE, mem_op_type);
    return E_NONE;
}

void
ce_serctl_print()
{
    sim_printf("ce_serctl_reg=0x%x\n",ce_serctl_reg);
}

/*
 * CE BITBUCKET simulators
 */
static u_long ce_rw_bitbucket;
static u_long ce_wo_bitbucket;

void
ce_rw_bitbucket_init()
{
    /* no init */
}

int
ce_rw_bitbucket_operation(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    if (size > WORD) {
        sim_printf("ce_rw_bitbucket_operation: double word ops not allowed.\n");
        return E_DACC;
    }
    do_mem_op(reg, &ce_rw_bitbucket, size, mem_op_type);
    return E_NONE;
}

void
ce_rw_bitbucket_print()
{
    sim_printf("ce_rw_bitbucket=0x%x\n",ce_rw_bitbucket);
}
static u_long ce_rw_bitbucket;

void
ce_wo_bitbucket_init()
{
    /* no init */
}

int
ce_wo_bitbucket_operation(address_offset, reg, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    if (size > WORD) {
        sim_printf("ce_ro_bitbucket_operation: double word ops not allowed.\n");
        return E_DACC;
    }
    if (mem_op_type == ST) {
        do_mem_op(reg, &ce_wo_bitbucket, size, mem_op_type);
        return E_NONE;
    } else {
        return E_DACC;  /* DACC exception unless it is a write operation */
    }
}

void
ce_wo_bitbucket_print()
{
    sim_printf("ce_rw_bitbucket\n");
}
