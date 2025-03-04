/* 88200 CMMU simulator.  *
 * Copyright (c) 1987, 1988, Tektronix Inc.  

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.

 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/cmmu.c,v 1.34 90/12/29 21:28:40 robertb Exp $ 
 */
#include "sim.h"
#include "cmmu.h"
#include "io.h"

#define CMMUMAX (8)
struct cmmu_registers *cmmu_table[CMMUMAX];

/*
 * This maps the 2 bits of logical address in A12 and A13 to
 * a cmmu number in 0..7 based on the cmmu configuration
 * as described by 'cmmus'.
 */
u_long cmmu_number_table[CMMUMAX];

static int wp;

int last_global;
int last_writethru;
int last_cacheinhibit;
int last_writeprotect;

u_long cmmus;
#define	SIMCMMU	"simcmmu"

/*
 * This converts a one-word structure to an unsigned integer.
 * On a 68020, this is a no-op.  But on a machine where integer
 * parameters are passed in registers an structures are passed in
 * memory, this function would have a structure-typed formal parameter
 * and return an unsigned:
 */
u_long s_to_u(x)
   struct d { u_long e; } *x;
{
   return x->e;
}

/*
 * These converts the other way, from u_long's to structures.
 */
struct segment_descriptor u_to_sd(x)
    u_long x;
{
    union { u_long i; struct segment_descriptor sd; } u;
    u.i = x;
    return u.sd;
}

struct page_descriptor u_to_pd(x)
    u_long x;
{
    union { u_long i; struct page_descriptor pd; } u;
    u.i = x;
    return u.pd;
}

struct area_descriptor u_to_ad(x)
    u_long x;
{
    union { u_long i; struct area_descriptor ad; } u;
    u.i = x;
    return u.ad;
}

u_long sctr_to_u(s)
    struct cmmu_sctr s;
{
    union { u_long i; struct cmmu_sctr sctr; } u;
    u.sctr = s;
    return u.i;
}

struct cmmu_sctr u_to_sctr(i)
    u_long i;
{
    union { u_long i; struct cmmu_sctr sctr; } u;
    u.i = i;
    return u.sctr;
}

/*
 * This translates a logical 88000 address to a physical 88000 address.
 * It an exception code to say if the translation was successful.
 */
int
l_to_p(usmode, logical_address, physical_address, cmmu, read_or_write)
    int usmode;
    u_long logical_address;
    u_long *physical_address;
    int      cmmu;
    int      read_or_write;
{
#define DACC_OR_CACC    (cmmu < 4 ? E_DACC : E_CACC)

    u_long segment_base;
    u_long segment_number;
    u_long page_number;
    u_long page_base;
    u_long page_frame_address;
    u_long page_offset;
    struct cmmu_registers *cmmu_regs;
    int i;

    if (!(cmmus & (1 << cmmu))) {
        err("l_to_p: cmmu not active\n", __LINE__, __FILE__);
    }

    /* This next line is new for the multi-processor simulator.
       We no longer support more than one code and one data CMMU per
       processor (though it would be easy to add back in).  So the
       choice of CMMU is based on whether we are loading code or
       accessing data and the selected CPU. -rcb 6/90 */

    cmmu = selected_processor + ((cmmu >= 4) ? 4 : 0);

    cmmu_regs = cmmu_table[cmmu];
    *physical_address = 0;
    last_writeprotect = 0;
    cmmu_regs->batc.valid = 0;
    /*
     * Check the hardwired BATC ports for control memory first.
     */
    if (usmode == M_SUPERVISOR && 
       (logical_address & 0xfff00000) == 0xfff00000) {
        *physical_address = logical_address;
        cmmu_regs->batc.valid = 1;

        last_writeprotect = cmmu_regs->batc.writeprotect = 0;
        last_cacheinhibit = cmmu_regs->batc.cacheinhibit  = 1;
        last_global       = cmmu_regs->batc.global        = 0;
        last_writethru    = cmmu_regs->batc.writethru     = 1;

        cmmu_regs->batc.supervisor = 1;
        cmmu_regs->batc.p_base_address = 
                                 0x1ffe | ((logical_address >> 19) & 1);
        cmmu_regs->batc.l_base_address = 
                                 0x1ffe | ((logical_address >> 19) & 1);
        cmmu_regs->segment_descriptor_address = -1;
        return E_NONE;
    }


    /*
     * Fetch the area descriptor out of the CMMU page.
     */
    if (usmode == M_USER) {
        cmmu_regs->area_descriptor = cmmu_regs->uapr;
    } else {
        cmmu_regs->area_descriptor = cmmu_regs->sapr;
    }

    /*
     * Save away information in case we are tracing execution or
     * we are doing a probe command.
     */
    last_cacheinhibit = cmmu_regs->area_descriptor.cacheinhibit;
    last_global       = cmmu_regs->area_descriptor.global;
    last_writethru    = cmmu_regs->area_descriptor.writethru;

    /*
     * If the segment table is not valid, make a straight-thru mapping.
     */
    if (!cmmu_regs->area_descriptor.valid) {
        cmmu_regs->segment_descriptor_address = -1;
        *physical_address = logical_address;
        return E_NONE;
    }

    /*
     * Search the BATC for valid entries that have the
     * same user/supervisor mode as that passed to us.  And its
     * logical address must match that passed to us in the top
     * 13 bits.
     */
    for (i = 0 ; i <= 7 ; i++) {
        struct cmmu_batc *b = &(cmmu_regs->bwp[i]);
        if (b->valid &&
            b->l_base_address == logical_address >> 19 &&
            b->supervisor == usmode) {
            
            cmmu_regs->batc = *b;
            *physical_address = (b->p_base_address << 19) | 
                                (logical_address & 0x7ffff);

            cmmu_regs->segment_descriptor_address = -1;

            last_cacheinhibit |= b->cacheinhibit;
            last_global       |= b->global;
            last_writethru    |= b->writethru;
            last_writeprotect  = b->writeprotect;

            if (last_writeprotect && read_or_write == WRITE) {
                return DACC_OR_CACC;
            } else {
                return E_NONE;
            }
        }
    }

    segment_base = cmmu_regs->area_descriptor.segment_table_base * PAGESIZE;
    segment_number = btos(logical_address);
    cmmu_regs->segment_descriptor_address = segment_base + segment_number * 4;
    cmmu_regs->segment_descriptor = 
                   u_to_sd(read_sim_w(cmmu_regs->segment_descriptor_address));
    if (sim_errno) {
        cmmu_regs->page_descriptor_address = -1;
        cmmu_regs->lsr.faultcode = 3;
        return DACC_OR_CACC;
    }

    /*
     * If the segment descriptor is not valid, return.
     */
    if (!cmmu_regs->segment_descriptor.valid) {
        cmmu_regs->page_descriptor_address = -1;
        cmmu_regs->lsr.faultcode = 4;
        return DACC_OR_CACC;
    }

    last_cacheinhibit |= cmmu_regs->segment_descriptor.cacheinhibit;
    last_global       |= cmmu_regs->segment_descriptor.global;
    last_writethru    |= cmmu_regs->segment_descriptor.writethru;

    /*
     * If the segment is supervisor-only and the mode of access is user,
     * then return with a not-translatable status.
     */
    if (usmode == M_USER && cmmu_regs->segment_descriptor.supervisor_only) {
        cmmu_regs->page_descriptor_address = -1;
        cmmu_regs->lsr.faultcode = 6;
        return DACC_OR_CACC;
    }

    page_base = cmmu_regs->segment_descriptor.page_table_base * PAGESIZE;
    page_number = (logical_address >> 12) & 0x3ff;
    cmmu_regs->page_descriptor_address = page_base + page_number * 4;
    cmmu_regs->page_descriptor = 
                      u_to_pd(read_sim_w(cmmu_regs->page_descriptor_address));
    if (sim_errno) {
        cmmu_regs->lsr.faultcode = 3;
        return DACC_OR_CACC;
    }

    /*
     * If the page descriptor is not valid, return with not-translatable
     * status.
     */
    if (!cmmu_regs->page_descriptor.valid) {
        cmmu_regs->lsr.faultcode = 5;
        return DACC_OR_CACC;
    }

    /*
     * If the page is supervisor-only and the mode of access is user,
     * return with a not-translatable status.
     */
    if (usmode == M_USER && cmmu_regs->page_descriptor.supervisor_only) {
        cmmu_regs->lsr.faultcode = 6;
        return DACC_OR_CACC;
    }

    page_frame_address = cmmu_regs->page_descriptor.page_frame_base * PAGESIZE;
    page_offset = poff(logical_address);
    *physical_address = page_frame_address + page_offset;

    last_writeprotect = cmmu_regs->segment_descriptor.writeprotect | 
         cmmu_regs->page_descriptor.writeprotect;

    if (read_or_write == WRITE) {
        /*
         * If the write-protect bits in either the segment descriptor
         * of the page descriptor are set and a write is attempted,
         * return an exception.
         */
        if (last_writeprotect) {
            cmmu_regs->lsr.faultcode = 7;
            return DACC_OR_CACC;
        }
        /*
         * Update the modified and used bits in the page descriptor.
         */
        cmmu_regs->page_descriptor.modified = 1;
        cmmu_regs->page_descriptor.used = 1;
        write_sim_w(cmmu_regs->page_descriptor_address, 
                    s_to_u(&cmmu_regs->page_descriptor));
        if (sim_errno) {
            cmmu_regs->lsr.faultcode = 3;
        };
    } else {
        /*
         * Update the used bit in the page descriptor.
         */
        cmmu_regs->page_descriptor.used = 1;
        write_sim_w(cmmu_regs->page_descriptor_address, 
                    s_to_u(&cmmu_regs->page_descriptor));
        if (sim_errno) {
            cmmu_regs->lsr.faultcode = 3;
        };
    }

    last_cacheinhibit |= cmmu_regs->page_descriptor.cacheinhibit;
    last_global       |= cmmu_regs->page_descriptor.global;
    last_writethru    |= cmmu_regs->page_descriptor.writethru;

    return E_NONE;

#undef  DACC_OR_CACC
}

       
/*
 * Called the first time that cmmu_init() is called.
 */
init_cmmu_vars()
{ 
    int i, n;

    cmmus = varvalue(SIMCMMU);
    if (cmmus > 255) {
      sim_printf("error, $%s not valid, must be in 0..255\n", SIMCMMU);
      cmmus = 0;
    }
    if (cmmus == 0) {
        cmmus = 0xff;
        setvar(SIMCMMU, cmmus);
    }
    for (i = 0 ; i < 8 ; i += 4) {
        switch ((cmmus >> i) & 0xf) {
            /*
             * These are the cases of 1 CMMU being plugged in.
             */
            case 1:  for (n = 0;n < 4;n++) cmmu_number_table[i+n] = i+0; break;
            case 2:  for (n = 0;n < 4;n++) cmmu_number_table[i+n] = i+1; break;
            case 4:  for (n = 0;n < 4;n++) cmmu_number_table[i+n] = i+2; break;
            case 8:  for (n = 0;n < 4;n++) cmmu_number_table[i+n] = i+3; break;

            /*
             * These are the cases of 2 CMMU's being plugged in.
             */
            case 3:
                cmmu_number_table[i+0] = i;
                cmmu_number_table[i+1] = i + 1;
                cmmu_number_table[i+2] = i;
                cmmu_number_table[i+3] = i + 1;
                break;

            case 0xc:
                cmmu_number_table[i+0] = i;
                cmmu_number_table[i+1] = i + 1;
                cmmu_number_table[i+2] = i;
                cmmu_number_table[i+3] = i + 1;
                break;

            /*
             * This is the case of all 4 CMMU's being plugged in.
             */
            case 15: for (n = 0;n < 4;n++) cmmu_number_table[i+n] = i+n; break;

            default:
                sim_printf("init_cmmu_vars: invalid cmmu configuration\n");
                exit(1);
            
        }
    }
}

/*
 * Called once for each cmmu that is in the io table at initialization.
 */
cmmu_init(cmmu)
{
    struct cmmu_registers *p;
    int i;

    init_cmmu_vars(); /* This is called more than once, but that's ok */
    /*
     * If this cmmu isn't plugged in, let the init routine for it know.
     */
    if (!(cmmus & (1 << cmmu))) {
        return -1;
    }

    p = cmmu_table[cmmu];
    if (!p) {
        p = (struct cmmu_registers *)sbrk(sizeof(struct cmmu_registers));
        if (!p) {
            sim_printf("cmmu: unable to sbrk for cmmu_registers.\n");
            exit(1);
        }
        cmmu_table[cmmu] = p;
    } else {
        bzero(p, sizeof(struct cmmu_registers));
    }

    p->idr.id = cmmu;
    p->idr.type = 5;
    p->idr.version = 0;

    p->sapr.cacheinhibit = 1;
    p->uapr.cacheinhibit = 1;
    p->clkvr = 0x3f0fc000;  /* */
    for (i = 0 ; i < 256 ; i++) {
        p->cache_clkvr[i] = p->clkvr;
    }
    p->page_descriptor_address = -1;
    p->segment_descriptor_address = -1;
    p->page_descriptor = u_to_pd(-1);
    p->segment_descriptor = u_to_sd(-1);
    p->area_descriptor = u_to_ad(-1);

    return 0;
}

/*
 * These routines used to call io_change_addr if cmmu_init didn't return
 * -1.  This is wrong, because reads and writes to non-existent CMMUs do not
 * cause DACCs.  Writes are ignored.  There is no good reason to do a read or
 * xmem, so these will continue to print an error message and cause a DACC, for
 * software development purposes.
 */
void
data_cmmu_0_init()
{
    (void) cmmu_init(0);
}

/*
 */
void
data_cmmu_1_init()
{
    (void) cmmu_init(1);
}

/*
 */
void
data_cmmu_2_init()
{
    (void) cmmu_init(2);
}

/*
 */
void
data_cmmu_3_init()
{
    (void) cmmu_init(3);
}

/*
 */
void
code_cmmu_0_init()
{
    (void) cmmu_init(4);
}

/*
 */
void
code_cmmu_1_init()
{
    (void) cmmu_init(5);
}

/*
 */
void
code_cmmu_2_init()
{
    (void) cmmu_init(6);
}

/*
 */
void
code_cmmu_3_init()
{
    (void) cmmu_init(7);
}

/*
 * This prints the registers of a CMMU in an informative way for the user.
 */
void
cmmu_print(cmmu)
    int cmmu;
{
    struct cmmu_registers *cmmu_regs;
    u_long i, first;

    if (!(cmmus & (1 << cmmu))) {
        sim_printf("cmmu %d: not active\n", cmmu);
        return;
    }

    cmmu_regs = cmmu_table[cmmu];

    sim_printf("cmmu %d:  idr=0x%08x  scmr=0x%08x  ssr=0x%08x  sadr=0x%08x\n",
                  cmmu, 
                  s_to_u(&cmmu_regs->idr),
                  s_to_u(&cmmu_regs->scmr), 
                  s_to_u(&cmmu_regs->ssr),
                  s_to_u(&cmmu_regs->sadr));

    sim_printf("          lsr=0x%x  ladr=0x%x\n",
                                s_to_u(&cmmu_regs->lsr), cmmu_regs->ladr);

    sim_printf("          sapr=0x%08x uapr=0x%08x\n", 
                            s_to_u(&cmmu_regs->sapr), s_to_u(&cmmu_regs->uapr));

    sim_printf("last area descriptor=0x%08x\n", 
                                     s_to_u(&cmmu_regs->area_descriptor));

    /*
     * If the segment descriptor address is -1, it means that it,
     * the segment descriptor itself, and the page descriptor and its
     * address are not valid.  In this case we don't display them.
     */
    if (cmmu_regs->segment_descriptor_address != -1) {
        sim_printf("last segment descriptor=0x%08x fetched from 0x%08x\n", 
                s_to_u(&cmmu_regs->segment_descriptor), 
                cmmu_regs->segment_descriptor_address);

        if (cmmu_regs->page_descriptor_address != -1) {
            sim_printf("last page descriptor=0x%08x fetched from 0x%08x\n", 
                        s_to_u(&cmmu_regs->page_descriptor), 
                        cmmu_regs->page_descriptor_address);
        }
    }

    /*
     * Display any non-zero batc entries.
     */
    first = 1;
    for (i = 0 ; i < 8 ; i++) {
        struct cmmu_batc *b = &cmmu_regs->bwp[i];
        if (b->valid) {
            if (first) {
                sim_printf("port #   logical       physical      raw value\n");
                first = 0;
            }
            sim_printf("  %d      0x%08x    0x%08x    0x%08x\n",
                          i, 
                          b->l_base_address << 19,
                          b->p_base_address << 19,
                          s_to_u(b));
        }
    }
}

void data_cmmu_0_print() { cmmu_print(0); }
void data_cmmu_1_print() { cmmu_print(1); }
void data_cmmu_2_print() { cmmu_print(2); }
void data_cmmu_3_print() { cmmu_print(3); }
void code_cmmu_0_print() { cmmu_print(4); }
void code_cmmu_1_print() { cmmu_print(5); }
void code_cmmu_2_print() { cmmu_print(6); }
void code_cmmu_3_print() { cmmu_print(7); }

/*
 * This executes the CMMU's probe command.  We are passed M_USER
 * or M_SUPERVISOR.
 */
static
int
probe(mode, cmmu)
    int mode;       /* M_USER or M_SUPERVISOR */
    int cmmu;       /* 0..7 */
{
    int status;
    struct cmmu_registers *cmmu_regs;
    u_long addr_to_trans;

    if (!(cmmus & (1 << cmmu))) {
        sim_printf("cmmu: probe internal error cmmu=%d, cmmus=0x%x\n", 
                                                    cmmu, cmmus);
        return E_DACC;
    }

    cmmu_regs = cmmu_table[cmmu];
    addr_to_trans = cmmu_regs->sadr;

    cmmu_regs->sadr = 0;
    status = l_to_p(mode, addr_to_trans, &(cmmu_regs->sadr), cmmu, READ);
    cmmu_regs->ssr.writeprotect = last_writeprotect;
    switch (status) {
        case E_NONE:
            cmmu_regs->ssr.valid = 1;
            break;

        default:
            cmmu_regs->ssr.valid = 0;
            return;
            break;
    }

    /*
     * The translation used the block address translation
     * mechanism.  Set the status register.
     */
    if (cmmu_regs->batc.valid) {
        /*
         * The access was translated using the BATC
         */
        cmmu_regs->ssr.copybackerror = 0;
        cmmu_regs->ssr.buserror = 0;
        cmmu_regs->ssr.writethru = cmmu_regs->batc.writethru;
        cmmu_regs->ssr.supervisoronly = mode;
        cmmu_regs->ssr.global = cmmu_regs->batc.global;
        cmmu_regs->ssr.cacheinhibit = cmmu_regs->batc.cacheinhibit;
        cmmu_regs->ssr.modified = 0;
        cmmu_regs->ssr.used = 0;
        cmmu_regs->ssr.batchit = 1;
        return;
    }

    /*
     * The translation used the paged address translation
     * mechanism.  Set the status register.
     */
    cmmu_regs->ssr.batchit = 0;
    cmmu_regs->ssr.used = cmmu_regs->page_descriptor.used;
    cmmu_regs->ssr.modified = cmmu_regs->page_descriptor.modified;

    cmmu_regs->ssr.cacheinhibit = last_cacheinhibit;
    cmmu_regs->ssr.global = last_global;
    cmmu_regs->ssr.supervisoronly = 
                   cmmu_regs->segment_descriptor.supervisor_only ||
                   cmmu_regs->page_descriptor.supervisor_only;

    cmmu_regs->ssr.writethru = last_writethru;
}

int data_cmmu_0_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{ 
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 0, override);
}

int data_cmmu_1_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 1, override);
}

int data_cmmu_2_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 2, override);
}

int data_cmmu_3_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 3, override);
}

int code_cmmu_0_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 4, override);
}

int code_cmmu_1_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 5, override);
}

int code_cmmu_2_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 6, override);
}

int code_cmmu_3_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    return 
      cmmu_operation(address_offset, reg_ptr, size, mem_op_type, 7, override);
}

/*
 * This translates the address offset into an offset within
 * a cmmu_registes structure.  Only the page offset part of the address
 * is used.  If the offset does not touch a cmmu register, -1 is returned.
 */
int
cmmu_register_ptr(offset)
    u_long offset;
{
    u_long o = offset;
    struct cmmu_registers s;

#define CHECK(l, u, f)  if ((l) <= o && o <= (u)) \
  return ((int)&(s.f) - (int)&s) + (o - (l));

    CHECK(CMMU_IDR, CMMU_SADR, idr);
    CHECK(CMMU_SCTR, CMMU_LADR, sctr);
    CHECK(CMMU_SAD, CMMU_UAD, sapr);
    CHECK(CMMU_BATC0, CMMU_BATC7, bwp[0]);
    CHECK(CMMU_CDR0, CMMU_CDR3, cdr[0]);
    CHECK(CMMU_CTR0, CMMU_CTR3, ctr[0]);
    CHECK(CMMU_CLKVR, CMMU_CLKVR, clkvr);

#undef CHECK

    return -1;
}

/*
 * Called to execute a load, store, or xmem with a CMMU register.
 */
int
cmmu_operation(address_offset, reg_ptr, size, mem_op_type, cmmu, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      cmmu;          /* 0..7 */
    int      override;
{
    static int cmmu_warning1 = 0;
    struct cmmu_registers *cmmu_regs;
    u_long dummy_zero[2];
    u_long old_sadr;
    struct cmmu_batc old_bwp[8];
    char   *mem_ptr;
    int    offset;
    int    i;

    if (size < WORD && mem_op_type == ST) {
        sim_printf("error: store into cmmu register smaller than word.\n");
        return E_DACC;
    }

    /*
     * If this CMMU doesn't exist, then writes are ignored.
     * Other operations cause an error message and a DACC -- this isn't the
     * way the hardware works, but it's useful for program development.
     */
    if (!(cmmus & (1 << cmmu))) {
	switch (mem_op_type) {
	case ST:
	    return E_NONE;
	case LD:
	case LD_U:
	    sim_printf("error: read from non-existent CMMU.\n");
	    return E_DACC;
	default:
	    sim_printf("error: bizarre operation on non-existent CMMU.\n");
	    return E_DACC;
	}
    }

    cmmu_regs = cmmu_table[cmmu];

    offset = cmmu_register_ptr(address_offset);

    if (offset == -1) {
        mem_ptr = (char *) &dummy_zero[0];
        dummy_zero[0] = 0;
        dummy_zero[1] = 0;
        sim_printf("access misses cmmu registers, raising dacc.\n");
        return E_DACC;
    } else {
        mem_ptr = (char *)cmmu_regs + offset;
    }

    /*
     * The BATC registers are write-only.  
     */
    if ((mem_op_type == LD || mem_op_type == LD_U) && !override &&
        (CMMU_BATC0 <= address_offset && address_offset <= CMMU_BATC7)){
        mem_ptr = (char *) &dummy_zero[0];
        dummy_zero[0] = 0;
        dummy_zero[1] = 0;
        sim_printf("warning: trying to read a BATC write port.\n");
    }

    /*
     * The BATC registers are write-only.  We let it go thru, but
     * annoy the user with a message.
     */
    if ((mem_op_type == XMEM || mem_op_type == XMEM_U) && !override &&
        (CMMU_BATC0 <= address_offset && address_offset <= CMMU_BATC7)){
        sim_printf("warning: allowing xmem with a BATC write port.\n");
    }

    old_sadr = cmmu_regs->sadr;

    if (CMMU_BATC0 <= address_offset && address_offset <= CMMU_BATC7) {
        for (i = 0 ; i < 8 ; i++) {
            old_bwp[i] = cmmu_regs->bwp[i];
        }
    }

    /*
     * Do the actual operation between the CPU register and the CMMU register.
     */
    do_mem_op(reg_ptr, mem_ptr, size, mem_op_type);

    if (mem_op_type == LD || mem_op_type == LD_U) {
        return E_NONE;
    }

    /*
     * If the program writes any of the BATC ports, we must dump the part
     * of the software tlb's that was mapped by the old contents of the BATC.
     */
    if (CMMU_BATC0 <= address_offset && address_offset <= CMMU_BATC7) {
        for (i = 0 ; i < 8 ; i++) {
            if (s_to_u(&old_bwp[i]) != s_to_u(&cmmu_regs->bwp[i])) {
                if (cmmu < 4) {
                    flush_tlb_range(old_bwp[i].l_base_address << 19, 
                                    0x80000, 
                                    old_bwp[i].supervisor);
                } else {
                    flush_deccache(old_bwp[i].supervisor);
                }
            }
        }
    }

    /*
     * In case this was a double store or exchange, zero the pad fields of
     * the cmmu_registers structure.
     */
    if (size == DWORD) {
        cmmu_regs->pad1 = 0;
        cmmu_regs->pad2 = 0;
        cmmu_regs->pad3 = 0;
        cmmu_regs->pad4 = 0;
        cmmu_regs->pad5 = 0;
        cmmu_regs->pad6 = 0;
    }

    cmmu_regs->idr.type = 5;
    cmmu_regs->idr.version = 0;
    cmmu_regs->idr.reserved = 0;

    cmmu_regs->lsr.reserved1 = 0;
    cmmu_regs->lsr.reserved2 = 0;

    cmmu_regs->ssr.reserved1 = 0;
    cmmu_regs->ssr.reserved2 = 0;
    cmmu_regs->ssr.reserved3 = 0;

    cmmu_regs->sctr.reserved1 = 0;
    cmmu_regs->sctr.reserved2 = 0;

    cmmu_regs->uapr.reserved1 = 0;
    cmmu_regs->uapr.reserved2 = 0;
    cmmu_regs->uapr.reserved3 = 0;

    cmmu_regs->sapr.reserved1 = 0;
    cmmu_regs->sapr.reserved2 = 0;
    cmmu_regs->sapr.reserved3 = 0;

    /*
     * If any of the cache data registers were touched, write
     * all four of them back to the cache data arrays.
     */
    if (CMMU_CDR0 <= address_offset && address_offset <= CMMU_CDR3) {
        i = (cmmu_regs->sadr >> 2) & 0x3ff;
        cmmu_regs->cache_data0[i] = cmmu_regs->cdr[0];
        cmmu_regs->cache_data1[i] = cmmu_regs->cdr[1];
        cmmu_regs->cache_data2[i] = cmmu_regs->cdr[2];
        cmmu_regs->cache_data3[i] = cmmu_regs->cdr[3];
    }

    /*
     * If any of the cache tag registers were touched, mask and
     * write them all back to the simulated cache tag arrays.
     */
    if (CMMU_CTR0 <= address_offset && address_offset <= CMMU_CTR3) {
        cmmu_regs->ctr[0] &= 0xfffff000;
        cmmu_regs->ctr[1] &= 0xfffff000;
        cmmu_regs->ctr[2] &= 0xfffff000;
        cmmu_regs->ctr[3] &= 0xfffff000;

        i = (cmmu_regs->sadr >> 4) & 0xff;
        cmmu_regs->cache_tag0[i] = cmmu_regs->ctr[0];
        cmmu_regs->cache_tag1[i] = cmmu_regs->ctr[1];
        cmmu_regs->cache_tag2[i] = cmmu_regs->ctr[2];
        cmmu_regs->cache_tag3[i] = cmmu_regs->ctr[3];
    }

    /*
     * If the cache clkvr register is addressed, mask it and
     * write it back to the clkvr array.
     */
    if (CMMU_CLKVR == address_offset) {
        cmmu_regs->clkvr &= 0x3ffff000;
        i = (cmmu_regs->sadr >> 4) & 0xff;
        cmmu_regs->cache_clkvr[i] = cmmu_regs->clkvr;
    }

    if (address_offset == CMMU_SCMR) {
        /*
         * The control register has been loaded, decode the command
         * code.
         */
        switch ((cmmu_regs->scmr >> 2) & 0xf) {
            case 0: case 1: case 2: case 3: case 4:/* no op*/
                break;

                break;

            case 6: /* data copyback only, no op for simulator */
                break;

            case 7: 
            case 5:
                /* 
                 * The 88000 program is telling us to flush our cache.
                 * We don't have a real data cache in the simulator,
                 * but we do have a cache of decoded instructions, so
                 * if a code cmmu is being flushed, flush the
                 * decoded instructions.
                 */ 
                if (cmmu >= 4) {
                    switch ((cmmu_regs->scmr) & 3) {
                        case 0:
                            cache_flush_line(cmmu_regs->sadr >> 4);
                            break;

                        case 1:
                            cache_flush_page(cmmu_regs->sadr >> 12);
                            break;

                        case 2:
                            cache_flush_segment(btos(cmmu_regs->sadr));
                            break;

                        case 3:
                            cache_flush_all();
                            break;
                    }
                }
                break;

            case 8: case 0xa: /* ATC probe user */
                probe(M_USER, cmmu);
                break;

            case 9: case 0xb: /* ATC probe supervisor. */
                probe(M_SUPERVISOR, cmmu);
                break;

            case 0xc: case 0xe: /* Flush user ATC */
                if (cmmu < 4) {
                    switch ((cmmu_regs->scmr) & 3) {
                        case 0:
                            sim_printf("cmmu: can't flush line of patc\n");
                            break;

                        case 1:
                            flush_user_tlb_page(cmmu_regs->sadr >> 12);
                            break;

                        case 2:
                            flush_user_tlb_segment(btos(cmmu_regs->sadr));
                            break;

                        case 3:
                            flush_user_tlb_all();
                            break;
                    }
                } else {
                    flush_deccache(M_USER);
                }
                break;

            case 0xd: case 0xf: /* Flush supervisor ATC */
                if (cmmu < 4) {
                    switch ((cmmu_regs->scmr) & 3) {
                        case 0:
                            sim_printf("cmmu: can't flush line of patc\n");
                            break;

                        case 1:
                            flush_supervisor_tlb_page(cmmu_regs->sadr >> 12);
                            break;

                        case 2:
                            flush_supervisor_tlb_segment(btos(cmmu_regs->sadr));
                            break;

                        case 3:
                            flush_supervisor_tlb_all();
                            break;
                    }
                } else {
                    flush_deccache(M_SUPERVISOR);
                }
                break;

            default:
                sim_printf("cmmu_operation: case error.\n");
                break;
        }
    }

    /*
     * If the address register has changed we need to reload the
     * cache data and tag registers.
     */
    if (old_sadr != cmmu_regs->sadr) {
        i = (cmmu_regs->sadr >> 2) & 0x3ff;
        cmmu_regs->cdr[0] = cmmu_regs->cache_data0[i];
        cmmu_regs->cdr[1] = cmmu_regs->cache_data1[i];
        cmmu_regs->cdr[2] = cmmu_regs->cache_data2[i];
        cmmu_regs->cdr[3] = cmmu_regs->cache_data3[i];

        i = (cmmu_regs->sadr >> 4) & 0xff;
        cmmu_regs->ctr[0] = cmmu_regs->cache_tag0[i];
        cmmu_regs->ctr[1] = cmmu_regs->cache_tag1[i];
        cmmu_regs->ctr[2] = cmmu_regs->cache_tag2[i];
        cmmu_regs->ctr[3] = cmmu_regs->cache_tag3[i];

        cmmu_regs->clkvr = cmmu_regs->cache_clkvr[i];
    }
    /*
     * The 4/18/88 manual doesn't say it, but the SCMR and the SCTR
     * registers are effectively or'd together on a read.
     */
    if (mem_op_type == ST) {
        cmmu_regs->scmr = 
          (cmmu_regs->scmr & 0x3f) | (sctr_to_u(cmmu_regs->sctr) & 0xe000);
        cmmu_regs->sctr = u_to_sctr(cmmu_regs->scmr);
    }
    return E_NONE;
}

/*
 * This is called when a bus error occurs because there is no
 * memory at the addressed location. We set the exception status
 * in the data and code cmmu associated with the access.
 */
void
cmmu_set_bus_error(physical_address)
    u_long physical_address;
{
    u_long cmmu;
    struct cmmu_registers *cmmu_regs;

    cmmu = DATA_CMMU(physical_address);
    cmmu_regs = cmmu_table[cmmu];
    cmmu_regs->lsr.faultcode = 3;

    cmmu = CODE_CMMU(physical_address);
    cmmu_regs = cmmu_table[cmmu];
    cmmu_regs->lsr.faultcode = 3;
}

/*
 * This flushes the all of the decoded pages.
 */
cache_flush_all()
{
    struct page *p;
    int i;

    for (i = 0 ; i < page_table_size ; i++) {
        if (p = page_table[i]) {
            clean_decoded_part(p->decoded_part);
        }
    }
    free_literals();
}

/*
 */
cache_flush_line(line)
    u_long line;
{
    cache_flush_page(line >> 8);
}

/*
 */
cache_flush_page(page)
    u_long page;
{
    struct page *p;

    if (page < page_table_size && (p = page_table[page])) {
        clean_decoded_part(p->decoded_part);
    }
}

/*
 */
cache_flush_segment(segment)
    u_long segment;
{
    cache_flush_all();
}
