/* Include file for remote.c, remcmd.c, and motomode.c
   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/remote.h,v 1.8 91/01/13 23:51:45 robertb Exp $ */

#ifndef _JBLEN
#include <setjmp.h>
#endif

#ifndef _TYPE_H
#include <sys/types.h>
#endif

#define boolean int
#define true 1
#define false 0
#define nil 0

#define REMOTE_ENV_BAUD		"REMOTEBAUD"

#define CAUGHT_EXCEPTION 1
#define CAUGHT_BREAKPOINT 2

#define M_USER 0
#define M_SUPERVISOR 1
#define M_CURMODE 2

/* Maximum number of processors.  */
#define MAX_PROCESSORS  4

extern boolean resetting_remote;
extern boolean nmiing_remote;
extern boolean remote_errno;
extern boolean downloaded;
extern boolean simulator;		/* True => we are using the 88k sim */
extern int stop_cause;
extern int selected_processor;

#define transmitting    1
#define receiving       2
#define PAGESIZE (4096)

typedef jmp_buf *timeout_action_t;
#define	ta_none		((timeout_action_t)0)
#define	ta_warning	((timeout_action_t)1)

/* Points to setjmp/longjmp buffer used by reset_remote().  Code in motomode.c
   longjmp's to this in one case when resetting_remote is true. */

extern int *reset_env;

/* We longjmp to this when the user types control-C. */

extern jmp_buf controlc_env;

#define	C_RESET			(0x81)
#define	C_NMI			(0x82)

#define	COMMAND_PREFIX1	(0x83)
#define	COMMAND_PREFIX2	(0x84)

/*
 * Commands that the host sends to the monitor.
 */
#define C_NONE			(-1)
#define	C_DOWNLOAD		(0x91)
#define	C_UPLOAD		(0x92)
#define	C_CHECKSUM		(0x93)
#define	C_END			(0x94)
#define	C_GO			(0x95)

#define	C_DATASTART		(0x96)
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
#define C_COPYMEM		(0xa4)
#define C_UPLOADUSER		(0xa5)
#define C_UPLOADNOINITUSER	(0xa6)
#define C_DOWNLOADUSER		(0xa7)
#define C_EXPECTEDTOKEN		(0xa8)
#define C_COMMERR		(0xa9)
#define	C_COMPRESSERR		(0xaa)
#define	C_BADCOMMAND		(0xab)
#define C_188SYSCALL		(0xac)

extern void pass_siginthandler();		/* In motomode.c */
extern u_char get_remote_char();
extern void put_remote_char();
extern void pass_command();
extern boolean passthroughmode;
extern void setbaud();

extern void send_word();
extern void send_short();
extern void send_byte();

extern unsigned get_char();
extern unsigned get_byte();
extern unsigned get_word();

extern void remote_zeromem();
extern void init_checksum();
extern void verify_checksum();
extern void send_checksum();
extern u_long checksum;
extern void send_command();
extern void do_pass_command();
extern boolean passthroughmode;
extern void setbaud();
extern char remote_tty_name[];

extern int remote_fd;
extern int current_baud;

extern void splhi();
extern void spllo();
extern void setvar();
extern char remote_tty_name[];
extern void invalidate_mem_cache();
extern void send_reset_signal();
extern void send_interrupt_signal();

/* Passed to target memory read/write routines to say whether we want
   the address to be interpreted in user or kernel space. */

#define M_USER 0
#define M_SUPERVISOR 1
#define M_CURMODE 2

extern u_char read_remote_b();
extern u_short read_remote_h();
extern u_long read_remote_w();
extern void write_remote_b();
extern void write_remote_h();
extern void write_remote_w();
extern void init_globals();
extern void init_buffers();

/* We keep track of bytes sent to and received from the target in these
   structures.  This supports the lastr and lastt commands. */

struct buffer {
  u_char *data;
  int    head, size; };

extern struct buffer *receive_buffer, *transmit_buffer;

/* We store the last BUFFERSIZE (or $buffersize, if it is set) lines that
   were or would have been displayed by having $traceremote set. This
   supports the lasta command. */

extern char **text_buffer_base, **text_buffer;
extern int text_buffer_index, text_buffer_size;

extern u_long bytes_transmitted;
extern u_long total_bytes_transmitted;
extern u_long bytes_received;
extern u_long total_bytes_received;
extern double start_time;
extern double end_time;
extern u_long total_bytes_downloaded;
extern char *exception_name();
extern boolean wait_for_exception();

extern u_long mon_compress_table_addr;	/* Only sent by XD-88 monitor */
extern u_long mon_register_area_addr[];
extern u_long mon_single_step_trap_addr;
extern u_long mon_version_addr;
extern u_long mon_report_comm_errors_addr;
extern u_char cpu_enabled[MAX_PROCESSORS];

extern u_long bytes_transmitted;

extern boolean target_running;
extern void rerr();
extern void dacc_rerr();
extern boolean ignore_errors; /* True while we are trying to interrupt target*/
extern char *comm_name();
extern u_long varvalue();
extern void reset_remote();
extern void handle_target_rerr();
extern void traceremote();

#define add_to_checksum(b) { register unsigned msb; \
  msb = (checksum += (b)) >> 31 ; checksum <<= 1; checksum |= msb; }

extern void flush_tty_line();
extern u_long forced_read_remote();
extern void get_monitor_addresses();
extern boolean dread_remote();
extern void check_for_delay_slot_after_interrupt();
extern void print_comm_statistics();

/* These are used by functions outside of the remote group of files. */

extern u_long remote_read_register();
extern void remote_write_register();
extern void init_globals();
extern int sim_errno;
