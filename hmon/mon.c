/*
 * $Header: /home/bigbird/Usr.U6/robertb/m88k/src/g88/hmon/RCS/mon.c,v 1.9 91/01/13 23:36:55 robertb Exp $
 * $Locker:  $
 *
	Copyright (c) 1987, 1988, 1989, 1990 by Tektronix, Inc.

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.
 *
 * This is the debug monitor for the Motorola 88000 CPU board.
 *
 * This code is reentrant to allow several CPU's to
 * share the same memory used by the debugger (in range 0x2000..0x7fff).
 * All variables are kept in a structure whose address is passed
 * from the assembly code that first gets control at initialization
 * and on exceptions.
 *
 * Contact Robert Bedichek at robertb@cs.washington.edu
 * if you have questions about this code.
 */

#include "mon.h"
#include "montraps.h"

#ifdef	lint
#define	volatile
#endif

/* Addresses of some of the registers of the 68692 UART that is on the
   188 console board. */

#define UART_STATUS_ADDR	(volatile unsigned char *)(0xfff82007)
#define	UART_CRA_ADDR		(volatile unsigned char *)(0xfff8200b)
#define	UART_DATA_ADDR		(volatile unsigned char *)(0xfff8200f)
#define	UART_ACR_ADDR		(volatile unsigned char *)(0xfff82013)

/* Writing this location will reset the whole machine. */

#define	GLBRES_ADDR		(volatile unsigned long *)(0xfff8700c)

/* If the top bit of this word is set, there is an abort interrupt 
   pending. */

#define	INT_STATUS_ADDR		(volatile unsigned long *)(0xfff84040)
#define	INT_ABORT_MASK		(0x80000000)

/* We write 0x4 to this address to clear the debug interrupt flip flop
   on the 188 board. */

#define	CLR_ABORT_ADDR		(volatile unsigned long *)(0xfff8408c)

extern void send_word();
extern void send_short();
extern void send_byte();
extern unsigned char get_byte();
extern unsigned short get_short();
extern unsigned long get_word();
extern unsigned char get_remote_char();
extern void send_command();
extern void download_cmd();
extern void upload_cmd();
extern void checksum_cmd();
extern void go_cmd();
extern void fill_cmd();
extern void retptrs_cmd();
extern void search_cmd();
extern void move_cmd();
extern void syscall188_cmd();
extern void init_checksum();
extern unsigned char fubyte();
extern void subyte();
extern void xrestart();

extern int report_comm_errors;
extern struct all_vars regs0, regs1, regs2, regs3;

enum get_flag {abort_if_int, keep_trying};

#define	VSTART	0x81
#define	VSTOP	0x82

enum parity_init { p_init, p_noinit };
enum usmode { m_user, m_supervisor };
#define	ABORT	0x81

struct all_vars *our_var_area();
extern void debug_monitor_entry_point();

struct compress {
  unsigned pattern;
  unsigned length;
};

struct compress compress_table[] = {
#include "compress.h"
};

#define	SIZEOF_COMPRESS_TABLE (sizeof(struct compress) * 256)

extern struct compress compress_table[];

#define	COMMAND_PREFIX1	(0x83)
#define	COMMAND_PREFIX2	(0x84)

/*
 * Commands that the host sends to the monitor.
 */
#define	C_DOWNLOAD		(0x91)
#define	C_UPLOAD		(0x92)
#define	C_CHECKSUM		(0x93)
#define	C_END			(0x94)
#define	C_GO			(0x95)
	
#define	C_DATASTART     	(0x96)
#define	C_CHECKSUMERR		(0x97)
#define	C_ACK			(0x98)
#define	C_DATAEND		(0x99)
#define	C_FILL			(0x9a)
#define	C_RETPTRS		(0x9b)
#define	C_EXCEPTION		(0x9c)
#define	C_DBWRITE		(0x9d)
#define	C_DBREAD		(0x9e)
#define C_SYNC			(0x9f)
#define C_DATAERROR		(0xa0)
#define C_UPLOADNOINIT		(0xa1)   
#define C_CHECKSUMNOINIT	(0xa2) 
#define C_SEARCH		(0xa3)
#define C_MOVE			(0xa4)
#define C_UPLOADUSER            (0xa5)
#define C_UPLOADNOINITUSER      (0xa6)
#define C_DOWNLOADUSER          (0xa7)
#define C_EXPECTEDTOKEN		(0xa8)
#define C_COMMERR               (0xa9)
#define C_COMPRESSERR           (0xaa)
#define C_BADCOMMAND            (0xab)
#define C_188SYSCALL            (0xac)

#define	L_BYTE		(1)
#define	L_SHORT		(2)
#define	L_WORD		(4)
#define	L_QUOTED1	(COMMAND_PREFIX1)
#define	L_QUOTED2	(COMMAND_PREFIX2)


#define	true	1
#define	false	0

#define PAGESIZE (4096)

#ifdef lint
struct all_vars regs0, regs1, regs2, regs3;
int report_comm_errors;
void lintfunction(arg)
{
    char *p = 0;

    (void)mon_dbread(p, (long)0);
    mon_dbwrite(p, (long)0);
    debug_monitor_entry_point();
    mon_init();
    lintfunction(arg);
}
#endif

/* Called when some kind of error occurs.  We keep track of the
   number of times this has happened and reset the machine if
   it has happened more than 10 times. */

void xrestart()
{
  register struct all_vars *p = our_var_area();

  if (p->dm_restart_count++ > 10) {
    *(volatile unsigned long *)GLBRES_ADDR = 0;
    for (;;);
  }
  restart();
}

/* This adds 'b' to the running checksum */

void add_to_checksum(p, b)
  register struct all_vars *p;
  register unsigned long b;
{
  register unsigned msb;
  register unsigned cs = p->dm_checksum;
  cs += b;
  msb = cs >> 31;
  cs <<= 1;
  cs |= msb;
  p->dm_checksum = cs;
}

/* Called to clean up after a DUART error.  We try to reset the
   port.  If this fixes the problem, we tell the host, otherwise
   we reset the machine. */

static void handle_duart_error(p, status)
  register struct all_vars *p;
  register unsigned char status;
{
  unsigned char s;

  *UART_CRA_ADDR = 0x40;
  s = *UART_STATUS_ADDR;
  if (s & 0xf0) {
    *GLBRES_ADDR = 0;
    for (;;);
  }
  if (report_comm_errors) {
    send_command(p, C_COMMERR);
    send_byte(p, status);
  }
}

/* This checks to see if there is a stop character in our receive buffer. 
   If so, we wait for another character from the host. */

void check_for_stop(p)
  register struct all_vars *p;
{
  register unsigned char c, status;

  status = *UART_STATUS_ADDR & 0xf1;
  if (status & 0xf0) {
    handle_duart_error(p, status);
  }
  if (status != 1) {
    return;
  }
  c = *UART_DATA_ADDR;
  /* If we get just a VSTART, assume that the corresponding VSTOP was
     thrown away by get_remote_char(). */
  if (c == VSTART) {
    return;
  }
  if (c == VSTOP) {
    do {
      status = *UART_STATUS_ADDR & 0xf1;
      if (status & 0xf0) {
        handle_duart_error(p, status);
      }
    } while (status != 1);
    c = *UART_DATA_ADDR;
    if (c == VSTART) {
      return;
    }
  }
  handle_duart_error(p, status);
}

/* This returns after some delay.  We do this so that SUN-OS doesn't
   think we are a bad terminal. We reproduce the loop that is used
   to generate the values passed to us to get approximately the same
   delay. */
delay(character_time)
  unsigned long character_time;
{
  do {
    unsigned char status = *UART_STATUS_ADDR & 0xf4;
    if (status & 0xf0) {
      fool_compiler();
    }
  } while (character_time-- > 0);
}

fool_compiler() {};

/*
 * This writes the passed byte to the comm line.
 */
void put_remote_char(p, c)
  register struct all_vars *p;
  register unsigned char c;
{
  register unsigned char  status;
  register long cnt = 0;

  check_for_stop(p);
  c &= 0xff;
  do {
    status = *UART_STATUS_ADDR & 0xf4;
    if (status & 0xf0) {
      handle_duart_error(p, status);
    }
    cnt++;
  } while (status != 4);
  p->dm_character_time = cnt;
  if (p->dm_checksumming) {
    add_to_checksum(p, (unsigned long)c);
  }
  *UART_DATA_ADDR = c;
}

/*
 * Sends the passed command to the other agent.
 */
void send_command(p, command)
    register struct all_vars *p;
    register unsigned char command;
{
    put_remote_char(p, COMMAND_PREFIX1);
    put_remote_char(p, command);
}

/* This returns a raw character from the comm line.  If checksumming
   is turned on, we add the received character to the running check
   sum total.  We can't call 188BUG to do this for us because some
   values that sees in the input register of the DUART will cause
   its command interpreter to be entered.  We ignore start/stop characters
   from the host */

unsigned char get_remote_char(p, flag)
  register struct all_vars *p;
  register enum get_flag flag;
{
  register unsigned char c, status;

  do {
    do {
      status = *UART_STATUS_ADDR & 0xf1;
      if (status & 0xf0) {
        handle_duart_error(p, status);
      }
      if (flag == abort_if_int && (*INT_STATUS_ADDR & INT_ABORT_MASK)) {
        return ABORT;
      }
    } while (status != 1);
    c = *UART_DATA_ADDR;
  } while (c == VSTART || c == VSTOP);
  if (p->dm_checksumming) {
    add_to_checksum(p, (unsigned long)c);
  }
  return c;
}


/* In the comments below, "the other agent" is the host.  */

/* This initializes the checksum */

void init_checksum(p)
  register struct all_vars *p;
{
  p->dm_checksum = 0;
  p->dm_checksumming = 1;
}

/* Sends the checksum to the other agent.  */

void send_checksum(p)
  register struct all_vars *p;
{
  p->dm_checksumming = 0;
  send_word(p, p->dm_checksum);
}

/* This sends the passed word to the other agent.  It does the
   data compression.  */

void send_word(p, word)
  register struct all_vars *p;
  register unsigned long word;
{
  register struct compress *ct = &compress_table[0];

  while (ct->length == 4 || ct->length == 0) {
    if (ct->pattern == word && ct->length == 4) {
      put_remote_char(p, (unsigned char)(ct - &compress_table[0]));
      return;
    }
    ct++;
  }
  send_short(p, (unsigned short)(word >> 16));
  send_short(p, (unsigned short)word);
}

/*
 * This sends a short to the other agent.
 */
void send_short(p, s)
  register struct all_vars *p;
  register unsigned short s;
{
  register struct compress *ct = &compress_table[SHORT_OFFSET];

  while (ct->length == 2 || ct->length == 0) {
    if (ct->pattern == s && ct->length == 2) {
      put_remote_char(p, (unsigned char)(ct - &compress_table[0]));
      return;
    }
    ct++;
  }
  send_byte(p, (unsigned char)(s >> 8));
  send_byte(p, (unsigned char)s);
}

/*
 * This sends a byte to the other agent.
 */
void send_byte(p, byte)
  register struct all_vars *p;
  register unsigned char byte;
{
  register struct compress *ct = &compress_table[BYTE_OFFSET];
  while (ct < &compress_table[256]) {
    if (ct->pattern == byte && ct->length == 1) {
      put_remote_char(p, (unsigned char)(ct - &compress_table[0]));
      return;
    }
    ct++;
  }
  /* We couldn't compress the byte.  Send it using the two byte
     sequence.  */

  if (byte & 0x80) {
    put_remote_char(p, COMMAND_PREFIX2);
  } else {
    put_remote_char(p, COMMAND_PREFIX1);
  }
  put_remote_char(p, byte & 0x7f);
}

/* This returns an unsigned long word from the comm port (from other agent).  */

unsigned long get_word(p)
  register struct all_vars *p;
{
  register unsigned long t = 0;
  register unsigned long i;

  for (i = 0 ; i < 4 ; i++) {
    t <<= 8;
    t |= get_byte(p, keep_trying);
  }
  return t;
}

/* This returns an unsigned short from the comm port (from other agent).  */

unsigned short get_short(p)
  register struct all_vars *p;
{
  register unsigned long w = get_byte(p, keep_trying);

  return (w << 8) | get_byte(p, keep_trying);
}

/* This returns a decode byte from the comm port (from other agent).  */

unsigned char get_byte(p, flag)
  register struct all_vars *p;
  enum get_flag flag;
{
  register unsigned char c;
  register struct compress *ct;

  if (p->dm_partial_length) {
    p->dm_partial_length--;
    c = (p->dm_partial >> 24) & 0xff;
    p->dm_partial <<= 8;
    return c;
  }

  c = get_remote_char(p, flag);
  /* If an abort interrupt happened while we were waiting, return 
     immediately. */
  if (c == ABORT) {
    return ABORT;
  }
  ct = &compress_table[c];
  switch (ct->length) {
    case L_QUOTED1:
      c = get_remote_char(p, flag);
      return c;

    case L_QUOTED2:
      c = get_remote_char(p, flag);
      return c + 0x80;

    case L_BYTE:
      return ct->pattern;

    case L_SHORT:
      p->dm_partial_length = 1;
      p->dm_partial = ct->pattern << 24;
      return (ct->pattern >> 8) & 0xff;

    case L_WORD:
      p->dm_partial_length = 3;
      p->dm_partial = ct->pattern << 8;
      return (ct->pattern >> 24) & 0xff;

    default:
      send_command(p, C_COMPRESSERR);
      xrestart();
      /*NOTREACHED*/
  }
}

/* This reads the checksum from the comm line and compares it
   with the value in the global 'checksum'.  */

void verify_checksum(p)
  register struct all_vars *p;
{
  register unsigned long received_checksum;

  p->dm_checksumming = 0;
  received_checksum = get_word(p);
  if (p->dm_checksum != received_checksum) {
    send_command(p, C_CHECKSUMERR);
    send_word(p, p->dm_checksum);
    send_word(p, received_checksum);
    xrestart();
  }
}

/* This looks for the passed command from the host.  */

void wait_for_token(p, token)
    register struct all_vars *p;
    register unsigned char token;
{
    register unsigned char c;

    if ((c = get_remote_char(p, keep_trying)) != COMMAND_PREFIX1) {
        send_command(p, C_EXPECTEDTOKEN);
        send_byte(p, COMMAND_PREFIX1);
        send_byte(p, c);
        xrestart();
    }

    if ((c = get_remote_char(p, keep_trying)) != token) {
        send_command(p, C_EXPECTEDTOKEN);
        send_byte(p, token);
        send_byte(p, c);
        xrestart();
    }
}

/*
 * Invalidate each I-cache that is installed.
 * Copyback-flush each D-cache that is installed. 
 * Its OK to invalidate CMMU's that are not turned on.
 * We load the system status register of each data CMMU after copyback
 * to make sure the copyback is done before we leave this routine and
 * start executing user code.  This should solve a cache-coherency problem
 * seen on 8 CMMU systems.  

 From Andrew Klossner:

  Here's the story on finding all the CMMUs, in order to give them
  invalidate and/or flush commands.

  The WHOAMI register is a 32-bit word located at address 0xfff88018.
  Within this word, bits 7:4 contain the Hypermodule configuration code,
  which is one of these:
  
	  0	4 CPU, 8 CMMU
	  1	2 CPU, 8 CMMU
	  2	1 CPU, 8 CMMU
	  5	2 CPU, 4 CMMU
	  6	1 CPU, 4 CMMU
	  A	1 CPU, 2 CMMU
  
  This gives you the number of CMMUs.  The CMMU base addresses are:
  
	  unsigned long *dcmmu188[4] = {
		  (unsigned long *)0xFFF6F000,
		  (unsigned long *)0xFFF5F000,
		  (unsigned long *)0xFFF3F000,
		  (unsigned long *)0xFFF7F000
	  };
	  unsigned long *ccmmu188[4] = {
		  (unsigned long *)0xFFF7E000,
		  (unsigned long *)0xFFF7D000,
		  (unsigned long *)0xFFF7B000,
		  (unsigned long *)0xFFF77000
	  };
  
  So, if you have 8 CMMUs, there's one at each of these addresses; if you
  have 4 CMMUs, only dcmmu188[0], dcmmu188[1], ccmmu188[0], and
  ccmmu188[1] are valid; if you have 2 CMMUs, only dcmmu188[0] and
  ccmmu188[0] are valid.
  
  I don't know whether you can write to a non-existent CMMU without
  taking a DACC.  We don't have a Hypermodule board with fewer than 8
  CMMUs, so we have no way of experimenting.  I recommend that your code
  not try to write to non-existent CMMUs.
  
  Bits 3:0 of the WHOAMI register contain the ordinal number (0 through
  3) of the CMMU through which the register was fetched.  This can be
  used to tell which CPU you are running on, by figuring out which CPU
  this CMMU is assigned to.  This depends on the configuration:
  
	  config 0:  CPU 0 owns DCMMU 0; CPU 1 owns DCMMU 1;
		     CPU 2 owns DCMMU 2; CPU 3 owns DCMMU 3.
	  config 1:  CPU 0 owns DCMMUs 0 and 1; CPU 1 owns DCMMUs 2 and 3.
	  config 2:  CPU 0 owns all DCMMUs.
	  config 5:  CPU 0 owns DCMMU 0; CPU 1 owns DCMMU 1.
	  config 6:  CPU 0 owns both DCMMUs.
	  config A:  CPU 0 owns the DCMMU.
  
 */

unsigned long *dcmmu188[4] = {
  (unsigned long *)0xFFF6F000,
  (unsigned long *)0xFFF5F000,
  (unsigned long *)0xFFF3F000,
  (unsigned long *)0xFFF7F000
 };

unsigned long *ccmmu188[4] = {
  (unsigned long *)0xFFF7E000,
  (unsigned long *)0xFFF7D000,
  (unsigned long *)0xFFF7B000,
  (unsigned long *)0xFFF77000
};
 
/*
struct hyper_config {
  char h_cpus, h_cmmus;
} hyper_config[16] = { 
  {4, 8}, {2, 8}, {1, 8}, {0, 0}, {0, 0}, {2, 4}, {1, 4}, {0, 0},
  {0, 0}, {0, 0}, {1, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
 */
             
/* Returns the CPU configuration code from the WHOAMI register. */
 
/*
static int
whoami()
{
  return (int)((*(volatile unsigned long *)0xfff88018) >> 4) & 0xf;
}
*/

static unsigned long
mycpunumber()
{
  switch ((int)(*(volatile unsigned long *)0xfff88018) & 0xf) {
    case 1: return 0;
    case 2: return 1;
    case 4: return 2;
    case 8: return 3;
  }
  return 0xffffffff;
}

/* Copies back the data caches and invalidates the code caches in
   preparation for executing "user" code */

void flush_my_cache()
{
  register volatile int ssr;
  register unsigned long cpu = mycpunumber();

  *(ccmmu188[cpu] + 1) = 0x17;   /* Invalidate */
  *(dcmmu188[cpu] + 1) = 0x1b;   /* Copyback */
  ssr = *(dcmmu188[cpu] + 2);    /* Load system status register to sync */

#ifdef	lint
  lintfunction(ssr);
#endif
}

/* Turn on the code cache.  We do this so that the monitor runs fast enough
   that it doesn't get over runs at 38.4 K baud */

void enable_my_code_cache()
{
  register unsigned long mycpu = mycpunumber();
  /* Force the CI bit in the SAPR to be off */
  if (mycpu < 4) {
    *(ccmmu188[mycpu] + 0x200/4) &= ~0x40;
  }
}

/* This is called by processors that did not get the host-communication-mutex.
   We don't try to talk to the host.  Instead we watch our run flag and
   return if it is non-zero.  We also watch for uncaught abort interrupts. 
   If we decide to take over communication with the host after an
   abort interrupt has gone unrecognized for a while, we send call
   talkk_to_host() and return a 0.  If we are told to start running 
   "user" code we return 1.  Otherwise we don't return. */

int wait_quietly(p)
  struct all_vars *p;
{
  register unsigned long *mb = p->dm_memory_breakpoint;
  register unsigned long mc = *mb;

  /* Let the host know that we are in the monitor now */
  p->dm_in_mon = IN_MON_MAGIC;

  while (p->dm_in_mon == IN_MON_MAGIC) {
    register int i;
    if (p->dm_memory_breakpoint != mb) {
      mb = p->dm_memory_breakpoint;
      mc = *mb;
    }

    /* Delay for a while so that we don't saturate the bus while
       we wait. If a memory breakpoint is set, spin watching
       the memory breakpoint location. */
    for (i = 0 ; i < 10000 ; i++) {
      if (mb && *mb != mc) {

        /* Perhaps the reason that we appear to have reached a memory
           breakpoint is that the user has changed the breakpoint
           address register.  If so, reset our memory breakpoint variables
           and don't interrupt the other processors. */

        if (p->dm_memory_breakpoint != mb) {
          mb = p->dm_memory_breakpoint;
          mc = *mb;
          break;
        }
        /* Generate an interrupt here. */
        break;
      }
    }

    /* Check to see if there is an abort interrupt pending.  If
       no one clears it for a long time, try acquiring the 
       host communication lock.  If everybody else is dead,
       we might as well respond and let the user examine
       memory. */

    i = 100000;
    while ((*INT_STATUS_ADDR & INT_ABORT_MASK) && --i > 0) ;
    if (i == 0) {
      /* Reset the abort interrupt. */
      *CLR_ABORT_ADDR = 4;

      if (acquire_host_comm_mutex()) {
        p->dm_exception_code = TR_INT;
        talk_to_host(p);
        return 0;
      }
    }
  }
  flush_my_cache();
  return 1;
}

/* Initialize the variables used in communication with the host and send an
   exception packet up the hose with our exception number and processor
   number. */
talk_to_host(p)
  struct all_vars *p;
{
  p->dm_partial = 0;
  p->dm_partial_length = 0;
  p->dm_checksumming = 0;
  p->dm_checksum = 0;

  if (p->dm_in_mon == IN_MON_MAGIC && p->dm_exception_code == TR_DACC) {
    send_command(p, C_DATAERROR);
  } else {
    p->dm_in_mon = IN_MON_MAGIC;
    send_command(p, C_EXCEPTION);
    send_word(p, mycpunumber());
    send_word(p, p->dm_exception_code);
  }
}

/* This is the monitor's entry point.  */
void
debug_monitor_entry_point()
{
  register unsigned char c;
  register struct all_vars *p = our_var_area();

  /* If we are able to acqure the lock, then we are the CPU that will
     talk to the host for now. The others will wait quietly on their
     'p->dm_in_mon' flag. */

  enable_my_code_cache();
  if (acquire_host_comm_mutex()) {
    if (p->dm_call_on_mon_entry) {
      (*p->dm_call_on_mon_entry)();
    }
    talk_to_host(p);
  } else {
    if (wait_quietly(p)) {
      return;
    }
  }    

  /* This is the main loop for the g88 monitor.  We execute this
     loop continuously while we are idle, looking for a COMMAND_PREFIX
     character.  */
  for (;;) {
    init_checksum(p);
    c = get_remote_char(p, keep_trying);
    if (c == COMMAND_PREFIX1) {
      c = get_remote_char(p, keep_trying);
      switch (c) {
        case C_DOWNLOAD: download_cmd(p, m_supervisor);         break;
        case C_UPLOAD:   upload_cmd(p, p_init, m_supervisor);   break;
        case C_CHECKSUM: checksum_cmd(p, p_init);               break;
        case C_GO:       
          go_cmd(p); 
          if (p->dm_in_mon == 0) {
            return;
          }
          if (wait_quietly(p)) {
            return;
          }
          break;

        case C_FILL:     fill_cmd(p);                           break;
        case C_RETPTRS:  retptrs_cmd(p);                        break;
        case C_SYNC:     sync_cmd(p);                           break;
        case C_UPLOADNOINIT:   
                         upload_cmd(p, p_noinit, m_supervisor); break;
        case C_CHECKSUMNOINIT: 
                         checksum_cmd(p, p_noinit);             break;
        case C_SEARCH:   search_cmd(p);                         break;
        case C_MOVE:     move_cmd(p);                           break;
        case C_UPLOADNOINITUSER: 
                         upload_cmd(p, p_noinit, m_user);       break;
        case C_UPLOADUSER: 
                         upload_cmd(p, p_init, m_user);         break;
        case C_188SYSCALL:
				 syscall188_cmd(p);                     break;

        default: send_command(p, C_BADCOMMAND);
                 send_byte(p, c);                               break;
      }
    }
  }
}

/*
 * This is the download command.  It has the form:
 *
 *   DOWNLOAD <address> <length> <data> <data> ... END        <checksum>
 *   0x82 0x91  ...       ...     ..     ..    ... 0x82 0x94    ...
 *
 * When we enter this function we have already received the DOWNLOAD
 * command sequence (p, 0x82, 0x91).
 */
void
download_cmd(p, usmode)
    register enum usmode usmode;
    register struct all_vars *p;
{
    register unsigned long addr, length;
    
    addr = get_word(p);
    length = get_word(p);
    /*
     * If the address is word aligned we read a word at a time
     * from the serial line.  And we store into memory a word
     * at a time.  This is important when dealing with the CMMU
     * registers, which have to be accessed as words and not
     * as bytes.  It also makes this run faster.
     */
    if ((addr & 3) == 0 && usmode != m_user) {
        for ( ; length >= 4 ; length -= 4) {
            *(unsigned long *)addr = get_word(p);
            addr += 4;
        }
    }
    if ((addr & 1) == 0 && usmode != m_user) {
        for ( ; length >= 2 ; length -= 2) {
            *(unsigned short *)addr = get_short(p);
            addr += 2;
        }
    }
    for ( ; length != 0 ; length--) {
        if (usmode == m_user) {
            subyte(addr, get_byte(p, keep_trying));
        } else {
            *(unsigned char *)addr = get_byte(p, keep_trying);
        }
        addr++;
    }
    wait_for_token(p, C_END);
    verify_checksum(p);
    send_command(p, C_ACK);
}

/*
 * This sends data from the target to the host.
 */
void
upload_cmd(p, parity_init, usmode)
    register struct all_vars *p;
    register enum parity_init parity_init;
    register enum usmode usmode;
{
    register unsigned long addr;
    register long length;

#ifdef	lint
    lintfunction((int)parity_init);
#endif

    addr = get_word(p);
    length = (long)get_word(p);
    wait_for_token(p, C_END);
    verify_checksum(p);

    init_checksum(p);
    send_command(p, C_DATASTART);
    /*
     * If the address is word aligned then we fetch and send
     * a word at a time.
     */
    if (usmode == m_supervisor) {
        if ((addr & 3) == 0) {
            for ( ; length >= 4 ; length -= 4) {
                send_word(p, *(unsigned long *)addr);
                addr += 4;
            }
        }
        if ((addr & 1) == 0) {
            for ( ; length >= 2 ; length -= 2) {
                send_short(p, *(unsigned short *)addr);
                addr += 2;
            }
        }
    }
    for ( ; length > 0 ; length--) {
        if (usmode == m_user) {
            send_byte(p, fubyte(addr));
        } else {
            send_byte(p, *(unsigned char *)addr);
        }
        addr++;
    }
    send_command(p, C_DATAEND);
    send_checksum(p);
}

/*
 * This sends a checksum up to the host for every 4k block, starting
 * at 'addr' for 'length' bytes.
 */
void checksum_cmd(p, parity_init)
    register struct all_vars *p;
    register enum parity_init parity_init;
{
    register unsigned long addr, count;
    register long length;
    register checksums;		/* To provide delays for SUN-OS */

    addr = get_word(p);
    length = (long)get_word(p);
    wait_for_token(p, C_END);
    verify_checksum(p);

    send_command(p, C_DATASTART);
    init_checksum(p);
    count = 0;
    checksums = 0;

    if ((addr & 3) == 0) {
        for ( ; length >= 4 ; length -= 4) {
            add_to_checksum(p, *(unsigned long *)addr);
            count += 4;
            if (count == 4096) {
                count = 0;
                send_checksum(p);
                if (checksums++ % 40 == 0) {
                  delay(p->dm_character_time * 400);
                }
                init_checksum(p);
            }
            addr += 4;
        }
    }
    for ( ; length > 0 ; length--) {
        add_to_checksum(p, (unsigned long)(*(unsigned char *)addr++));
        if (++count == 4096) {
            count = 0;
            send_checksum(p);
            init_checksum(p);
        }
    }
    if (count) {
        send_checksum(p);
    }
    send_command(p, C_DATAEND);
}

/*
 * This loads up the registers with the contents of the register save
 * area and does an rte.
 */
void go_cmd(p)
    register struct all_vars *p;
{
  char buf[4];
  int i;

  /* The host sends us 4 bytes, one for each processor.  If the byte
     for a processor is non-zero, it means that we should let it go. */
  for (i = 0 ; i < sizeof buf ; i++) {
    buf[i] = get_byte(p, keep_trying);
  }
  wait_for_token(p, C_END);
  verify_checksum(p);
  flush_my_cache();
  send_command(p, C_ACK);
  release_host_comm_mutex(0); 
  if (p->dm_call_on_mon_exit) {
    (*p->dm_call_on_mon_exit)();
  }
  if (buf[0]) {
    regs0.dm_in_mon = 0;
  }
  if (buf[1]) {
    regs1.dm_in_mon = 0;
  }
  if (buf[2]) {
    regs2.dm_in_mon = 0;
  }
  if (buf[3]) {
    regs3.dm_in_mon = 0;
  }
}

/*
 * This fills an area of memory starting at 'addr' for 'length' bytes
 * with zeros.
 */
void fill_cmd(p)
    register struct all_vars *p;
{
    register unsigned long addr, length;

    addr = get_word(p);
    length = get_word(p);
    wait_for_token(p, C_END);
    verify_checksum(p);
    if ((addr & 3) == 0) {
        for ( ; length >= 4 ; length -= 4) {
            *(unsigned long *)addr = 0;
            addr += 4;
        }
    }

    for ( ; length != 0 ; length--) {
        *(unsigned char *)addr++ = 0;
    }
    send_command(p, C_ACK);
}

/*
 * This sends back key pointers for the debugger to use in future commands.
 */
void retptrs_cmd(p)
    register struct all_vars *p;
{
    extern single_step_trap();
    extern char *date_string;
    extern char *time_string;

    wait_for_token(p, C_END);
    verify_checksum(p);

    init_checksum(p);
    send_command(p, C_DATASTART);
    send_word(p, (unsigned long)&regs0);
    send_word(p, (unsigned long)&regs1);
    send_word(p, (unsigned long)&regs2);
    send_word(p, (unsigned long)&regs3);
    send_word(p, (unsigned long)single_step_trap);
    send_word(p, (unsigned long)&date_string[0]);
    send_word(p, (unsigned long)&time_string[0]);
    send_word(p, (unsigned long)&report_comm_errors);
    send_command(p, C_DATAEND);
    send_checksum(p);
}

/*
 * Just echo the SYNC character.
 */
sync_cmd(p)
    register struct all_vars *p;
{
    p->dm_partial = 0;
    p->dm_partial_length = 0;
    send_command(p, C_SYNC);
}

/*
 * Send a packet with the passed word.
 */
void send_word_in_packet(p, w)
    register struct all_vars *p;
    register unsigned long w;
{
    init_checksum(p);
    send_command(p, C_DATASTART);
    send_word(p, w);
    send_command(p, C_DATAEND);
    send_checksum(p);
}

/*
 * This is the search command.  It has the form:
 *
 *   C_SEARCH <address> <length> <stride> <pattern1> <pattern2> 
 *            <mask1> <mask2> <parity-flag> C_END <checksum>
 *
 *  We search starting at <address>. We step <length> times,
 *  by <stride> bytes after each comparison.  Each comparison is
 *  eight bytes, and is done with <pattern1> <pattern2> and contents
 *  of memory after anding with <mask1> <mask2>
 *
 *  The first match terminates the search.  The address of the match
 *  is returned.  If no match is found, -1 is returned.
 */
void
search_cmd(p)
    register struct all_vars *p;
{
    register unsigned long addr, pattern1, pattern2;
    register unsigned long mask1, mask2, parity_flag;
    register long length, stride;
    
    addr = get_word(p);
    length = (long)get_word(p);
    stride = (long)get_word(p);
    pattern1 = get_word(p);
    pattern2 = get_word(p);
    mask1 = get_word(p);
    mask2 = get_word(p);
    parity_flag = get_word(p);
    wait_for_token(p, C_END);
    verify_checksum(p);

    /*
     * Make sure that we don't go into an endless loop by making sure
     * that the stride is positive.
     */
    if (stride <= 0) {
        stride = 1;
    }

    /*
     * If the address is word-aligned, and will stay word-aligned,
     * do the search by words, it's faster.
     */
    if ((addr & 3) == 0 && (stride & 3) == 0) {
        while ((length -= stride) >= 0) {
            if (pattern1 == (*(unsigned long *)addr & mask1) &&
                pattern2 == (*(unsigned long *)(addr+4) & mask2)) {
                send_word_in_packet(p, addr);
                return;
            }
            addr += stride;
        }
    }  else {
        while ((length -= stride) >= 0) {
            unsigned long word1 = (*(unsigned char *)addr << 24)       |
                                  (*(unsigned char *)(addr + 1) << 16) |
                                  (*(unsigned char *)(addr + 2) << 8)  |
                                   *(unsigned char *)(addr + 3);

            unsigned long word2 = (*(unsigned char *)(addr + 4) << 24) |
                                  (*(unsigned char *)(addr + 5) << 16) |
                                  (*(unsigned char *)(addr + 6) << 8)  |
                                   *(unsigned char *)(addr + 7);


            if (pattern1 == (word1 & mask1) &&
                pattern2 == (word2 & mask2)) {
                send_word_in_packet(p, addr);
                return;
            }
            addr += stride;
        }
    }
    send_word_in_packet(p, (unsigned long)-1);
}

/*
 * This is the move command.  It has the form:
 *
 *   C_MOVE <source address> <destination address> <length> 
 *                         <parity-flag> C_END <checksum>
 *  We move the block of memory starting at <source address> and
 *  extending for <length> bytes to memory starting at <destination address>
 */
void
move_cmd(p)
    register struct all_vars *p;
{
    register unsigned long src_addr, dst_addr;
    register unsigned long parity_flag;
    register long length;
    
    src_addr = get_word(p);
    dst_addr = get_word(p);
    length = (long)get_word(p);
    parity_flag = get_word(p);
    wait_for_token(p, C_END);
    verify_checksum(p);

    for ( ; length > 0 ; length--) {
        *(unsigned char *)dst_addr = *(unsigned char *)src_addr;
        src_addr++;
        dst_addr++;
    }
    send_command(p, C_ACK);
}

/*
 * Call the 188 BUG.
 *   C_188SYSCALL <code> <arg1> <arg2> <arg3> <arg4> C_END <checksum>
 *
 * We reply with C_ACK right away.  
 * If the call to 188BUG returns, we send C_DATASTART <return_val> C_DATAEND
 * I don't know what the maximum number of arguments that a 188BUG 
 * syscall takes, I guess that 4 is enough.
 */
void
syscall188_cmd(p)
    register struct all_vars *p;
{
    register unsigned long code = get_word(p);
    register unsigned long arg1 = get_word(p);
    register unsigned long arg2 = get_word(p);
    register unsigned long arg3 = get_word(p);
    register unsigned long arg4 = get_word(p);
    register unsigned long return_val;

    wait_for_token(p, C_END);
    verify_checksum(p);

    send_command(p, C_ACK);
    return_val = syscall(code, arg1, arg2, arg3, arg4);
    init_checksum(p);
    send_command(p, C_DATASTART);
    send_word(p, return_val);
    send_command(p, C_DATAEND);
    send_checksum(p);
}

/* This is called when the kernel wants to read characters from
 * the remote console.  */
mon_dbread(buffer, length)
    register char *buffer;
    register long length;
{
  register int i, actual_length;
  struct all_vars *p = our_var_area();

  while (!acquire_host_comm_mutex());
  init_checksum(p);
  send_command(p, C_DBREAD);
  send_word(p, (unsigned long)length);
  send_checksum(p);

  init_checksum(p);
  wait_for_token(p, C_DATASTART);
  actual_length = get_word(p);
  for (i = 0 ; i < actual_length ; i++) {
    unsigned char c = get_byte(p, abort_if_int);
    if (c == 0xff) {
      release_host_comm_mutex(1);
      return i;
    }
    *buffer++ = c;
  }
  wait_for_token(p, C_DATAEND);
  verify_checksum(p);
  release_host_comm_mutex(1);
  return actual_length;
}

#define MAXPACKETSIZE	20
/*
 * This is called when the kernel wants to write characters to
 * the remote console.
 */
mon_dbwrite(buffer, length)
    register char *buffer;
    register long length;
{
  register struct all_vars *p = our_var_area();
  register cnt = length;

  while (!acquire_host_comm_mutex());
  init_checksum(p);
  send_command(p, C_DBWRITE);
  send_word(p, (unsigned long)length);
  send_command(p, C_DATASTART);
  while (length-- > 0) {
    send_byte(p, (unsigned char)(*buffer++));
  }
  send_command(p, C_DATAEND);
  send_checksum(p);
  delay(p->dm_character_time * cnt * 50);
  release_host_comm_mutex(1);
}

char *date_string = __DATE__;
char *time_string = __TIME__;
