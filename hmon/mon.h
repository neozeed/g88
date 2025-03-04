/*
 * $Header: /home/bigbird/Usr.U6/robertb/m88k/src/g88/hmon/RCS/mon.h,v 1.5 91/01/13 23:38:55 robertb Exp $
 * $Locker:  $
 *
	Copyright (c) 1987, 1988, 1989, 1990 by Tektronix, Inc.

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.

 * Header file for the 88000 debug monitor.
 */

#define MONSTACKSIZE	(2048)

/*
 * This structure holds all of the variables that the debug monitor
 * needs that span function calls.
 */
#ifndef ASSEMBLER
#define	soff(field)	(char *)&(v.field)-(char *)&v

struct all_vars {
    unsigned long dm_genregs[32];
    unsigned long dm_ip;
    unsigned long dm_pid;
    unsigned long dm_psr;
    unsigned long dm_tpsr;
    unsigned long dm_ssbr;
    unsigned long dm_sxip;
    unsigned long dm_snip;
    unsigned long dm_sfip;
    unsigned long dm_vbr;
    unsigned long dm_dmt0;
    unsigned long dm_dmd0;
    unsigned long dm_dma0;
    unsigned long dm_dmt1;
    unsigned long dm_dmd1;
    unsigned long dm_dma1;
    unsigned long dm_dmt2;
    unsigned long dm_dmd2;
    unsigned long dm_dma2;
    unsigned long dm_sr0;
    unsigned long dm_sr1;
    unsigned long dm_sr2;
    unsigned long dm_sr3;
    unsigned long dm_fpecr;
    unsigned long dm_fphs1;
    unsigned long dm_fpls1;
    unsigned long dm_fphs2;
    unsigned long dm_fpls2;
    unsigned long dm_fppt;
    unsigned long dm_fprh;
    unsigned long dm_fprl;
    unsigned long dm_fpit;
    unsigned long dm_fpsr;
    unsigned long dm_fpcr;
    unsigned long dm_ce_intr_msk;
    unsigned long dm_come_from;    /* Place holder, only used w/ simulator */
    unsigned long *dm_memory_breakpoint;
    unsigned long dm_exception_code;

    unsigned long dm_panic_code;
    unsigned long dm_checksum;
    unsigned long dm_checksumming;
    unsigned long dm_partial;
    unsigned long dm_partial_length; 
    unsigned long dm_in_mon;
    unsigned long dm_dacc_address;
    void (*dm_call_on_mon_entry)();
    void (*dm_call_on_mon_exit)();
    unsigned long dm_restart_count;
    unsigned long dm_character_time;
    unsigned char dm_stackarea[MONSTACKSIZE];
};

#endif

/*
 * We pick an unlikely number of this so that it is unlikely that
 * by stomping on our memory, the program being debugged will not
 * confuse us.
 */
#define IN_MON_MAGIC	(0x31415926)

