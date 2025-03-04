/*
 * This requalifies the decoded-instruction pointer.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/misc.c,v 1.37 91/01/13 23:59:01 robertb Exp $
 */

#include "sim.h"
#include "cmmu.h"
#include "decode.h"
#include "format.h"
#include "fields88.h"
#include <strings.h>
#include <signal.h>

int dummy = 0;

/*
 * This makes the shadow registers have the correct values.  We
 * only need to do something if shadowing is enabled.  This is 
 * called before ldcr's and xcr's are executed.
 */
void
load_shadow_regs(ip, delayed_ip)
    u_long ip;
    u_long delayed_ip;
{
    if ((PSR & 1) == 0) {
        SXIP = ip | 2;
        if (delayed_p) {
            SNIP = delayed_ip | 2;
        } else {
            SNIP = SXIP + 4;
        }
        SFIP = SNIP + 4;
        DMT0 = DMT1 = DMT2 = 0;
        DMD0 = DMD1 = DMD2 = 0;
        DMA0 = DMA1 = DMA2 = 0;
    }
}

/*
 * This is called after a value is stored into a control register.
 * We make sure that the contents of the registers are still valid
 * and that we can continue execution correctly.
 */
void
fixup_control_regs()
{
    int endian_test = 1;
    int host_endian;
    int m88k_endian;
    static warned = 0;

    host_endian = *((u_char *)&endian_test + sizeof(int) - 1);
    m88k_endian = !PSR_ENDIAN;

    if (host_endian != m88k_endian && !warned) {
        warned = 1;
        sim_printf("warning: endian-ness of simulated 88k must match host.\n");
        sim_printf(" host is %s-endian and 88k is in %s-endian mode.\n",
                    host_endian ? "big" : "little",
                    m88k_endian ? "big" : "little");
 
    }
    VBR &= ~PAGEMASK;  /* zero lower 12 bits of vector base register. */

    TPSR = (TPSR | 0x3f0) & 0xf800e3ff;
    PSR = (PSR | 0x3f0) & 0xf800e3ff;
}

/*
 * This is called when a DACC happens.  We set the DMT registers here.
 */
set_dmt_regs(l_addr, reg_ptr, size, mem_op_type, read_or_write, usmode)
    u_long l_addr;
    u_long *reg_ptr;
    u_long size;
    int mem_op_type;
    int read_or_write;
    int usmode;
{
    u_long regnum;
    u_long w;

    /*
     * Put the register number and a valid bit in
     * the Data Memory Transaction register.
     */
    if (reg_ptr == (u_long *)&dummy_r0) {
        regnum = 0;
    } else {
        regnum = (int)(reg_ptr - &regs[0]);
    }
    if (regnum > 31) {
        err("set_dmt_regs: register number > 31", __LINE__, __FILE__);
    }

    /*
     * Set scoreboard bits if the operation was any kind of load
     * (this includes XMEM's.
     */
    if (mem_op_type != ST) {
        SBR |= 1 << regnum;

        if (size == DWORD) {
            SBR |= 1 << (regnum + 1);
        }
    }

    /*
     * If shadowing is turned off, don't load any of the DMT registers.
     */
    if (PSR & 1) {
        return;
    }
    DMT0 = (regnum << 7) | 1 | (usmode << 14) | (PSR_ENDIAN << 15);
    /*
     * Set the data memory address register to the logical address
     * of the word that we faulted on.
     */
    DMA0 = l_addr & ~3;

    /*
     * Set the READBAR bit in dmt0 if this transaction was a write.
     */
    if (read_or_write == WRITE) {
        DMT0 |= 2;
    }

    /*
     * If the operation was an exchange, set the "lock bus"
     * bit in the Data Memory Transaction register (bit 12).
     */
    if (mem_op_type == XMEM || mem_op_type == XMEM_U) {
        DMT0 |= 0x1000;
    }

    /*
     * If the operation was signed, set the signed bit
     * in the Data Memory Trasaction register (bit 6).
     */
    if (mem_op_type == LD || mem_op_type == XMEM) {
        DMT0 |= 0x40;
    }

    /*
     * Here we set the data memory transaction and data memory
     * data registers according the the size of operation
     * (byte/short/word/double) and the address.  Something
     * that is not in the 88k user's manual is that the
     * data memory data register is pre-shifted to align
     * the data with the memory system.  I deduced this
     * from the system V kernel from Moto. -rcb
     *
     * Ah, and Moto does it again!  They changed the chip, the kernel,
     * but didn't tell us.  Ha ha ha.  Very funny.  We just wasted
     * about 8 hours of engineering time.
     *
     * Arrrg! The data *is* smeared, but it is also replicated.
     */
    switch (size) {
        case BYTE:
            w = *reg_ptr & 0xff;
            DMD0 = (w << 24) | (w << 16) | (w << 8) | w;
            switch (l_addr & 3) {
                case 0:
                    DMT0 |= 0x20;
                    break;

                case 1:
                    DMT0 |= 0x10;
                    break;

                case 2:
                    DMT0 |= 0x8;
                    break;

                case 3:
                    DMT0 |= 0x4;
                    break;
            }
            break;

        case HALF:
            w = *reg_ptr & 0xffff;
            DMD0 = (w << 16) | w;
            if (l_addr & 2) {
                DMT0 |= 0xc;
            } else {
                DMT0 |= 0x30;
            }
            break;

        case WORD:
            DMT0 |= 0x3c;
            DMD0 = *reg_ptr;
            break;

        case DWORD:
            DMT0 |= 0x3c;
            DMD0 = *reg_ptr;

            /*
             * If this is a double word operation,
             * set the second (of three) Data Memory
             * register set.
             */
            DMA1 = DMA0 + 4;
            if (read_or_write == WRITE) {
                DMD1 = *(reg_ptr + 1);
            }
            /*
             * Copy the data memory transaction register that we just
             * set up (dmt0) to dmt1, but make dmt1's register number
             * be 1 more than dmt0's register number.
             */
            DMT1 = DMT0 + 0x80;

            /*
             * Set the DOUB1 bit in dmt0 to signify a double word
             * transaction.
             */
            DMT0 |= 0x2000;
            DMD1 = *(reg_ptr + 1);
            break;
    }
}

/*
 * This is called when the CALC_ADDR macro in execute.c finds a zero
 * in a tlb slot or when the logical address it out of range of the
 * software tlb's.  We return a exception code if an exception 
 * should be generated, E_NONE otherwise.
 */
int
l_mem_op(l_addr, reg_ptr, size, mem_op_type, usmode)
    u_long l_addr;
    u_long *reg_ptr;          /* Points to register to load or store. */
    u_long size;              /* 1, 2, 4, or 8                        */
    u_long mem_op_type;       /* LD, ST, XMEM, LD_U, or XMEM_U        */
    int    usmode;            /* 0: user 1: supervisor                */
{
    u_long  l_page, l_seg;
    struct  page *page_ptr;
    u_long  physical_address;
    u_long  physical_page;
    int     exception;
    u_char    *page_base_address;
    u_long  *mem_ptr;
    int     read_or_write;
    u_char  ***tlb1;
    u_char  **tlb2;

    /*
     * Translate the logical address to a 88000 physical address.  If
     * we get a translation error, return to the executer with an exception
     * indication.
     */
    if (mem_op_type == ST || mem_op_type == XMEM || 
        mem_op_type == XMEM_U) {
        read_or_write = WRITE;
    } else {
        read_or_write = READ;
    }

    exception = l_to_p(usmode, 
                       l_addr, 
                       &physical_address, 
                       DATA_CMMU(l_addr),
                       read_or_write);

    /*
     * Hack for Andrew.
     */
    if (exception == E_DACC && 
        usmode == M_SUPERVISOR &&
        sim_catch_exception[1024]) {
        sim_catch_exception[E_DACC] = 1;
    }

    if (exception != E_NONE && (PSR & 1) == 0) {
        set_dmt_regs(l_addr, reg_ptr, size, mem_op_type, read_or_write, usmode);
    }
    if (exception != E_NONE) {
        return exception;
    }

    /*
     * force the alignment, if it was misaligned and alignment checking
     * was on we wouldn't be here.
     */
    physical_address &= ~(size - 1);

    /*
     * If the physical address is greater than the memory size, see
     * if the memory operation hits an IO register.
     */
    if (physical_address >= memory_size) {

        exception = io_operation(physical_address, 
                                 reg_ptr, 
                                 size, 
                                 mem_op_type, 
                                 0);

        if (exception != E_NONE) {
            set_dmt_regs(l_addr, 
                         reg_ptr, 
                         size, 
                         mem_op_type, 
                         read_or_write, 
                         usmode);

            cmmu_set_bus_error(physical_address);
        }
        /*
         * Hack for Andrew.
         */
        if (exception == E_DACC && 
            usmode == M_SUPERVISOR &&
            sim_catch_exception[1024]) {
            sim_catch_exception[E_DACC] = 1;
        }
        if (tracing) {
            emit_trace_record(physical_address, 
                              *reg_ptr, 
                              size, 
                              usmode, 
                              mem_op_type);
        }
        return exception;
    }

    /*
     * Compute a pointer to the base address of the data part of
     * the addressed page.
     */
    physical_page = physical_address >> 12;
    page_ptr = page_table[physical_page];
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(poff(physical_address));
        page_table[physical_page] = page_ptr;
    }
    page_base_address = (u_char *)&(page_ptr->values[0]);

    /*
     * Here we do that actual load, store, or xmem.
     */
    mem_ptr = (u_long *)(page_base_address + poff(l_addr));
    do_mem_op(reg_ptr, mem_ptr, size, mem_op_type);

    /*
     * If we are tracing the execution, we don't want to load
     * the tlb's because we want every memory operation to
     * cause this function (l_mem_op) to be called.  So
     * we return before the code to load the tlb's is executed.
     */
    if (tracing) {
        emit_trace_record(physical_address, 
                          *reg_ptr, 
                          size, 
                          usmode, 
                          mem_op_type);
        return E_NONE;
    }

    /*
     * Put a pointer to the page in the tlb so that the next time
     * the program references this page it won't have to go to the
     * expense of calling this function.
     *
     * Whether this is a read or write, we load the "load" tlb.
     */
    l_seg = btos(l_addr);
    l_page = btop(l_addr);

    tlb1 = (usmode == M_USER) ? &u_load_tlb[0] : &s_load_tlb[0];
    tlb2 = tlb1[l_seg];
    if (tlb2 == &defaultpagetlb[0]) {
        tlb2 = (u_char **)sbrk(PAGETLB * sizeof(char *));
        if (!tlb2) {
           err("l_mem_op: sbrk err for 2nd lev tlb\n", __LINE__, __FILE__);
        }
        tlb1[l_seg] = tlb2;
    }
    tlb2[l_page] = page_base_address;

    /*
     * If this is the kind of instruction (ST, XMEM) that uses the
     * "store" tlb, then load the right part of the "store" tlb
     * with a pointer to the data part of the page.
     */
    if (read_or_write == WRITE) {
        tlb1 = (usmode == M_USER) ? &u_store_tlb[0] : &s_store_tlb[0];
        tlb2 = tlb1[l_seg];
        if (tlb2 == &defaultpagetlb[0]) {
            tlb2 = (u_char **)sbrk(PAGETLB * sizeof(char *));
            if (!tlb2) {
               err("l_mem_op: sbrk err for 2nd levl tlb\n", __LINE__, __FILE__);
            }
            tlb1[l_seg] = tlb2;
        } 
        if (tlb2[l_page]) {
            err("l_mem_op: faulted page is in TLB!\n", __LINE__, __FILE__);
        }
        tlb2[l_page] = page_base_address;
    }

    return E_NONE;
}

/*
 * This does the actual moving of data in a slow load, store, or xmem
 * The first two parameters are host pointers.  The first points
 * to the register to load, store, or exchange with.  The second points
 * into a simulated page (i.e., into the 'values' part).
 */
do_mem_op(reg_ptr, mem_ptr, size, mem_op_type)
    u_long *reg_ptr;
    u_long *mem_ptr;
    u_long size;
    u_long mem_op_type;
{
    switch (mem_op_type) {
        case LD:
            switch (size) {
                case BYTE:
                    *reg_ptr = *(char *)mem_ptr;
                    break;

                case HALF:
                    *reg_ptr = *(short *)mem_ptr;
                    break;

                case WORD:
                    *reg_ptr = *mem_ptr;
                    break;

                case DWORD:
                    *reg_ptr = *mem_ptr;
                    *(reg_ptr + 1) = *(mem_ptr + 1);
                    break;

                default:
                    sim_printf("do_mem_op(%X, %X, %X, %X), case error.\n",
                                    reg_ptr, mem_ptr, size, mem_op_type);
            }
            break;

        case ST:
            switch (size) {
                case BYTE:
                    *(u_char *)mem_ptr = *reg_ptr;
                    break;

                case HALF:
                    *(u_short *)mem_ptr = *reg_ptr;
                    break;

                case WORD:
                    *mem_ptr = *reg_ptr;
                    break;

                case DWORD:
                    *mem_ptr = *reg_ptr;
                    *(mem_ptr + 1) = *(reg_ptr + 1);
                    break;

                default:
                    sim_printf("do_mem_op(%X, %X, %X, %X), case error.\n",
                                    reg_ptr, mem_ptr, size, mem_op_type);
            }
            break;

        case XMEM:
            switch (size) {
                int t;

                case BYTE:
                    t = *(char *)mem_ptr;
                    *(char *)mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case HALF:
                    t = *(short *)mem_ptr;
                    *(short *)mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case WORD:
                    t = *mem_ptr;
                    *mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case DWORD:
                    t = *mem_ptr;
                    *mem_ptr = *reg_ptr;
                    *reg_ptr = t;

                    t = *(mem_ptr + 1);
                    *(mem_ptr + 1) = *(reg_ptr + 1);
                    *(reg_ptr + 1) = t;
                    break;

                default:
                    sim_printf("do_mem_op(%X, %X, %X, %X), case error.\n",
                                    reg_ptr, mem_ptr, size, mem_op_type);
            }
            break;

        case LD_U:
            switch (size) {
                case BYTE:
                    *reg_ptr = *(u_char *)mem_ptr;
                    break;

                case HALF:
                    *reg_ptr = *(u_short *)mem_ptr;
                    break;

                case WORD:
                    *reg_ptr = *mem_ptr;
                    break;

                case DWORD:
                    *reg_ptr = *mem_ptr;
                    *(reg_ptr + 1) = *(mem_ptr + 1);
                    break;

                default:
                    sim_printf("do_mem_op(%X, %X, %X, %X), case error.\n",
                                    reg_ptr, mem_ptr, size, mem_op_type);
            }
            break;

        case XMEM_U:
            switch (size) {
                u_long t;

                case BYTE:
                    t = *(u_char *)mem_ptr;
                    *(u_char *)mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case HALF:
                    t = *(u_short *)mem_ptr;
                    *(u_short *)mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case WORD:
                    t = *mem_ptr;
                    *mem_ptr = *reg_ptr;
                    *reg_ptr = t;
                    break;

                case DWORD:
                    t = *mem_ptr;
                    *mem_ptr = *reg_ptr;
                    *reg_ptr = t;

                    t = *(mem_ptr + 1);
                    *(mem_ptr + 1) = *(reg_ptr + 1);
                    *(reg_ptr + 1) = t;
                    break;

                default:
                    sim_printf("do_mem_op(%X, %X, %X, %X), case error.\n",
                                    reg_ptr, mem_ptr, size, mem_op_type);
            }
            break;

        default:
            sim_printf("do_mem_op: case error on mem_op_type.\n");
            break;
    }
    /*
     * In case the operation that we just performed modified r0 or
     * the word just after r31, zero them.
     */
    regs[0] = 0;
    regs[32] = 0;
}

/*
 * This keeps a list of decoded pages for the IO space.
 * We are passed the physical address and we return a
 * decoded pointer.
 */
struct decoded_i *
io_decoded_ptr(physical_address, pagepptr)
    u_long physical_address;
    struct decoded_i **pagepptr;
{
    struct io_list *p;
    u_long page_number;
    u_long page_offset;
    int hit;

    page_number = physical_address >> 12;

    /*
     * See if a decoded part for this address has already been made.
     */
    hit = 0;
    for (p = io_list ; p ; p = p->next_p) {
        if (p->page_number == page_number) {
            hit = 1;
            break;
        }
    }

    /*
     * If we couldn't find the decoded part for this physical address,
     * make a new structure and put it on the list.
     */
    if (!hit) {
        p = (struct io_list *)malloc(sizeof(struct io_list));
        if (!p) {
            sim_printf("io_decoded_ptr: unable to malloc.\n");
            exit(1);
        }
        p->page_number = page_number;
        p->decoded_part = allocate_decoded_part();
        p->next_p = io_list;
        io_list = p;
    }

    page_offset = poff(physical_address);
    if (pagepptr) {
        *pagepptr = &(p->decoded_part->decoded_i[0]);
    }
    return &(p->decoded_part->decoded_i[page_offset / 4]);
}

/*
 * This translates a logical 88000 address to a decoded pointer.
 *
 * This function has the side effect of allocating the decoded part
 * of the page if it hasn't been allocated yet.  
 */
struct decoded_i *
l_to_d(logical_address, usmode, pagepptr)
    u_long logical_address;
    int      usmode;
    struct decoded_i **pagepptr;
{
    u_long physical_address;
    int      status;
    u_long page, pagela;
    u_long page_offset;
    struct page *page_ptr;
    struct decitem *decp;
    struct decitem *lastdecp = &deccache[usmode][DECMAX];

    pagela = logical_address & ~PAGEMASK;
    for (decp = &deccache[usmode][0] ; decp < lastdecp ; decp++) {
        if (pagela == decp->pagela && decp->pagep) {
            if (pagepptr) {
                *pagepptr = decp->pagep;
            }
            return decp->pagep + poff(logical_address) / 4;
        }
    }

    status = l_to_p(usmode, logical_address, &physical_address, 
                    CODE_CMMU(logical_address), READ);

    if (status != E_NONE) {
        return (struct decoded_i *)0;
    }

    if (physical_address >= memory_size) {
        return io_decoded_ptr(physical_address, pagepptr);
    }

    page = physical_address >> 12;
    page_ptr = page_table[page];

    /*
     * If the page is not yet allocated, allocate it.
     */
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(ip & ~PAGEMASK);
        page_table[page] = page_ptr;
    }

    /*
     * If there isn't a decoded part for the page yet, make one.
     * We have to return its address and you can't take the
     * address of something that doesn't exist.
     */
    if (!page_ptr->decoded_part) {
        page_ptr->decoded_part = allocate_decoded_part();
    }

    /*
     * If we were called with a non-zero 'pagepptr' then we've been
     * called to look up the decoded pointer of an active code location.
     * In this case, put this translated page address into our little cache.
     */
    if (pagepptr) {
        decp = nextdecp[usmode];
        decp->pagep = *pagepptr = &(page_ptr->decoded_part->decoded_i[0]);
        decp->pagela = pagela;

        /*
         * Increment our next-slot-to-load pointer.  If it hits the
         * end of the table, set it to the first element.  This is
         * makes it a FIFO cache.
         */
        nextdecp[usmode]++;
        if (nextdecp[usmode] == lastdecp) {
            nextdecp[usmode] = &deccache[usmode][0];
        }
    }

    page_offset = poff(physical_address);

    return  &(page_ptr->decoded_part->decoded_i[page_offset / 4]);
}

/*
 * Called when an assertion failure happens.
 */
void
err(reason_string, line_number, file_string)
    char *reason_string;
    int  line_number;
    char *file_string;
{
    sim_printf("fatal error in simulator: %s at line %d in file %s\n",
            reason_string, line_number, file_string);
    exit(1);
}

/*
 * Print a map of the use of the simulator's physical memory.
 * We print a '.', 'I', or 'D' for every 4k page in memory.
 *
 * '.' means the page has never been touched.
 *
 * 'D' means that the page has been touched, but no instructions
 *     on the page have been executed.
 *
 * 'I' means that instructions have been executed on the page.
 */
void
sim_printmap()
{
    int i;
    struct page *p;

    for (i = 0 ; i < page_table_size ; i++) {
        p = page_table[i];
        if (i % 64 == 0) {
            if (i > 0) {
                sim_printf("\n");
            }
            sim_printf("0%08X: ", i * PAGESIZE);
        }

        if (!p) {
            sim_printf(".");
        } else {
            if (p->decoded_part) {
                sim_printf("I");
            } else {
                sim_printf("D");
            }
        }
    }
    sim_printf("\n");
}

/*
 * Returns a copy of the passed string.
 */
char *
sim_strdup(s)
    char *s;
{
    return s == 0 ? 0 : strcpy(malloc(strlen(s)+1), s);
}

/*
 * Insert a single step breakpoint at the instruction after
 * the one pointed to by the passed pointer.
 */
void
insert_ss_breakpoint(ip, usmode)
    u_long ip;
{
    struct decoded_i *p;

    extern sim_ss_breakpoint(), sim_not_decoded();

    /*
     * Set a single-step breakpoint on the next instruction.
     * This is superfluous if the current instruction is
     * an unconditional branch or an rte.  But it doesn't hurt
     * to have an extra single-step branch lying around and its
     * easier not to do the test.
     */
    p = l_to_d(ip+4, usmode, 0);
    if (p) {
        p->norm_e_addr = sim_ss_breakpoint;
    }

    if (tracing) {
        int    exception;
        u_long instr, physical_address;
        /*
         * Find the instruction so that we can include it in the trace
         * record.
         */
        exception = l_to_p(usmode, ip, &physical_address, CODE_CMMU(ip), READ);
        if (exception != E_NONE) {
            instr = 0;
        } else {
            instr = read_sim_w(physical_address);
        }

        emit_trace_record(physical_address, instr, WORD, usmode, FETCH);
    }

    /*
     * Set the interrupt flag so that the simulator will stop after
     * branches.
     */
    sim_interrupt_flag |= INT_SINGLESTEP;

    /*
     * Clear any single-step breakpoint set on the current
     * instruction.  We do this last in case the current
     * instruction is a spin-loop branch (a branch to itself).
     */
    p = l_to_d(ip, usmode, 0);
    if (p) {
        p->norm_e_addr = sim_not_decoded;
    }
}

/*
 * To help fool the optimizer.
 */
int sim_zero()
{
    return 0;
}

#include <stdio.h>

FILE *etrace_fd;

/*
 * Opens the trace data file.
 */
void
open_trace_file(fname)
    char *fname;
{
    if ((etrace_fd = fopen(fname, "a")) ==  0) {
        sim_printf("unable to open %s for append\n", fname);
        tracing = 0;
    }
}

/*
 * Emits trace data to a file.
 */
void
emit_trace_record(physical_address, data, size, usmode, mem_op_type)
    u_long physical_address;
    u_long data;
    int    size;
    int    usmode;
    int    mem_op_type;
{
    struct etrace et;

    et.physical_address = physical_address;
    et.lowdata = data & 0x3f;
    switch (mem_op_type) {
        case LD:   case LD_U:    et.write = 0; et.lock = 0; et.fetch = 0; break;
        case ST:                 et.write = 1; et.lock = 0; et.fetch = 0; break;
        case XMEM: case XMEM_U:  et.write = 1; et.lock = 1; et.fetch = 0; break;
        case FETCH:              et.write = 0; et.lock = 0; et.fetch = 1; break;
    }
    if (size > DWORD) {
        sim_printf("emit_trace_record: unexpected size.\n");
    }

    et.size = size;
    et.usmode = usmode;
    et.global = last_global;
    et.cacheinhibit = last_cacheinhibit;
    et.writethru = last_writethru;
    

    /*
     * If this is a double word operation, turn it into two
     * word operations at adjacent word addresses.
     */
    if (size == DWORD) {
        et.size = WORD;
        fwrite(&et, sizeof(struct etrace), 1, etrace_fd);
        physical_address += 4;
    }

    fwrite(&et, sizeof(struct etrace), 1, etrace_fd);
}

/*
 * Closes the trace file.
 */
void
close_trace_file()
{
    if (fclose(etrace_fd) != 0) {
       sim_printf("close_trace_file: error in closing file.\n");
    }
    tracing = 0;
}

struct sig {
    int  (*func)();
    int  cnt;
} sigtab[NSIG];

/*
 * This is the signal handler for all signals.  We just record the
 * occurance of the signal and return.  Later, when the instruction
 * executer checks sim_interrupt_flag, the handlers will actually be
 * called.
 */
void
simsighandler(sig, code, scp)
    int sig, code;
    struct sigcontext *scp;
{
    if (sig >= NSIG) {
        sim_printf("simsighandler: signal number %d is out of range\n", signal);
        return;
    }
    if (sim_in_execution) {
        if (sigtab[sig].cnt++ > 1000) {
            sim_printf("simsighandler: overrun on signal %d\n", sig);
        }
        sim_interrupt_flag |= INT_SIGNAL;
    } else {
	if (sigtab[sig].func) {
            (*sigtab[sig].func)();
        } else {
            sim_printf("simsighandler: stray signal %d\n", sig);
        }
    }
}

/*
 * This implements the simulator's own signal handling scheme.  Its
 * like the UNIX mechanism on which it is based.  Signal handlers
 * in this scheme will only be called between 88k instructions.
 */
void
simsignal(sig, func)
    int (*func)();
    u_long sig;
{
    if (sig >= NSIG) {
        sim_printf("sethandler: signal number %d is out of range\n", signal);
        return;
    }
    sigtab[sig].func = func;
    sigtab[sig].cnt = 0;
    signal(sig, simsighandler);
}

/*
 * This is called by the front end when it finds that sim_interrupt_flag
 * has INT_SIGNAL or'd into it.  We call any signal handlers as many
 * times as needed.
 */
void
callhandlers()
{
    int i;

    for (i = 0 ; i < NSIG ; i++) {
        while (sigtab[i].cnt > 0) {
            sigtab[i].cnt--;
            (*sigtab[i].func)();
        }
    }
}
