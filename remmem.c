/* Memory and register access functions for cross-debugging

   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/remmem.c,v 1.7 91/01/13 23:56:17 robertb Exp $
   $Locker:  $  */


#include <ctype.h>
#include <stdio.h>

#include "defs.h"
#include "param.h"
#include "ui.h"
#include "remote.h"

#define CACHELINESIZE       (128)
#define CACHELINEMASK       (CACHELINESIZE - 1)

static void dwrite_remote();
static boolean in_sram();
static boolean in_rom();
static char *regname();

struct mem_cache {
    int    usmode;
    u_long addr;
    struct mem_cache *next;
    u_char values[CACHELINESIZE]; } *mem_cache = nil;

/*
 * This is indexed by gdb register numbers and elements are word
 * offsets into the monitor ROM's register area.
 */
char regmap[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		30, 31,
		38,	/* best guess for $pc */
		33,	/* pid */
		35,	/* psr (really this is the epsr) */
		35,	/* epsr */
		36,	/* ssbr */
		37,	/* sxip */
		38,	/* snip */
		39,	/* sfip */
		40,	/* vbr */
		41,	/* dmt0 */
		42,	/* dmd0 */
		43,	/* dma0 */
		44,	/* dmt1 */
		45,	/* dmd1 */
		46,	/* dma1 */
		47,	/* dmt2 */
		48,	/* dmd2 */
		49,	/* dma2 */
		50,	/* sr0 */
		51,	/* sr1 */
		52,	/* sr2 */
		53,	/* sr3 */
		54,	/* fpecr */
		55,	/* fphs1 */
		56,	/* fpls1 */
		57,	/* fphs2 */
		58,	/* fpls2 */
		59,	/* fppt */
		60,	/* fprh */
		61,	/* fprl */
		62,	/* fpit */
		63,	/* fpsr */
		64,	/* fpcr */
		65,	/* ceimr */
		-1,	/* comefrom */
		67,	/* membrk */
		-1,	/* stackbase */
		-1,	/* ramsize */
		-1,	/* synthfp */
	};

/* Read the remote registers into the block REGS.  */

void
remote_fetch_registers (regs)
     char *regs;
{
  int i;

  for (i = 0; i < REGISTER_BYTES; i++) {
    regs[i] = read_remote_b(mon_register_area_addr[selected_processor] + i, 
                            M_SUPERVISOR);
  }
}

/* Store the remote registers from the contents of the block REGS.  */

void
remote_store_registers (regs)
     char *regs;
{
    int i;

    for (i = 0; i < REGISTER_BYTES; i++) {
        write_remote_b(mon_register_area_addr[selected_processor] + i, 
                      regs[i], M_SUPERVISOR);
    }
}

/* Read a word from remote address ADDR and return it.
   This goes through the data cache.  */

int
remote_fetch_word (addr, usmode)
     CORE_ADDR addr;
     int usmode;
{
    return read_remote_w(addr, usmode);
}

/* Write a word WORD into remote address ADDR.
   This goes through the data cache.  */

void
remote_store_word (addr, word)
     CORE_ADDR addr;
     int word;
{
    write_remote_w (addr, word, M_SUPERVISOR);
}


/* This is called when we get a C_DATAERROR from the target instead of
   something else.  If we are in motomode, the monitor has saved the
   DMA0 register in the per-processor monitor structure.  We upload that
   address and display it for the user. */

void dacc_rerr(message_postfix)
  char *message_postfix;
{
  u_long dacc_address;
  if (simulator) {
    rerr("Error while %s", message_postfix);
  }
  if (!varvalue("motomode")) {
    rerr("Target recieved DACC exception while %s", message_postfix);
  }
  rerr("Target got DACC.");
}

/*
 * Returns a register from the target.
 */
u_long remote_read_register(n)
    unsigned n;
{
  int processor = selected_processor;
  if (simulator) {
    return reg_sim(n);
  }

  /* We use the last processor, if it is idling, to implement memory
     breakpoints. */

  if (n == MEMBRK_REGNUM) {
    processor = MAX_PROCESSORS - 1;
  }

  /* The register numbering in the U-area, and thus of gdb, is
     a different from the register numbering in the target
     debug ROMs.  */

  return read_remote_w(mon_register_area_addr[processor] + 4 * regmap[n], 
                   M_SUPERVISOR);
}

/*
 * Puts a new value into a register in the target.
 */
void remote_write_register(n, w)
  unsigned n;
  u_long   w;
{
  int processor = selected_processor;
  if (simulator) {
    setreg_sim(n, w);
    return;
  }

  /* We use the last processor, if it is idling, to implement memory
     breakpoints. */
  if (n == MEMBRK_REGNUM) {
    processor = MAX_PROCESSORS - 1;
  }

  write_remote_w(mon_register_area_addr[processor] + 4 * regmap[n], 
                 w, M_SUPERVISOR);
}

/*
 * Read for the remote's data space.
 */
boolean dread_remote(buff, addr, nbytes, usmode)
    u_char *buff;
    u_long addr;
    u_long nbytes;
    int usmode;
{
    int i;
    if (simulator) {
      rerr("dread_remote called in simulator mode");
    }

    if (usmode != M_USER && usmode != M_SUPERVISOR) {
        usmode = (((remote_read_register(PSR_REGNUM) >> 31) & 1) == 0) ? 
				M_USER : M_SUPERVISOR;
    }
    /*
     * Read as manys full words as we can.
     */
    for (i = 0 ; i < (int)nbytes - 3 ; addr += 4) {
        u_long w;
        int j;

        w = read_remote_w(addr, usmode);
        if (remote_errno) {
            return true;
        }
        for (j = 24 ; j >= 0 && i < nbytes ; i++, j -= 8 ) {
            buff[i] = w >> j;
        }
    }

    /*
     * Now read a halfword, if we can.
     */
    for ( ; i < (int)nbytes - 1 ; i += 2, addr += 2) {
        u_long w;

        w = read_remote_h(addr, usmode);
        if (remote_errno) {
            return true;
        }
        buff[i] = w >> 8;
        buff[i+1] = w;
    }

    /*
     * Now read a byte, if we can.
     */
    for ( ; i < nbytes ; i++, addr++) {
        buff[i] = read_remote_b(addr, usmode);
        if (remote_errno) {
            return true;
        }
    }
    return false;
}

#ifdef DWRITE_NEEDED
/* This isn't used for anything right now.  But keep the code around
   in case it ever is. */
/*
 * Write to the remote data space.
 */
static void dwrite_remote(buff, addr, nbytes, usmode)
    unsigned char *buff;
    u_long addr;
    u_long nbytes;
    int usmode;
{
    int i;

    if (simulator) {
      rerr("dwrite_remote called in simulator mode");
    }
    if (usmode != M_USER && usmode != M_SUPERVISOR) {
        usmode = (((remote_read_register(PSR_REGNUM) >> 31) & 1) == 0) ? M_USER : M_SUPERVISOR;
    }
    for (i = 0 ; i < (int)nbytes - 3 ; addr += 4) {
        u_long w;
        int j;

        w = 0;
        for (j = 24 ; j >= 0  && i < nbytes ; i++, j -= 8 ) {
            w |= (buff[i] & 0xff) << j;
        }
        write_remote_w(addr, w, usmode);
        if (remote_errno) {
            rerr("dwrite: error at physical address 0x%x\n", addr);
        }
    }

    /*
     * Now write halfwords, if we can.
     */
    for ( ; i < (int)nbytes - 1 ; i += 2, addr += 2) {
        u_short w;

        w = (buff[i] << 8) | buff[i+1];
        write_remote_h(addr, w, usmode);
        if (remote_errno) {
            rerr("dwrite: error at physical address 0x%x\n", addr);
        }
    }

    /*
     * Now write bytes, if we can.
     */
    for (; i < nbytes ; i++, addr++) {
        write_remote_b(addr, buff[i], usmode);
        if (remote_errno) {
            rerr("dwrite: error at physical address 0x%x\n", addr);
        }
    }
}
#endif

/*
 * Read a byte, short, or word from the target
 */
u_long forced_read_remote(addr, size, usmode)
    u_long addr;
    int    size;
{
  u_long w;
  u_char command;

  remote_errno = false;
  if (addr & (size - 1)) {
    rerr("Reading %d bytes from 0x%x would cause a MA exception.",
        size, addr);
  }
  if (simulator) {
    u_long w, pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "supervisor" : "user");
    }
    switch (size) {
      case 1: w = read_sim_b(pa); break;
      case 2: w = read_sim_h(pa); break;
      case 4: w = read_sim_w(pa); break;
      default:
        rerr("Case error in forced_read_remote()");
    }
    if (sim_errno) {
      dacc_rerr("reading simulation memory");
    }
    return w;
  }
    splhi();
    init_checksum();
    if (varvalue("noparityinit")) {
        command = (usmode == M_USER) ? C_UPLOADNOINITUSER : C_UPLOADNOINIT;
    } else {
        command = (usmode == M_USER) ? C_UPLOADUSER : C_UPLOAD;
    }
    traceremote("forced_read_remote: at 0x%x, size=%d", addr, size);
    send_command(command);
    send_word(addr);
    send_word((u_long)size);
    send_command(C_END);
    send_checksum();

    init_checksum();
    switch (wait_for_token(C_DATASTART, C_DATAERROR, ta_warning, 1)) {
        case C_DATASTART:
            switch (size) {
                case 1: w = get_byte(); break;
                case 2: w = get_short(); break;
                case 4: w = get_word(); break;
                default:
                    rerr("Case error in forced_read_remote");
            }
           
            traceremote(", target returns 0x%x\n", w);
            if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
                rerr("forced_read_remote: Didn't find C_DATAEND");
            } else {
                verify_checksum();
            }
            break;

        case C_DATAERROR:
            remote_errno = true;
            w = 0;
            traceremote(", target gets DACC\n");
            break;

        default:
            ui_fprintf(stderr, 
                             "forced_read_remote: Didn't find C_DATASTART\n");
            w = 0;
    }
    spllo();
    return w;
}

/*
 * Read a byte, short, or long from the remote.
 */
static u_long read_remote(addr, size, usmode)
    u_long addr;
    int size;
{
    u_long offset;
    u_char *p;

    struct mem_cache *m = mem_cache;
    remote_errno = false;

    /*
     * If we are reading regular memory, and not IO space, cache
     * the data that we read back.  Also, read more than we need
     * for this request.
     */
    if (addr >= 0x80000000 && !in_rom(addr) && !in_sram(addr)) {
        return forced_read_remote(addr, size, usmode);
    }

    offset = addr & CACHELINEMASK;

    while (m) {
        if (m->addr == (~CACHELINEMASK & addr) && m->usmode == usmode) {
            break;
        }
        m = m->next;
    }
        
    /*
     * If we couldn't find the data we need in cache, then read a line
     * out of memory.
     */
    if (m == nil) {
        int i;
        u_char command;

        splhi();
        m = (struct mem_cache *)malloc(sizeof(struct mem_cache));
        init_checksum();
        m->addr = addr & ~CACHELINEMASK;
        m->usmode = usmode;
        traceremote("read_remote: filling cache line, addr=0x%x\n",
                              m->addr);
        if (varvalue("noparityinit")) {
            command = (usmode == M_USER) ? C_UPLOADNOINITUSER : C_UPLOADNOINIT;
        } else {
            command = (usmode == M_USER) ? C_UPLOADUSER : C_UPLOAD;
        }
        send_command(command);
        send_word(m->addr);
        send_word((u_long)CACHELINESIZE);
        send_command(C_END);
        send_checksum();

        init_checksum();
        switch (wait_for_token(C_DATASTART, C_DATAERROR, ta_warning, 1)) {
            case C_DATASTART:
                for (i = 0 ; i < CACHELINESIZE ; i++) {
                    m->values[i] = get_byte();
                }
                if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
                    free(m);
                    rerr("read_remote: Didn't find C_DATAEND.");
                } else {
                    verify_checksum();
                }
                m->next = mem_cache;
                mem_cache = m;
                break;

            case C_DATAERROR:
                remote_errno = true;
                traceremote("read_remote: dacc at 0x%x\n", addr);
                free((char *)m);
                m = nil;
                break;

            default:
                free((char *)m);
                m = nil;
                rerr("read_remote: Didn't find C_DATASTART.");
                break;
        }
        spllo();
    }

    /*
     * In the unlikely case that this access spans a cache-line
     * boundary, or if there was a problem reading the whole block,
     * just read it explicitly from the target.
     */
    if (m == nil) {
        return forced_read_remote(addr, size, usmode);
    }
    p = &m->values[offset];
    switch (size) {
        case 1: return *p;      
        case 2: return (*p << 8) | *(p + 1); 
        case 4: return  (*p << 24) | 
                        (*(p + 1) << 16) |
                        (*(p + 2) << 8) |
                        (*(p + 3));
        default:
          rerr("Case error in read_remote");
            /* NOTREACHED */
    }
}

/*
 * Read a byte from the remote's memory.
 */
u_char read_remote_b(addr, usmode)
    u_long addr;
{
  if (simulator) {
    u_char b;
    u_long pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    b = read_sim_b(pa);
    if (sim_errno) {
      dacc_rerr("reading byte from simulation memory");
    }
    return b;
  }
  
  return (u_char)read_remote(addr, 1, usmode);
}

/*
 * Read a half word from the remote's memory.
 */
u_short read_remote_h(addr, usmode)
  u_long addr;
{
  if (simulator) {
    u_short h;
    u_long pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    h = read_sim_h(pa);
    if (sim_errno) {
      dacc_rerr("reading short from simulation memory");
    }
    return h;
  }

  return (u_short)read_remote(addr, 2, usmode);
}

/*
 * Read a word from the remote's memory.
 */
u_long read_remote_w(addr, usmode)
  u_long addr;
{
  if (simulator) {
    u_long w, pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    w = read_sim_w(pa);
    if (sim_errno) {
      dacc_rerr("reading word from simulation memory");
    }
    return w;
  }

  return read_remote(addr, 4, usmode);
}

/*
 * This updates our memory cache.
 */
static void write_thru_mem_cache(addr, w, size, usmode)
    u_long addr;
    u_long w;
    int    size;
{
    struct mem_cache *m = mem_cache;
    u_long offset;

   if (simulator) return;

    while (m && (m->addr != (addr & ~CACHELINEMASK) || m->usmode != usmode)) {
        m = m->next;
    }
    if (m != nil) {
        offset = addr & CACHELINEMASK;
        switch (size) {
            case 1:               m->values[offset] = w; break;
            case 2:  *(u_short *)&m->values[offset] = w; break;
            case 4:  *(u_long  *)&m->values[offset] = w; break;
            default:
              rerr("Case error in write_thru_mem_cache()");
        }
    }
}

/*
 * Write a byte, short, or word into the remote's memory.
 */
static void write_remote(addr, w, size, usmode)
    u_long addr;
    u_long w;
    int    size;
{
    remote_errno = false;
    if (size != 1 && size != 2 && size != 4) {
        rerr("write_remote: bad size");
    }
    if (usmode != M_USER && usmode != M_SUPERVISOR) {
        usmode = (((remote_read_register(PSR_REGNUM) >> 31) & 1) == 0) ? M_USER : M_SUPERVISOR;
    }

    splhi();
    if (varvalue("traceremote")) {
        int a;

        ui_fprintf(stderr, "writing 0x%x to target at 0x%x ", w, addr);
        a = addr - mon_register_area_addr[selected_processor];
        /*
         * '66' below should really be a symbolic constant.  Anyway,
         * its the number of register slots in the monitor register
         * array. -rcb
         */
        if (0 <= a && a < (66 * 4)) {
            ui_fprintf(stderr, " (%s)\n", regname(a/4));
        } else {
            ui_fprintf(stderr, "\n");
        }
    }
    write_thru_mem_cache(addr, w, size, usmode);
    init_checksum();
    send_command((usmode == M_USER) ? C_DOWNLOADUSER : C_DOWNLOAD);
    send_word(addr);
    send_word((u_long)size);
    switch (size) {
        case 1: send_byte((u_char)w); break;
        case 2: send_short((u_short)w); break;
        case 4: send_word(w); break;
    }

    send_command(C_END);
    send_checksum();
    switch (wait_for_token(C_ACK, C_DATAERROR, ta_warning, 1)) {
        case C_ACK:
            break;

        case C_DATAERROR:
            remote_errno = true;
            sleep(1);
            flush_tty_line();
            dacc_rerr("writing target memroy");
            break;

        default:
            rerr("Didn't find C_ACK in write_remote");
    }
    spllo();
}

/*
 * Write a byte into the remote's memory.
 */
void write_remote_b(addr, w, usmode)
  u_long addr;
  u_char w;
{
  if (simulator) {
    u_long pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    write_sim_b(pa, w);
  } else {
    write_remote(addr, (u_long)w, 1, usmode);
  }
}

/*
 * Write a halfword into the remote's memory.
 */
void write_remote_h(addr, w, usmode)
  u_long addr;
  u_short w;
{
  if (simulator) {
    u_long pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    write_sim_h(pa, w);
  } else {
    write_remote(addr, (u_long)w, 2, usmode);
  }
}

/*
 * Write a word into the remote's memory.
 */
void write_remote_w(addr, w, usmode)
  u_long addr;
  u_long w;
{
  if (simulator) {
    u_long pa;
    if (sim_v_to_p(addr, &pa, usmode) != -1) {
      rerr("Error translating 0x%x in %s mode to a physical address\n",
                              addr, usmode ? "user" : "supervisor");
    }
    write_sim_w(pa, w);
  } else {
    write_remote(addr, w, 4, usmode);
  }
}

/*
 * This invalidates the cache that we keep of target memory.  Every time
 * the target executes, even for a single-step, we dump this cache.
 */
void invalidate_mem_cache()
{
    struct mem_cache *m1, *m = mem_cache;

    mem_cache = nil;
    while (m) {
        m1 = m->next;
        free((char *)m);
        m = m1;
    }
}

/*
 * Returns true if the passed address is in ROM, false otherwise
 */
static boolean in_rom(addr)
    u_long addr;
{
  if (varvalue("motomode")) {
    return (0xffc00000 <= addr && addr <= 0xffc7ffff);
  } else {
    return (0xfe000000 <= addr && addr <= 0xfe03ffff);
  }
}

/*
 * Returns true if the passed address is in static RAM, false otherwise
 */
static boolean in_sram(addr)
    u_long addr;
{
  if (varvalue("motomode")) {
    return (0xffe00000 <= addr && addr <= 0xffe1ffff);
  } else {
    return false;
  }
}

static char *reg_name[] = {
 "$r0",    "$r1",    "$r2",    "$r3",    "$r4",    "$r5",    "$r6",    "$r7", 
 "$r8",    "$r9",    "$r10",   "$r11",   "$r12",   "$r13",   "$r14",   "$r15", 
 "$r16",   "$r17",   "$r18",   "$r19",   "$r20",   "$r21",   "$r22",   "$r23", 
 "$r24",   "$r25",   "$r26",   "$r27",   "$r28",   "$r29",   "$r30",   "$r31", 

 "$ip",    "$pid",   "$psr",   "$epsr",  "$ssbr",  "$sxip",  "$snip",  "$sfip", 

 "$vbr",   "$dmt0",  "$dmd0",  "$dma0",  "$dmt1",  "$dmd1",  "$dma1", "$dmt2",
 "$dmd2",  "$dma2",  "$sr0",   "$sr1",   "$sr2",   "$sr3",   
 "$fpecr", "$fphs1", "$fpls1", "$fphs2", "$fpls2", "$fppt",  "$fprh",  "$fprl",
 "$fpit",  "$fpsr",  "$fpcr", 
 "$ceimr", "$comefrom", "$membrk", "$stackbase", "$ramsize"};

/*
 * Returns the register name for the passed register number.
 */
static 
char *regname(n)
    unsigned n;
{
    char *s;
    if (n < sizeof(reg_name)/sizeof(char *)) {
        s = reg_name[n];
        if (s == nil) {
            s = "invalid-register-number";
        }
    } else {
        s = "out-of-range-register-number";
    }
    return s;
}
