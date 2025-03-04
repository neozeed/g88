/*
 * Macros for getting various fields of 78000 instructions.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 * 
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/fields88.h,v 1.4 88/02/12 16:54:16 robertb Exp $
 */

#define OPCODE(i) (((i) >> 27) & 0x1f)
#define OFFSET(i) ((i) & 0xffff)
#define B5(i)     (((i) >> 21) & 0x1f)
#define	W5(i)	  (((i) >> 5) & 0x1f)
#define	O5(i)     ((i) & 0x1f)
#define S1(i)     (((i) >> 16) & 0x1f)
#define S2(i)     ((i) & 0x1f)
#define D(i)      (((i) >> 21) & 0x1f)
#define M5(i)      (((i) >> 21) & 0x1f)
#define LIT16(i)  ((i) & 0xffff)
#define I(i)      (((i) >> 9) & 1)
#define CONDITION(i) (((i) >> 21) & 0x1f)
#define	REL16(i)  (((((int)i) & 0xffff) << 16) >> 14)
#define	REL26(i)  (((((int)i) & 0x3ffffff) << 6) >> 4)
#define	FCRS(i)	  (((i) >> 5) & 0x3f)
#define	CRS(i)	  (((i) >> 5) & 0x3f)
#define	VEC(i)	  ((i) & 0x1ff)
#define	SFU(i)	  (((i) >> 11) & 7)
