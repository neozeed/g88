/*
 * For 88200 CMMU simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/cmmu.h,v 1.10 88/06/28 16:42:43 robertb Exp $
 */

#define CMMU_IDR    (0)

#define CMMU_SCMR   (4)
#define CMMU_SSR    (8)
#define CMMU_SADR   (0xc)
#define CMMU_SCTR   (0x104)
#define CMMU_LSR    (0x108)
#define CMMU_LADR   (0x10c)

#define CMMU_SAD    (0x200)
#define CMMU_UAD    (0x204)
#define CMMU_BATC0  (0x400)
#define CMMU_BATC1  (0x404)
#define CMMU_BATC2  (0x408)
#define CMMU_BATC3  (0x40c)
#define CMMU_BATC4  (0x410)
#define CMMU_BATC5  (0x414)
#define CMMU_BATC6  (0x418)
#define CMMU_BATC7  (0x41c)

#define CMMU_CDR0   (0x800)
#define CMMU_CDR1   (0x804)
#define CMMU_CDR2   (0x808)
#define CMMU_CDR3   (0x80c)

#define CMMU_CTR0   (0x840)
#define CMMU_CTR1   (0x844)
#define CMMU_CTR2   (0x848)
#define CMMU_CTR3   (0x84c)

#define CMMU_CLKVR  (0x880)

extern void cmmu_set_bus_error();

extern struct cmmu_registers *cmmu_table[];

/*
 * CMMU ID register
 */
struct cmmu_idr {
        u_long      id:8,
                  type:3,
               version:5,
              reserved:16;
};

/*
 * CMMU Local Status Register
 */
struct cmmu_lsr {
    u_long          reserved1:13,
                    faultcode:3,
                    reserved2:16;
};

/*
 * CMMU System Status Register
 */
struct cmmu_ssr {
    u_long          reserved1:16,
                copybackerror:1,
                     buserror:1,
                    reserved2:4,
                    writethru:1,
               supervisoronly:1,
                       global:1,
                 cacheinhibit:1,
                    reserved3:1,
                     modified:1,
                         used:1,
                 writeprotect:1,
                      batchit:1,
                        valid:1;
};

/*
 * CMMU System Control Register
 */
struct cmmu_sctr {
    u_long          reserved1:16,
                 parityenable:1,
                  snoopenable:1,
          priorityarbitration:1,
                    reserved2:13;
};

/*
 * CMMU Area descriptor
 */
struct area_descriptor {
    u_long segment_table_base:20,
                    reserved1:2,
                    writethru:1,
                    reserved2:1,
                       global:1,
                 cacheinhibit:1,
                    reserved3:5,
                        valid:1;
};

/*
 * Segment descriptor
 */
struct segment_descriptor {
    u_long page_table_base:20,
                          :2,
                 writethru:1,
           supervisor_only:1,
                    global:1,
              cacheinhibit:1,
                          :3,
              writeprotect:1,
                          :1,
                     valid:1;
};

/*
 * Page descriptor
 */
struct page_descriptor {
    u_long page_frame_base:20,
                          :2,
                 writethru:1,
           supervisor_only:1,
                    global:1,
              cacheinhibit:1,
                          :1,
                  modified:1,
                      used:1,
              writeprotect:1,
                          :1,
                     valid:1;
};

/*
 * Block port register.
 */
struct cmmu_batc {
    u_long   l_base_address:13,
             p_base_address:13,
                 supervisor:1,
                  writethru:1,
                     global:1,
               cacheinhibit:1,
               writeprotect:1,
                      valid:1;
};

/*
 * The simulator's representation of a CMMU's registers
 * and caches.  The pad fields are to absorb double word
 * accesses.  They are zero'd after every double word
 * store just in case they were written to.
 */
struct cmmu_registers {
    struct cmmu_idr idr;
    u_long scmr;
    struct cmmu_ssr ssr;
    u_long sadr, pad0;

    struct cmmu_sctr sctr;
    struct cmmu_lsr lsr;
    u_long ladr, pad1;

    struct area_descriptor sapr, uapr;
    u_long pad2;

    struct cmmu_batc bwp[8];
    u_long pad3;

    u_long cdr[4], pad4;
    u_long ctr[4], pad5;
    u_long clkvr, pad6;

    u_long cache_data0[1024];
    u_long cache_data1[1024];
    u_long cache_data2[1024];
    u_long cache_data3[1024];

    u_long cache_tag0[256];
    u_long cache_tag1[256];
    u_long cache_tag2[256];
    u_long cache_tag3[256]; 

    u_long cache_clkvr[256];

    /*
     * The following fields are here so that the pio command
     * can display their contents and for the probe command.
     */
    struct segment_descriptor segment_descriptor;
    u_long segment_descriptor_address;

    struct page_descriptor page_descriptor;
    u_long page_descriptor_address;

    struct area_descriptor area_descriptor;
    struct cmmu_batc batc;
};

extern int last_global;
extern int last_writethru;
extern int last_cacheinhibit;
