/*
 * This contains the 88000 simulator initialization
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/siminit.c,v 1.30 90/07/01 12:31:44 robertb Exp $
 */

#include "sim.h"
#include <signal.h>

/*
 * Largest physical memory allowed, 256 MB.
 */
#define MAX_MEM_SIZE     (256 * 1024 * 1024)

/*
 * Default physical memory.
 */
#define DEF_MEM_SIZE	(8 * 1024 * 1024)

u_long memory_size;
u_long page_table_size;

u_long regs[33];     /* 33 instead of 32 for double loads of r31 */
u_long sfu0_regs[64];
u_long sfu1_regs[64];

char sim_catch_exception[2 * 512 + 1];
int sim_exception;

u_long ip;
u_long delayed_ip;

int sim_errno;
int sim_interrupt_flag;
int sim_in_execution;

int fp_exception;

struct decoded_i *delayed_p;

/*
 * These are *software* tlb's.  Two for user mode and two for supervisor.
 *
 * They are indexed with logical segment numbers.  The values are pointers
 * to our tlb's page tables (not to be confused with 88k page tables).
 */
u_char **u_load_tlb[SEGTLB];
u_char **s_load_tlb[SEGTLB];

u_char **u_store_tlb[SEGTLB];
u_char **s_store_tlb[SEGTLB];

u_char *defaultpagetlb[PAGETLB] ;

struct page **page_table = 0;

struct io_list *io_list = 0;

struct decitem *nextdecp[2];

/*
 * This flushes all user's tlb entries for a given page.
 */
void
flush_user_tlb_page(page)
    u_long page;
{
    u_long seg = page >> 10;

    if (seg >= SEGTLB) {
        err("flush_user_tlb_page: segment out of range\n", __LINE__, __FILE__);
    }

    if (u_load_tlb[seg] != &defaultpagetlb[0]) {
        u_load_tlb[seg][page & 0x3ff] = 0;
    }
    if (u_store_tlb[seg] != &defaultpagetlb[0]) {
        u_store_tlb[seg][page & 0x3ff] = 0;
    }
}

/*
 * This flushes all user's tlb entries for a given segment.
 */
void
flush_user_tlb_segment(segment)
    u_long segment;
{
    if (segment >= SEGTLB) {
        err("flush_user_tlb_segment: out of range\n", __LINE__, __FILE__);
    }
    if (u_load_tlb[segment] != &defaultpagetlb[0]) {
        bzero(u_load_tlb[segment], PAGETLB * sizeof(char *));
    }
    if (u_store_tlb[segment] != &defaultpagetlb[0]) {
        bzero(u_store_tlb[segment], PAGETLB * sizeof(char *));
    }
}

/*
 * This flushes all user tlb entries.
 */
void
flush_user_tlb_all()
{
    int i;

    for (i = 0 ; i < SEGTLB ; i++) {
        flush_user_tlb_segment(i);
    }
}

/*
 * This flushes all supervisor tlb entries for a given page.
 */
void
flush_supervisor_tlb_page(page)
    u_long page;
{
    u_long seg = page >> 10;

    if (seg >= SEGTLB) {
        err("flush_supervisor_tlb_page: seg out of range\n", __LINE__,__FILE__);
    }

    if (s_load_tlb[seg] != &defaultpagetlb[0]) {
        s_load_tlb[seg][page & 0x3ff] = 0;
    }
    if (s_store_tlb[seg] != &defaultpagetlb[0]) {
        s_store_tlb[seg][page & 0x3ff] = 0;
    }
}

/*
 * This flushes all supervisor tlb entries for a given segment.
 */
void
flush_supervisor_tlb_segment(segment)
    u_long segment;
{
    if (segment >= SEGTLB) {
        err("flush_supervisor_tlb_segment: out of range\n", __LINE__,__FILE__);
    }
    if (s_load_tlb[segment] != &defaultpagetlb[0]) {
        bzero(s_load_tlb[segment], PAGETLB * sizeof(char *));
    }
    if (s_store_tlb[segment] != &defaultpagetlb[0]) {
        bzero(s_store_tlb[segment], PAGETLB * sizeof(char *));
    }
}

/*
 * This flushes all supervisor tlb entries.
 */
void
flush_supervisor_tlb_all()
{
    int i;

    for (i = 0 ; i < SEGTLB ; i++) {
        flush_supervisor_tlb_segment(i);
    }
}

/*
 * This flushes tlb entries for the passed logical address and
 * extending for 'length' bytes.
 */
void
flush_tlb_range(logical_address, length, usmode)
    u_long logical_address;
    u_long length;
    int    usmode;
{
    u_long i, seg, page, npages;

    npages = (length + 4095) >> 12;

    if (usmode == M_USER) {
        for (i = 0 ; i < npages ; i++, logical_address += PAGESIZE) {
            seg = btos(logical_address);
            page = btop(logical_address);
            if (u_load_tlb[seg] != &defaultpagetlb[0]) {
                u_load_tlb[seg][page] = (u_char *)0;
            }
            if (u_store_tlb[seg] != &defaultpagetlb[0]) {
                u_store_tlb[seg][page] = (u_char *)0;
            }
         }
    } else {
        for (i = 0 ; i < npages ; i++, logical_address += PAGESIZE) {
            seg = btos(logical_address);
            page = btop(logical_address);
            if (s_load_tlb[seg] != &defaultpagetlb[0]) {
                s_load_tlb[seg][page] = (u_char *)0;
            }
            if (s_store_tlb[seg] != &defaultpagetlb[0]) {
                s_store_tlb[seg][page] = (u_char *)0;
            }
         }
    }
}

void
flush_io_decoded_list()
{
    struct io_list *p;

    while (io_list) {
        p = io_list->next_p;
        free(io_list);
        io_list = p;
    }
}

void
flush_io_decoded_list_entry(physical_address)
    u_long physical_address;
{
    struct io_list *p;
    u_long page_number;

    page_number = physical_address >> 12;

    p = io_list;
    while (p) {
        if (p->page_number == page_number) {
            clean_decoded_part(p->decoded_part);
            break;
        }
        p = p->next_p;
    }
}

void
flush_deccache(usmode)
    int usmode;
{
    bzero(&deccache[usmode][0], sizeof(struct decitem) * DECMAX);
    nextdecp[usmode] = &deccache[usmode][0];
}

void
fpeint()
{
    fp_exception = 1;
}

/*
 * This is called when the user types "init".
 * We keep the contents of memory intact, but drop every last bit
 * of translation caching that we have.
 */
sim_flush_all_translations()
{
    int i;
    struct page *p;

    /*
     * Clean all the decoded parts of pages allocated.
     */
    for (i = 0 ; i < page_table_size ; i++) {
        if (p = page_table[i]) {
            clean_decoded_part(p->decoded_part);
        }
    }

    flush_io_decoded_list();

    if (u_load_tlb[0] == (u_char **)0) {
        for (i = 0 ; i < SEGTLB ; i++) {
            u_load_tlb[i] = &defaultpagetlb[0];
            u_store_tlb[i] = &defaultpagetlb[0];
            s_load_tlb[i] = &defaultpagetlb[0];
            s_store_tlb[i] = &defaultpagetlb[0];
        }
    }

    flush_user_tlb_all();
    flush_supervisor_tlb_all();

    flush_deccache(M_SUPERVISOR);
    flush_deccache(M_USER);
    free_literals();
}

/*
 * This initializes the 88000 simulator.
 */
void
sim_init()
{
    u_long i;
    struct page *p;
    char *sim_mem;

    sim_in_execution = 0;
    sim_exception = E_NONE;

    sim_errno = 0;
    sim_interrupt_flag = 0;
    fp_exception = 0;

    if (page_table == 0) {
        memory_size = varvalue("simmem") * PAGESIZE;
        if (memory_size) {
            if (memory_size == 0 || memory_size > MAX_MEM_SIZE) {
                sim_printf("warning: $simmem set to invalid value=%d\n", 
                                                          memory_size/PAGESIZE);
                sim_printf(" $simmem is the number of 4k pages of physical\n");
                sim_printf(" simulated memory, valid values are in 1..%d\n",
                                                      MAX_MEM_SIZE/PAGESIZE);
                memory_size = DEF_MEM_SIZE;
                setvar("simmem", memory_size/PAGESIZE);
            }
        } else {
            memory_size = DEF_MEM_SIZE;
            setvar("simmem", memory_size/PAGESIZE);
        }
        page_table_size = memory_size/PAGESIZE;
        page_table = (struct page **)
                      calloc(page_table_size, sizeof(struct page *));
    }

    /*
     * Clean all the pages allocated.
     */
    for (i = 0 ; i < page_table_size ; i++) {
        if (p = page_table[i]) {
            clean_page(p);
        }
    }

    flush_io_decoded_list();
    for (i = 0 ; i < 32 ; i++) {
        regs[i] = 0;
    }

    for (i = 0 ; i < 64 ; i++) {
        sfu0_regs[i] = 0;
        sfu1_regs[i] = 0;
    }
    PID = 0x000000fe;
    PSR = 0x800003ff;
    RAMSIZE = memory_size;
    ip = 0;

    delayed_p = (struct decoded_i *)0;
    delayed_ip = 0;

    if (u_load_tlb[0] == (u_char **)0) {
        for (i = 0 ; i < SEGTLB ; i++) {
            u_load_tlb[i] = &defaultpagetlb[0];
            u_store_tlb[i] = &defaultpagetlb[0];
            s_load_tlb[i] = &defaultpagetlb[0];
            s_store_tlb[i] = &defaultpagetlb[0];
        }
    }

    flush_user_tlb_all();
    flush_supervisor_tlb_all();

    builddecodetable();
    io_init();
    signal(SIGFPE, fpeint);
    flush_deccache(M_SUPERVISOR);
    flush_deccache(M_USER);
    free_literals();
    gdb_sim_init();
}
