/* Commands for cross-debugging

   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/remcmd.c,v 1.12 91/01/13 23:54:33 robertb Exp $
   $Locker:  $

This file is part of GDB. */


#include <ctype.h>
#include <stdio.h>
#include <sys/fcntl.h>	/* For O_RDONLY */
#include <sys/time.h>

#include "defs.h"
#include "param.h"
#include "remote.h"
#include "ui.h"
#include "assert.h"

static CORE_ADDR next_address;
static u_long checksum_of();
static boolean dl_remote();
static void check_remote();
static void check_sim();

char objname[200];              /* Filename of executable to download */
CORE_ADDR bss_start, entry_point;
u_long bss_size;

/* Number of the currently-selected processor */
int selected_processor;

/*
 * This copies a block of target memory
 */
static void remote_copymem(src_addr, dst_addr, length)
    u_long src_addr, dst_addr;
    int length;
{
  if (simulator) {
    sim_copymem(src_addr, dst_addr, length);
    return;
  }
    splhi();
    invalidate_mem_cache();
    init_checksum();
    send_command(C_COPYMEM);
    send_word(src_addr);
    send_word(dst_addr);
    send_word(length);
    send_word(!varvalue("noparityinit"));
    traceremote("Copying memory 0x%x - 0x%x to 0x%x\n", 
                                 src_addr, src_addr + length - 1, dst_addr);
    send_command(C_END);
    send_checksum();
    if (wait_for_token(C_ACK, C_NONE, ta_warning, (int)(2+length/50000)) !=C_ACK) {
        rerr("Didn't find C_ACK in remote_copymem");
    }
    spllo();
}
/* This searches target memory.  If the pattern is found the address of
   the matching location is returned, otherwise -1 is returned. */

u_long remote_searchmem(addr, length, pattern1, pattern2, mask1, mask2, stride)
    u_long addr, pattern1, pattern2, mask1, mask2;
    long length, stride;
{
  u_long search_addr;

  if ((mask1 & pattern1) != pattern1) {
    rerr("Mask of 0x%x, pattern of 0x%x can not match any word",
                                                              mask1, pattern1);
  }
  if ((mask2 & pattern2) != pattern2) {
    rerr("Mask of 0x%x, pattern of 0x%x can not match any word", 
                                                              mask2, pattern2);
  }
  if (simulator) {
    extern u_long sim_searchmem();
    if (pattern2 != 0 || mask2 != 0) {
      rerr("Simulator's memory seach routine can only do 32-bit patterns");
    }
    return sim_searchmem(addr, length, pattern1, mask1, stride);
  }
  init_checksum();
  splhi();
  send_command(C_SEARCH);
  send_word(addr);
  send_word(length);
  send_word(stride);
  send_word(pattern1);
  send_word(pattern2);
  send_word(mask1);
  send_word(mask2);
  send_word(!varvalue("noparityinit"));
  traceremote("Searching memory adr=0x%x, len=%d, stride=%d, p=0x%x, m=0x%x\n",
               addr, length, stride, pattern1, mask1);
  send_command(C_END);
  send_checksum();

  init_checksum();
  switch (wait_for_token(C_DATASTART, 
                         C_DATAERROR, 
                         ta_warning, 
                         (2+length/50000)) ) {
    case C_DATASTART:
      search_addr = get_word();
      if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
        rerr("Didn't find C_DATAEND in search");
      }
      verify_checksum();
      break;

    case C_DATAERROR:
      dacc_rerr("searching memory");
      break;

    default:
      rerr("Didn't get C_DATASTART from target");
        break;
    }
    spllo();
    return search_addr;
}

/*
 * This displays some statistics.
 */
void print_comm_statistics()
{
    double elapsed_time;
    double bytes_per_second;
    double effective_bps;

    if (varvalue("tracedownload")) {

        ui_fprintf(stdout, "Total of %d bytes received\n", 
                total_bytes_received + bytes_received);

        ui_fprintf(stdout, "Total of %d bytes transmitted\n", 
                total_bytes_transmitted + bytes_transmitted);

        elapsed_time = end_time - start_time;

        if (elapsed_time > 0.0) {
            bytes_per_second = (double)total_bytes_transmitted / elapsed_time;
            effective_bps = (double)total_bytes_downloaded / elapsed_time;

            ui_fprintf(stdout, "\n");
            ui_fprintf(stdout, 
           "Elapsed time of last download = %8.2f\n", elapsed_time);

            ui_fprintf(stdout, 
           "Actual transfer rate of last download %8.2f\n", bytes_per_second);

            ui_fprintf(stdout, 
           "Effective transfer rate of last download = %8.2f\n", effective_bps);
        }
    }
}

/*
 * Download the data pointed to by the first parameter to
 * the target.  We first find the checksum of each 4K block
 * in the the target by doing a checksum command to the
 * target.  Then we checksum each 4K block to be downloaded.
 * Only blocks whose checksums don't match are downloaded.
 */
static void download(object_fd, start_addr, length)
    int object_fd;
    u_long start_addr;
    u_long length;
{
    u_long *checksums;
    u_long blocks;
    u_long i;
    u_long addr;

    assert((length & 3) == 0);

  if (simulator) {
    sim_readfile(object_fd, start_addr, length);
    return;
  }

    total_bytes_transmitted += bytes_transmitted;
    total_bytes_received += bytes_received;
    bytes_transmitted = 0;
    bytes_received = 0;

    /*
     * Allocate an array of booleans to store the flag for each
     * block.
     */
    blocks = (length + (PAGESIZE - 1)) / PAGESIZE;
    checksums = (u_long *)malloc((unsigned)blocks * sizeof(unsigned *));

    /*
     * Send the checksum command to the target.
     */
    if (varvalue("tracedownload")) {
        ui_fprintf(stderr, 
           "Asking target for checksums on %d blocks starting at 0x%x\n",
                                                blocks, start_addr);
    }

    traceremote("Checksumming 0x%x bytes starting at 0x%x\n",
                            length, start_addr);
    init_checksum();
    splhi();
    send_command(varvalue("noparityinit") ? C_CHECKSUMNOINIT : C_CHECKSUM);
    send_word(start_addr);
    send_word(length);
    send_command(C_END);
    send_checksum();

    switch (wait_for_token(C_DATASTART, C_DATAERROR, ta_warning, 2)) {
        case C_DATASTART:
            break;

        case C_DATAERROR:
            dacc_rerr("checksumming before download");

        default:
            rerr("Didn't find C_DATASTART in download");
    }
    for (i = 0 ; i < blocks ; i++) {
        u_long block_size;

        block_size = (i == (blocks - 1)) ? (length % PAGESIZE) : PAGESIZE;
        if (block_size == 0) {
            block_size = PAGESIZE;
        }
        checksums[i] = get_word();
    }

    if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
        rerr("Didn't find C_DATAEND in download");
    }
    spllo();

    addr = start_addr;
    invalidate_mem_cache();
    for (i = 0 ; i < blocks ; i++) {
        u_long block_size;
        u_long *wp, filebuffer[PAGESIZE/sizeof(u_long)];

        block_size = (i == (blocks - 1)) ? (length % PAGESIZE) : PAGESIZE;
        if (block_size == 0) {
            block_size = PAGESIZE;
        }
        QUIT;
        wp = filebuffer;
        if (read(object_fd, &wp[0], block_size) != block_size) {
            rerr("Error reading block %d from %s", i, objname);
        }

        if (checksums[i] != checksum_of(wp, block_size)) {
            unsigned cnt;

            if (varvalue("tracedownload")) {
                ui_fprintf(stderr, "Downloading %d bytes to 0x%x\n",
                              block_size, addr);
            } else {
                ui_fprintf(stderr, "*");
            }
            init_checksum();
            splhi();
            send_command(C_DOWNLOAD);
            send_word(addr);
            send_word(block_size);
            for (cnt = 0 ; cnt < block_size ; cnt += 4) {
                send_word(*wp++);
            }
            send_command(C_END);
            send_checksum();
            switch (wait_for_token(C_ACK, C_DATAERROR, ta_warning, 2)) {
                case C_ACK:
                    break;

                case C_DATAERROR:
                    dacc_rerr("writing target memory");
                  
                default:
                    rerr("Didn't find C_ACK in download");
            }
            spllo();
        } else {
            ui_fprintf(stderr, ".");
            wp += block_size / sizeof(unsigned *);
        }
        addr += block_size;
    }
    if (varvalue("tracedownload")) {
        ui_fprintf(stderr, "%d bytes sent to target.\n", bytes_transmitted);
        ui_fprintf(stderr, "%d bytes received from target.\n", bytes_received);
    } else {
        ui_fprintf(stderr, "\n");
    }
    free((char *)checksums);
}

/*
 * This returns the checksum of the array of words pointed to
 * by the first parameter.  'length' is in bytes, it should be
 * a multiple of 4.
 */
static u_long checksum_of(wp, length)
    register u_long *wp;
    register u_long length;
{
    assert((length & 3) == 0);

    init_checksum();
    while (length != 0) {
        add_to_checksum(*wp++);
        length -= 4;
    }
    return checksum;
}

/*
 * Allows the user to explicitly zero a block of memory.
 */
static void bzero_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the bzero command.\n");
  }
  if (exp) {
    u_long length;
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    space_index = (char *) index (exp, ' ');
    if (space_index == (char *)0) {
      rerr("Usage: bzero <address> <length>");
    }
    *space_index = '\0';
    next_address = parse_and_eval_address (exp);
    length = parse_and_eval_address (space_index + 1);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
    remote_zeromem(next_address, length);
  }
}

/*
 * Allows the user to explicitly copy a block of memory.
 */
static void bcopy_command(exp, from_tty)
    char *exp;
{
  if (exp) {
    u_long length, source_address, destination_address;
    char *first_space_index;
    char *second_space_index;

  if (!remote_debugging) {
    rerr("You must attach the target before using the bcopy command.\n");
  }
    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    first_space_index = (char *) index (exp, ' ');
    if (first_space_index == (char *)0) {
      rerr("Usage: bcopy <source-address> <destination-address> <length>");
    }
    *first_space_index = '\0';
    source_address = parse_and_eval_address (exp);

    second_space_index = (char *) index (first_space_index + 1, ' ');
    if (second_space_index == (char *)0) {
      rerr("Usage: bcopy <source-address> <destination-address> <length>");
    }
    *second_space_index = '\0';
    destination_address = parse_and_eval_address (first_space_index + 1);

    length = parse_and_eval_address (second_space_index + 1);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
    remote_copymem(source_address, destination_address, length);
  }
}

/* This command allows the user to search target memory for a pattern
   word.  The form is:  sw <address> <length> <pattern>  */

static void sw_command(exp, from_tty)
    char *exp;
{
  static u_long starting_address, pattern, mask, stride;
  static long length;
  u_long match_address;

  if (!remote_debugging) {
    rerr("You must attach the target before using sw.\n");
  }
  if (exp && *exp != '\0') {
    char *first_space_index;
    char *second_space_index;

    first_space_index = (char *) index (exp, ' ');
    if (first_space_index == (char *)0) {
      rerr("Usage: sw <address> <length> <pattern>");
    }
    *first_space_index = '\0';
    starting_address = parse_and_eval_address (exp);

    second_space_index = (char *) index (first_space_index + 1, ' ');
    if (second_space_index == (char *)0) {
      rerr("Usage: sw <address> <length> <pattern>");
    }
    *second_space_index = '\0';
    length = parse_and_eval_address (first_space_index + 1);

    pattern = parse_and_eval_address (second_space_index + 1);
    *exp = '\0';
  }

  mask = varvalue("mask");
  if (mask == 0) {
    mask = 0xffffffff;
    setvar("mask", mask);
    ui_fprintf(stdout, "Setting $mask to default value of 0x%x\n", mask);
  }
  stride = varvalue("stride");
  if (stride == 0) {
    stride = 4;
    setvar("stride", stride);
    ui_fprintf(stdout, "Setting $stride to default value of %d\n", stride);
  }
  ui_fprintf(stdout, 
      "Searching 0x%x to 0x%x for 0x%x, mask=0x%x, stride=%d bytes\n",
      starting_address, starting_address + length - stride, pattern, mask, stride);

  match_address = remote_searchmem(starting_address, 
                                    length, 
                                    pattern, 0, 
                                    mask, 0, 
                                    stride);
  if (match_address == 0xffffffff) {
    ui_fprintf(stdout, "No match\n");
    starting_address = starting_address + length;
    length = 0;
  } else {
    ui_fprintf(stdout, "Match at 0x%x: 0x%x\n", 
                       match_address,
                       read_remote_w(match_address, M_SUPERVISOR));
    length -= (match_address - starting_address);
    length = (length < 0) ? 0 : length;
  }
}

static void terror_command(exp, from_tty)
    char *exp;
{
  static terror_flag = 0;

  if (!remote_debugging) {
    rerr("You must attach the target before using sw.\n");
  }
  terror_flag = !terror_flag;
  write_remote_w(mon_report_comm_errors_addr, terror_flag, M_SUPERVISOR);
  ui_fprintf(stdout, 
         "Communication error reporting by the target monitor is now %s\n", 
          terror_flag ? "on" : "off");
} 

/* Does the 'flush' command.  */
static void flush_command()
{
  if (remote_fd != -1) {
    flush_tty_line();
  }
  invalidate_mem_cache();
#ifdef CLOSE_OPEN_A_GOOD_IDEA
  close_control_port();		/* Will be reopen automatically */
  close_debug_port();
  open_debug_port();
#endif
}

/* Print the contents of the passed buffer. */

static void do_last_command(buffer, str)
  struct buffer *buffer;
  char *str;
{
  u_char prev_char = 0;
  int i,len;
  if (str == (char *)0 || *str == '\0') {
    len = buffer->size;
  } else {
    len = atoi(str);
    if (len > buffer->size) {
      len = buffer->size;
      ui_fprintf(stderr, "Not that many bytes in the buffer\n");
    }
  }
  
  for (i = 0 ; i < len ; i++) {
    u_char c = buffer->data[(i + buffer->head) % buffer->size];
    printf("%s ", comm_name(c, prev_char));
    prev_char = c;
  }
  printf("\n");
}

/* Display recent lines of remote buffer. */

static void lasta_command(str)
  char *str;
{
  char **p;
  int len;

  if (text_buffer_base == (char **)0) {
    rerr("Cross-debugging trace buffer not initialized");
  }
  if (str == (char *)0 || *str == '\0') {
    len = text_buffer_size > 10 ? 10 : text_buffer_size;
  } else {
    len = atoi(str);
  }
  if (len > text_buffer_size) {
    ui_fprintf(stderr, "Not that many lines in the text buffer\n");
    len = text_buffer_size;
  }
  p = (text_buffer == text_buffer_base) ? 
                                text_buffer_base + text_buffer_size - 1 : 
                                text_buffer - 1;

  ui_fprintf(stdout, "Last %d trace records in reverse time order\n", len);
  for (; *p != (char *)0 && len > 0 ; len--) {
    ui_fprintf(stdout, "%s", *p);
    p--;
    if (p == text_buffer_base - 1) {
      p = text_buffer_base + text_buffer_size - 1;
    }
  }
}

/* Show the last $buffersize characters sent to the target. */

static void lastt_command(str)
  char *str;
{
  do_last_command(transmit_buffer, str);
}

/* Show the last $buffersize characters received from the target. */

static void lastr_command(str)
  char *str;
{
  do_last_command(receive_buffer, str);
}

/* Reset the target.  We may have done this after the target went out
   to lunch, make sure that we've removed any breakpoints.  */ 
static void init_command()
{
  if (simulator) {
    do_sim_reset();
  } else {
    if (remote_fd != -1) {
      reset_remote();
    }
  }
  flush_cached_frames();
  remove_breakpoints ();
  remove_step_breakpoint ();
}

/* Interrupt the target.  */ 
static void interrupt_command()
{
  if (simulator) {
    ui_fprintf(stderr, "This command has no effect on the simulator\n");
  } else {
    if (remote_fd != -1) {
      send_interrupt_signal();
    }
  }
}

/* Select a processor */
void pselect_command(exp, from_tty)
  char *exp;
{
  static char buf[100];
  if (exp) {
    int i = parse_and_eval_address (exp);
    if (i >= 0 & i < MAX_PROCESSORS) {
      if (simulator) {
        sim_select_processor(i);
      } else {
        remote_select_processor(i, false);
      }
      sprintf(buf, "[%d] (gdb)", i);
      set_either_prompt_command (0, buf);
      flush_cached_frames();
      set_first_frame();
    } else {
      ui_fprintf(stderr, "Processor number is out of range\n");
    }
  } else {
    ui_fprintf(stdout, "Currently selected processor is number %d\n",
    selected_processor);
  }
}

/* Enable a processor.  If no argument is given to this command
   it displays the enabled processors. */
void penable_command(exp, from_tty)
  char *exp;
{
  int i;
  if (exp) {
    if (strcmp(exp, "all") == 0) {
      for (i = 0 ; i < MAX_PROCESSORS ; i++) {
        cpu_enabled[i] = 1;
      }
    } else {
      i = parse_and_eval_address (exp);
      if (i >= 0 & i < MAX_PROCESSORS) {
        cpu_enabled[i] = 1;
      } else {
        ui_fprintf(stderr, "Processor number is out of range\n");
      }
    }
  } else {
    ui_fprintf(stdout, "Enabled processors: ");
    for (i = 0 ; i < MAX_PROCESSORS ; i++) {
      if (cpu_enabled[i]) {
        ui_fprintf(stdout, "%d ", i);
      }
    }
    ui_fprintf(stdout, "\n");
  }
}

/* Disable a processor or show disabled processors (if no argument is
   given) */
void pdisable_command(exp, from_tty)
  char *exp;
{
  int i;
  if (exp) {
    if (strcmp(exp, "all") == 0) {
      for (i = 0 ; i < MAX_PROCESSORS ; i++) {
        cpu_enabled[i] = 0;
      }
    } else {
      i = parse_and_eval_address (exp);
      if (i >= 0 & i < MAX_PROCESSORS) {
        cpu_enabled[i] = 0;
      } else {
        ui_fprintf(stderr, "Processor number is out of range\n");
      }
    }
  } else {
    ui_fprintf(stderr, "Disabled processors: ");
    for (i = 0 ; i < MAX_PROCESSORS ; i++) {
      if (!cpu_enabled[i]) {
        ui_fprintf(stdout, "%d ", i);
      }
    }
    ui_fprintf(stdout, "\n");
  }
}

/* Display the status of the processors. */
pstat_command(exp, from_tty)
{
  int i;
  if (!remote_debugging) {
    rerr("You must attach the target before useing the pstat command.");
  }
  for (i = 0 ; i < MAX_PROCESSORS ; i++) {
    int exception_code = processor_exception_code(i);
    ui_fprintf(stdout, "[%d] %s %s  last exception=%s\n", 
                        i, 
                        cpu_enabled[i] ? "enabled " : "disabled",
                        in_monitor(i) ? "in monitor" : "running   ",
                        exception_name(exception_code));
  }
}

/* Print many of the useful 88100 registers.  This overlaps with "info regs."
   We print the Data Memory Unit registers if they are valid. */
static void regs_command()
{
  int j, i;

  for (i = 0 ; i < 8 ; i++) {
    for (j = i ; j < i + 25 ; j += 8) {
      if (i == 0 && j == 0) {
        ui_fprintf(stdout, "                   ");
      } else {
        ui_fprintf(stdout, "r%-3d 0x%08x    ", j, read_hard_register(j));
      }
    }
    ui_fprintf(stdout, "\n");
  }

  ui_fprintf(stdout, "sxip 0x%08x    ", read_hard_register(SXIP_REGNUM));
  ui_fprintf(stdout, "snip 0x%08x    ", read_hard_register(SNIP_REGNUM));
  ui_fprintf(stdout, "sfip 0x%08x    ", read_hard_register(SFIP_REGNUM));
  if (simulator) {
    ui_fprintf(stdout, "nip  0x%08x", read_hard_register(SYNTH_PC_REGNUM));
  }
  ui_putchar('\n');

  if (remote_debugging) {
    ui_fprintf(stdout, "vbr  0x%08x    ", remote_read_register(VBR_REGNUM));
    ui_fprintf(stdout, "psr  0x%08x    ", remote_read_register(PSR_REGNUM));

    if (simulator) {
      ui_fprintf(stdout, "epsr 0x%08x    ", remote_read_register(EPSR_REGNUM));
    }
    /* -rcb 6/90 
    if (!varvalue("motomode")) {
      ui_fprintf(stdout, "ceimr 0x%08x", remote_read_register(CEIMR_REGNUM));
    }
     */

    ui_putchar('\n');
    ui_fprintf(stdout, "sr0  0x%08x    ", remote_read_register(SR0_REGNUM));
    ui_fprintf(stdout, "sr1  0x%08x    ", remote_read_register(SR1_REGNUM));
    ui_fprintf(stdout, "sr2  0x%08x    ", remote_read_register(SR2_REGNUM));
    ui_fprintf(stdout, "sr3  0x%08x    ", remote_read_register(SR3_REGNUM));
    ui_putchar('\n');
  
    if (remote_read_register(DMT0_REGNUM) & 1) {
      ui_fprintf(stdout, "dmt0 0x%08x    ", remote_read_register(DMT0_REGNUM));
      ui_fprintf(stdout, "dmd0 0x%08x    ", remote_read_register(DMD0_REGNUM));
      ui_fprintf(stdout, "dma0 0x%08x    ", remote_read_register(DMA0_REGNUM));
      ui_putchar('\n');
    }
  
    if (remote_read_register(DMT1_REGNUM) & 1) {
      ui_fprintf(stdout, "dmt1 0x%08x    ", remote_read_register(DMT1_REGNUM));
      ui_fprintf(stdout, "dmd1 0x%08x    ", remote_read_register(DMD1_REGNUM));
      ui_fprintf(stdout, "dma1 0x%08x    ", remote_read_register(DMA1_REGNUM));
      ui_putchar('\n');
    }
  
    if (remote_read_register(DMT2_REGNUM) & 1) {
      ui_fprintf(stdout, "dmt2 0x%08x    ", remote_read_register(DMT2_REGNUM));
      ui_fprintf(stdout, "dmd2 0x%08x    ", remote_read_register(DMD2_REGNUM));
      ui_fprintf(stdout, "dma2 0x%08x    ", remote_read_register(DMA2_REGNUM));
      ui_putchar('\n');
    }
  }
  if (simulator) {
    ui_fprintf(stdout, "come 0x%08x    ", 
                       remote_read_register(COMEFROM_REGNUM));
    ui_fprintf(stdout, "sbas 0x%08x    ", 
                       remote_read_register(STACKBASE_REGNUM));
  }
  ui_fprintf(stdout, "mbrk 0x%08x    ", remote_read_register(MEMBRK_REGNUM));
  if (simulator) {
    ui_fprintf(stdout, "rams 0x%08x",  remote_read_register(RAMSIZE_REGNUM));
  }
  ui_fprintf(stdout, "\n");
}
  
/*
 * This reads the code and data into simulator or remote memory.  It also
 * zero's the bss region.
 */
static void dl_command(exp, from_tty)
    char *exp;
{
  int f;
  extern CORE_ADDR text_start, text_end;
  extern CORE_ADDR exec_data_start, exec_data_end;
//  extern data_size, text_offset, exec_data_offset;
  extern text_offset, exec_data_offset;
    
  u_long text_size = text_end - text_start;
  u_long data_size = exec_data_end - exec_data_start;
  int i, save;

  if (!remote_debugging) {
    rerr("You must attach the target before using dl.\n");
  }
    remote_errno = false;
    f = open(objname, O_RDONLY, 0);
    if (f < 0) {
        rerr("can't open %s", objname);
    }

    if (!simulator && text_start < (varvalue("motomode") ? 0x10000 : 0x8000)) {
      rerr("Text starts too low (0x%x), would overwrite ROM's area", 
             text_start);
    }
    lseek(f, text_offset, 0);
    if (!dl_remote(f, text_start, text_size,
                      exec_data_start, data_size,
                      bss_start, bss_size)) {
        rerr("Error in download");
    } else {
        setvar("noparityinit", 1);
    }

  decode_instr_before_stepping(fetch_instruction(entry_point));  
  save = selected_processor;
  for (i = 0 ; i < MAX_PROCESSORS ; i++) {
    if (simulator) {
      sim_select_processor(i);
      remote_write_register(SYNTH_PC_REGNUM, entry_point);
    } else {
      remote_select_processor(i, false);
      remote_write_register(SNIP_REGNUM, entry_point | IP_VALID);
      remote_write_register(SFIP_REGNUM, (entry_point + 4) | IP_VALID);
    }
  }
  if (simulator) {
    sim_select_processor(save);
  } else {
    remote_select_processor(save, true);
  }
  if (close(f) < 0) {
    rerr("Error closing object file.\n");
  }
}

/*
 * This checks that the host's copy of the text and data matches that
 * in the target.
 */
static void checkdl_command(exp, from_tty)
  char *exp;
{
  int f;
  extern CORE_ADDR text_start, text_end;
  extern text_offset;
  
  u_long text_size = text_end - text_start;

  if (!remote_debugging) {
    rerr("You must attach the target before using the checkdl command.");
  }
  remote_errno = false;

  f = open(objname, O_RDONLY, 0);
  if (f < 0) {
    rerr("can't open %s", objname);
  }

  lseek(f, text_offset, 0);
  if (simulator) {
    check_sim(f, text_start, text_size);
  } else {
    check_remote(f, text_start, text_size);
  }

  if (close(f) < 0) {
    rerr("Error closing object file.\n");
  }
}

/*
 * Read a single byte, no more, no less, from the target.
 */
static void rb_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the rb command.");
  }
  if (exp) {
    if (*exp != '\0') {
      next_address = parse_and_eval_address (exp);

     /* Cause expression not to be there any more
         if this command is repeated with Newline.
         But don't clobber a user-defined command's definition.  */

      if (from_tty) {
        *exp = '\0';
      }
    }
    ui_fprintf(stdout, "0x%08x: 0x%02x\n", next_address, 
              forced_read_remote(next_address, sizeof(char), M_SUPERVISOR));
  }
}

/*
 * Read a single halfword, no more, no less, from the target.
 */
static void rh_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the rh command.");
  }
  if (exp) {
    if (*exp != '\0') {
      next_address = parse_and_eval_address (exp);

     /* Cause expression not to be there any more
         if this command is repeated with Newline.
         But don't clobber a user-defined command's definition.  */

      if (from_tty) {
        *exp = '\0';
      }
    }
    ui_fprintf(stdout, "0x%08x: 0x%02x\n", next_address, 
           forced_read_remote(next_address, sizeof(short), M_SUPERVISOR));
  }
}

/*
 * Read a single word, no more, no less, from the target.
 */
static void rw_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the rw command.");
  }
  if (exp) {
    if (*exp != '\0') {
      next_address = parse_and_eval_address (exp);

     /* Cause expression not to be there any more
         if this command is repeated with Newline.
         But don't clobber a user-defined command's definition.  */

      if (from_tty) {
        *exp = '\0';
      }
    }
    ui_fprintf(stdout, "0x%08x: 0x%02x\n", next_address, 
                forced_read_remote(next_address, sizeof(long), M_SUPERVISOR));
  }
}

/*
 * Write a single byte to target memory.
 */
static void wb_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the wb command.");
  }
  if (exp) {
    u_long data_to_write;
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    space_index = (char *) index (exp, ' ');
    if (space_index == (char *)0) {
      rerr("Usage: wb <address> <byte>");
    }
    *space_index = '\0';
    next_address = parse_and_eval_address (exp);
    data_to_write = parse_and_eval_address (space_index + 1);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
    write_remote_b(next_address, data_to_write, M_SUPERVISOR);
  }
}

/*
 * Write a halfword to target memory.
 */
static void wh_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the wh command.");
  }
  if (exp) {
    u_long data_to_write;
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    space_index = (char *) index (exp, ' ');
    if (space_index == (char *)0) {
      rerr("Usage: wh <address> <half-word>");
    }
    *space_index = '\0';
    next_address = parse_and_eval_address (exp);
    data_to_write = parse_and_eval_address (space_index + 1);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
    write_remote_h(next_address, data_to_write, M_SUPERVISOR);
  }
}

/*
 * Write a word that we read from standard input to target memory.
 */
static void ww_command(exp, from_tty)
    char *exp;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using the ww command.");
  }
  if (exp) {
    u_long data_to_write;
    char *space_index;

    if (*exp == '\0') {
      rerr("This command can not be repeated with a newline");
    }

    space_index = (char *) index (exp, ' ');
    if (space_index == (char *)0) {
      rerr("Usage: ww <address> <word>");
    }
    *space_index = '\0';
    next_address = parse_and_eval_address (exp);
    data_to_write = parse_and_eval_address (space_index + 1);

   /* Cause expression not to be there any more
      if this command is repeated with Newline.
      But don't clobber a user-defined command's definition.  */

    if (from_tty) {
      *exp = '\0';
    }
    write_remote_w(next_address, data_to_write, M_SUPERVISOR);
  }
}

static u_long data_cmmu_address;

/*
 * This finds a working CMMU and puts its base address in data_cmmu_address.
 */
static void findcmmu()
{
  int i;

  if (data_cmmu_address != 0) {
      return;
  }
  if (varvalue("motomode")) {
    data_cmmu_address = 0xfff6f000;
    return;
  }

  for (i = 0 ; i < 4 ; i++) {
    u_long sapr_address = 0xfff00000 + (i * PAGESIZE) + 0x200;
    u_long sapr = read_remote_w(sapr_address, M_SUPERVISOR);
    if (sapr != sapr_address) {
      data_cmmu_address = 0xfff00000 + (i * PAGESIZE);
      return;
    }
  }
  rerr("findcmmu(): couldn't find a working CMMU");
}

/* This is the cross-debugger's routine to translate kernel virtual
   addresses to physical addresses.  */
static boolean remote_v_to_p(addr, pa_ptr, usmode)
    u_long *pa_ptr;
    u_long addr;
    int    usmode;
{
    u_long  system_status_register;
    u_long  area_pointer = read_remote_w(data_cmmu_address + 
                                      (usmode ? 0x204 : 0x200), M_SUPERVISOR);
    findcmmu();
    if ((area_pointer & 1) == 0) {
        *pa_ptr = addr;
        return false;
    }

    /* Write the system address register of one of the data
       CMMU's with the address that we want to translate.  */
    write_remote_w(data_cmmu_address + 0xc, addr, M_SUPERVISOR);

    /* Write the system command register of one of the data
       CMMU's with the probe-user command.  */
    write_remote_w(data_cmmu_address + 4, 
                   (usmode == M_USER) ? 0x20 : 0x24, M_SUPERVISOR);

    /* Test the valid bit of the system status register to
       make sure that the translation was valid.  */
    system_status_register = read_remote_w(data_cmmu_address + 8, M_SUPERVISOR);
    if ((system_status_register & 1) == 0) {
        return true;
    }
    *pa_ptr = read_remote_w(data_cmmu_address + 0xc, M_SUPERVISOR);
    return false;
}

/* List of names of bits in the 88200's system status register.  */
char *ssr_fieldlist[] = {
    "valid",                 /* bit 0 */
    "batc",                  /* bit 1 */
    "write protected",       /* bit 2 */
    "used",                  /* bit 3 */
    "modified",              /* bit 4 */
    "",                      /* bit 5 */
    "cache inhibit",         /* bit 6 */
    "global",                /* bit 7 */
    "supervisor only",       /* bit 8 */
    "write through",         /* bit 9 */
    "", "", "", "",          /* bit 10 through 13 */
    "bus error" };           /* bit 14 */

/* Print the system status register.  */
static void print_ssr()
{
    int i;
    u_long system_status_register;

    findcmmu();

    system_status_register = read_remote_w(data_cmmu_address + 8, M_SUPERVISOR);

    ui_fprintf(stderr, "ssr: 0x%08X ", system_status_register);
    for (i = 0 ; i < sizeof(ssr_fieldlist)/sizeof(char *) ; i++) {
        if (system_status_register & (1 << i)) {
            ui_fprintf(stderr, "<%s>", ssr_fieldlist[i]);
        }
    }
    ui_fprintf(stderr, "\n");
}

char *apr_fieldlist[] = {
    "translation enable",    /* bit 0 */
    "",                      /* bit 1 */
    "",                      /* bit 2 */
    "",                      /* bit 3 */
    "",                      /* bit 4 */
    "",                      /* bit 5 */
    "cache inhibit",         /* bit 6 */
    "global",                /* bit 7 */
    "",                      /* bit 8 */
    "write through"          /* bit 9 */
};

/* Print the supervisor and user area pointers in one of the data CMMUs. */
static void print_apr()
{
    int i;
    u_long area_pointer;

    findcmmu();
    ui_fprintf(stderr, "supervisor area pointer: 0x%08X ", 
         area_pointer = read_remote_w(data_cmmu_address + 0x200, M_SUPERVISOR));
    for (i = 0 ; i < sizeof(apr_fieldlist)/sizeof(char *) ; i++) {
        if (area_pointer & (1 << i)) {
            ui_fprintf(stderr, "<%s>", apr_fieldlist[i]);
        }
    }
    ui_fprintf(stderr, "\n");

    ui_fprintf(stderr, "user area pointer: 0x%08X ", 
         area_pointer = read_remote_w(data_cmmu_address + 0x204, M_SUPERVISOR));
    for (i = 0 ; i < sizeof(apr_fieldlist)/sizeof(char *) ; i++) {
        if (area_pointer & (1 << i)) {
            ui_fprintf(stderr, "<%s>", apr_fieldlist[i]);
        }
    }
    ui_fprintf(stderr, "\n");
}

#ifdef	CMMU_BUG_FIXED
  /* As far as I can tell, there is a bug with the CMMU probe operation.
     The 88200 manual says that on a probe error, it writes the pfar and 
     pfsr with error information.  It doesn't seem to do this at all.
     These routines print those registers, so now we have no usr for
     these routines. :-( */

/* Print the P bus fault address register of one of the CMMUs */
static void print_pfar()
{
    u_long pbus_fault_address_register;

    findcmmu();
    pbus_fault_address_register = 
                       read_remote_w(data_cmmu_address + 0x10c, M_SUPERVISOR);
    ui_fprintf(stderr, "P bus fault address register: 0x%x\n", 
                                                  pbus_fault_address_register);
}

/* Print the P bus status register of one of the data CMMUs */
static void print_pfsr()
{
  static char *pfsr_names[8] = { 
    "Success", "Bus error", "Segment Fault", "Page Fault", 
    "Supervisor Violation", "Write Violation" };
  u_long pfsr;

  findcmmu();
  pfsr = read_remote_w(data_cmmu_address + 0x108, M_SUPERVISOR);
  ui_fprintf(stderr, "P bus fault status register: %s\n",
       pfsr_names[(pfsr >> 16) & 7]);
}
#endif

/* Called to probe kernel and user virtual addresses */
static void do_probe_command(exp, from_tty, usmode)
  char *exp;
{
  CORE_ADDR address_to_probe, physical_address;

  if (!remote_debugging) {
    rerr("You must attach the target before using the probe command.");
  }
  if (!exp) {
    rerr("usage: probe <addr>");
  }
  address_to_probe = parse_and_eval_address (exp);
  if (simulator) {
    if (sim_v_to_p(address_to_probe, &physical_address, usmode) != -1) {
      ui_fprintf(stderr, "No translation of 0x%x\n", address_to_probe);
    } else {
      ui_fprintf(stdout, "physical address: 0x%08x\n", physical_address);
    }
  } else {
    if (remote_v_to_p(address_to_probe, &physical_address, usmode)) {
      ui_fprintf(stderr, "No translation of 0x%x\n", address_to_probe);
      print_apr();

#ifdef CMMU_BUG_FIXED
      print_pfar();
      print_pfsr();
#endif
    } else {
      ui_fprintf(stdout, "physical address: 0x%08x\n", physical_address);
      print_ssr();
    }
  }
}

/* Tell the user what physical address a given virtual address maps to. */
static void probe_command(exp, from_tty)
  char *exp;
{
  do_probe_command(exp, from_tty, M_SUPERVISOR);
}

/* Tell the user what physical address a given virtual address maps to. */
static void uprobe_command(exp, from_tty)
  char *exp;
{
  do_probe_command(exp, from_tty, M_USER);
}

/*
 * Check the text passed in against the text in the target's memory.
 */
static void check_remote(object_fd, t_start, t_length)
    int object_fd;
    u_long  t_start; 
    u_long  t_length;
{
    u_long *checksums;
    u_long blocks;
    u_long i;
    u_long addr;

  if (simulator) {
    check_sim(object_fd, t_start, t_length);
    return;
  }
    /*
     * We first find the checksum of each 4K block
     * in the the target by doing a checksum command to the
     * target.  Then we checksum each 4K block to be downloaded.
     * We then upload and compare blocks that were different.
     */

    assert((t_length & 3) == 0);

    /*
     * Allocate an array of booleans to store the flag for each
     * block.
     */
    blocks = (t_length + (PAGESIZE - 1)) / PAGESIZE;
    checksums = (u_long *)malloc((unsigned)(blocks * sizeof(unsigned *)));

    /*
     * Send the checksum command to the target.
     */
     traceremote("Asking target for checksums on %d blocks starting at 0x%x\n",
                                                blocks, t_start);
    traceremote("Checksumming 0x%x bytes starting at 0x%x\n",
                            t_length, t_start);
    init_checksum();
    splhi();
    send_command(varvalue("noparityinit") ? C_CHECKSUMNOINIT : C_CHECKSUM);
    send_word(t_start);
    send_word(t_length);
    send_command(C_END);
    send_checksum();

    switch (wait_for_token(C_DATASTART, C_DATAERROR, ta_warning, 3 /* seconds */)) {
        case C_DATASTART:
            break;

        case C_DATAERROR:
            dacc_rerr("checksumming block");

        default:
            rerr("Didn't find C_DATASTART in checkdl");
    }
    for (i = 0 ; i < blocks ; i++) {
        checksums[i] = get_word();
    }

    if (wait_for_token(C_DATAEND, C_NONE, ta_warning, 1) != C_DATAEND) {
        rerr("Didn't find C_DATAEND in checkdl");
    }
    spllo();

    addr = t_start;
    for (i = 0 ; i < blocks ; i++) {
        u_long block_size;
        u_long buf[PAGESIZE/sizeof(u_long)];
        u_long *wp, filebuffer[PAGESIZE/sizeof(u_long)];
	QUIT;
        block_size = (i == (blocks - 1)) ? (t_length % PAGESIZE) : PAGESIZE;
        if (block_size == 0) {
            block_size = PAGESIZE;
        }
        wp = filebuffer;
        if (read(object_fd, &wp[0], block_size) != block_size) {
            rerr("Error reading from object file");
        }

        if (checksums[i] != checksum_of(wp, block_size)) {
            unsigned cnt;

            if (varvalue("tracedownload")) {
                ui_fprintf(stderr, 
                       "Uploading block %d at address 0x%x of length %d\n",
                              i, addr, block_size);
            } 
            if (dread_remote((u_char *)&buf[0], addr, block_size, M_SUPERVISOR)) {
                rerr("Error reading %d bytes at 0x%x in checkdl", block_size, addr);
            }
            for (cnt = 0 ; cnt < block_size ; cnt += sizeof(u_long)) {
	        QUIT;
                if (*wp != buf[cnt / sizeof(u_long)]) {
                    ui_fprintf(stdout, "%08x : expected: %08x  target: %08x\n",
                              addr, *wp, buf[cnt / sizeof(u_long)]);
                }
                wp++;
                addr += sizeof(u_long);
            }
        } else {
            wp += block_size / sizeof(u_long);
            addr += block_size;
        }
    }
    free((char *)checksums);
}

/*
 * Download the target system.  Returns true if the download was
 * successful, returns false if an error was detected.
 */
static boolean dl_remote(object_fd, t_start, t_length, 
                             d_start, d_length, 
                             b_start, b_length)
    int object_fd;
    u_long  t_start; 
    u_long  t_length;
    u_long  d_start;
    u_long  d_length;
    u_long  b_start;
    u_long  b_length;
{
    struct timeval tv;
    struct timezone tz;

    total_bytes_transmitted = 0;
    total_bytes_received = 0;
    total_bytes_downloaded = t_length + d_length;

    gettimeofday(&tv, &tz);
    start_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;

    /*
     * Download the text and data, zero bss.
     */
    download(object_fd, t_start, t_length);
    mark_breakpoints_out();
    download(object_fd, d_start, d_length);
    remote_zeromem(b_start, b_length);

    gettimeofday(&tv, &tz);
    end_time = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;

    print_comm_statistics();

    downloaded = true;
    return true;
}

/*
 * This fills a block of target memory with zeros.
 */
void remote_zeromem(addr, length)
    u_long addr;
    int length;
{
  if (simulator) {
    sim_zeromem(addr, length);
    return;
  }
    splhi();

    invalidate_mem_cache();
    init_checksum();
    send_command(C_FILL);
    send_word(addr);
    send_word(length);
    traceremote("Zeroing memory from 0x%x to 0x%x\n", addr, addr + length);
    send_command(C_END);
    send_checksum();
    if (wait_for_token(C_ACK, C_NONE, ta_warning, 
                                  (int)(2+length/500000)) !=C_ACK) {
        rerr("Didn't find C_ACK in remote_zeromem");
    }
    spllo();
}

void
_initialize_remotecmd()
{
  add_com("dl", class_run, dl_command,
     "Download text and data to target.");
  add_com("checkdl", class_run, checkdl_command,
     "Check downloaded text and data in target match host copy.");
  add_com("bzero", class_run, bzero_command,
     "Zero a block of target memory, usage: bzero <address> <length>");
  add_com("bcopy", class_run, bcopy_command,
     "Copy a block of target memory, usage: bcopy <saddr> <daddr> <length>");
  add_com("flush", class_run, flush_command,
     "Flush the host cache of target memory and target comm buffer.");
  add_com("init", class_run, init_command, "Reset the target.");
  add_com("interrupt", class_run, interrupt_command, "Send an interrupt signal to the target.");
  add_com("lastr", class_run, lastr_command, 
             "Show last 100 characters received from the target.");
  add_com("lastt", class_run, lastt_command, 
             "Show last 100 characters sent to the target.");
  add_com("lasta", class_run, lasta_command,
             "Show last 100 trace messages in reverse order.");
  add_com("pselect", class_run, pselect_command,
     "Select a processor");
  add_com("penable", class_run, penable_command,
     "Enable a processor");
  add_com("pdisable", class_run, pdisable_command,
     "Disable a processor");
  add_com("pstat", class_run, pstat_command,
     "Display status of processors");
  add_com("probe", class_run, probe_command, 
     "Translate a kernel virtual address to a physical address.");
  add_com("uprobe", class_run, uprobe_command, 
     "Translate a user virtual address to a physical address.");
  add_com("regs", class_run, regs_command, "Print the registers.");
  add_com("rb", class_run, rb_command,
     "Read a single byte from the target memory.");
  add_com("rh", class_run, rh_command,
     "Read a single halfword from the target memory.");
  add_com("rw", class_run, rw_command,
     "Read a single word from the target memory.");
  add_com("sw", class_run, sw_command,
"Search target memory for a word pattern, usage: sw <addr> <length> <pattern>.");
  add_com("terror", class_run, terror_command,
     "Toggle the target debug monitor's show-communiction-errors flag");
  add_com("wb", class_run, wb_command,
     "Write a single byte to the target memory.");
  add_com("wh", class_run, wh_command,
     "Write a single halfword to the target memory.");
  add_com("ww", class_run, ww_command,
     "Write a single word from the target memory.");
}
