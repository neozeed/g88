
/* Simulators for interrupt mechanism, DUART, and WHOAMI registers
   on the MVME 188 CPU board from Motorola.  Not all of the registers 
   are modelled.  

   CIO (Counter/Timer) simulator added 8/22/90 */

#include "sim.h"
#include <sys/time.h>
#include <signal.h>

/* Words that model the four interrupt mask registers and the interrupt
   status register. */
u_long ien0, ien1, ien2, ien3, ist;

/* Called when a change of state in the machine may have enabled
   an interrupt. */
void check_for_interrupt()
{
  int i;

  switch (selected_processor) {
    case 0: i = ien0; break;
    case 1: i = ien1; break;
    case 2: i = ien2; break;
    case 3: i = ien3; break;
  }
  i &= ist;
  if (i) {
    sim_interrupt_flag |= INT_DEVICE;
  }
}

/* Pass the number of the bit (0..31) in the interrupt status register
   that you want to set.  This will cause an interrupt any processor
   that has unmasked this interrupt. */
void set_ist(bit)
  int bit;
{
  ist |= 1 << bit;
  check_for_interrupt();;
}

/* Pass the number of the bit (0..31) in the interrupt status register
   that you want to reset. */
void clear_ist(bit)
  int bit;
{
  ist &= ~(1 << bit);
}

void ien0_init() { ien0 = 0; }
void ien1_init() { ien1 = 0; }
void ien2_init() { ien2 = 0; }
void ien3_init() { ien3 = 0; }
void ien_init() {}
void ist_init() { ist = 0; }
void setswi_init() {}
void clrswi_init() {}
void istate_init() {}
void clrint_init() {}


int ien0_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt enable register does not support double operations\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ien0, size, mem_op_type);
  check_for_interrupt();
  return E_NONE;
}

int ien1_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt enable register does not support double operations\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ien1, size, mem_op_type);
  check_for_interrupt();
  return E_NONE;
}

int ien2_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt enable register does not support double operations\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ien2, size, mem_op_type);
  check_for_interrupt();
  return E_NONE;
}

int ien3_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt enable register does not support double operations\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ien3, size, mem_op_type);
  check_for_interrupt();
  return E_NONE;
}

int ien_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt enable register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != ST) {
    sim_printf("ien is write-only\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ien0, size, mem_op_type);
  do_mem_op(reg_ptr, &ien1, size, mem_op_type);
  do_mem_op(reg_ptr, &ien2, size, mem_op_type);
  do_mem_op(reg_ptr, &ien3, size, mem_op_type);
  check_for_interrupt();
  return E_NONE;
}

int ist_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("interrupt status register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != LD && mem_op_type != LD_U) {
    sim_printf("interrupt status register is read-only\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &ist, size, mem_op_type);
  return E_NONE;
}

/* Set the software interrupt bits in the interrupt status register
   that correspond to set bits in the register being stored to this
   device. */
int setswi_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("setswi register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != ST) {
    sim_printf("setswi is write-only\n");
    return E_DACC;
  }
  ist |= (*reg_ptr & 0xf) | (((*reg_ptr >> 4) & 0xf) << 24);
  return E_NONE;
}

/* Clear the software interrupt bits in the interrupt status register
   that correspond to set bits in the register being stored to this
   device. */
int clrswi_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("clrswi register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != ST) {
    sim_printf("clrswi is write-only\n");
    return E_DACC;
  }
  ist &= ~((*reg_ptr & 0xf) | (((*reg_ptr >> 4) & 0xf) << 24));
  return E_NONE;
}

int istate_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("istate register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != LD && mem_op_type != LD_U) {
    sim_printf("istate register is read-only\n");
    return E_DACC;
  }
  sim_printf("istate register is not implemented\n");
  return E_DACC;
}

int clrint_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("clrint register does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != ST) {
    sim_printf("clrint is write-only\n");
    return E_DACC;
  }
  sim_printf("clrint register is not implemented\n");
  return E_DACC;
}

void ien0_print() {sim_printf("ien0 = 0x%x\n", ien0);}
void ien1_print() {sim_printf("ien1 = 0x%x\n", ien1);}
void ien2_print() {sim_printf("ien2 = 0x%x\n", ien2);}
void ien3_print() {sim_printf("ien3 = 0x%x\n", ien3);}
void ien_print() {ien0_print(); ien1_print(); ien2_print(); ien3_print(); }
void ist_print() 
{
  sim_printf("ist = 0x%x  %s\n", ist, (ist & (1<< 20)) ? "<cio>" : "");
}

void setswi_print() {}
void clrswi_print() {}
void istate_print() {}
void clrint_print() {}



/* This is the model for the WHOAMI register of a 4-CPU, 8-CMMU 188
   board.  See page 4-51 of the
   MVME188 VMEmodule RISC Microcomputer User's Manual. */
void whoami_init() {}
int whoami_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  int whoami;
  if (size > WORD) {
    sim_printf("duart does not support double operations\n");
    return E_DACC;
  }
  if (mem_op_type != LD && mem_op_type != LD_U) {
    sim_printf("whoami register is read-only\n");
    return E_DACC;
  }
  whoami = 1 << selected_processor;
  do_mem_op(reg_ptr, &whoami, size, mem_op_type);
  return E_NONE;
}

void whoami_print() 
{
  int whoami = 1 << selected_processor;
  sim_printf("whoami= 0x%x\n", whoami);
}

/* This simulator is a subset of the functions provided by the
   Zilog 8536 Counter/Timer and Parallel I/O Unit. 

   We do not model: 
    connections of the parallel ports to anything
    interrupt vectors

/* This structure defines the registers inside of the CIO */

struct cio_regs {

  /* Main control registers */
  u_char c_micr;		/* Master interrupt control		   */
  u_char c_mccr;		/* Master configuration control		   */
  u_char c_porta_iv;		/* Port A's interrupt vector		   */
  u_char c_portb_iv;		/* Port B's interrupt vector		   */
  u_char c_ctim_iv;		/* Counter/Timer's interrupt vector	   */
  u_char c_portc_pol;		/* Port C's data path polarity		   */
  u_char c_portc_dir;		/* Port C's data direction		   */
  u_char c_portc_ioctl;		/* Port C's I/O control			   */

  /* Most often accessed registers */
  u_char c_porta_com;		/* Port A's command and status		   */
  u_char c_portb_com;		/* Port B's command and status		   */
  u_char c_ctim1_com;		/* Coutner/Timer 1's command and status	   */
  u_char c_ctim2_com;		/* Coutner/Timer 2's command and status	   */
  u_char c_ctim3_com;		/* Coutner/Timer 3's command and status	   */
  u_char c_porta_data;		/* Port A's data (can be accessed directly)*/
  u_char c_portb_data;		/* Port B's data (can be accessed directly)*/
  u_char c_portc_data;		/* Port C's data (can be accessed directly)*/

  /* Counter/Timer related registers */
  u_char c_ctime1_cur_msb;	/* Counter/Timer 1's current count MSB	   */
  u_char c_ctime1_cur_lsb;	/* Counter/Timer 1's current count LSB	   */
  u_char c_ctime2_cur_msb;	/* Counter/Timer 2's current count MSB	   */
  u_char c_ctime2_cur_lsb;	/* Counter/Timer 2's current count LSB	   */
  u_char c_ctime3_cur_msb;	/* Counter/Timer 3's current count MSB	   */
  u_char c_ctime3_cur_lsb;	/* Counter/Timer 3's current count LSB	   */

  u_char c_ctime1_const_msb;	/* Counter/Timer 1's time constant MSB	   */
  u_char c_ctime1_const_lsb;	/* Counter/Timer 1's time constant LSB	   */
  u_char c_ctime2_const_msb;	/* Counter/Timer 2's time constant MSB	   */
  u_char c_ctime2_const_lsb;	/* Counter/Timer 2's time constant LSB	   */
  u_char c_ctime3_const_msb;	/* Counter/Timer 3's time constant MSB	   */
  u_char c_ctime3_const_lsb;	/* Counter/Timer 3's time constant LSB	   */

  u_char c_tim1_mode;		/* Counter/Timer 1's mode specification	   */
  u_char c_tim2_mode;		/* Counter/Timer 2's mode specification	   */
  u_char c_tim3_mode;		/* Counter/Timer 3's mode specification	   */

  u_char c_current_vector;

/* Port A specification registers */
  u_char c_porta_mode;		/* Port A's mode specification		   */
  u_char c_porta_hand;		/* Port A's handshake specification	   */
  u_char c_porta_polarity;
  u_char c_porta_dir;
  u_char c_porta_ioctl;
  u_char c_porta_pat_pol;
  u_char c_porta_pat_tran;
  u_char c_porta_pat_mask;

/* Port B specification registers */
  u_char c_portb_mode;		/* Port B's mode specification		   */
  u_char c_portb_hand;		/* Port B's handshake specification	   */
  u_char c_portb_polarity;
  u_char c_portb_dir;
  u_char c_portb_ioctl;
  u_char c_portb_pat_pol;
  u_char c_portb_pat_tran;
  u_char c_portb_pat_mask;

  /* We pad this structure to make it be 64 bytes long.
     This is to handle the case when the pointer register is loaded
     with an out-of-range value. */

  u_char c_pad[0x40 - 0x2f]; } cio_regs;

/* This models the states that the CIO can be in */

enum cio_state {cio_reset, cio_state0, cio_state1} cio_state;

/* This points to the next register to be read from state 0 or 1 and
   the next register to be written from state 1. */

static u_char *cio_reg_pointer;
extern void check_for_cio_interrupt();

void set_cio_interrupt()
{
  cio_regs.c_ctim1_com |= 0x20;
  check_for_cio_interrupt();
}

/* This does to the simulated CIO what a reset command or signal
   does to the real CIO */

void cio_do_reset()
{
  cio_reg_pointer = &cio_regs.c_micr;
  bzero(&cio_regs, sizeof(struct cio_regs));
  cio_regs.c_micr = 1;
  cio_state = cio_reset;
  cio_regs.c_porta_com = 0x8; /* Set output-register-empty bit, others clear */
  cio_regs.c_portb_com = 0x8; /* Set output-register-empty bit, others clear */
}

/* This decodes writes to the port and coutner/timer 
   command/status registers */

void cio_write_command_reg(command, cio_reg_pointer)
  int command;
  u_char *cio_reg_pointer;
{
  extern int timer_on, timer_count;
  switch ((command >> 5) & 7) {
    case 0: break;                              /* Null code */
    case 1: *cio_reg_pointer &= ~0xa0; break;   /* Clear IP and IUS */
    case 2: *cio_reg_pointer |= 0x80;  break;   /* Set IUS */
    case 3: *cio_reg_pointer &= ~0x80; break;   /* Clear IUS */
    case 4: *cio_reg_pointer |= 0x20; break;    /* Set IP */
    case 5: *cio_reg_pointer &= ~0x20; break;   /* Clear IP */
    case 6: *cio_reg_pointer |= 0x40;  break;   /* Set IE */
    case 7: *cio_reg_pointer &= ~0x40; break;   /* Clear IE */
  }
  if (cio_reg_pointer == &cio_regs.c_ctim1_com ||
      cio_reg_pointer == &cio_regs.c_ctim2_com ||
      cio_reg_pointer == &cio_regs.c_ctim3_com) {

    /* Set the timer based on the gate bit in the command. */
    int old_timer_on = timer_on;
    int ticks_per_int = varvalue("simticksperint");
    if (ticks_per_int == 0) {
      sim_printf("$simticksperint not set, setting to 3\n");
      setvar("simticksperint", 3);
      ticks_per_int = 3;
    }
    timer_on = (command & 4) ? ticks_per_int : 0;

    if (timer_on && !old_timer_on) {
      HoldOscillator();
      ReleaseOscillator();
      timer_count = timer_on;
    }

    *cio_reg_pointer &= ~4;
    *cio_reg_pointer |= command & 4;

    /* We don't do anything with the trigger command bit in the
       counter/timer commands.  */
  }
}

/* Check to see if the program just did something that should
   enable the process virtual timer, i.e., that enabled the
   simulated CIO timer.

   Really, we can go ahead and just start the timer.  We don't actually
   interrupt the simulated CPU unless all the right enable bits are set */


/* Check to see if there is an interrupt pending in the CIO, and if so,
   propogate it to the CPU's interrupt logic.

   There are five interrupt sources within the CIO. We check them in
   their priority order.  A set interrupt-in-service (IUS) bit in a higher
   priority source will mask lower priority interrupts. */

void check_for_cio_interrupt()
{
  /* Check to see that the master interrupt enable bit is on. */
  if (!(cio_regs.c_micr & 0x80)) {
    clear_ist(21);
    return;
  }

  /* Check to see that the Counter/Timer 3 interrupt-in-service bit is off */
  if (cio_regs.c_ctim3_com & 0x80) {
    return;
  }

  /* See if an unmasked interrupt from Counter/Timer 3 is pending. */
  if ((cio_regs.c_ctim3_com & 0x60) == 0x60) {
    set_ist(21);
    return;
  }

  /* Check to see that the port A interrupt-in-service bit is off */
  if (cio_regs.c_porta_com & 0x80) {
    return;
  }

  /* See if an unmasked interrupt from port A is pending. */
  if ((cio_regs.c_porta_com & 0x60) == 0x60) {
    set_ist(21);
    return;
  }

  /* Check to see that the Counter/Timer 2 interrupt-in-service bit is off */
  if (cio_regs.c_ctim2_com & 0x80) {
    return;
  }

  /* See if an unmasked interrupt from Counter/Timer 2 is pending. */
  if ((cio_regs.c_ctim2_com & 0x60) == 0x60) {
    set_ist(21);
    return;
  }

  /* Check to see that the port B interrupt-in-service bit is off */
  if (cio_regs.c_portb_com & 0x80) {
    return;
  }

  /* See if an unmasked interrupt from port B is pending. */
  if ((cio_regs.c_portb_com & 0x60) == 0x60) {
    set_ist(21);
    return;
  }

  /* Check to see that the Counter/Timer 1 interrupt-in-service bit is off */
  if (cio_regs.c_ctim1_com & 0x80) {
    return;
  }

  /* See if an unmasked interrupt from Counter/Timer 1 is pending. */
  if ((cio_regs.c_ctim1_com & 0x60) == 0x60) {
    set_ist(21);
    return;
  }
  clear_ist(21);
}

/* Reload the current count register fromthe time constant register for
   the specified coutner/timer. */


void reload_timer(t)
  int t;
{
  switch (t) {
    case 1:
      cio_regs.c_ctime1_cur_msb = cio_regs.c_ctime1_const_msb;
      cio_regs.c_ctime1_cur_lsb = cio_regs.c_ctime1_const_lsb;
      break;

    case 2:
      cio_regs.c_ctime2_cur_msb = cio_regs.c_ctime2_const_msb;
      cio_regs.c_ctime2_cur_lsb = cio_regs.c_ctime2_const_lsb;
      break;

    case 3:
      cio_regs.c_ctime3_cur_msb = cio_regs.c_ctime3_const_msb;
      cio_regs.c_ctime3_cur_lsb = cio_regs.c_ctime3_const_lsb;
      break;

    default:
      sim_printf("Case error in reload_timer() (in 88k simulator) \n");
  }
}

/* This is called by the code in io.c when the simulator is first
   started and when the user types 'init' */

void cio_init() 
{
  cio_do_reset();
}

/* This is called when the simulated program or the front end accesses
   the counter/timer simulator. */

int cio_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
  if (size != WORD) {
    sim_printf("cio does not support byte, short, or double operations\n");
    return E_DACC;
  }

  if (mem_op_type != LD && mem_op_type != LD_U && mem_op_type != ST) {
    sim_printf("cio only supports ld and st operations\n");
    return E_DACC;
  }

  /* There are only four byte locations that are addressable by the
     processor.  Switch on the address offset to find which location
     is being accessed. */

  switch (address_offset) {
    case 0:
      do_mem_op(reg_ptr, &cio_regs.c_porta_data, BYTE, mem_op_type);
      break;

    case 4:
      do_mem_op(reg_ptr, &cio_regs.c_portb_data, BYTE, mem_op_type);
      break;

    case 8:
      do_mem_op(reg_ptr, &cio_regs.c_portc_data, BYTE, mem_op_type);
      break;

    case 12:
      switch (cio_state) {
        case cio_reset:
          do_mem_op(reg_ptr, &cio_regs.c_micr, BYTE, mem_op_type);
          if (cio_regs.c_micr & 1) {
            cio_regs.c_micr = 1;	/* Force top 7 bits to zero */
          } else {
            cio_state = cio_state0;
          }
          break;

        /* We come to this state after reset and after writing a
           register from state 1.  A write will update the cio-internal-
           register-pointer.  A read will return whatever this pointer
           register currently points to.  */

        case cio_state0:
          if (mem_op_type == ST) {
            cio_reg_pointer = (u_char *)&cio_regs + (*reg_ptr & 0x3f);
            cio_state = cio_state1;
          } else {
            do_mem_op(reg_ptr, cio_reg_pointer, BYTE, mem_op_type);
          }
          break;

        /* We come to this state after writing the pointer registers
           (from state 0). A read or a write will return or write to
           the register pointed to by the cio-internal-register-pointer
           and change to state 0. */

        case cio_state1:
          if (mem_op_type == ST && 
               (cio_reg_pointer == &cio_regs.c_porta_com ||
                cio_reg_pointer == &cio_regs.c_portb_com ||
                cio_reg_pointer == &cio_regs.c_ctim1_com ||
                cio_reg_pointer == &cio_regs.c_ctim2_com ||
                cio_reg_pointer == &cio_regs.c_ctim3_com)) {
            cio_write_command_reg(*reg_ptr, cio_reg_pointer);
          } else {
            do_mem_op(reg_ptr, cio_reg_pointer, BYTE, mem_op_type);
          }
          cio_state = cio_state0;
          break;

        default:
          sim_printf("Case error in CIO simulator!\n");
          return E_DACC;
          break;
      }
      break;

    default:
      sim_printf("lower two bits of cio address must be 00\n");
      return E_DACC;
  }
  check_for_cio_interrupt();
  return E_NONE;
}

/* This is called when the user enters the "pio" command with the
   device index for the CIO. We display information that we think
   will be useful to him or her. */

void cio_print()
{
  sim_printf("CIO: %s.\n", cio_state == cio_reset ? "reset state" : 
                         (cio_state == cio_state0 ? 
         "next write will modify register pointer" : 
         "next write will modify register pointed to"));

  sim_printf(" Counter/Timer 1, current value=0x%x   time constant=0x%x\n",
             (cio_regs.c_ctime1_cur_msb << 8) | cio_regs.c_ctime1_cur_lsb,
             (cio_regs.c_ctime1_const_msb << 8) | cio_regs.c_ctime1_const_lsb);

  sim_printf(" Counter/Timer 2, current value=0x%x   time constant=0x%x\n",
             (cio_regs.c_ctime2_cur_msb << 8) | cio_regs.c_ctime2_cur_lsb,
             (cio_regs.c_ctime2_const_msb << 8) | cio_regs.c_ctime2_const_lsb);

  sim_printf(" Counter/Timer 3, current value=0x%x   time constant=0x%x\n",
             (cio_regs.c_ctime3_cur_msb << 8) | cio_regs.c_ctime3_cur_lsb,
             (cio_regs.c_ctime3_const_msb << 8) | cio_regs.c_ctime3_const_lsb);

  sim_printf(" Master interrupt control = 0x%x\n", cio_regs.c_micr);
  sim_printf(" Master configuration control = 0x%x\n", cio_regs.c_mccr);
  sim_printf(" Counter/Timer 1, command/status = 0x%x\n", cio_regs.c_ctim1_com);
  sim_printf(" Counter/Timer 1, mode spec = 0x%x\n", cio_regs.c_tim1_mode);
}
