/*
 * DEFS.h
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/DEFS.h,v 1.2 88/01/25 15:44:43 dougs Exp $
 * $Locker:  $
 *
 * Copyright (c) 1986, Tektronix Inc.
 * All Rights Reserved
 *
 */


#ifdef PROF
#define	ENTRY(x,y,m)	.globl _/**/x; .quad;  _/**/x: link a6,#-y; \
			movem.l #m,-y(a6); lea _p_/**/x,a0; \
			jsr mcount; .data; _p_/**/x: .long 0; .text

#define	Q_ENTRY(x)	.globl _/**/x; .quad; _/**/x: link a6,#0;\
   			lea _p_/**/x,a0; jsr mcount;\
			.data; _p_/**/x: .long 0; .text

/* profiling for longjmp, similar to sys	*/
#define	L_ENTRY(x)	.globl _/**/x; .quad; _/**/x: \
			.data; _p_/**/x: .long 0; \
			.text; link a6,#0; lea _p_/**/x,a0; \
			 jsr mcount; unlk a6 


.globl mcount

#define	ARG0	8(a6)		/* access first parameter		*/
#define	ARG1	16(a6)		/* access 2nd parameter, 1st is double	*/
#define	ARG1I	12(a6)		/* access 2nd parameter, 1st is int	*/

#define EXITM	done:   unlk    a6; \
			rts

#else
#define	ENTRY(x,y,m)	.globl _/**/x; .quad; _/**/x: \
			link a6,#-y; movem.l #m,-y(a6)

#define	Q_ENTRY(x)	.globl _/**/x; .quad; _/**/x: link a6,#0

#define	L_ENTRY(x)	.globl _/**/x; .quad; _/**/x: 


#define	ARG0	8(a6)		/* access first parameter		*/
#define	ARG1	16(a6)		/* access 2nd parameter, 1st is double	*/
#define	ARG1I	12(a6)		/* access 2nd parameter, 1st is int	*/

#define EXITM	done: 	unlk    a6; \
			rts

#endif

