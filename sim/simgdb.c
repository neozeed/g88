/*
   Download the simulator.
   (c) 1990 Free Software Foundation, Inc.
  
   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/simgdb.c,v 1.10 91/01/13 23:59:46 robertb Exp $
 */

#include <stdio.h>
#include "m-m88k.h"
#undef BREAKPOINT
#undef QUIT
#include "../defs.h"
#undef BREAKPOINT
#undef QUIT
#include "sim.h"

/* Maximum number of processors.  */
#define MAX_PROCESSORS  4

extern u_char cpu_enabled[MAX_PROCESSORS];

/* extern u_short *mask_ptr;	/* CE's interrupt mask register. */
extern long comefrom;		/* debug come-from register */
extern long membrk;		/* memory breakpoint register */

/* Number of the currently-selected processor */
extern int selected_processor;

/*
 * Reset the registers and control registers to their initial values, and
 * call all the device simulator init routines, but don't change memory.
 * This is only called when the 'init' command is given.
 */
void do_sim_reset()
{
    int i;

    for (i = 0 ; i < 32 ; i++) {
        regs[i] = 0;
    }

    for (i = 0 ; i < 64 ; i++) {
        sfu0_regs[i] = 0;
        sfu1_regs[i] = 0;
    }
    setreg_sim(PID_REGNUM, 0x000000fe);
    setreg_sim(PSR_REGNUM, 0x800003ff);
    setreg_sim(SNIP_REGNUM, 0);
    setreg_sim(SFIP_REGNUM, 0);

    delayed_p = 0;
    delayed_ip = 0;
    io_init();
    sim_flush_all_translations();

    sim_exception = E_NONE;
    selected_processor = 0;
    set_either_prompt_command(0, "[0] (gdb)");
}

/*
 * Fetches a simulator register.
 */
u_long reg_sim(n)
	int n;
{
    if (n <= SP_REGNUM) {
        return regs[n];
    }
    if (n == SYNTH_PC_REGNUM) {
      return ip;
    }
    if (n <= SR3_REGNUM) {
        return sfu0_regs[n-PID_REGNUM];
    }
    if (n <= FPCR_REGNUM) {
        return sfu1_regs[n-FPECR_REGNUM];
    }
    /*
    if (n == CEIMR_REGNUM) {
        return *mask_ptr;
    }
    */
    if (n == COMEFROM_REGNUM) {
        return comefrom;
    }
    if (n == MEMBRK_REGNUM) {
        return membrk;
    }
    if (n == STACKBASE_REGNUM) {
        return sfu1_regs[9];
    }
    if (n == RAMSIZE_REGNUM) {
        return sfu1_regs[10];
    }
    sim_printf("reg(0x%x) out of range.\n", n);
    return 0;
}

/*
 * Set a simualtor register.
 */
setreg_sim(n, w)
  int n;
  u_long w;
{
	if (n == 0 && w != 0) {
		sim_printf("Assignment of a non-zero value to r0 not allowed.\n");
		return;
	}
  if (n <= SP_REGNUM) {
    regs[n] = w;
    return;
  }
  if (n == SYNTH_PC_REGNUM) {
    ip = w;
    return;
  }
  if (n <= SR3_REGNUM) {
    sfu0_regs[n-PID_REGNUM] = w;
    return;
  }
  if (n <= FPCR_REGNUM) {
    sfu1_regs[n-FPECR_REGNUM] = w;
    return;
  }
  /*
  if (n == CEIMR_REGNUM) {
    *mask_ptr = w;
    return;
  }
  */
  if (n == COMEFROM_REGNUM) {
    comefrom = w;	/* why would you want to do this? */
    return;
  }
  if (n == MEMBRK_REGNUM) {
    membrk = w;
    return;
  }
  if (n == STACKBASE_REGNUM) {
    sfu1_regs[9] = w;
    return;
  }
  /* Can't set $RAMSIZE */
  sim_printf("setreg_sim(0x%x, 0x%x) out of range.\n", n, w);
}

#include "../ui.h"

sim_printf(fmt, a, b, c, d, e, f, g, h, i, j, k, l, m)
  char *fmt;
{
  ui_fprintf(stdout, fmt, a, b, c, d, e, f, g, h, i, j, k, l, m);
}

/* Called by the trivial console simulator to get a character from
   the user. */
char sim_consgetchar()
{
  return getchar();
}

/* Called by the trivial console simulator to write a character to
   the screen. */
sim_consputchar(c)
{
  ui_putc(c, stdout);
  ui_fflush(stdout);
}

/* Copy a block of memory in simulation memory. */
void sim_copymem(src_addr, dst_addr, length)
  u_long src_addr, dst_addr;
  int length;
{
  u_long src_phys_addr, dst_phys_addr;
  u_long last_src_tran_addr = 1;
  u_long last_dst_tran_addr = 1;


  for ( ; length > 0 ; length--) {
    if ((src_addr & (PAGESIZE-1)) != last_src_tran_addr) {
      if (sim_v_to_p(src_addr, &src_phys_addr, M_SUPERVISOR) != -1) {
        sim_printf("Error translating kernel virtual address 0x%x\n", src_addr);
        return;
      }
      last_src_tran_addr = src_addr & (PAGESIZE-1);
    }
    if ((dst_addr & (PAGESIZE-1)) != last_dst_tran_addr) {
      if (sim_v_to_p(dst_addr, &dst_phys_addr, M_SUPERVISOR) != -1) {
        sim_printf("Error translating kernel virtual address 0x%x\n", dst_addr);
        return;
      }
      last_dst_tran_addr = dst_addr & (PAGESIZE-1);
    }
    write_sim_b(dst_phys_addr, read_sim_b(src_phys_addr));
    src_addr++;
    src_phys_addr++;
    dst_addr++;
    dst_phys_addr++;
  }
}

/* Copy a block of memory in simulation memory. */
void check_sim(fd, addr, length)
  int fd;
  u_long addr;
  int length;
{
  u_long phys_addr;
  u_long last_tran_addr = 1;
  u_long *coff = (u_long *)malloc(length);
  u_long *coff_base = coff;

  if (!coff) {
    rerr("Couldn't malloc space for COFF file.");
  }
  if (read(fd, coff, length) != length) {
    sim_printf("Error reading COFF file.\n");
    return;
  }
  for ( ; length > 0 ; length -= 4) {
    u_long sim_w, file_w;
    if ((addr & (PAGESIZE-1)) != last_tran_addr) {
      if (sim_v_to_p(addr, &phys_addr, M_SUPERVISOR) != -1) {
        sim_printf("Error translating kernel virtual address 0x%x\n", addr);
        return;
      }
      last_tran_addr = addr & (PAGESIZE-1);
    }
    sim_w = read_sim_w(phys_addr);
    if (sim_w != *coff) {
      QUIT;
      sim_printf("0x%x: 0x%08x 0x%08x\n", addr, sim_w, *coff);
    }
    coff++;
    addr += sizeof sim_w;
    phys_addr += sizeof sim_w;
  }
  free(coff_base);
}

/* Search simulation memory starting at kernel virtual address 'addr'
   for a word that when and'ed by 'mask' equals
   'pattern'.  Increment the address by 'stride' on
   each interation of the search.  Search a range of 'length' bytes.
   Return the address of the first match, or -1 if no match is found. */

u_long sim_searchmem(addr, length, pattern, mask, stride)
  u_long addr, pattern, mask;
  int length, stride;
{
  u_long phys_addr, w;

  for ( ; length > 0 ; length -= stride) {
    if (((addr & (PAGESIZE - 1)) + 4) > PAGESIZE) {
      rerr(
        "Can't deal with alignment in sim_searchmem, addr=0x%x stride=0x%x\n", 
                                                   addr, stride);
    }
    if (sim_v_to_p(addr, &phys_addr, M_SUPERVISOR) != -1) {
      rerr("Error translating kernel virtual address 0x%x\n", addr);
    }
    w = read_sim_w(phys_addr);
    if ((w & mask) == pattern) {
      return addr;
    }
    addr += stride;
  }
  return (u_long)-1;
}

/* Display a list of all the simulated devices, their addresses, the
   number of times each has been accessed, and their device index. */
static void devs_command(exp, from_tty)
  char *exp;
  int from_tty;
{
  u_long device_index = 0;
  if (exp) {
    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    device_index = parse_and_eval_address (exp);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
  }
  sim_io_print_devices(device_index);
}

/* Display records that we've been storing the io trace buffer.  The
   first argument to this command is either a physical address or a
   device index.  The second argument is optional is the number of
   trace records to display. */

static void io_command(exp, from_tty)
  char *exp;
  int from_tty;
{
  u_long physical_address = 0;
  u_long number_to_show = 0;
  if (exp) {
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    space_index = (char *) index (exp, ' ');
    if (space_index == (char *)0) {
      number_to_show = 30;
    } else {
      *space_index = '\0';
      number_to_show = parse_and_eval_address (space_index + 1);
    }
    physical_address = parse_and_eval_address (exp);
  }
  sim_io_trace(physical_address, number_to_show);
}

/* Display detailed information for a particular IO device. */

static void pio_command(exp, from_tty)
  char *exp;
{
  u_long device_index = 0;
  if (exp) {
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    device_index = parse_and_eval_address (exp);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
  }
  sim_io_print(device_index);
}

static char *ex_names[] = {
 "reset", "int", "cacc", "dacc", "ma", "opc", "prv", 
 "bnd", "idiv", "ovf", "err"};

/* This returns a string that describes the exception whose code is
   passed */

static char *ex_name(code)
    u_short code;
{
    static char buf[1000];

    if (code < (sizeof ex_names) / sizeof (char *)) {
      return ex_names[code];
    } else {
      sprintf(buf, "%d", code);
      return &buf[0];
    }
}

/* This lets the user tell the simulator to stop when a given exception
   occurs.  Further, this is qualified with the user/supervisor mode.
   In the code below, exception numbers in 0..511 refer to exceptions
   that occur in supervisor mode.  Numbers in 512..1023 refer to exceptions
   that occur in user mode. 

   If flag == 1 we are doing the catch command.
   If flag == 0 we are doing the ignore command. */

static void do_ecatch_eignore_command(exp, from_tty, flag)
  char *exp;
{
  int i;
  char *flag_string = flag ? "caught" : "ignored";
  int mode;
  if (exp) {
    if (strcmp(exp, "all") == 0) {
      for (i = 0 ; i < 512 ; i++) {
        sim_catch_exception[i] = flag;
      }
      return;
    }
    if (strcmp(exp, "uall") == 0) {
      for (i = 0 ; i < 512 ; i++) {
        sim_catch_exception[i] = flag;
      }
      return;
    }
    if (exp[0] == 'u') {
      mode = 512;
      exp++;
    } else {
      mode = 0;
    }
    for (i = 0 ; i < sizeof(ex_names) / sizeof(char *) ; i++) {
      if (strcmp(exp, ex_names[i]) == 0) {
        sim_catch_exception[mode + i] = flag;
        return;
      }
    }
    i = parse_and_eval_address (exp);
    if (i < 0 || i > 511) {
      rerr("Exception code out of range");
    }
    sim_catch_exception[mode + i] = flag;
  } else {
    for (mode = 0 ; mode < 1024 ; mode += 512) {
      char *mode_string = mode == 0 ? "supervisor" : "user";
      QUIT;
      sim_printf("Exceptions that will be %s in %s mode:\n", 
                                          flag_string, mode_string);
      for (i = 0 ; i < 512 ; i++) {
        if (sim_catch_exception[mode + i] == flag) {
          sim_printf("%s", ex_name(i));
          i++;
          if (11 < i && i < 512 && sim_catch_exception[mode + i] == flag) {
            sim_printf(" through ");
            for ( ; sim_catch_exception[mode + i] == flag && i < 512 ; i++);
            i--;
            sim_printf("%s", ex_name(i));
          } else {
            i--;
          }
          if (i < 512 ) {
            sim_printf(", ");
          }
        }
      }
      sim_printf("\n");
    }
  }
}

/* If no argument is specified, prints signals being caught.
   If "all" is specified, all exceptions while in supervisor mode will be
   caughcaught.  If "uall", all exceptions while in user mode will be caught.
   Otherwise, the argument is a number in 0..511 of the signal to be caught.*/

static void ecatch_command(exp, from_tty)
  char *exp;
{
  do_ecatch_eignore_command(exp, from_tty, 1);
}

/* If no argument is specified, prints signals being ignored.
   If "all" is specified, all exceptions while in supervisor mode will be
   ignored.  If "uall", all exceptions while in user mode will be ignored.
   Otherwise, the argument is a number in 0..511 of the signal to be ignored.*/

static void eignore_command(exp, from_tty)
  char *exp;
{
  do_ecatch_eignore_command(exp, from_tty, 0);
}

struct processor {
  u_long general_registers[32];
  u_long sfu0_registers[64];
  u_long sfu1_registers[64];
  struct decoded_i *delayed_p;
  u_long delayed_ip;
} processors[MAX_PROCESSORS];


int waiting_for_mpswitch_interrupt;

/* Number of ticks left before we switch to the next processor. */
int mp_slice;

/* This translates kernel logical addresses to physical.
   This is called by the front end.  */

int sim_v_to_p(la, pa, usmode)
    u_long la;
    u_long *pa;
    int usmode;
{
    if (usmode == M_CURMODE) {
        usmode = PSR_US_MODE;
    }
    return l_to_p(usmode, la, pa, selected_processor, READ);
}

/* Returns the number of enabled processors. */
int number_of_enabled_processors()
{
  int i;
  int cnt = 0;
  for (i = 0 ; i < MAX_PROCESSORS ; i++) {
    cnt += cpu_enabled[i];
  }
  return cnt;
}

/* Copy the passed processor's registers to the working registers */
static void set_regs(processor)
  int processor;
{
  int i;

  for (i = 0 ; i < 32 ; i++) {
    regs[i] = processors[processor].general_registers[i];
  }

  for (i = 0 ; i < 64 ; i++) {
    sfu0_regs[i] = processors[processor].sfu0_registers[i];
    sfu1_regs[i] = processors[processor].sfu1_registers[i];
  }
  delayed_p = processors[processor].delayed_p;
  delayed_ip = processors[processor].delayed_ip;
}

/* Copy the working registers to the slot of the passed processor
   number. */
static void read_regs(processor)
  int processor;
{
  int i;
  for (i = 0 ; i < 32 ; i++) {
     processors[processor].general_registers[i] = regs[i];
  }

  for (i = 0 ; i < 64 ; i++) {
    processors[processor].sfu0_registers[i] = sfu0_regs[i];
    processors[processor].sfu1_registers[i] = sfu1_regs[i];
  }
  processors[processor].delayed_p = delayed_p;
  processors[processor].delayed_ip = delayed_ip;
}

/* Save the working registers in the currently-selected processor's
   slot.  Make the passed processor number be the new current  processor
   number and copy its registers to the working registers. */
void sim_select_processor(processor)
  int processor;
{
  read_regs(selected_processor);
  selected_processor = processor;
  set_regs(selected_processor);
  flush_user_tlb_all();
  flush_supervisor_tlb_all();
  flush_deccache(M_SUPERVISOR);
  flush_deccache(M_USER);
  if (varvalue("showswitch")) {
    ui_fprintf(stdout, "MP:%d ", processor);
    ui_fflush(stdout);
  }
}

/* Selects the next enabled processor */
void select_next_processor()
{
  int i;
  int pn;
  for (i = 0 ; i < MAX_PROCESSORS ; i++) {
    pn = (selected_processor + i + 1) % MAX_PROCESSORS;
    if (cpu_enabled[pn]) {
      break;
    }
  }
  sim_select_processor(pn);
}

/* Called by the SIGVTALRM signal handler when we are waiting for
   these interrupts to switch to the next processor. */
void multiprocessor_switch()
{
  mp_slice--;
  if (mp_slice == 0) {
    mp_slice = varvalue("simslice");
    sim_interrupt_flag |= INT_MPSWITCH;
  }
}

#include <sys/signal.h>
/* Let the simulated 88000 system execute instructions.  If step is
   non-zero, execute one instruction and return.  If step is zero,
   execute instructions until a breakpoint, front-end interrupt,
   or caught exception is encountered. */
int sim_resume(step)
  int step;
{
  int retcode;
  extern stop_cause;
  char buf[100];

  sim_interrupt_flag &= ~INT_FRONT_END;

  if (number_of_enabled_processors() == 0) {
    sim_printf("No processors are enabled.\n");
    stop_cause |= (1 << SIGINT);
    return;
  }

  if (cpu_enabled[selected_processor] == 0) {
    sim_printf("The current processor is not enabled.\n");
    stop_cause |= (1 << SIGINT);
    return;
  }

  check_for_interrupt();
  if (step) {
    retcode = sim(step);
  } else {
    if (number_of_enabled_processors() == 1) {
      retcode = sim(0);
    } else {
      if (varvalue("simslice") == 0 || varvalue("simtick") == 0) {
        retcode = NONE;
        while (retcode == NONE)  {
	  retcode = sim(1);
	  if (retcode == NONE) {
	    select_next_processor();
	  }
        }
      } else {
	waiting_for_mpswitch_interrupt = 1;
	mp_slice = varvalue("simslice");
	retcode = MPSWITCH;
	while (retcode == MPSWITCH) {
	  retcode = sim(0);
	  if (retcode == MPSWITCH) {
	    select_next_processor();
	  }
	}
	waiting_for_mpswitch_interrupt = 0;
      }
    }
  }

  switch (retcode) {
    extern char *exception_name();

    case NONE:
    case BREAKPOINT: 
      stop_cause |= (1 << SIGTRAP); 
      break;

    case INTERRUPT: 
      stop_cause |= (1 << SIGINT);
      break;

    case HOSED: 
      stop_cause |= (1 << SIGINT);
      sim_printf("Simulated 88100 is hosed, last exception: %s\n",
           sim_exception == E_NONE ? "none" : exception_name(sim_exception));
      break;

    case CAUGHT_EXCEPTION:
      stop_cause |= (1 << SIGINT);
      sim_printf("Caught %s\n", exception_name(sim_exception));
      break;

    default:
      sim_printf("return code from the simulator is %d\n", retcode);
      err("Case error in sim_resume", __LINE__, __FILE__);
  }
  sprintf(buf, "[%d] (gdb)", selected_processor);
  set_either_prompt_command (0, buf);
}

/* Called by sim_init when the simulator is first started and every
   time the init command is given. */
void multiprocessor_initialization()
{
  int i;
  for (i = 0 ; i < MAX_PROCESSORS ; i++) {
    read_regs(i);
  }
  selected_processor = 0;
}

/* Called by code in remote.c when a control-C has been detected. */
void sim_interrupt()
{
  sim_interrupt_flag |= INT_FRONT_END;
}

#include <signal.h>
/* This is called by the code in siminit.c */
gdb_sim_init()
{
  extern remote_debugging;
  extern stop_after_attach;
  extern stop_cause;

  init_globals();
  remote_debugging = 1;
  stop_after_attach = 1;
  /* What ptrace reports after attaching a process */
  stop_cause |= (1 << SIGTRAP);
  multiprocessor_initialization();
  selected_processor = 0;
  set_either_prompt_command(0, "[0] (gdb)");
}

/* We need to establish these commands before doing the attach so
   gdb can grok them when it reads the .gdbinit file. */

_initialize_simgdb()
{
  init_globals();
  add_com("printmap", class_run, sim_printmap,
     "Display status of physical memory pages.");

  add_com("io", class_run, io_command,
     "Write a single word from the target memory.");

  add_com("devs", class_run, devs_command,
     "Display list of devices");

  add_com("pio", class_run, pio_command,
     "Display detailed device state information");

  add_com("ecatch", class_run, ecatch_command,
     "Catch an 88000 exception (0..511): supervisor mode (512..1023): user");

  add_com("eignore", class_run, eignore_command,
     "Ignore an 88000 exception (0..511): supervisor mode (512..1023): user");
}
