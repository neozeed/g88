/*
 * monlow.s of remote dbx-compatible Debug Monitor for Moto 188 board
 *
 * $Header: /home/bigbird/Usr.U6/robertb/m88k/src/g88/hmon/RCS/monlow.s,v 1.8 91/01/13 23:39:33 robertb Exp $
 * $Locker:  $
 *

	Copyright (c) 1987, 1988, 1989, 1990 by Tektronix, Inc.

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.
 */

#define	ASSEMBLER

#include "monoffsets.h"
#include "montraps.h"
#include "psl.h"
#include "mon.h"

/* NOTE THAT IF THE TEXT AND DATA OF THIS MONITOR GROW 
   THE ADDRESS of _regs MAY HAVE TO BE CHANGED */

; Address of the register images of the program being debugged by the
; Tektronix debugger.
	_regs0 = 0xffe1c000 ; In SRAM above debug monitor text
	_regs1 = 0xffe1d000 ; In SRAM above debug monitor text
	_regs2 = 0xffe1e000 ; In SRAM above debug monitor text
	_regs3 = 0xffe1f000 ; In SRAM above debug monitor text

; This is were we save the preserved and linker register values that
; belong to the Tektronix monitor before we do a 188BUG syscall.  We
; restore these registers after the syscall.
	mon_preserved_save_area = _regs0-128

; Address of the flag that conrols whether we will report communication
; errors to the host.
	_report_comm_errors = _regs0-20

; Address of the image of the instruction that was at location 8 when this
; monitor was invoked.  We retore the instruction at location 8 when we
; call 188BUG.  The processor jumps to location 8 when it recognizes an
; interrupt.
	int_vec_save = _regs0-16

; Like 'int_vec_save', but for data access exceptions.
	dacc_vec_save = _regs0-12

; Address of the image of SR3 (aka cr20) when this monitor was invoked.
; Thus, the word at this address is 188BUG's value of SR2.  We must restore
; SR3 from this location before calling 188BUG.
	sr3_save = _regs0-8

; Address of the mutual exclusion flag
	mutex = _regs0-4

; We write 0x4 to this address to clear the debug interrupt flip flop
; on the 188 board.
	clrint = 0xfff8408c    

; We read this to find out which CPU we are.
	whoami = 0xfff88018

; We put this in cr17 so that when kernel starts it knows that it was
; booted by gdb.
	magic_cookie =  0xbed1ccec

	.text
	.globl	_debug_monitor_entry_point
	.globl	_single_step_trap
        .globl  _our_var_area
	.globl  _acquire_host_comm_mutex
	.globl  _release_host_comm_mutex
	.globl	_report_comm_errors
	.globl	_restart
	.globl	_regs0
	.globl	_regs1
	.globl	_regs2
	.globl	_regs3

	.globl	_fubyte
	.globl	_subyte
	.globl	_syscall
/*
 *  Initialization code.
 *  This code assumes that 188BUG's vector table starts at 0 
 */

_cold_start_address:
	br	_start
	br	_warm_start_address
	br	_mon_dbread
	br	_mon_dbwrite
	br	_set_entry_exit_functions
	br	_forked_cpu_entry

; entry point for set_entry_exit_functions
; The kernel passes pointers to functions in r2 and r3.
; Each time the debug monitor is entered, it calls the first of these
; functions.  On exit it calls the second.

_set_entry_exit_functions:
	or	r5,r0,r2
	or	r6,r0,r1
	bsr	_our_var_area
	st	r5,r2,DM_CALL_ON_MON_ENTRY
	st	r3,r2,DM_CALL_ON_MON_EXIT
	jmp	r6

_start:
	or.u	r2,r0,hi16(mutex)
	st	r0,r2,lo16(mutex)

	or.u	r2,r0,hi16(_report_comm_errors)
	st	r0,r2,lo16(_report_comm_errors)

; Zero the whole data area.  The only field we really depend on being
; zero is the dm_in_mon field.  But it makes for a cleaner register
; display.

	or.u	r3,r0,hi16(_regs0)
	or	r3,r3,lo16(_regs0)
	or	r2,r0,0x4000

zero_reg_area_loop:
	st	r0,r3,0
	subu	r2,r2,4
	addu	r3,r3,4
	bcnd	ne0,r2,zero_reg_area_loop

; Attempt to fork off all 4 CPUs.  We know at least one of these
; will fail (the one for the current processor).  But there is no
; harm in failing and it makes the code more general.

	or	r2,r0,0
	or.u	r3,r0,hi16(_forked_cpu_entry)
	or	r3,r3,lo16(_forked_cpu_entry)
	or	r9,r0,0x100
	tb0	0,r0,496		; Call 188BUG with .FORKMPU

	or	r2,r0,1
	or.u	r3,r0,hi16(_forked_cpu_entry)
	or	r3,r3,lo16(_forked_cpu_entry)
	or	r9,r0,0x100
	tb0	0,r0,496		; Call 188BUG with .FORKMPU

	or	r2,r0,2
	or.u	r3,r0,hi16(_forked_cpu_entry)
	or	r3,r3,lo16(_forked_cpu_entry)
	or	r9,r0,0x100
	tb0	0,r0,496		; Call 188BUG with .FORKMPU

	or	r2,r0,3
	or.u	r3,r0,hi16(_forked_cpu_entry)
	or	r3,r3,lo16(_forked_cpu_entry)
	or	r9,r0,0x100
	tb0	0,r0,496		; Call 188BUG with .FORKMPU


; Save the value in cr20 so that we can call 188BUG.
	ldcr	r3,cr20
	or.u	r2,r0,hi16(sr3_save)
	st	r3,r2,lo16(sr3_save)
	bsr	_setup_exception_table_cold

fork_reentry:
	bsr	_our_var_area
	st	r0,r2,DM_CALL_ON_MON_ENTRY	; default: don't call
	st	r0,r2,DM_CALL_ON_MON_EXIT

	ldcr	r3,PSR
	and	r3,r3,lo16(~(PSR_SFD1|PSR_SFRZ|PSR_IND))
	st	r3,r2,DM_REG_TPSR	; Start the user with a reasonable PSR
	or	r3,r3,PSR_IND		; Disable interrupt while in monitor.
	stcr	r3,PSR			; as a test. -rcb 4/15/90
	ldcr	r3,VBR
	st	r3,r2,DM_REG_VBR

	ldcr	r3,SR3			; Copy 188BUG's value in cr20 to the
	st	r3,r2,DM_REG_SR3	; user's area so it can call 188BUG.

	or.u	r3,r0,hi16(magic_cookie) ; Put code into cr17 so that kernel
	or	r3,r3,lo16(magic_cookie) ; knows that it was booted by gdb.
	st	r3,r2,DM_REG_SR0

; Start the session off with a reasonable stack pointer that is above
; the code.  Stack pointers below the text segment confuse gdb.
	subu	r3,r2,0x8000
	st	r3,r2,DM_REG_R31

_restart:
	bsr	_our_var_area       /* Get address of our chuck of lo mem   */
	addu	r31,r2,DM_STACK
	bsr	_our_var_area
	or	r3,r0,TR_RESET       /* set reset trap_code on entry         */
	st	r3,r2,DM_EXCEPTION_CODE

debug_mon_entry:
	or.u	r3,r0,hi16(clrint)	; Write a 1 to bit 2 of the CLRINT
	or	r2,r0,4			; location to clear NMI that may have
	st	r2,r3,lo16(clrint)	; happened.

; Use debugger's exception table.  And write the four exception table
; entries that we use.  Sometimes 188BUG overwrites them.
	stcr	r0,VBR			
	bsr	_setup_exception_table_warm

	bsr	_our_var_area       /* Get address of our chuck of lo mem   */
	addu	r31,r2,DM_STACK
	bsr	_debug_monitor_entry_point

/*
 * The monitor returns to this point when it wants to single step
 * or otherwise execute the program being cross-debugged.
 */

; Disable interrupts and freeze shadow registers while we restore 
; state (otherwise we can't restore the shadow registers).
	ldcr	r1,PSR
	or	r1,r1,(PSR_IND|PSR_SFRZ)
        tb1     0,r0,511  /* trap not taken- to ensure SSBR clear before SFRZ */
	stcr	r1,PSR
    
	bsr	_our_var_area

; No need to load the PID or the DMU registers, as they are read-only.
; The PSR is about to be reloaded at the RTE, so
;  there is no point in loading it here.

	ld	r1,r2,DM_REG_TPSR
; Make sure shadow registers are not frozen after the rte 
	and	r1,r1,lo16(~PSR_SFRZ)  
	stcr	r1,TPSR

; We don't load the ssbr because it would be too easy for the user
; to lock up the machine without meaning too.  This would happen
; in most cases if the ssbr was set to a non-zero value.

; No need to load sxip, it is ignored on an rte.

	ld	r1,r2,DM_REG_SNIP
	stcr	r1,SNIP

	ld	r1,r2,DM_REG_SFIP
	stcr	r1,SFIP

	ld	r1,r2,DM_REG_VBR
	stcr	r1,VBR

	ld	r1,r2,DM_REG_SR0
	stcr	r1,SR0

	ld	r1,r2,DM_REG_SR1
	stcr	r1,SR1

	ld	r1,r2,DM_REG_SR2
	stcr	r1,SR2

	ld	r1,r2,DM_REG_SR3
	stcr	r1,SR3

	ld	r1,r2,DM_REG_FPECR
	fstcr	r1,FPECR

; The FPHS1, FPLS1, FPHS2, FPLS2, FPPT, FPIT, FPRH, and
; FPRL registers are read-only

	ld	r1,r2,DM_REG_FPSR
	fstcr	r1,FPSR

	ld	r1,r2,DM_REG_FPCR
	fstcr	r1,FPCR

	ld	r1,r2,DM_REG_R1
	ld	r3,r2,DM_REG_R3
	ld.d	r4,r2,DM_REG_R4
	ld.d	r6,r2,DM_REG_R6
	ld.d	r8,r2,DM_REG_R8
	ld.d	r10,r2,DM_REG_R10
	ld.d	r12,r2,DM_REG_R12
	ld.d	r14,r2,DM_REG_R14
	ld.d	r16,r2,DM_REG_R16
	ld.d	r18,r2,DM_REG_R18
	ld.d	r20,r2,DM_REG_R20
	ld.d	r22,r2,DM_REG_R22
	ld.d	r24,r2,DM_REG_R24
	ld.d	r26,r2,DM_REG_R26
	ld.d	r28,r2,DM_REG_R28
	ld.d	r30,r2,DM_REG_R30

	ld	r2,r2,DM_REG_R2
        tb1     0,r0,511    /* trap not taken required for bug in 88K silicon */
	rte	""	/* Restart the user     */

; Return a pointer in r2 to our per-processor monitor area.  We use
; only r2 and return by jumping through r1.

_our_var_area:
	or.u	r2,r0,hi16(whoami)
	ld	r2,r2,lo16(whoami)
	mak	r2,r2,4<2>
	or.u	r2,r2,hi16(mon_data_area_table)
	ld	r2,r2,lo16(mon_data_area_table)
        jmp     r1

mon_data_area_table:
	.word	0		; 0 not valid
	.word	_regs0		; 1 CPU 0
	.word	_regs1		; 2 CPU 1
	.word	0		; 3 not valid
	.word	_regs2		; 4 CPU2
	.word	0		; 5 not valid
	.word	0		; 6 not valid
	.word	0		; 7 not valid
	.word	_regs3		; 8 CPU3
	

;------------------------------------------------------------------------------
; Reenable the FPU without saving registers.  We do this if we
; got a dacc or an interrupt while in the monitor

reenable_fpu:
	ldcr	r1,DMT0
	bb0	DMT_VALID_BIT,r1,rlabel_1f
	bb1	DMT_READBAR_BIT,r1,rlabel_1f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r3,SSBR
	clr	r3,r3,r1
	stcr	r3,SSBR

rlabel_1f:
	ldcr	r1,DMT1
	bb0	DMT_VALID_BIT,r1,rlabel_2f
	bb1	DMT_READBAR_BIT,r1,rlabel_2f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r3,SSBR
	clr	r3,r3,r1
	stcr	r3,SSBR

rlabel_2f:	
	ldcr	r1,DMT2
	bb0	DMT_VALID_BIT,r1,rlabel_3f
	bb1	DMT_READBAR_BIT,r1,rlabel_3f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r3,SSBR
	clr	r3,r3,r1
	stcr	r3,SSBR

rlabel_3f:	

; Set the SNIP to zero and the SFIP to point to the "tb1 0,r0,0" 
; instruction below.  When we execute the rte
; instruction, we will transfer to the tb1 instruction.  The tb1 instruction
; will wait for the FP unit to serialize (finish).  

; Point the SNIP at the tb1 instruction.
; Point the SFIP at the instruction following the tb1.

	or.u	r1,r0,hi16(_serializing_tb1_1)
	or	r1,r1,lo16(_serializing_tb1_1+IP_VALID)
	stcr	r1,SNIP
	addu	r1,r1,4
	stcr	r1,SFIP

; Set up PSR to enable floating point and enable exceptions.
	ldcr	r1,PSR
	and	r1,r1,lo16(~(PSR_SFD1|PSR_SFRZ))
	stcr	r1,TPSR

; Execute the trap instruction with the FPU enabled, to serialize it.
        tb1     0,r0,511    /* trap not taken required for bug in 88K silicon */
	rte	""			; turn on FPU, enable exceptions

_serializing_tb1_1:
	tb1	0,r0,0			; This will never trap.
	br	debug_mon_entry


/*
 * This puts the passed byte in the user memory address
 * specified by the first parameter.
 *
 * subyte(addr, byte)
 */
_subyte:
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	st.b.usr r3,r2,r0
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	jmp	r1

/*
 * This returns the byte in the physical memory address
 * specified by the first parameter.
 *
 * unsigned char fubyte(addr) 
 */
_fubyte:
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	ld.b.usr r2,r2,r0
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	or	r0,r0,r0	/* chip bug- NOP required for .usr */
	jmp	r1

/*
 * This is used to call the 188BUG routines.
 *
 *  int syscall(code, arg1, arg2, arg3)
 */
_syscall:
; Save the monitor's registers
	or.u	r13,r0,hi16(mon_preserved_save_area)
	or	r13,r13,lo16(mon_preserved_save_area)
	st.d	r14,r13,0
	st.d	r16,r13,8
	st.d	r18,r13,16
	st.d	r20,r13,24
	st.d	r22,r13,32
	st.d	r24,r13,40
	st.d	r26,r13,48
	st.d	r28,r13,56
	st.d	r30,r13,64
	st	r1,r13,72

	or.u	r10,r0,hi16(sr3_save)	; Restore 188BUG's sr3 so that we
	ld	r11,r10,lo16(sr3_save)	; can call 188BUG routines and so that
	stcr	r11,SR3			; we can debug this monitor with 188BUG

; Restore 188BUG's interrupt and DACC exception vector table instructions
; so that the abort button (or the kludge card) will cause the 188BUG
; routine for interrupt to be invoked and accessing invalid
; locations will have the right effect (i.e., 188BUG will catch these things
; instead of the Tektronix monitor).

	or.u	r10,r0,hi16(int_vec_save)
	ld	r11,r10,lo16(int_vec_save)
	st	r11,r0,8*TR_INT

	or.u	r10,r0,hi16(dacc_vec_save)
	ld	r11,r10,lo16(dacc_vec_save)
	st	r11,r0,8*TR_DACC

	or	r9,r0,r2
	or	r2,r0,r3
	or	r3,r0,r4
	or	r4,r0,r5
	or	r5,r0,r6

	subu	r13,r9,0x63	; See if this is a .RETURN syscall, if so
	bcnd	ne0,r13,not_ret	; load the registers with the user's values.
				; Except r9, of course,

	bsr	_our_var_area
	or	r9,r2,r0
	ld	r1,r9,DM_REG_R1
	ld.d	r2,r9,DM_REG_R2
	ld.d	r4,r9,DM_REG_R4
	ld.d	r6,r9,DM_REG_R6
	ld	r8,r9,DM_REG_R8
	ld.d	r10,r9,DM_REG_R10
	ld.d	r12,r9,DM_REG_R12
	ld.d	r14,r9,DM_REG_R14
	ld.d	r16,r9,DM_REG_R16
	ld.d	r18,r9,DM_REG_R18
	ld.d	r20,r9,DM_REG_R20
	ld.d	r22,r9,DM_REG_R22
	ld.d	r24,r9,DM_REG_R24
	ld.d	r26,r9,DM_REG_R26
	ld.d	r28,r9,DM_REG_R28
	ld.d	r30,r9,DM_REG_R30
	or	r9,r0,0x63

	tb0	0,r0,496		; Call 188BUG with .RETURN

_warm_start_address:
	or.u	r9,r0,hi16(whoami)
	ld	r9,r9,lo16(whoami)
	mak	r9,r9,4<2>
	or.u	r9,r9,hi16(mon_data_area_table)
	ld	r9,r9,lo16(mon_data_area_table)
	st	r1,r9,DM_REG_R1
	st.d	r2,r9,DM_REG_R2
	st.d	r4,r9,DM_REG_R4
	st.d	r6,r9,DM_REG_R6
	st	r8,r9,DM_REG_R8
	st.d	r10,r9,DM_REG_R10
	st.d	r12,r9,DM_REG_R12
	st.d	r14,r9,DM_REG_R14
	st.d	r16,r9,DM_REG_R16
	st.d	r18,r9,DM_REG_R18
	st.d	r20,r9,DM_REG_R20
	st.d	r22,r9,DM_REG_R22
	st.d	r24,r9,DM_REG_R24
	st.d	r26,r9,DM_REG_R26
	st.d	r28,r9,DM_REG_R28
	st.d	r30,r9,DM_REG_R30
	br	restore_monitor_registers

not_ret:
	tb0	0,r0,496

restore_monitor_registers:
	or.u	r13,r0,hi16(mon_preserved_save_area)
	or	r13,r13,lo16(mon_preserved_save_area)
	ld.d	r14,r13,0
	ld.d	r16,r13,8
	ld.d	r18,r13,16
	ld.d	r20,r13,24
	ld.d	r22,r13,32
	ld.d	r24,r13,40
	ld.d	r26,r13,48
	ld.d	r28,r13,56
	ld.d	r30,r13,64
	ld	r1,r13,72

	ldcr	r11,SR3			; time we call it, we can make it look
	or.u	r10,r0,hi16(sr3_save)	; Save 188BUG's sr3 so that the next
	st	r11,r10,lo16(sr3_save)	; like we haven't been using sr3.

; Fall through 
_setup_exception_table_cold:
; Save 188BUG's dacc and interrupt exception table entry instructions.
	ld	r13,r0,8*TR_DACC
	or.u	r10,r0,hi16(dacc_vec_save)
	st	r13,r10,lo16(dacc_vec_save)

	ld	r13,r0,8*TR_INT
	or.u	r10,r0,hi16(int_vec_save)
	st	r13,r10,lo16(int_vec_save)

_setup_exception_table_warm:
/* Install branch instruction in exception table entry for interrupt.  The
   branch instruction will branch to our handler for interrupt. */
	or.u	r13,r0,hi16(_interrupt_ex)
	or	r13,r13,lo16(_interrupt_ex)
	subu	r13,r13,8*TR_INT
	ext	r13,r13,0<2>	; r13 := word offset for branch instruction
	and.u	r13,r13,0x03ff	; zero opcode field of branch instruction
	or.u	r13,r13,0xc000	; or in the opcode for branch
	st	r13,r0,8*TR_INT	; install new branch instruction in vector table

/* Install a branch instruction in the exception table entry for breakpoint. */
	or.u	r13,r0,hi16(_breakpoint_ex)
	or	r13,r13,lo16(_breakpoint_ex)
	subu	r13,r13,8*TR_BPT
	ext	r13,r13,0<2>	; r13 := word offset for branch instruction
	and.u	r13,r13,0x03ff	; zero opcode field of branch instruction
	or.u	r13,r13,0xc000	; or in the opcode for branch
	st	r13,r0,8*TR_BPT	; install new branch instruction in vector table

/* Install a branch intruction in the exception table entry for single-step */
	or.u	r13,r0,hi16(_single_step_ex)
	or	r13,r13,lo16(_single_step_ex)
	subu	r13,r13,8*TR_TRC
	ext	r13,r13,0<2>	; r13 := word offset for branch instruction
	and.u	r13,r13,0x03ff	; zero opcode field of branch instruction
	or.u	r13,r13,0xc000	; or in the opcode for branch
	st	r13,r0,8*TR_TRC	; install new branch instruction in vector table

/* Install a branch intruction in the exception table entry for dacc */
	or.u	r13,r0,hi16(_data_acc_ex)
	or	r13,r13,lo16(_data_acc_ex)
	subu	r13,r13,8*TR_DACC
	ext	r13,r13,0<2>	; r13 := word offset for branch instruction
	and.u	r13,r13,0x03ff	; zero opcode field of branch instruction
	or.u	r13,r13,0xc000	; or in the opcode for branch
	st	r13,r0,8*TR_DACC; install new branch instruction in vector table

	jmp	r1

/*
 * This uses SR1, saves r1 in the var area, make r3 point to the
 * var area, leaves the user's r3 in SR3. and branches to _save_state_routine.
 */

#define SAVE_STATE(code) \
	stcr	r3,SR1                                               // \
	or.u	r3,r0,hi16(whoami)                                   // \
	ld	r3,r3,lo16(whoami)                                   // \
	mak	r3,r3,4<2>                                           // \
	or.u	r3,r3,hi16(mon_data_area_table)                      // \
	ld	r3,r3,lo16(mon_data_area_table)                      // \
	st	r1,r3,DM_REG_R1                                      // \
	or	r1,r0,code                                           // \
	br	_save_state_routine


/* The way we could like to write it:
#define SAVE_STATE(code) 
	stcr	r3,SR3          
	or.u	r3,r0,hi16(whoami)
	ld	r3,r3,lo16(whoami)
	mak	r3,r3,4<2>
	or.u	r3,r3,hi16(mon_data_area_table)
	ld	r3,r3,lo16(mon_data_area_table)
	st	r1,r3,DM_REG_R1 
	or	r1,r0,code     
	br	_save_state_routine

*/

/*
 * This should only be branched to by the SAVE_STATE macro.  We
 * enter with: SR3 having the user's r3, r3 having a pointer to the
 * monitor area, DM_REG_R1 has user's r1, and r1 has the exception code.
 */

_save_state_routine:
	st	r1,r3,DM_EXCEPTION_CODE
	st	r2,r3,DM_REG_R2

; Save the most of the control registers before synchronizing the
; FPU.

	ldcr	r1,SR0
	st	r1,r3,DM_REG_SR0

	ldcr	r1,SR1
	st	r1,r3,DM_REG_SR1

	ldcr	r1,SR2
	st	r1,r3,DM_REG_SR2

	ldcr	r1,SR3
	st	r1,r3,DM_REG_SR3

	ldcr	r1,PID
	st	r1,r3,DM_REG_PID

	ldcr	r1,VBR
	st	r1,r3,DM_REG_VBR

	ldcr	r1,SSBR
	st	r1,r3,DM_REG_SSBR

	fldcr	r1,FPECR
	st	r1,r3,DM_REG_FPECR

	fldcr	r1,FPHS1
	st	r1,r3,DM_REG_FPHS1

	fldcr	r1,FPLS1
	st	r1,r3,DM_REG_FPLS1

	fldcr	r1,FPHS2
	st	r1,r3,DM_REG_FPHS2

	fldcr	r1,FPLS2
	st	r1,r3,DM_REG_FPLS2

	fldcr	r1,FPPT
	st	r1,r3,DM_REG_FPPT

	fldcr	r1,FPRH
	st	r1,r3,DM_REG_FPRH

	fldcr	r1,FPRL
	st	r1,r3,DM_REG_FPRL

	fldcr	r1,FPIT
	st	r1,r3,DM_REG_FPIT

	fldcr	r1,FPSR
	st	r1,r3,DM_REG_FPSR

	fldcr	r1,FPCR
	st	r1,r3,DM_REG_FPCR

	ldcr	r1,DMD0	
	st	r1,r3,DM_REG_DMD0

	ldcr	r1,DMA0	
	st	r1,r3,DM_REG_DMA0

; If any data memory operations have outstanding, zero the SSBR bits
; corresponding to their destination registers.  If we don't do this,
; the tb1 instruction below will hang.

        tb1     0,r0,511                ; work around chip bug.
	ldcr	r1,DMT0
	st	r1,r3,DM_REG_DMT0
	bb0	DMT_VALID_BIT,r1,label_1f
	bb1	DMT_READBAR_BIT,r1,label_1f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR

label_1f:	ldcr	r1,DMD1                    
	st	r1,r3,DM_REG_DMD1

	ldcr	r1,DMA1	
	st	r1,r3,DM_REG_DMA1

	ldcr	r1,DMT1
	st	r1,r3,DM_REG_DMT1

	bb0	DMT_VALID_BIT,r1,label_2f
	bb1	DMT_READBAR_BIT,r1,label_2f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR

label_2f:	ldcr	r1,DMD2
	st	r1,r3,DM_REG_DMD2

	ldcr	r1,DMA2
	st	r1,r3,DM_REG_DMA2

	ldcr	r1,DMT2
	st	r1,r3,DM_REG_DMT2

	bb0	DMT_VALID_BIT,r1,label_3f
	bb1	DMT_READBAR_BIT,r1,label_3f	; stores don't set SBR bits.
	extu	r1,r1,DMT_DREG_FIELD	; r1 := register number of load dest.
	or	r1,r1,0x20		; 1-bit-wide field, used in clr instr.
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR

label_3f:

; Check to see if we got a precise floating point exception.  If we did
; we need to clear the scoreboard bit for the destination register of the
; faulting instruction.

	fldcr	r1,FPECR
	mask	r1,r1,0xf8		; Just look at precise exception bits
	bcnd	eq0,r1,no_precise_fp_exception
	fldcr	r1,FPPT
	mask	r1,r1,0x1f		; Get register number of destination
	or	r1,r1,0x20
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR
	fldcr	r2,FPPT
	bb0	5,r2,single_precision

; The precise fp exception was the result of an instruction with a double
; precision result.  So we have to clear the scoreboard bit for the
; second destinaion register.

	addu	r1,r1,1
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR

no_precise_fp_exception:
single_precision:

; Check to see if we got an imprecise floating point exception.  If we
; did and it was the result of an instruction with a double precision
; result, we need to clear the scoreboard bit for the second destination
; register..
	fldcr	r1,FPECR
	mask	r1,r1,0x7		; Just look at the imprecise bits
	bcnd	eq0,r1,no_imprecise_fp_exception
	fldcr	r1,FPIT
	mask	r1,r1,0x1f		; Get register number of destination
	addu	r1,r1,0x21		; Mask clr reg and add one.
	ldcr	r2,SSBR
	clr	r2,r2,r1
	stcr	r2,SSBR

no_imprecise_fp_exception:

; Save the shadow, trap-time, and SR0 registers.
	ldcr	r1,SXIP			; save instruction pipeline
	st	r1,r3,DM_REG_SXIP
	ldcr	r1,SNIP
	st	r1,r3,DM_REG_SNIP

        bb1     1,r1,label_9f                 ; use SFIP if SNIP is not valid.
        ldcr    r1,SFIP
label_9f:
        and     r1,r1,lo16(~3)          ; clear the valid and exception bits.
        st      r1,r3,DM_REG_IP            ; for the debugger's benefit.

	ldcr	r1,SFIP
	st	r1,r3,DM_REG_SFIP
	ldcr	r1,TPSR			; save trap-time PSR in both the slot
	st	r1,r3,DM_REG_TPSR	; for the tpsr and the psr.
	st	r1,r3,DM_REG_PSR

; Set the SNIP to zero and the SFIP to point to the "tb1 0,r0,0" 
; instruction below.  When we execute the rte
; instruction, we will transfer to the tb1 instruction.  The tb1 instruction
; will wait for the FP unit to serialize (finish).  In the course of this, it
; may take a floating point imprecise exception.

; We may have to execute this trap instruction several times,
; once for each floating exception that's waiting to happen.

; Point the SNIP at the tb1 instruction.
; Point the SFIP at the instruction following the tb1.

	or.u	r1,r0,hi16(_serializing_tb1)
	or	r1,r1,lo16(_serializing_tb1+IP_VALID)
	stcr	r1,SNIP
	addu	r1,r1,4
	stcr	r1,SFIP

; Set up PSR to enable floating point and enable exceptions and leave
; interrupts disabled.
	ldcr	r1,PSR
	and	r1,r1,lo16(~(PSR_SFD1|PSR_SFRZ))
	stcr	r1,TPSR
  
; Load up user's registers now and save them when done serializing,
; in case one of them will be written by the FPU.  
	ld	r1,r3,DM_REG_R1
	ld	r2,r3,DM_REG_R2
	xcr	r3,r3,SR1         ; Restore user's r3, save mon pointer in SR1.

; Execute the trap instruction with the FPU enabled, to serialize it.
; When done serializing, the trap will take us to Xint_serialized
; with exceptions and the FPU disabled again.
        tb1     0,r0,511    /* trap not taken required for bug in 88K silicon */
	rte	""			; turn on FPU, enable exceptions

_serializing_tb1:
	tb1	0,r0,0			; This will never trap.

	xcr	r3,r3,SR1	; Save user's r3, restore mon area pointer to r3

; Save the user's general regisisters in the regs[] array.
	st	r1,r3,DM_REG_R1
	st	r2,r3,DM_REG_R2

	ldcr	r1,SR1		; Get user's r3 back
	st	r1,r3,DM_REG_R3

	st.d	r4,r3,DM_REG_R4
	st.d	r6,r3,DM_REG_R6
	st.d	r8,r3,DM_REG_R8
	st.d	r10,r3,DM_REG_R10
	st.d	r12,r3,DM_REG_R12
	st.d	r14,r3,DM_REG_R14
	st.d	r16,r3,DM_REG_R16
	st.d	r18,r3,DM_REG_R18
	st.d	r20,r3,DM_REG_R20
	st.d	r22,r3,DM_REG_R22
	st.d	r24,r3,DM_REG_R24
	st.d	r26,r3,DM_REG_R26
	st.d	r28,r3,DM_REG_R28
	st.d	r30,r3,DM_REG_R30

	br	debug_mon_entry

/*
 * This file contains all the entry points into the monitor code and
 * the mechanisms which save and restore the user context.
 */

_single_step_trap:
	tb0	0,r0,TR_TRC

_interrupt_ex:
; If the interrupt happened while we were in the monitor, then don't
; save the registers, just re-enable the FPU and drop into the main
; monitor loop.
	stcr	r3,SR1
	or.u	r3,r0,hi16(whoami)
	ld	r3,r3,lo16(whoami)
	mak	r3,r3,4<2>
	or.u	r3,r3,hi16(mon_data_area_table)
	ld	r3,r3,lo16(mon_data_area_table)
	ld	r3,r3,DM_IN_MON
	xor	r3,r3,lo16(IN_MON_MAGIC)
	xor.u	r3,r3,hi16(IN_MON_MAGIC)
	bcnd	eq0,r3,interrupt_while_in_mon
	ldcr	r3,SR1
	SAVE_STATE(TR_INT)

interrupt_while_in_mon:
	bsr	_our_var_area
	or	r3,r0,TR_INT
	st	r3,r2,DM_EXCEPTION_CODE

; Save the address that the processor choked on so that gdb can look
; at it and tell the user.
	ldcr	r5,DMA0
	st	r5,r2,DM_DACC_ADDRESS
	br	reenable_fpu


_data_acc_ex:
; If the dacc happened while we were in the monitor, then don't
; save the registers, just re-enable the FPU and drop into the main
; monitor loop.
	stcr	r3,SR1
	or.u	r3,r0,hi16(whoami)
	ld	r3,r3,lo16(whoami)
	mak	r3,r3,4<2>
	or.u	r3,r3,hi16(mon_data_area_table)
	ld	r3,r3,lo16(mon_data_area_table)
	ld	r3,r3,DM_IN_MON
	xor	r3,r3,lo16(IN_MON_MAGIC)
	xor.u	r3,r3,hi16(IN_MON_MAGIC)
	bcnd	eq0,r3,dacc_while_in_mon
	ldcr	r3,SR1
	SAVE_STATE(TR_DACC)

dacc_while_in_mon:
	bsr	_our_var_area
	or	r3,r0,TR_DACC
	st	r3,r2,DM_EXCEPTION_CODE
	br	reenable_fpu

_single_step_ex:		
	stcr	r3,SR1
	or.u	r3,r0,hi16(whoami)
	ld	r3,r3,lo16(whoami)
	mak	r3,r3,4<2>
	or.u	r3,r3,hi16(mon_data_area_table)
	ld	r3,r3,lo16(mon_data_area_table)
	st	r1,r3,DM_REG_R1       
	or	r1,r0,TR_TRC
	br	_save_state_routine

_breakpoint_ex:		SAVE_STATE(TR_BPT)

/* Try to get the lock.  Return 1 if we got it, zero if we didn't 
   In any case, disable interrupts. */

_acquire_host_comm_mutex:
	ldcr	r3,PSR
	or	r3,r3,PSR_IND		; Disable interrupt while in monitor.
	stcr	r3,PSR			; as a test. -rcb 4/15/90
	or.u	r4,r0,hi16(whoami)	; For debugging, put our whoami register
	ld	r4,r4,lo16(whoami)	; contents in the semaphore.
	or.u	r3,r0,hi16(mutex)
	xmem	r4,r3,lo16(mutex)
	or	r2,r0,r0
	bcnd	ne0,r4,didnt_get_it
	or	r2,r0,1
didnt_get_it:
	jmp	r1

/* release_host_comm_mutex(interrupt_enable).  Release the lock on
   host communications, reenable interrupts if the passed flag is true. */

_release_host_comm_mutex:
	or.u	r3,r0,hi16(mutex)
	st	r0,r3,lo16(mutex)
	bcnd	eq0,r2,leave

	ldcr	r3,PSR
	and	r3,r3,lo16(~PSR_IND)
	stcr	r3,PSR
leave:
	jmp	r1

; Delay the forked CPUs for a while to make sure that CPU 0 gets 
; into the monitor first.  We do this as a convenience to the user, 
; who may have code that only runs on CPU 0.

_forked_cpu_entry:
	or	r2,r0,0xffff
fork_wait_loop:
	sub	r2,r2,1
	bcnd	ne0,r2,fork_wait_loop
	br	fork_reentry
