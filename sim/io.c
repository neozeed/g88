/*
 * Simulated 88000 IO devices.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/io.c,v 1.55 90/10/30 23:52:51 robertb Exp $
 */

#include "sim.h"
#include "io.h"

#define SIMTRACE    "simtrace"
#define	SIMETRACE	"simetrace"

static struct io_trace *io_trace;
static struct io_trace *io_first_trace;
static struct io_trace *io_last_trace;
static u_long io_trace_vals;
static u_long io_trace_buf_size;

#define IO_TRACE_BUF_DEFAULT_SIZE   (20000)

/* 
 * You need to change code if you change this.
 */
#define IO_CACHE_SIZE       (256)

struct io_trace *io_trace_buf;

#define ROUTINES(name)  0,                     \
                        "name",                \
                         name##_init,          \
                         name##_operation,     \
                         name##_print


struct io_dev io_dev_tab[] = {
0xfff6f000, PAGESIZE,           0, ROUTINES(data_cmmu_0),
0xfff5f000, PAGESIZE,           0, ROUTINES(data_cmmu_1),
0xfff3f000, PAGESIZE,           0, ROUTINES(data_cmmu_2),
0xfff7f000, PAGESIZE,           0, ROUTINES(data_cmmu_3),

0xfff7e000, PAGESIZE,           0, ROUTINES(code_cmmu_0),
0xfff7d000, PAGESIZE,           0, ROUTINES(code_cmmu_1),
0xfff7b000, PAGESIZE,           0, ROUTINES(code_cmmu_2),
0xfff77000, PAGESIZE,           0, ROUTINES(code_cmmu_3),

0xfff82000, 0x40,		0, ROUTINES(duart),
0xfff83000, 16,			0, ROUTINES(cio),
0xfff84004, 4,			0, ROUTINES(ien0),
0xfff84008, 4,			0, ROUTINES(ien1),
0xfff84010, 4,			0, ROUTINES(ien2),
0xfff84020, 4,			0, ROUTINES(ien3),
0xfff8403c, 4,			0, ROUTINES(ien),

0xfff84040, 4,			0, ROUTINES(ist),
0xfff84080, 4,			0, ROUTINES(setswi),
0xfff84084, 4,			0, ROUTINES(clrswi),
0xfff84088, 4,			0, ROUTINES(istate),
0xfff8408c, 4,			0, ROUTINES(clrint),
0xfff88018, 4,			0, ROUTINES(whoami),

0xffff0000, 1,                  0, ROUTINES(console),
0xffff0004, 4,			0, ROUTINES(trivial_timer),
0xffff000c, 0x14,               0, ROUTINES(special),
0xffff0020, 0x10,               1, ROUTINES(disk1),
0xffff0030, 0x10,               1, ROUTINES(disk2),
0xffff0040, 0x10,               1, ROUTINES(disk3),
0xffff0050, 0x10,               1, ROUTINES(disk4),
0xffff0060, 0x10,               0, ROUTINES(gettime)
};
#undef ROUTINES

#define IO_DEV_TAB_SIZE (sizeof(io_dev_tab) / sizeof(struct io_dev))
#define IO_HASH(addr)   (((addr) >> 12) & 0xff)

static struct io_dev *last = &io_dev_tab[IO_DEV_TAB_SIZE];

static struct io_dev *io_cache[IO_CACHE_SIZE];

/* Start of trivial timer device, as requested by Rueven */

/* stubs for timer routines. */

#include <sys/time.h>
#include <signal.h>
int timer_on;
int ticks, timer_count;

/* This is the signal handler for SIGVTALRM.  This is called by the
   kernel every $simtick microseconds.  It does double duty: it may
   do the trivial timer's interrupt (a simulated device), and it
   may call the multiprocessor switch routine. */

static void Oscillator()
{
  if (timer_on) {
    ticks++;
    timer_count--;
    if (timer_count == 0) {
      timer_count = timer_on;
      set_cio_interrupt();
      /* sim_interrupt_flag |= INT_DEVICE; */
    }
  }
  if (waiting_for_mpswitch_interrupt) {
    multiprocessor_switch();
  }
}

/* Turn off the SIGVTALRM, regardless of the setting of timer_on. */
void HoldOscillator()
{
#ifdef HAVE_ITIMER_VIRTUAL
  struct itimerval    new_timer_value, old_timer_value;
      
  new_timer_value.it_interval.tv_sec = 0;
  new_timer_value.it_interval.tv_usec = 0;
  new_timer_value.it_value.tv_sec = 0;
  new_timer_value.it_value.tv_usec = 0;
  if (setitimer (ITIMER_VIRTUAL, &new_timer_value, &old_timer_value) != 0) {
    sim_printf("g88: setitimer(ITIMER_VIRTUAL,..) in HoldOscillator() returns error\n");
  }
#endif
}

/* Turn on SIGVTALRM only if the trivial timer is on or if we are
   waiting for a multiprocessor switch interrupt. */
void ReleaseOscillator()
{
#ifdef HAVE_ITIMER_VIRTUAL
  struct itimerval new_timer_value, old_timer_value;
  u_long usec;

  if (timer_on > 0 || waiting_for_mpswitch_interrupt) {
    /* start the UTek clock */
    (void) simsignal(SIGVTALRM, Oscillator);
    usec = varvalue("simtick");
    if (usec < 10000) {
      sim_printf("Warning: $simtick is less than 10000 microseconds, but\n\
SUN-OS will not interrupt any faster, so $simtick is effectively 10000\n");
      sim_printf("Setting simtick to 10000\n");
      setvar("simtick", usec = 10000);
    }
    new_timer_value.it_interval.tv_sec = usec / 1000000;
    new_timer_value.it_interval.tv_usec = usec % 1000000;
    new_timer_value.it_value = new_timer_value.it_interval;
    if (setitimer (ITIMER_VIRTUAL, &new_timer_value, &old_timer_value) != 0) {
      sim_printf("g88: setitimer() in ReleaseOscillator() returns error.\n");
    }
  }
#endif
}

/* The trivial timer pulls the interrupt line of the currently 
   selected 88100 periodically.  The period is the product of the
   value in the program-accessable register (modeled by 'timer_on')
   and the convenience variable "$simtick".  If this product is
   zero, no interrupts are generated. This IO model was requested
   by Rueven Koblick.  -rcb 6/90 */
void trivial_timer_init() 
{
  timer_on = 0;
  ticks = 0;
}

int trivial_timer_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
  if (size > WORD) {
    sim_printf("trivial timer does not support double operations\n");
    return E_DACC;
  }
  do_mem_op(reg_ptr, &timer_on, size, mem_op_type);
  timer_count = timer_on;
  if (!override) {
    HoldOscillator();
    ReleaseOscillator();
  }
  return E_NONE;
}

void trivial_timer_print()
{
  sim_printf("timer is %s, %d ticks since last init\n",
		 timer_on ? "on" : "off", ticks);
}

/*
 * This builds the cache of IO addresses that we use to speed
 * the look-up of IO devices by address.
 */
io_buildcache()
{
    struct io_dev *io_p;
    int i;

    /*
     * Build the IO address cache.
     */
    for (i = 0 ; i < IO_CACHE_SIZE ; i++) {
        io_cache[i] = &io_dev_tab[0];
        for (io_p = &io_dev_tab[0] ; io_p != last ; io_p++) {
            if (io_p->size > 0 && i == ((io_p->l_addr >> 12) & 0xff)) {
                io_cache[i] = io_p;
                break;
            }
        }
    }
}

/*
 * This is called by sim_init to initialize the IO device simulators.
 * We call the initialization function for each entry in the IO table.
 */
void
io_init()
{
    struct io_dev *io_p;
    int tracelen;

    if (!io_trace_buf) {
        io_trace_buf_size = varvalue(SIMTRACE);
        if (io_trace_buf_size != 0) {
            if (io_trace_buf_size < 1 || io_trace_buf_size > 10000000) {
    
                sim_printf("$%s = %d, but should be in 1..10000000\n", 
                            SIMTRACE, io_trace_buf_size);
    
                sim_printf("using %d instead.\n", IO_TRACE_BUF_DEFAULT_SIZE);
                io_trace_buf_size = IO_TRACE_BUF_DEFAULT_SIZE;
                setvar(SIMTRACE, io_trace_buf_size);
            }
        } else {
            io_trace_buf_size = IO_TRACE_BUF_DEFAULT_SIZE;
            setvar(SIMTRACE, io_trace_buf_size);
        }
        io_trace_buf = (struct io_trace *)
                              sbrk(io_trace_buf_size * sizeof(struct io_trace));
    }
    io_trace = io_trace_buf;
    io_first_trace = io_trace;
    io_last_trace = &io_trace_buf[io_trace_buf_size - 1];
    io_trace_vals = 0;

    for (io_p = &io_dev_tab[0] ; io_p != last ; io_p++) {
        io_p->cnt = 0;
        (*io_p->init)(0);
    }

    io_buildcache();
}


/*
 * This is called to restore the state of the IO simulators after
 * reading a checkpoin file.
 */
void
sim_io_restore()
{
    struct io_dev *io_p;

    for (io_p = &io_dev_tab[0] ; io_p != last ; io_p++) {
        if (io_p->restore) {
            (*io_p->init)(1);
        }
    }

}

/*
 * This function changes the address and length of an IO device
 * to be changed while the simulator is running.
 * The simulator is named by its operation function, a point to
 * this function is passed as the first parameter.
 *
 * A 0 is returned if the device was found, a -1 is returned 
 * otherwise.
 */
int
io_change_addr(operation_function, new_address, new_length)
    int (*operation_function)();
    u_long new_address;
    u_long new_length;
{
    struct io_dev *io_p;

    for (io_p = &io_dev_tab[0] ; io_p != last ; io_p++) {
        if (io_p->operation == operation_function) {
            io_p->l_addr = new_address;
            io_p->size = new_length;
            if (new_length > 0) {
                io_buildcache();
            }
            return 0;
        }
    }
    return -1;
}

/*
 * This prints the names and addresses of all the simulated devices
 * in the device table.
 */
void
sim_io_print_devices(dev)
{
    struct io_dev *io_p;
    int index = 1;

    for (io_p = &io_dev_tab[0] ; io_p < last ; io_p++, index++) {
        if (dev == index || (io_p->size > 0 && dev == 0)) {
            QUIT;
            if (io_p->size > 0 && io_p->l_addr > 0) {
                sim_printf("[%2d] [%6d] from 0x%08x to 0x%08x is the %s\n", 
                        index, io_p->cnt, io_p->l_addr, 
                        io_p->l_addr + io_p->size - 1, io_p->name);
            } else {
                sim_printf("[%2d] [%6d] [disabled] is the %s\n", 
                        index, io_p->cnt,  io_p->name);
            }
        }
    }
}

/*
 * Called when there is a load, store, or xmem to/from a physical address
 * for which there is no simulated memory.  We return 0 if the operation
 * was successful and the exception code for a data access fault otherwise.
 */
int
io_operation(physical_address, value_ptr, size, mem_op_type, override)
    u_long physical_address;
    u_long *value_ptr;
    u_long size;
    u_long mem_op_type;
    int      override;
{
    u_long address_offset;
    struct io_dev *io_p;
    int exception_code;
    int count;
    struct io_trace *local_trace_ptr;

    io_p = io_cache[IO_HASH(physical_address)];
    for (count = 0 ; count < IO_DEV_TAB_SIZE ; count++) {
        if (io_p->l_addr <= physical_address &&
            physical_address < io_p->l_addr + io_p->size) {

            if (mem_op_type == ST   || 
                mem_op_type == XMEM || 
                mem_op_type == XMEM_U) {
                    flush_io_decoded_list_entry(physical_address);
            }

            address_offset = physical_address - io_p->l_addr;

            local_trace_ptr = io_trace;
            io_trace++;
            if (io_trace > io_last_trace) {
                io_trace = io_first_trace;
            }
            if (io_trace_vals < io_trace_buf_size) {
                io_trace_vals++;
            }
            local_trace_ptr->addr = physical_address;
            local_trace_ptr->io_ptr = io_p;
            local_trace_ptr->ip = ip;
            local_trace_ptr->mem_op_type = mem_op_type;
            local_trace_ptr->size = size;
            if (mem_op_type == ST) {
                local_trace_ptr->value = *value_ptr;
            }

            io_p->cnt++;
            local_trace_ptr->cnt = io_p->cnt;
            exception_code = (*io_p->operation)
                    (address_offset, value_ptr, size, mem_op_type, override);

            if (mem_op_type != ST) {
                local_trace_ptr->value = *value_ptr;
            }

            return exception_code;
        }
        io_p++;
        if (io_p == last) {
            io_p = &io_dev_tab[0];
        }
    }
    cmmu_set_bus_error(physical_address);
    return E_DACC;
}

/*
 * Called when there user wishes to see the IO device's state in
 * a convenient form.  The parameter can either be a small integer
 * that indexes the IO table or the physical address of the device.
 */
void
sim_io_print(physical_address)
    u_long physical_address;
{
    struct io_dev *io_p;

    if (physical_address <= IO_DEV_TAB_SIZE) {
        io_p = &io_dev_tab[physical_address - 1];
        if (io_p->size == 0) {
            sim_printf("%s is not active\n", io_p->name);
            return;
        }
        physical_address = io_p->l_addr;
    }

    for (io_p = &io_dev_tab[0] ; io_p < last ; io_p++) {
        QUIT;
        if (io_p->l_addr <= physical_address &&
            physical_address < io_p->l_addr + io_p->size) {

            (*io_p->print)();
            return;
        }
    }
    sim_printf("no device responds to 0x%08x.\n", physical_address);
}

/*
 * Called when the user wants to see a IO transaction history.
 */
void
sim_io_trace(physical_address, number_to_show)
    u_long physical_address;
    u_long number_to_show;
{
    struct io_trace *t;
    struct io_dev *dev_to_display;
    u_long i;

    static char size_names[] = { '0', 'b', 'h', '3',
                            ' ', '5', '6', '7', 'd' };

    static char *op_names[] = { "inv ", "ld", "st", 
                                "xmem", "ld", 
                                "xmem" };

    static int size_widths[] = { 8, 2, 4, 8, 8, 8, 8, 8 };

    if (number_to_show <= 0) {
        return;
    }

    if (physical_address <= IO_DEV_TAB_SIZE) {
        dev_to_display = &io_dev_tab[physical_address-1];
    } else {
        dev_to_display = 0;
    }

    t = io_trace - 1;

    for (i = 0 ; i < io_trace_vals ; i++) {
        QUIT;
        if (t < io_first_trace) {
            t = io_last_trace;
        }

        check_pointer(t);
        check_pointer(t->io_ptr);
        assert(io_first_trace <= t && t <= io_last_trace);

        /*
         * We display the trace record if we are displaying all the
         * records for a given device and this record is from that
         * device.  If we are displaying by addressed IO location
         * and the address in the record matches that passed we
         * we display the record.  And if we are displaying every
         * thing we display the record.
         */
        if (physical_address == 0 ||
            t->io_ptr == dev_to_display ||
            t->addr == physical_address) {
            int op = t->mem_op_type;

            sim_printf(
        "%6d: ip=0x%08x  %s%c%c%c  data=0x%08x addr=0x%08x  %s+0x%x\n",
                    t->cnt,
                    t->ip, 
                    op_names[op], 
                    t->size == WORD ? ' ' : '.',
                    size_names[t->size],
                    (op == LD_U || op == XMEM_U) && t->size < WORD ? 'u' : ' ',
                    t->value,
                    t->addr,
                    t->io_ptr->name,
                    t->addr - t->io_ptr->l_addr);

            if (--number_to_show == 0) {
                return;
            }
        }
        t--;
    }
}

/*
 * true if we are in trace-execution mode (very slow).
 */
int tracing;

/*
 * Points to the string to use as the filename of the execution trace
 * file.
 */
static char *simetrace;

/*
 * This is a psuedo-device, when stored to in its first location
 * (offset 0) it causes the simulator to return to the front 
 * end after the next branch instruction.
 *
 * The location at offset 4 controls execution tracing.
 */
void special_init(restore) 
{
    if (restore) {
        if (tracing) {
            open_trace_file(simetrace);
        }
    } else {
        tracing = 0;
    }
}


/*
 * This implements both the return-to-front-end device and the
 * execution-trace control flag device.
 */
int
special_operation(address_offset, reg_ptr, size, mem_op_type, override)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    int    override;
{
    if (mem_op_type != ST) {
        sim_printf("special_operation: only stores are supported.\n");
        return E_DACC;
    } 

    if (size != WORD) {
        sim_printf("special_operation: only word size is supported.\n");
        return E_DACC;
    }

    switch (address_offset) {
        case 0:
            sim_interrupt_flag |= INT_INTERNAL;
            break;

        case 4:
            tracing = *reg_ptr;
            if (tracing) {
                simetrace = SIMETRACE;
                if (!simetrace) {
                    sim_printf(
                   "special_operation: tracing turned on, but %s is not set.\n",
                                                              SIMETRACE);
                    tracing = 0;
                } else {
                    open_trace_file(simetrace);
                }
            } else {
                close_trace_file();
            }
            break;
    }

    return E_NONE;
}

void special_print() 
{
    sim_printf("tracing is %s\n", tracing ? "on" : "off");
}
