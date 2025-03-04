/* Execution functions for cross-debugging

   Copyright (C) 1986, 1987, 1988, 1989 Free Software Foundation, Inc.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/remrun.c,v 1.8 91/01/13 23:51:02 robertb Exp $
   $Locker:  $ */


#include <ctype.h>
#include "defs.h"
#include "param.h"
#include "ui.h"
#include "frame.h"
#include "inferior.h"
#include "wait.h"

#include <stdio.h>
#include <signal.h>

#include "remote.h"
#include "fields88.h"
#include "decode.h"
#include "format.h"
#include "montraps.h"

#define CAUGHT_EXCEPTION 1
#define CAUGHT_BREAKPOINT 2

static u_long delayed_ip;
static u_long *delayed_p;
u_long last_iword;
static struct instr_info *instr_info;
static CORE_ADDR ld_st_addr;

unsigned exception_code;

static void ss_remote();
static int go_remote();

boolean target_running;

int
remote_resume (step, signal_number)
     int step, signal_number;
{
  if (!remote_debugging) {
    rerr("You must attach the target before using this command.\n");
  }
  if (simulator) {
    sim_resume(step);
  } else {
    if (!downloaded) {
      downloaded = true;                    /* Just give this warning once. */
      rerr("Target hasn't been downloaded yet");
    }
    if (step) {
      ss_remote();
    } else {
      go_remote();
    }
  }
}

/* Wait until the remote machine stops, then return,
   storing status in STATUS just as `wait' would.  */

int
remote_wait (status)
  WAITTYPE *status;
{
  WSETEXIT ((*status), 0);
  if (stop_cause & (1 << SIGINT)) {
    stop_cause &= ~(1 << SIGINT);
    WSETSTOP ((*status), SIGINT);
  } else if (stop_cause & (1 << SIGTRAP)) {
    stop_cause &= ~(1 << SIGTRAP);
    WSETSTOP ((*status), SIGTRAP);
  } else {
    rerr("remote_wait: stop_cause=0x%x", stop_cause);
  }
}

char *exception_names[] = {
 "reset exception", 
 "interrupt exception", 
 "code access exception", 
 "data access exception", 
 "misaligned access exception",
 "unimplemented opcode exception", 
 "privilege violation exception", 
 "bounds check exception", 
 "illegal integer divide exception", 
 "integer overflow exception", 
 "error"};

/* This returns a string that describes the exception whose code is
   passed */

char *exception_name(code)
    u_short code;
{
    static char buf[1000];

    if (code < (sizeof exception_names) / sizeof (char *)) {
      return exception_names[code];
    } else {
      if (code == TR_TRC) return "single step";
      if (code == TR_BPT) return "breakpoint";
      sprintf(buf, "ex%d", code);
      return &buf[0];
    }
}

/*
 * Codes returned by classify_instruction(), used in ss_remote().
 */
#define I_NON_CONTROL   1
#define I_CBRANCH       2
#define I_CBRANCH_N     3
#define I_TRAP          4
#define I_JMP           5
#define I_JMP_N         6
#define I_BSR           7
#define I_BSR_N         8
#define I_JSR		9
#define I_JSR_N		10
#define I_BR		11
#define I_BR_N		12
#define I_BB		13
#define I_BB_N		14
#define	I_TRAP_NOT_TAKEN 15

/*
 * This classifies the passed 88000 instruction for ss_remote().  This
 * code could be in ss_remote, since that is the only place this is called,
 * but I thought it was clearer to make a function out of it.
 */
static int classify_instruction(instr)
    u_long instr;
{
    struct instr_info *instr_info;
    extern edata;

    instr_info = remote_instruction_lookup(instr);
    if (instr_info == (struct instr_info *)0) {
      rerr("classify: could not find instruction 0x%08x in table\n", instr);
    }
    switch (instr_info->format) {
        case IPREL:
            if (((instr >> 27) & 0x1f) == 0x19) {
                return (instr & 0x04000000) ? I_BSR_N : I_BSR;
            }
            return (instr & 0x04000000) ? I_BR_N : I_BR;

        case BITBRANCH:
            return (instr & 0x04000000) ? I_BB_N : I_BB;

        case CBRANCH:
            return (instr & 0x04000000) ? I_CBRANCH_N : I_CBRANCH;

        case JMP:
            if ((instr & 0xfffffbe0) == 0xf400c800) {
                return (instr & 0x00000400) ? I_JSR_N : I_JSR;
            }
         
            return (instr & 0x00000400) ? I_JMP_N : I_JMP;

        case TRAP:
            /* Return I_TRAP_NOT_TAKEN if the this instruction won't
               trap.  This way we can single step over these traps.
               Otherwise we have to tell the user that we can't do it */
            if (((remote_read_register(S1(instr)) & (1 << B5(instr))) != 0)
                        != ((instr & 0x800) != 0)) {
                return I_TRAP_NOT_TAKEN;
            }
            /* Assume tbnd, and tcnd instructions trap */
        case TBND:
        case TCND:
        case RTE:
            return I_TRAP;

        default:
            /* Special case for reg-reg tbnd instruction, because
               it is marked a reg-reg instruction (probably to make
               disassembly of it correct. */
            if (0xf400f800 == (instr & 0xffe0ffe0)) {
                return I_TRAP;
            }
            return I_NON_CONTROL;
    }
}

/*
 * Single steps the processor.  If the instruction that the IP is pointing
 * at is a control instruction then we must simulate it.  Sometimes we
 * do this by calling the simulator.  In simple cases we just do the
 * instruction right here.
 */
static void ss_remote()
{
    u_long instr;
    u_long real_ip, new_ip;
    extern u_long delayed_ip;
    boolean success;

    if (!cpu_enabled[selected_processor]) {
      rerr("Current processor not enabled");
    }
    real_ip = remote_read_register(SNIP_REGNUM) & ~(IP_VALID|IP_EXCEPTION);
    if (dread_remote(&instr, real_ip, sizeof(instr), M_CURMODE)) {
        rerr("ss_remote: unable to read address 0x%x\n", real_ip);
    }
    switch (classify_instruction(instr)) {
        int i;
        case I_NON_CONTROL:
            if ((remote_read_register(PSR_REGNUM) & 0x80000000) == 0) {
                rerr("Sorry, can't single step in user mode\n");
                /* NOTREACHED */
            }

            traceremote("Single-stepping CPU %d with snip=0x%x sfip=0x%x\n",
                         selected_processor,
                         remote_read_register(SNIP_REGNUM), 
                         mon_single_step_trap_addr | IP_VALID);
            /*
             * Single step the processor by pointing the 
             * Next-Instruction-Pointer at the instruction to be 
             * executed and the Fetched-Instruction-Pointer at the
             * trap instruction in the ROM.
             */
            remote_write_register(SFIP_REGNUM,
				  mon_single_step_trap_addr | IP_VALID);

            splhi();
            invalidate_mem_cache();
            init_checksum();
            send_command(C_GO);

            /* Enable only the currently selected processor */
            for (i = 0 ; i < MAX_PROCESSORS ; i++) {
              send_byte(i == selected_processor);
            }

            send_command(C_END);
            send_checksum();
            if (wait_for_token(C_ACK, C_NONE, ta_warning, 1) != C_ACK) {
                ui_fprintf(stderr, "Didn't find C_ACK in ss_remote\n");
            } else {
                success = wait_for_exception(TR_TRC, ta_warning, 1);
                if (!success) {
                    ui_fprintf(stderr, "the ip may be incorrect.\n");
                }

                /*
                 * If the instruction that we just executed on the target
                 * was in a branch delay slot, then we get the new IP value
                 * from 'delayed_ip', the simulator variable that is sometimes
                 * also set in this function.  Otherwise, we just 
                 * increment the previous IP value.
                 */
                if (delayed_p) {
                    new_ip = (u_long)delayed_ip;
                    delayed_p = nil;
                } else {
                    new_ip = real_ip + 4;
                }
                /*
                 * Make the sxip, snip, and sfip look reasonable.  After
                 * the single-step they are pointing at instructions in
                 * the ROM.
                 */
                remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
                remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
                remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            }
            spllo();
            break;

        case I_BR:
            if (delayed_p) {
                rerr("ss_remote: br instruction in delay slot");
            }
            traceremote("ss_remote: simulating br instr.\n");
            new_ip = REL26(instr) + (int)real_ip;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_BR_N:
            if (delayed_p) {
                rerr("ss_remote: br.n instruction in delay slot");
            }
            traceremote("ss_remote simulating br.n instr.\n");
            new_ip = real_ip + 4;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            delayed_p = (u_long *)1;
            delayed_ip = REL26(instr) + real_ip;
            remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
            break;

        case I_BB:
            if (delayed_p) {
                rerr("ss_remote: bb? instruction in delay slot");
            }
            traceremote("ss_remote: simulating bb? instr.\n");
            /*
             * Exclusive-or the "this-is-a-bb1-instruction" bit with
             * the selected bit in the register designated in the instruction
             * that we are simulating.
             */
            if (((instr >> 27) & 1) ==
                  ((remote_read_register(S1(instr)) >> B5(instr)) & 1)) {
              new_ip = REL16(instr) + (int)real_ip;
            } else {
              new_ip = real_ip + 4;
            }
             /*
              * Make the sxip, snip, and sfip look reasonable.  After
              * the single-step they are pointing at instructions in
              * the ROM.
              */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_BB_N:
            if (delayed_p) {
                rerr("ss_remote: bb?.n instruction in delay slot");
            }
            traceremote("ss_remote simulating bb?.n instr.\n");
            new_ip = real_ip + 4;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);

            /*
             * Exclusive-or the "this-is-a-bb1-instruction" bit with
             * the selected bit in the register designated in the instruction
             * that we are simulating.
             */
            if (((instr >> 27) & 1) ==
                  ((remote_read_register(S1(instr)) >> B5(instr)) & 1)) {
              delayed_p = (u_long *)1;
              delayed_ip = REL16(instr) + real_ip;
              remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
            } else {
              new_ip = real_ip + 4;
              remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            }
            break;

        case I_CBRANCH:
            if (delayed_p) {
                rerr("ss_remote: bcnd instruction in delay slot");
            }
            traceremote("ss_remote: simulating bcnd instruction\n");
            {
              int m5 = M5(instr);
              int r = remote_read_register(S1(instr));
              int index_into_m5 = (((r >> 31) & 1) << 1) | 
                                   ((r & 0x7fffffff) == 0);
              if ((m5 >> index_into_m5) & 1) {
                new_ip = REL16(instr) + (int)real_ip;
              } else {
                new_ip = real_ip + 4;
              }
            }
             /*
              * Make the sxip, snip, and sfip look reasonable.  After
              * the single-step they are pointing at instructions in
              * the ROM.
              */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_CBRANCH_N:
            if (delayed_p) {
                rerr("ss_remote: bcnd.n instruction in delay slot");
            }
            traceremote("ss_remote simulating bcnd.n instruction\n");
            new_ip = real_ip + 4;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            {
              int m5 = M5(instr);
              int r = remote_read_register(S1(instr));
              int index_into_m5 = (((r >> 31) & 1) << 1) | 
                                   ((r & 0x7fffffff) == 0);
              if ((m5 >> index_into_m5) & 1) {
                delayed_p = (u_long *)1;
                delayed_ip = REL16(instr) + real_ip;
                remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
              } else {
                new_ip = real_ip + 4;
                remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
              }
            }
            break;

        case I_BSR:
            if (delayed_p) {
                rerr("ss_remote: bsr in delay slot");
            }
            traceremote("ss_remote: simulating bsr instr.\n");
            remote_write_register(R1_REGNUM, real_ip + 4);
            new_ip = REL26(instr) + (int)real_ip;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_BSR_N:
            if (delayed_p) {
                rerr("ss_remote: bsr.n instruction in delay slot");
            }
            traceremote("ss_remote simulating bsr.n instr.\n");
            delayed_p = (u_long *)1;
            delayed_ip = REL26(instr) + real_ip;
            remote_write_register(R1_REGNUM, real_ip + 8);
            new_ip = real_ip + 4;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
            break;

        case I_JMP:
            if (delayed_p) {
                rerr("ss_remote: jmp instruction in delay slot");
            }
            traceremote("ss_remote simulating normal jmp instr.\n");
            new_ip = remote_read_register(S2(instr));
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_JMP_N:
            if (delayed_p) {
                rerr("ss_remote: jmp.n instruction in delay slot");
            }
            traceremote("ss_remote simulating jmp.n instr.\n");

            new_ip = real_ip + 4;
            delayed_ip = remote_read_register(S2(instr));
            delayed_p = (u_long *)1;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
            break;

        case I_JSR:
            if (delayed_p) {
                rerr("ss_remote: jsr instruction in delay slot");
            }
            traceremote("ss_remote simulating normal jsr instr.\n");
            new_ip = remote_read_register(S2(instr));
            remote_write_register(R1_REGNUM, real_ip + 4);
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, (new_ip + 4) | IP_VALID);
            break;

        case I_JSR_N:
            if (delayed_p) {
                rerr("ss_remote: jsr.n instruction in delay slot");
            }
            traceremote("ss_remote simulating jsr.n instr.\n");

            new_ip = real_ip + 4;
            remote_write_register(R1_REGNUM, real_ip + 8);
            delayed_ip = remote_read_register(S2(instr));
            delayed_p = (u_long *)1;
            /*
             * Make the shadow ip's look like they would on the
             * real machine if the real machine had hit a breakpoint here.
             */
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
            break;

        case I_TRAP_NOT_TAKEN:
            /*
             * We know that this won't really trap, just advance the pc.
             */
            traceremote("ss_remote: tb0 or tb1 that won't trap\n");
            new_ip = real_ip + 4;
            remote_write_register(SXIP_REGNUM, real_ip | IP_VALID);
            remote_write_register(SNIP_REGNUM, new_ip | IP_VALID);
            remote_write_register(SFIP_REGNUM, delayed_ip + 4 | IP_VALID);
            break;

        case I_TRAP:
            rerr(
           "Can't single-step this instruction (tb0, tb1, tcnd, tbnd, or rte)");
             break;

        default:
            ui_fprintf(stderr, "ss_remote: case error at 0x%x\n", real_ip);
            break;
    }
    stop_cause |= (1 << SIGTRAP);
    if (nmiing_remote) {
        rerr("Remote stopped after single-stepping");
    }
}

/*
 * See if we stopped in a branch delay slot.  If so, set 'delayed_p'
 * to a non-zero value and set 'delayed_ip' to the delayed-branch 
 * target.
 */
static void check_for_delay_slot_after_breakpoint()
{
    u_long sxip = remote_read_register(SXIP_REGNUM);
    u_long snip = remote_read_register(SNIP_REGNUM);
    /*
     * Since we the last instruction that was executed was
     * a trap, the SXIP_REGNUM should be valid with no exception
     * pending.
     */
    if ((sxip & (IP_VALID|IP_EXCEPTION)) != IP_VALID) {
        ui_fprintf(stderr, "Warning: sxip=0x%x\n", sxip);
    }
    sxip &= ~(IP_VALID|IP_EXCEPTION);
    /*
     * Test to see if we hit a breakpoint in a branch delay
     * slot by comparing the sxip and the snip.
     */
    if ((snip & (IP_VALID|IP_EXCEPTION)) == IP_VALID) {
        snip &= ~(IP_VALID|IP_EXCEPTION);
        if (sxip != snip - 4) {
            delayed_p = (u_long *)1;
            delayed_ip = snip;
        } else {
            delayed_p = (u_long *)0;
        }
    }
}

/*
 * See if we stopped in a branch delay slot.  If so, set 'delayed_p'
 * to a non-zero value and set 'delayed_ip' to the delayed-branch 
 * target.
 */
void check_for_delay_slot_after_interrupt()
{
    u_long snip = remote_read_register(SNIP_REGNUM);
    u_long sfip = remote_read_register(SFIP_REGNUM);

    /*
     * Test to see if we hit a breakpoint in a branch delay
     * slot by comparing the snip and the sfip to see if they
     * are adjacent.
     */
    if ((snip & (IP_VALID|IP_EXCEPTION)) == IP_VALID &&
	(sfip & (IP_VALID|IP_EXCEPTION)) == IP_VALID) {
        snip &= ~(IP_VALID|IP_EXCEPTION);
        sfip &= ~(IP_VALID|IP_EXCEPTION);
        if (snip != sfip - 4) {
            delayed_p = (u_long *)1;
            delayed_ip = sfip;
        } 
    }
}

kludge_pc()
{
  remote_write_register(SNIP_REGNUM, remote_read_register(SXIP_REGNUM));
  remote_write_register(SFIP_REGNUM, remote_read_register(SXIP_REGNUM)+4);
}

/*
 * Let the processor run until it hits a breakpoint.
 */
static int go_remote()
{
  int i;
#ifdef NOT_DEF /* We don't want to do this with gdb, right? */
    /*
     * If we are stopped in a branch delay slot, point the snip
     * at the instruction in the branch delay slot and the sfip
     * at the delayed-branch target.
     *
     * Otherwise mark the snip invalid (with a zero) and point
     * the sfip at the instruction to execute.
     */
    if (delayed_p) {
        delayed_p = 0;
        remote_write_register(SFIP_REGNUM, delayed_ip | IP_VALID);
    } else {
        remote_write_register(SNIP_REGNUM, 0);
    }
#endif

    if (!cpu_enabled[selected_processor]) {
      rerr("Current processor not enabled");
    }
    traceremote("Starting CPUs %s%s%s%s with snip=0x%x sfip=0x%x\n",
                         cpu_enabled[0] ? "0 " : "",
                         cpu_enabled[1] ? "1 " : "",
                         cpu_enabled[2] ? "2 " : "",
                         cpu_enabled[3] ? "3" : "",
                         remote_read_register(SNIP_REGNUM), 
                         remote_read_register(SFIP_REGNUM));
    invalidate_mem_cache();
    init_checksum();
    splhi();
    send_command(C_GO);
    for (i = 0 ; i < MAX_PROCESSORS ; i++) {
      if (i == selected_processor) {
        send_byte(1);
      } else {
        send_byte(cpu_enabled[i]);
      }
    }
    send_command(C_END);
    send_checksum();
    if (wait_for_token(C_ACK, C_NONE, ta_warning, 1) != C_ACK) {
        ui_fprintf(stderr, "Didn't find C_ACK in go_remote\n");
    } else {
        target_running = true;
        if (varvalue("exitaftercontinue")) {
            ui_fprintf(stderr, "Target left running ..\n");
            exit(0);
        }
        if (setjmp(controlc_env)) {
          return CAUGHT_EXCEPTION;
        }
        spllo();

        if (wait_for_exception(TR_BPT, ta_none, 5)) {
            traceremote("After breakpoint, sxip=0x%x snip=0x%x sfip=0x%x\n",
                               remote_read_register(SXIP_REGNUM), 
                               remote_read_register(SNIP_REGNUM), 
                               remote_read_register(SFIP_REGNUM));
            check_for_delay_slot_after_breakpoint();
            kludge_pc();
            stop_cause |= (1 << SIGTRAP);
            return CAUGHT_BREAKPOINT;
        } 
    }
    spllo();
    return CAUGHT_EXCEPTION;
}

/*
 * Decode the instruction that we are about to execute.  If it is
 * a load or store, calculate the effective address.  We will use this
 * later in "print_effect", but we must calculate it now in case one
 * of the registers used to form the address is the destination of a
 * load instruction.
 */
void decode_instr_before_stepping(iword)
    u_long iword;
{
    int shift;

    last_iword = iword;
    instr_info = remote_instruction_lookup(iword);
    switch (instr_info->format) {
        case LDLIT: case STLIT:
            ld_st_addr = read_register(S1(iword)) + LIT16(iword);
            break;

        case LDRO: case STRO:
            ld_st_addr = read_register(S1(iword)) + 
                         read_register(S2(iword));
            break;

        case LDRI: case STRI:
            switch ((iword >> 10) & 3) {
                case 0: shift = 3; break;
                case 1: shift = 2; break;
                case 2: shift = 1; break;
                case 3: shift = 0; break;
            }
 
            ld_st_addr = read_register(S1(iword)) + 
                         (read_register(S2(iword)) << shift);
            break;
    }
}

/*
 * Print the effect of this instruction.  We assume that this was
 * the instruction that was just executed.  XMEM instructions are not
 * handled correctly.
 */
void print_effect()
{
    int r;

    if (last_iword == 0) {
        return;
    }
    r = D(last_iword);
    switch (instr_info->format) {
        case INTRL: case INTRR: case BITFIELD: case INTRR2OP:
        case LDCR:  case FLDCR: case ROT: case INTRR_S1_S2:
            ui_fprintf(stdout, "r%-2d=0x%-8x\n",
                             r, read_register(r));
            break;

        case LDLIT: case LDRO: case LDRI:
            ui_fprintf(stdout, "r%-2d=0x%-8x loaded from 0x%-8x\n", 
                             r, read_register(r), ld_st_addr);
            break;

        case STLIT: case STRO: case STRI:
            ui_fprintf(stdout, "r%-2d=0x%-8x stored to 0x%-8x\n", 
                             r, read_register(r), ld_st_addr);
            break;

        default:
            break;
    }
    last_iword = 0;
}
