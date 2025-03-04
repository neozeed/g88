/*
 * 88000 SCC68692 DUART simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/duart.c,v 1.3 90/11/05 11:35:45 wilde Exp $
 *
 * This simulates the SCC68692 Serial Communications Controller.
 * This simulator looks at the environment variables SIMTTYA and
 * SIMTTYB.  If either SIMTTYA or SIMTTYB is set, it is interpreted
 * as the device filename of a serial port.  SIMTTYA controls the A
 * port of the SCC and SIMTTYB controls the B port.  If one of these
 * environment variables is not set, its port's data is dropped on the
 * floor.
 *
 * Setting the time constant register in the A or B port will cause the
 * simulator to change the baud of the tty line (it does this with an
 * ioctl).  Likewise, setting the flow control bit in register 3 causes
 * the simulator to do an ioctl on the tty line to set hardware flow
 * control.  Loopback mode is supported.
 *
 * None of the synchronous modes are supported.
 */

/*
 * Table of contents:
 *
 * unimpl - print an unimplemented-function error message.
 * clear_in_buff - clear a typeahead buffer.
 * enqueue - add a character to a typeahead buffer.
 * dequeue - get a character from a typeahead buffer.
 * check_tty_line - read any typeahead that's available on a single line.
 * check_tty_lines - read any typeahead on both lines.
 * hardware_reset - simulate a hardware reset of the DUART.
 * init_serial_line - initialize a serial tty line.
 * duart_init - initialize the duart simulator.
 * duart_init_side - initialize one side of the duart.
 * duart_operation - do a single operation (load or store).
 * reset_xmit - do a transmitter reset.
 * reset_recv - do a receiver reset.
 * do_cntl - handle IO operations with the control register.
 * do_data - handle operations with the SCC's data port.
 * disp_field - display the bit fields associated with a DUART register.
 * disp_baud_rate - display the baud rate.
 * duart_print - display the DUART's internal state.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/termio.h>

#include "sim.h"
#include "io.h"

extern int errno;
char *getenv();

#define MR1   		0
#define MR2   		1
#define DUART_WR	0
#define DUART_RD	1
#define PORT_OFFSET(ab) ((ab == DUART_A) ? 0 : 8)
#define DUART_SIZE      16
#define DUART_A         0
#define DUART_B         1

/*
 * This models the registers of the DUART.
 */
static u_char du_registers[2][DUART_SIZE];

/*
 * This maintains the DUART mode register pointers
 */
static u_long reg_pointer[2];

static u_long enables[2];

static u_char output_reg;

static int warned;

/*
 * Circular typeahead buffers.
 */
#define IN_BUFFSIZ  (20000)		/* size of buffer */
static u_char in_buff[2][IN_BUFFSIZ];	/* the buffers */
static u_char *in_buff_front[2];	/* ptr to next char to read */
static u_char *in_buff_rear[2];		/* ptr to next char to write */
static u_long in_buff_count[2];		/* number of bytes in buffer */

/*
 * File descriptor to use for reading and writing the tty line.
 */
static int duart_fd[2];

/*
 * Name of the tty special file.
 */
static char *du_filename[2];

/*
 * True if this tty line has been inited.  Used to decide whether
 * to open it on restore.
 */
static char du_inited[2];

static u_long reset_cnt, nmi_cnt, total_chars_read[2];

/*
 * DUART simulator debugging flag.
 */
int sdbg;

static void set_sim_baud();

/*
 * unimpl - print an unimplemented-function error message.
 */
void
unimpl(fmt)
    char *fmt;
{
    sim_printf("duart: unimplemented: %s\n", fmt);
}

/*
 * check_for_nmi - send a RESET or NMI if the character is a magic cookie.
 * Here we simulate the cross-debugging kludge
 * board that yanks the reset line if it sees
 * a 0x81 go by and the NMI line if it sees
 * a 0x82.
 */
static void check_for_nmi(c)
    u_char c;
{
    switch (c) {
        case 0x81:
            sim_interrupt_flag |= INT_RESET;
            reset_cnt++;
            break;

        case 0x82:
            set_ist(31);
            nmi_cnt++;
            break;

        default:
            break;
    }
}

/*
 * clear_in_buff - clear a typeahead buffer.
 */
clear_in_buff(a_or_b)
int a_or_b;
{
	in_buff_front[a_or_b] = in_buff[a_or_b];
	in_buff_rear [a_or_b] = in_buff[a_or_b];
	in_buff_count[a_or_b] = 0;
}

/*
 * enqueue - add a character to a typeahead buffer.
 */
static enqueue(a_or_b, c)
int a_or_b, c;
{
    if (in_buff_count[a_or_b] >= IN_BUFFSIZ) {
	return;	/* no room */
    }
    ++in_buff_count[a_or_b];
    *in_buff_rear[a_or_b]++ = c;
    if (in_buff_rear[a_or_b] >= &in_buff[a_or_b][IN_BUFFSIZ]) {
	in_buff_rear[a_or_b] = in_buff[a_or_b];
    }
}

/*
 * dequeue - get a character from a typeahead buffer.
 */
static int dequeue(a_or_b)
int a_or_b;
{
    int c;

    if (in_buff_count[a_or_b]==0) {
	sim_printf("duart: dequeue but count=0\n");
	return 0;
    }
    c = *in_buff_front[a_or_b]++;
    if (--in_buff_count[a_or_b]) {
	if (in_buff_front[a_or_b] >= &in_buff[a_or_b][IN_BUFFSIZ]) {
	    in_buff_front[a_or_b] = in_buff[a_or_b];
	}
    } else {
	clear_in_buff(a_or_b);
    }
    return c;
}

/*
 * check_tty_line - read any typeahead that's available.
 * This reads in more characters from a tty line if any are
 * waiting.  It does not stop and wait for characters.
 */
static void check_tty_line(a_or_b)
{
    int i, chars_read, fd;
    int cnt;
    int chars_to_read;
    unsigned char buf[1];

    /*
     * Don't do anything if this port is not connected to a tty.
     */
    if (!du_filename[a_or_b] && (a_or_b != DUART_A)) {
        return;
    }
	
    fd = du_filename[a_or_b] ? duart_fd[a_or_b] : 0;

    /*
     * See how many characters there are to read.
     * If there are none, don't try to read any.
     */
	if (ioctl(fd, FIONREAD, &chars_to_read) == -1) {
		sim_printf("duart: ioctl(FIONREAD) failed, errno=%d\n", errno);
		return;
    	}
    if (chars_to_read==0) {
	return;
    }

    /*
     * Append as many characters as there are waiting, up to the
     * limit of the buffer, to the input buffer.
     */
    while (chars_to_read) {
	chars_read = read(fd, buf, 1);
	if (chars_read != 1) {
	    sim_printf("duart: read error code %d, errno=%d\n",
			chars_read, errno);
	    return;
	}
	enqueue(a_or_b, buf[0]);
	total_chars_read[a_or_b] += 1;
	chars_to_read -= 1;
        if (a_or_b == DUART_B) {
            check_for_nmi(buf[0]);
        }
    }

    /*
     * If the receiver isn't enabled, throw away the characters.
     */
    if (!(enables[a_or_b] & 1)) {
	clear_in_buff(a_or_b);
	return;
    }

    /*
     * Set Rx character available.
     * If Rx interrupts are enabled, generate a CE interrupt.
     */
    du_registers[DUART_RD][PORT_OFFSET(a_or_b)+1] |= 3;
    du_registers[DUART_RD][PORT_OFFSET(a_or_b)+3] = *in_buff_front[a_or_b];
    if (!(du_registers[MR1][PORT_OFFSET(a_or_b)] & 0x40) ) {
	du_registers[DUART_RD][5] |= a_or_b == DUART_A ? 2 : 0x20;
        if (du_registers[DUART_RD][5] & du_registers[DUART_WR][5]) {
           set_ist(17);
        }
    }
}

/*
 * check_tty_lines - read any typeahead on both lines.
 */
static void check_tty_lines()
{
    check_tty_line(DUART_A);
    check_tty_line(DUART_B);
}

/*
 * hardware_reset - simulate a hardware reset of the DUART.
 * This sets the bits in the DUART's simulated registers to the
 * hardware-reset state.
 */
static void hardware_reset()
{
    int i;

    for (i = 0 ; i < DUART_SIZE ; i++) {
        du_registers[DUART_RD][i] = 0;
        du_registers[DUART_WR][i] = 0;
    }
    reg_pointer[DUART_A] = 0;
    reg_pointer[DUART_B] = 0;
    enables[DUART_A] = 0;
    enables[DUART_B] = 0;
    output_reg = 0;

    /*
     * Hardware reset values
     */
    du_registers[DUART_RD][12] = 0xF;
    du_registers[DUART_WR][12] = 0xF;
}

/*
 * init_serial_line - initialize a serial tty line.
 */
static void init_serial_line(a_or_b)
    int a_or_b;
{
  int time_constant;
  int speed;
  struct termio t;
  struct sgttyb b;

  char *env_tty_name;
  int baud_rate_flags;

  if (!du_filename[a_or_b]) {
    return;
  }

  if (duart_fd[a_or_b] == 0) {
    if ((duart_fd[a_or_b] = open(du_filename[a_or_b], O_RDWR|O_NDELAY)) < 0) {
      if (!warned) {
        sim_printf("duart: cannot open `%s', using debug terminal\n", 
                                      du_filename[a_or_b]);
        warned = 1;
      }
      duart_fd[a_or_b] = 0;
      du_inited[a_or_b] = 1;
      return;
    }
  }

  /*
   * Based on the time constant that the 88k program loaded the
   * scc with, do an ioctl to set the baud of the host tty port
   * that we are using.  This assumes the scc is being clocked
   * at the rate that the hardware designers of the Tek CE board
   * told me.  -rcb
   */
  time_constant = du_registers[DUART_WR][PORT_OFFSET(a_or_b)+1];

  /* During initialization, quietly set for 9600 baud. */
  /* if (!du_inited[a_or_b] && time_constant==0) { */
  if (time_constant==0) { /* don't offend the pty's */
    time_constant = 0xb;
  }

  /*
   * If we already have a tty line open, close it to conserve
   * on file descriptors.
   */
  /* if (duart_fd[a_or_b] > 0) {
    if (close(duart_fd[a_or_b]) != 0) {
      sim_printf("\nUnable to close tty line\n");
    }
  }

  if  ((duart_fd[a_or_b]  = open(du_filename[a_or_b], O_RDWR | O_NDELAY)) < 0) {
    duart_fd[a_or_b] = 0;
    sim_printf("Cannot open %s, but was able to open it earlier\n",
                                  du_filename[a_or_b]);
    return;
  }
*/
  if (ioctl(duart_fd[a_or_b], TCGETA, &t) < 0) {
    rerr("open_remote_tty: Error in doing TCGETA ioctl to %s", 
                                du_filename[a_or_b]);
  }

  t.c_oflag = 0;
  t.c_cflag = CS8 | CREAD | CLOCAL | (t.c_cflag & CBAUD);
  t.c_line = 0;

  t.c_cc[VMIN] = 1;	/* Make sure this is 0 or 1, doesn't matter which */

  t.c_iflag = IGNBRK | IGNPAR;

  t.c_lflag = 0;
  if (ioctl(duart_fd[a_or_b], TCSETA, &t) < 0) {
    err("open_remote_tty: Error in doing TCSETA ioctl");
  }

  /* Make IO with the remote device synchronous.  */
  if (fcntl(duart_fd[a_or_b], F_SETFL, 0) < 0) {
    err("open_remote_tty: Error doing fcntl to set flags on %s", 
                                                   du_filename[a_or_b]);
  }

  if (ioctl(duart_fd[a_or_b], TIOCGETP, &b) < 0) {
    rerr("open_remote_tty: Error in doing TIOCGETP ioctl to %s", 
                                du_filename[a_or_b]);
  }
  b.sg_flags |= RAW;
  if (ioctl(duart_fd[a_or_b], TIOCSETP, &b) < 0) {
    err("open_remote_tty: Error in doing TIOCSETP ioctl");
  }

  speed = csr_to_baud(time_constant);
  set_sim_baud(speed, a_or_b);

  /*
   * If the auto-enables bit in the Receive Parameters registers
   * is set, tell the host OS to do hardware flow-control.
   */
/* #ifdef HMMM
  if (write_registers[a_or_b][3] & 0x20) {
    localmask |= /* LDOCTS|LDODTR */;
/*  } */

  (void)simsignal(SIGIO, check_tty_lines);
  (void)simsignal(SIGHUP, SIG_IGN); 

  /*
   * We get a signal if there is IO for the tty line.
   */

  if (fcntl(duart_fd[a_or_b], F_SETOWN, getpid()) == -1) {
    sim_printf("duart: error doing fcntl with F_SETOWN to %s\n",
                                 du_filename[a_or_b]);
  }
  if (fcntl(duart_fd[a_or_b], F_SETFL, FASYNC) == -1) {
    sim_printf("duart: error doing fcntl with F_SETFL to %s\n",
                                 du_filename[a_or_b]);
  }
/* #endif */

  du_inited[a_or_b] = 1;
}

/*
 * csr_to_baud - convert CSR code to actual baud rate
 */
static int csr_to_baud(code)
int code;
{
    int brg_code,speed;
    static int lookup[32] = { 50, 110, 134, 200,
                              300, 600, 1200, 1050,
                              2400, 4800, 7200, 9600,
                              38400, -1, -1, -1,
                              75, 110, 134, 150,
                              300, 600, 1200, 2000,
                              2400, 4800, 1800, 9600,
                              19200, -1, -1, -1 };
    
    brg_code = ((du_registers[DUART_WR][4]&0x80) >> 3) | (code&0xf);
    speed = lookup[brg_code];
    if (speed == -1) {
       sim_printf("duart: bad baud rate 0x%04x, using 9600 baud\n", code);
       speed = 9600;
    }
    return speed;
}

/*
 * duart_init_side - initialize one side of the duart simulator.
 */
void duart_init_side(restore, a_or_b)
    int a_or_b;
{
    if (!restore) {
        du_filename[a_or_b] = a_or_b == DUART_A ? getenv("SIMTTYA") : getenv("SIMTTYB");
        clear_in_buff(a_or_b);
        total_chars_read[a_or_b] = 0;
        du_inited[a_or_b] = 0;
        init_serial_line(a_or_b);
    } else {
        if (du_inited[a_or_b]) {
            init_serial_line(a_or_b);
        }
    }
}

/*
 * duart_init - initialize the a and b sides of the duart simulator.
 */
void duart_init(restore)
{ 
    hardware_reset();
    duart_init_side(restore, DUART_A);
    duart_init_side(restore, DUART_B);
    if (!restore) {
        reset_cnt = 0;
        nmi_cnt = 0;
        sdbg = varvalue("sdbg") != 0;
    }
}

/*
 * duart_operation - do a single operation (load or store)
 */
int duart_operation(address_offset, reg_ptr, size, mem_op_type, override)
    unsigned address_offset;
    unsigned *reg_ptr;
    unsigned size;
    unsigned mem_op_type;
    int      override;
{
    int was_an_interrupt = du_registers[DUART_WR][5] & du_registers[DUART_RD][5];
    int is_an_interrupt;
    int status;

    if (size != WORD) {
        sim_printf("duart: IO should use word operations.\n");
        return E_DACC;
    }

    if (mem_op_type == XMEM || mem_op_type == XMEM_U) {
        sim_printf("duart: simulator does not support XMEM and XMEM_U\n");
        return E_DACC;
    }


    switch (address_offset) {
        case 0:
        case 4:
        case 8:
        case 0x10:
        case 0x14:
        case 0x18:
        case 0x1C:
        case 0x20:
        case 0x24:
        case 0x28:
        case 0x30:
        case 0x34:
        case 0x38:
        case 0x3C:
            status = do_cntl(address_offset, reg_ptr, mem_op_type);
            break;

        case 0xC:
            status = do_data(reg_ptr, mem_op_type, DUART_A);
            break;

        case 0x2C:
            status = do_data(reg_ptr, mem_op_type, DUART_B);
            break;

        default:
            sim_printf("duart_operation: address offset=0x%x\n", address_offset);
            status = E_DACC;
            break;
    }

    is_an_interrupt = du_registers[DUART_RD][5] & du_registers[DUART_WR][5];
    if (was_an_interrupt && !is_an_interrupt) {
        clear_ist(17);
    } else {
        if (!was_an_interrupt && is_an_interrupt) {
            set_ist(17);
        }
    }
    return status;
}

/*
 * reset_xmit - do a transmitter reset.
 */
static void reset_xmit(a_or_b)
    int a_or_b;
{
    int i,index;
    index = PORT_OFFSET(a_or_b);

    du_registers[DUART_RD][index+1] &= ~0x0c;
    du_registers[DUART_RD][5] &= ( a_or_b == DUART_A ? ~1 : ~0x10);

    du_registers[DUART_WR][index+3] = 0;

    enables[index] &= ~2;

    init_serial_line(a_or_b);
}

/*
 * reset_recv - do a receiver reset.
 */
static void reset_recv(a_or_b)
    int a_or_b;
{
    int i,index;
    index = PORT_OFFSET(a_or_b);

    du_registers[DUART_RD][index+1] &= ~0xf3;
    du_registers[DUART_RD][index+3] = 0;

    du_registers[DUART_RD][5] &= (a_or_b == DUART_A ? ~6 : ~0x60);

    enables[index] &= ~1;

    clear_in_buff(a_or_b);
    init_serial_line(a_or_b);
}

/*
 * do_cntl - handle IO operations with the control register.
 */
static int do_cntl(address_offset, reg_ptr, mem_op_type)
    unsigned address_offset;
    u_long *reg_ptr;
    int mem_op_type;
{
    u_long i;
    int a_or_b, index;

    switch (mem_op_type) {
        char byte, oldbyte;

        case ST:
            byte = *reg_ptr;
            a_or_b =  (address_offset>>5) == 0 ? DUART_A : DUART_B;
            index = PORT_OFFSET(a_or_b);

            if (address_offset & 0x1f) {
                oldbyte = du_registers[DUART_WR][address_offset>>2];
                du_registers[DUART_WR][address_offset>>2] = byte;
            } else {  /* write to mode registers */
                oldbyte = du_registers[reg_pointer[a_or_b]][index];
                du_registers[reg_pointer[a_or_b]][index] = byte;
                reg_pointer[a_or_b] = MR2;
            }

            switch (address_offset) {
                case 0:
                case 0x20:
                    if (reg_pointer[a_or_b] == MR1) {
                       /* 
                        * write to mode register 1 
                        */


                       if (byte & 0x40) {
                          unimpl("FIFO Full interrupt");
                       }

                       if ((byte & 0x18) == 0x18) {
                          unimpl("multidrop mode");
                       }
                    } else {
                       /*
                        * write to mode register 2
                        */
                    }
                    break;
               case 4:
               case 0x24:
                    /* 
                     * clock select register 
                     */
                    if (((byte & 0xf0) >> 4) == (byte & 0xf)) {
                    } else {
                       sim_printf("duart:transmit clock <> receive clock\n");
                    }
                    init_serial_line(a_or_b);	
                    break;

               case 8:
               case 0x28: 
                    /*
                     * Execute the command in bits 7-4 of the command register.
                     */
                    switch ((byte >> 4) & 0xF) {
                        case 0:
                            /*
                             * Null command (no effect).
                             */
                            break;

                        case 1:
                            /*
                             * Reset MR pointer.
                             */
                            reg_pointer[a_or_b] = MR1;
                            break;

                        case 2:
                            /*
                             * Reset receiver.
                             */
                            reset_recv(a_or_b);
			    break;

                        case 3:
                            /* 
                             * Reset transmitter.
                             */
                            reset_xmit(a_or_b);
                            break;

                        case 4:
                            /*
                             * Reset error status.
                             */
                            break;

                        case 5:
                            /*
                             * Reset ch. A break change interrupt.
                             */
                            break;

                        case 6:
                            /*
                             * Start break.
                             */
                            break;

                        case 7:
                            /* 
                             * Stop break.
                             */
                            break;

                        case 8:
                            /*
                             * Assert RTSN.
                             */
                            break;

                        case 9:
                            /*
                             * Negate RTSN.
                             */
                            break;

                        case 0xA:
                            /*
                             * Set timeout mode on.
                             */
                            break;

                        case 0xB:
                        case 0xD:
                            /*
                             * Unused command.
                             */
                            break;

                        case 0xC:
                            /*
                             * Disable timeout mode.
                             */
                            break;

                        case 0xE:
                            /*
                             * Power down mode on.
                             */
                            unimpl("power down mode on");
                            break;

                        case 0xF:
                            /*
                             * Disable power down mode.
                             */
                            unimpl("disable power down mode");
                            break;
                    }
                   
                    switch ((byte & 0xC) >> 2) { /* decode CR 3:2 */

                        case 0:
                            break;
                        case 1:
                            enables[a_or_b] |= 2;
                            du_registers[DUART_RD][index+1] |= 0x0c;
                            du_registers[DUART_RD][5] |= (a_or_b==DUART_A ? 1 : 0x10);
			    break;
                        case 2:
                            enables[a_or_b] &= ~2;
                            du_registers[DUART_RD][index+1] &= ~0x0c;
                            du_registers[DUART_RD][5] &= ( a_or_b==DUART_A ? ~1 : ~0x10);
                            break;
                        case 3:
                            sim_printf("duart: simultaneous enable/disable of Tx\n");
                            break;

                    }

                    switch (byte & 0x3) { /* decode CR 1:0 */

                        case 0:
                            break;
                        case 1:
                            enables[a_or_b] |= 1;
                            break;
                        case 2:			
                            /*  
                             *  disable receiver - clear RxRDY and FFULL bits,
                             *     and flush input buffer
                             */

                            enables[a_or_b] &= ~1;
                            du_registers[DUART_RD][index+1] &= ~3;
                            du_registers[DUART_RD][5] &= ( a_or_b==DUART_A ? ~6 : ~0x60);
                            clear_in_buff(a_or_b);
                            break;
                        case 3:
                            sim_printf("duart: simultaneous enable/disable of Rx \n");
                            break;

                    }

                    break;

                case 0x18:
                    /* 
                     * load C/T upper reg
                     */
                    du_registers[DUART_RD][6] = du_registers[DUART_WR][6];
                    break;

                case 0x1C:
                    /*
                     * load C/T lower reg
                     */
                    du_registers[DUART_RD][7] = du_registers[DUART_WR][7];
                    break;

                case 0x38:
                    /*
                     * set output port bits
                     */
                    output_reg |= du_registers[DUART_WR][14];
                    break;

                case 0x3C:
                    /*
                     * reset output port bits
                     */
                    output_reg &= ~du_registers[DUART_WR][15];
                    break;

            }

            break;


        case LD:
        case LD_U:
            /*
             * The program is reading a status register.
             */
            if (address_offset & 0x1f) {
                *reg_ptr = (u_long)du_registers[DUART_RD][address_offset>>2];
            } else {
                a_or_b =  (address_offset>>5) == 0 ? DUART_A : DUART_B;
                *reg_ptr = (u_long)du_registers[reg_pointer[a_or_b]][PORT_OFFSET(a_or_b)];
                reg_pointer[a_or_b] = MR2;
            }
            break;

        default:
            sim_printf("duart_operation: case error.\n");
            return E_DACC;
    }
    return E_NONE;
}

/*
 * do_data - handle operations with the SCC's data port.
 */
static int do_data(reg_ptr, mem_op_type, a_or_b)
    u_long *reg_ptr;
    int mem_op_type;
    int a_or_b;
{
    int index;
    index = PORT_OFFSET(a_or_b);

    switch (mem_op_type) {

        case LD:
        case LD_U:
            if (enables[a_or_b] & 1) {  /* if Rx is enabled */
                /*
                 * If we are in local loop mode, get the character from the
                 * transmit buffer.
                 */
                if ((du_registers[MR2][index] & 0xc0) == 0x80) {
                    du_registers[DUART_RD][index+3] = du_registers[DUART_WR][index+3];
                   /*
                    * clear the "Rx char avail" and "Rx IP" bits.
                    */
                    du_registers[DUART_RD][index+1] &= ~3;
                    if (!(du_registers[MR1][index] & 0x40)) {
                       du_registers[DUART_RD][5] &=
                       a_or_b==DUART_A ? ~2 : ~0x20;
                    }
                } else {
                    /*
                     * We are not in loop mode, and the receiver is enabled.
		     * Get the character from the buffer and put it into the
		     * receive character register.
		     * If there is no character (or if the receiver is
		     * disabled), the last character remains in the buffer.
		     * Block SIGIO during this operation to establish
		     * exclusive access to our input buffer.
                     */
                    if (in_buff_count[a_or_b] > 0) {
			du_registers[DUART_RD][index+3] = dequeue(a_or_b);

			/*
			 * If this is the last character from the buffer,
			 * clear the "Rx char avail" and "Rx IP" bits.
			 */
                        if (in_buff_count[a_or_b] == 0) {
			    du_registers[DUART_RD][index+1] &= ~3;
			    if (!(du_registers[MR1][index] & 0x40)) {
                               du_registers[DUART_RD][5] &=
				a_or_b==DUART_A ? ~2 : ~0x20;
                            }
			}
                    }
                }
            }

            *reg_ptr = (u_long)du_registers[DUART_RD][index+3];

            break;

        case ST:
            if ((enables[a_or_b] & 2) == 0) {
                sim_printf("duart: character write with transmitter disabled.\n");
            }
            du_registers[DUART_WR][index+3] = *reg_ptr;
            /*
             * If there is no tty line connected to this port,
             * just drop the characters on the floor.
             */
            if (du_filename[a_or_b] != 0 &&
                write(duart_fd[a_or_b], &du_registers[DUART_WR][index+3], 1)!= 1) {
                sim_printf("duart: write error on %s\n", du_filename[a_or_b]);
                return E_ERR;
            }
            /*
             * If we are in local loopback mode, set the receive-buffer-full
             * flag.
	     *** Should set receiver IP if receiver interrupts enabled.
             */
            if ((du_registers[MR2][index] & 0xc0) == 0x80) {
                du_registers[DUART_RD][index+1] |= 3;
                if (!(du_registers[MR1][index] & 0x40)) {
                   du_registers[DUART_RD][5] |= (a_or_b==DUART_A ? 2 : 0x20);
                }

            }
            /*
             * If Tx interrupts are enabled, generate a CE interrupt.
             */
            du_registers[DUART_RD][5] |= a_or_b == DUART_A ? 1 : 0x10;
            break;

        default:
            sim_printf("duart: case error in duart_operation\n");
            break;
    }

    return E_NONE;
}

struct fields {
	unsigned char mask;
	unsigned char value;
	char *desc;
};

static struct fields WR0f[] = {
        0x80, 0x80, "Rx RTS control enable",
        0x40, 0x00, "RxINT on RxRDY",
        0x40, 0x40, "RxINT on fifo full",
        0x20, 0x00, "err mode = char",
        0x20, 0x20, "err mode = block",
        0x18, 0x00, "with parity",
        0x18, 0x08, "force parity",
        0x18, 0x10, "no parity",
        0x18, 0x18, "multidrop mode",
        0x04, 0x00, "parity = even",
        0x04, 0x04, "parity = odd",
        0x03, 0x00, "5 bits/char",
        0x03, 0x01, "6 bits/char",
        0x03, 0x02, "7 bits/char",
        0x03, 0x03, "8 bits/char",
        0x00, 0x00, 0
};

static struct fields WR2f[] = {
        0xf0, 0x10, "reset MR pointer",
        0xf0, 0x20, "reset receiver",
        0xf0, 0x30, "reset transmitter",
        0xf0, 0x40, "reset error status",
        0xf0, 0x50, "reset break change interrupt",
        0xf0, 0x60, "start break",
        0xf0, 0x70, "stop break",
        0xf0, 0x80, "assert RTSN",
        0xf0, 0x90, "negate RTSN",
        0xf0, 0xa0, "set timeout mode on",
        0xf0, 0xc0, "disable timeout mode",
        0xf0, 0xe0, "power down mode on",
        0xf0, 0xf0, "disable power down mode",
        0x0c, 0x04, "enable Tx",
        0x0c, 0x08, "disable Tx",
        0x03, 0x01, "enable Rx",
        0x03, 0x02, "disable Rx",
	0x00, 0x00, 0
};

static struct fields WR4f[] = {
        0x80, 0x00, "baud rate set 1",
        0x80, 0x80, "baud rate set 2",
        0x70, 0x00, "ctr mode, ext. (IP2) clock",
        0x70, 0x10, "ctr mode, TxCA 1x clock",
        0x70, 0x20, "ctr mode, TxCB 1x clock",
        0x70, 0x30, "ctr mode, crystal/ext clk /16",
        0x70, 0x40, "timer mode, (IP2) clock",
        0x70, 0x50, "timer mode, (IP2) clock/16",
        0x70, 0x60, "timer mode, crystal clk",
        0x70, 0x70, "timer mode, crystal clk /16",
        0x08, 0x00, "dis IP3 change int.",
        0x08, 0x08, "enb IP3 change int.",
        0x04, 0x00, "dis IP2 change int",
        0x04, 0x04, "enb IP2 change int.",
        0x02, 0x00, "dis IP1 change int",
        0x02, 0x02, "enb IP1 change int.",
        0x01, 0x00, "dis IP0 change int",
        0x01, 0x01, "enb IP0 change int.",
	0x00, 0x00, 0
};

static struct fields WR5f[] = {
        0x80, 0x80, "in port change int. enb",
        0x40, 0x40, "delta BREAK B int. enb",
        0x20, 0x20, "RxRDY/FFULL B int. enb",
        0x10, 0x10, "TxRDYB int. enb",
        0x08, 0x08, "counter ready int. enb",
        0x04, 0x04, "delta BREAK A int. enb",
        0x02, 0x02, "RxRDY/FFULL A int. enb",
        0x01, 0x01, "TxRDYA int. enable",
	0x00, 0x00, 0
};

static struct fields WR13f[] = {
        0x80, 0x00, "OP7=OPR[7]",
        0x80, 0x80, "OP7=TxRDYB",
        0x40, 0x00, "OP6=OPR[6]",
        0x40, 0x40, "OP6=TxRDYA",
        0x20, 0x00, "OP5=OPR[5]",
        0x20, 0x20, "OP5=RxRDY/FFULLB",
        0x10, 0x00, "OP4=OPR[4]",
        0x10, 0x10, "OP4=RxRDY/FFULLA",
        0x0c, 0x00, "OP3=OPR[3]",
        0x0c, 0x04, "OP3=C/T output",
        0x0c, 0x08, "OP3=Tx clock B (1x)",
        0x0c, 0x0c, "OP3=Rx clock B (1x)",
        0x03, 0x00, "OP3=OPR[2]",
        0x03, 0x01, "OP3=Tx clock A (16x)",
        0x03, 0x02, "OP3=Tx clock A (1x)",
        0x03, 0x03, "OP3=Rx clock A (1x)",
	0x00, 0x00, 0
};

static struct fields RR0f[] = {
	0xC0, 0x40, "ch. A auto-echo",
        0xC0, 0x80, "ch. A lcl loopback",
        0xC0, 0xC0, "ch. A remote loopback",
        0x20, 0x00, "dis ch. A Tx RTS control",
        0x20, 0x20, "enb ch. A Tx RTS control",
        0x10, 0x00, "dis ch. A CTS Tx control",
        0x10, 0x10, "enb ch. A CTS Tx control",
        0x00, 0x00, 0
};

static struct fields RR1f[] = {
        0x80, 0x80, "rec'd break",
        0x40, 0x40, "framing err",
        0x20, 0x20, "parity err",
        0x10, 0x10, "overrun err",
        0x08, 0x00, "Tx not empty",
        0x08, 0x08, "Tx empty",
        0x04, 0x00, "Tx not ready",
        0x04, 0x04, "Tx ready",
        0x02, 0x00, "FIFO not full",
        0x02, 0x02, "FIFO full",
        0x01, 0x00, "recv char not ready",
        0x01, 0x01, "recv char ready",
	0x00, 0x00, 0
};

static struct fields RR4f[] = {
        0x80, 0x80, "IP3 lev change",
        0x40, 0x40, "IP2 lev change",
        0x20, 0x20, "IP1 lev change",
        0x10, 0x10, "IP0 lev change",
        0x08, 0x00, "IP3 low",
        0x08, 0x08, "IP3 high",
        0x04, 0x00, "IP2 low",
        0x04, 0x04, "IP2 high",
        0x02, 0x00, "IP1 low",
        0x02, 0x02, "IP1 high",
        0x01, 0x00, "IP0 low",
        0x01, 0x01, "IP0 high",
	0x00, 0x00, 0
};

static struct fields RR5f[] = {
        0x80, 0x80, "in port change int.",
        0x40, 0x40, "delta BREAK B int.",
        0x20, 0x20, "RxRDY/FFULL B int.",
        0x10, 0x10, "TXRDY B int.",
        0x08, 0x08, "counter ready int.",
        0x04, 0x04, "delta BREAK A int.",
        0x02, 0x02, "RxRDY/FFULL A int.",
        0x01, 0x01, "TxRDY A int.",
	0x00, 0x00, 0
};

/* RR15f is the same as WR15f */

static struct fields *WRfields[] = {
	WR0f,     0,  WR2f,     0, WR4f,  WR5f,     0,     0,
	WR0f,     0,  WR2f,     0,    0, WR13f,     0,     0
};

static struct fields *RRfields[] = {
	RR0f,  RR1f,     0,     0, RR4f,  RR5f,     0,     0,
	RR0f,  RR1f,     0,     0,    0,     0,     0,     0
};

/*
 * disp_field - display the bit fields associated with a DUART register.
 */
disp_fields(fieldp, value)
    register struct fields *fieldp;
    register long value;
{
    char buf[1024];

    /* If there's no field structure, say nothing. */
    if (!fieldp) {
	return;
    }

    buf[0] = '\0';

    /* For each (mask,value) pair which matches this value, append the
       associated descriptor string to the buffer, separated by vertical bars.
       */
    while (fieldp->mask) {
	if ((fieldp->mask & value) == fieldp->value) {
	    if (buf[0]) {
		strcat(buf, "|");
	    }
	    strcat(buf, fieldp->desc);
	}
	++fieldp;
    }

    /* If the buffer will fit on the remainder of this line, put it there,
       otherwise print it on its own line. */
    if (strlen(buf) > (80-34)) {
	sim_printf("\n  %s", buf);
    } else {
	sim_printf(" %s", buf);
    }
}

/*
 * disp_baud_rate - display the baud rate.
 */
disp_baud_rate(a_or_b)
    unsigned a_or_b;
{
    register int timeConstant;
    int baudRate,index;
    index = PORT_OFFSET(a_or_b) + 1;

    if ((du_registers[DUART_WR][index] & 0xf) < 13 ) {
       baudRate = csr_to_baud((int)du_registers[DUART_WR][index]);
       sim_printf(" baud rate=%d", baudRate);
    } else {
       sim_printf(" external clock, baud rate undefined");
    }
}

/*
 * disp_port_stats - display port statistics.
 */
disp_port_stats(a_or_b)
    int a_or_b;
{
    int i;
    if (a_or_b == DUART_A) {
       sim_printf("Port A: ");
    } else {
       sim_printf("Port B: ");
    }
    if (enables[a_or_b] & 1) {
       sim_printf("Rx enabled, ");
    } else {
       sim_printf("Rx disabled, ");
    }
    if (enables[a_or_b] & 2) {
       sim_printf("Tx enabled\n");
    } else {
       sim_printf("Tx disabled\n");
    }

    if (du_filename[a_or_b]) {
        int cnt = in_buff_count[a_or_b];
        u_char *cp;

        sim_printf("    unread chars=%d front pointer=%d rear pointer=%d total chars read=%d\n",
               cnt, in_buff_front[a_or_b]-in_buff[a_or_b],
               in_buff_rear [a_or_b]-in_buff[a_or_b], total_chars_read[a_or_b]);

        if (cnt > 0) {
            int lim = cnt <= 16 ? cnt : 16;
            sim_printf("    First %d unread characters in buffer:\n", lim);
            cp = in_buff_front[a_or_b];
            sim_printf("    ");
            for (i = 0; i < lim ; i++) {
                sim_printf("0x%x ", *cp++);
                if (cp >= &in_buff[a_or_b][IN_BUFFSIZ]) {
                    cp = in_buff[a_or_b];
                }
            }
            sim_printf("\n");
        }
    }
}

/*
 * duart_print - display the DUART's internal state.
 */
void duart_print()
{
    int i;
    register struct fields **fieldpp;
    static char *wr_names[DUART_SIZE] = {
            "mode register 1 A    ",
            "clock-select reg. A  ",
            "command register A   ",
            "transmit buffer A    ",
            "auxiliary ctrl. reg. ",
            "interrupt mask       ",
            "counter/timer upper  ",
            "counter/timer lower  ",
            "mode register 1 B    ",
            "clock-select reg. B  ",
            "command register B   ",
            "transmit buffer B    ",
            "interrupt vector     ",
            "output port config.  ",
            "OPR bit set          ",
            "OPR bit reset        " };

    static char *rd_names[DUART_SIZE] = {
            "mode register 2 A    ",
            "status register A    ",
            "",
            "receive buffer A     ",
            "input port change reg",
            "interrupt status     ",
            "counter MSB          ",
            "counter LSB          ",
            "mode register 2 B    ",
            "status register B    ",
            "",
            "receive buffer B     ",
            "interrupt vector     ",	
            "input port           ",	
            "start counter cmd    ",    /* unused */
            "stop counter cmd     " };  /* unused */

    if (du_filename[DUART_A]) {
        sim_printf("Simulated DUART port A uses %s\n", du_filename[DUART_A]);
    } else {
        sim_printf("Simulated DUART port A is not connected to a tty\n");
    }
    if (du_filename[DUART_B]) {
        sim_printf("Simulated DUART port B uses %s\n", du_filename[DUART_B]);
    } else {
        sim_printf("Simulated DUART port B is not connected to a tty\n");
    }



    /* Display write registers. */
    for (i = 0, fieldpp = WRfields; i<=15; i++, fieldpp++) {
        sim_printf("[WR%-2d] %20s = 0x%02x",
                    i, wr_names[i], du_registers[DUART_WR][i]);
	switch (i) {
            case 1:
               disp_baud_rate(DUART_A);
               break;
            case 9:
	       disp_baud_rate(DUART_B);
               break;
            default:
	       disp_fields(*fieldpp, du_registers[DUART_WR ][i]);
	}
	sim_printf("\n");
    }

    /* Display read registers, only through RR13 */
    for (i = 0, fieldpp = RRfields; i<=14; i++, fieldpp++) {
        if (strlen(rd_names[i])) {
            sim_printf("[RR%-2d] %20s = 0x%02x",
                         i, rd_names[i], du_registers[DUART_RD][i]);
	    disp_fields(*fieldpp, du_registers[DUART_RD][i]);
	    sim_printf("\n");
	}
    }
    sim_printf("reset count=%d  nmi count=%d\n", reset_cnt, nmi_cnt);
    disp_port_stats(DUART_A);
    disp_port_stats(DUART_B);

}

/*
 * Set the data rate of the remote tty line.
 */
static void set_sim_baud(new_baud, a_or_b)
  int new_baud;
  int a_or_b;
{
  struct termio t;
  int baud_code;

  switch (new_baud) {
    case 50:	baud_code = B50;	break;
    case 75:	baud_code = B75;	break;
    case 110:	baud_code = B110;	break;
    case 134:   baud_code = B134;       break;
    case 150:   baud_code = B150;       break;
    case 200:	baud_code = B200;	break;
    case 300:	baud_code = B300;	break;
    case 600:	baud_code = B600;	break;
    case 1200:	baud_code = B1200;	break;
    case 1800:	baud_code = B1800;	break;
    case 2400:	baud_code = B2400;	break;
    case 4800:	baud_code = B4800;	break;
    case 9600:	baud_code = B9600;	break;
    case 19200:	baud_code = EXTA;	break;

    case 0:
    case 38400:	baud_code = EXTB;	break;

    default:
      err("Can't set remote tty line to %d baud", new_baud);
  }
  if (ioctl(duart_fd[a_or_b], TCGETA, &t) < 0) {
    err("setbaud: Error in doing TCGETA ioctl to %s", du_filename[a_or_b]);
  }

  t.c_cflag = (t.c_cflag & ~CBAUD) | baud_code;

  if (ioctl(duart_fd[a_or_b], TCSETA, &t) < 0) {
    err("setbaud: Error in doing TCSETA ioctl to %s", du_filename[a_or_b]);
  }
}
