/*
 * montraps.h	Trap codes ( How we got into main() )
 *		part of debug monitor
 *
	Copyright (c) 1987, 1988, 1989, 1990 by Tektronix, Inc.

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.

 * $Header: /tmp_mnt/u2/cs568/g88/hmon/RCS/montraps.h,v 1.2 90/05/16 10:23:16 robertb Exp $
 * $Locker:  $
 */

#define TR_RESET	0	/* Power-on reset (pseudo trap)		*/
#define	TR_INT		1	/* Interrupt.							*/
#define	TR_CACC		2	/* Code access fault.					*/
#define	TR_DACC		3	/* Data access fault.                   */
#define	TR_MA		4	/* Misaligned access.					*/
#define	TR_OPC		5	/* Unimplemented opcode.				*/
#define	TR_PRV		6	/* Privilege violation.					*/
#define	TR_BND		7	/* Array bounds violation.				*/
#define	TR_IDE		8	/* Integer divide error.				*/
#define	TR_IOV		9	/* Integer overflow.					*/
#define	TR_ERR		10	/* Occurs when an exception occurs with ex dis */

#define TR_TRC		253	/* Trace trap.                          */
#define	TR_BPT		254	/* Breakpoint trap.						*/
#define	TR_BAD		14	/* Bad (unexpected) trap.				*/
