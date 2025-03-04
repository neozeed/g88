/*	psl.h	1.1	(Berkeley)	83/04/07	*/
/*
 * psl.h
 *
 * $Header: /tmp_mnt/u2/cs568/g88/hmon/RCS/psl.h,v 1.2 90/05/16 10:23:19 robertb Exp $
 * $Locker:  $
 *
	Copyright (c) 1987, 1988, 1989, 1990 by Tektronix, Inc.

    It may freely be redistributed and modified so long as the copyright
    notices and credit attributions remain intact.

 * MC78000 Integer Unit control registers (including the processor 
 * status register)
 */


/*
 * In the following defines,
 *	NAME is a mask to AND against a word to select the field;
 *	NAME_BIT is defined for single-bit masks; it's the bit number,
 *		for use in BB1 instruction;
 *	NAME_FIELD is of the form width<offset>, for use in field manipulation
 *		instructions.
 */

/*
 * Here are the processor control register numbers for the integer control
 * unit.  These names match those defined in the MC78000 User's Manual,
 * Revision 0.4, dated October 7, 1987.
 */
#define	PID	cr0		/* processor identification register */
#define	PSR	cr1		/* processor status register */
#define	TPSR	cr2		/* trap time processor status register */
#define	SSBR	cr3		/* shadow scoreboard register */
#define	SXIP	cr4		/* shadow execute instruction pointer */
#define	SNIP	cr5		/* shadow next instruction pointer */
#define	SFIP	cr6		/* shadow fetch instruction pointer */
#define	VBR	cr7		/* vector base register */
#define	DMT0	cr8		/* data memory transaction register 0 */
#define	DMD0	cr9		/* data memory data register 0 */
#define	DMA0	cr10		/* data memory address register 0 */
#define	DMT1	cr11		/* data memory transaction register 1 */
#define	DMD1	cr12		/* data memory data register 1 */
#define	DMA1	cr13		/* data memory address register 1 */
#define	DMT2	cr14		/* data memory transaction register 2 */
#define	DMD2	cr15		/* data memory data register 2 */
#define	DMA2	cr16		/* data memory address register 2 */
#define	SR0	cr17		/* supervisor storage register 0 */
#define	SR1	cr18		/* supervisor storage register 1 */
#define	SR2	cr19		/* supervisor storage register 2 */
#define	SR3	cr20		/* supervisor storage register 3 */

/*
 * PID: processor identification register
 */
#define PID_VERSION_FIELD 7<1>	/* identifies particular processor*/
#define PID_VERSION	0x000000fe
#define PID_CHECKER_BIT	1	/* cpu is running in checker mode*/
#define	PID_CHECKER	(1<<PID_CHECKER_BIT)

/*
 * PSR: processor status register
 * TPSR: trapped processor status register
 */
#define PSR_MODE_BIT	31		/* supervisor mode */
#define PSR_MODE	(1<<PSR_MODE_BIT)
#define PSR_BO_BIT	30		/* little-endian byte ordering */
#define PSR_BO		(1<<PSR_BO_BIT)
#define PSR_SER_BIT	29		/* serial (not concurrent) operation */
#define PSR_SER		(1<<PSR_SER_BIT)
#define PSR_C_BIT	28		/* arithmetic carry */
#define PSR_C		(1<<PSR_C_BIT)
#define PSR_SFD1_BIT	3		/* SFU1 (FPU) disabled */
#define PSR_SFD1	(1<<PSR_SFD1_BIT)
#define PSR_MXM_BIT	2		/* misaligned data access exception
					   disabled */
#define PSR_MXM		(1<<PSR_MXM_BIT)
#define PSR_IND_BIT	1		/* interrupt disabled */
#define PSR_IND		(1<<PSR_IND_BIT)
#define PSR_SFRZ_BIT	0		/* shadow registers frozen (all
					   exceptions disabled) */
#define PSR_SFRZ	(1<<PSR_SFRZ_BIT)

/* Bit definitions for printf("%b") */
#define	PSR_BITS	"\1SFRZ\2IND\3MXM\4SFD1\35C\36SER\37BO\40MODE"

/*
 * SXIP: shadow execute instruction pointer
 * SNIP: shadow next instruction pointer
 * SFIP: shadow fetch instruction pointer
 */
#define IP_PC_FIELD	30<2>		/* program counter for instruction */
#define	IP_PC		0xfffffffc
#define IP_VALID_BIT	1		/* instruction is valid */
#define IP_VALID	(1<<IP_VALID_BIT)
#define IP_FAULTED_BIT	0		/* not delivered due to data fault */
#define IP_FAULTED	(1<<IP_FAULTED_BIT)

/*
 * VBR: vector base register
 */
#define VBR_MASK_FIELD	24<12>		/* the active bits in this register */
#define VBR_MASK	0xfffff000

/*
 * DMT0, DMT1, DMT2: data memory transaction registers
 */
#define DMT_B0_BIT	15		/* little endian byte ordering */
#define	DMT_BO		(1<<DMT_B0_BIT)
#define	DMT_DAS_BIT	14		/* supervisor data memory space */
#define	DMT_DAS		(1<<DMT_DAS_BIT)
#define	DMT_DOUB1_BIT	13		/* first half of double word trnsctn */
#define	DMT_DOUB1	(1<<DMT_DOUB1_BIT)
#define	DMT_LCK_BIT	12		/* lock memory bus (xmem inst) */
#define	DMT_LCK		(1<<DMT_LCK_BIT)
#define	DMT_DREG_FIELD	5<7>		/* destination register number */
#define	DMT_DREG	0x00000f80
#define	DMT_SIGNED_BIT	6		/* loaded value will be sign extnded */
#define	DMT_SIGNED	(1<<DMT_SIGNED_BIT)
#define	DMT_ST3_BIT	5		/* strobe for most significant byte */
#define	DMT_ST3		(1<<DMT_ST3_BIT)
#define	DMT_ST2_BIT	4		/* strobe for second-most sig byte */
#define	DMT_ST2		(1<<DMT_ST2_BIT)
#define	DMT_ST1_BIT	3		/* strobe for second-least sig byte */
#define	DMT_ST1		(1<<DMT_ST1_BIT)
#define	DMT_ST0_BIT	2		/* strobe for least significant byte */
#define	DMT_ST0		(1<<DMT_ST0_BIT)
#define	DMT_READBAR_BIT	1		/* operation is write */
#define	DMT_READBAR	(1<<DMT_READBAR_BIT)
#define	DMT_VALID_BIT	0		/* transaction is not a nop */
#define	DMT_VALID	(1<<DMT_VALID_BIT)

/*
 * Processor control registers for the floating point unit.
 */
#define FPECR	fcr0		/* FP exception cause register */
#define FPHS1	fcr1		/* FP source 1 operand high register */
#define FPLS1	fcr2		/* FP source 1 operand low register */
#define FPHS2	fcr3		/* FP source 2 operand high register */
#define FPLS2	fcr4		/* FP source 2 operand low register */
#define FPPT	fcr5		/* FP precise operation type register */
#define FPRH	fcr6		/* FP result high register */
#define FPRL	fcr7		/* FP result low register */
#define FPIT	fcr8		/* FP imprecise control register */
#define FPSR	fcr62		/* FP status register (user accessible) */
#define FPCR	fcr63		/* FP control register (user accessible) */

/*
 * FPECR: floating point exception cause register
 */
		/* The following bits are meaningful on a precise exception. */
#define FPECR_FIOV_BIT	7		/* conversion to integer overflow */
#define FPECR_FIOV	(1<<FPECR_FIOV_BIT)
#define FPECR_FUNIMP_BIT 6		/* unimplemented FP instruction */
#define FPECR_FUNIMP	(1<<FPECR_FUNIMP_BIT)
#define FPECR_FPRV_BIT	5		/* privilege violation (user access
					   to control register other than
					   FPSR or FPCR) */
#define FPECR_FPRV	(1<<FPECR_FPRV_BIT)
#define FPECR_FROP_BIT	4		/* reserved operand */
#define FPECR_FROP	(1<<FPECR_FROP_BIT)
#define FPECR_FDVZ_BIT	3		/* floating divide-by-zero */
#define FPECR_FDVZ	(1<<FPECR_FDVZ_BIT)

		/* The following bits are UNDEFINED on a precise exception. */
#define FPECR_FUNF_BIT	2		/* floating underflow */
#define FPECR_FUNF	(1<<FPECR_FUNF_BIT)
#define FPECR_FOVF_BIT	1		/* floating overflow */
#define FPECR_FOVF	(1<<FPECR_FOVR_BIT)
#define FPECR_FINX_BIT	0		/* floating inexact */
#define FPECR_FINX	(1<<FPECR_FINX)

/*
 * FPHS1: floating point source 1 operand high register
 * FPHS2: floating point source 2 operand high register
 */
#define FPHS_SIGN_BIT	31		/* sign bit */
#define FPHS_SIGN	(1<<FPHS1_SIGN_BIT)
#define FPHS_EXPONENT_FIELD 11<20>	/* exponent */
#define FPHS_EXPONENT	0x7FF00000
#define FPHS_MANTISSA_FIELD 20<0>	/* mantissa */
#define FPHS_MANTISSA	0x000FFFFF

/*
 * FPPT: floating point precise operation type register
 */
#define FPPT_OPERATION_FIELD 11<5>	/* bits 15-5 of faulting instruction */
#define FPPT_OPERATION	0x0000FFE0
#define FPPT_DEST_FIELD	5<0>		/* destination register number */
#define FPPT_DEST	0x0000001F

/*
 * FPRH: floating point result high register
 * The kernel must use this register to construct the result if the
 * exception handler is disabled through the appropriate bit in the FPCR.
 */
#define FPRH_SIGN_BIT	31		/* result sign */
#define FPRH_SIGN	(1<<FPRH_SIGN_BIT)
#define FPRH_RNDMODE_FIELD 2<29>	/* rounding mode for the result:
					   00 - round to nearest
					   01 - round towards zero
					   10 - round towards -infinity
					   11 - round towards +infinity */
#define FPRH_RNDMODE	0x60000000
#define FPRH_GUARD_BIT	28		/* guard bit for the result */
#define FPRH_GUARD	(1<<FPRH_GUARD_BIT)
#define FPRH_ROUND_BIT	27		/* round bit for the result */
#define FPRH_ROUND	(1<<FPRH_ROUND_BIT)
#define FPRH_STICKY_BIT	26		/* sticky bit for the result */
#define FPRH_STICKY	(1<<FPRH_STICKY_BIT)
#define FPRH_ADDONE_BIT	25		/* mantissa was rounded by adding 1 */
#define FPRH_ADDONE	(1<<FPRH_ADDONE_BIT)
#define FPRH_HIGH21_FIELD 21<0>		/* high 21 bits of result mantissa */
#define FPRH_HIGH21	0x001FFFFF

/*
 * FPIT: floating point imprecise control register
 */
#define FPIT_RESEXP_FIELD 12<20>	/* result exponent, zero if result=0 */
#define FPIT_RESEXP	0xFFF00000
#define FPIT_OPERATION_FIELD 5<11>	/* bits 15-11 of faulting instructn */
#define FPIT_OPERATION	0x0000F800
#define FPIT_DESTSIZE_BIT 10		/* double precision destination */
#define FPIT_DESTSIZE	(1<<FPIT_DESTSIZE_BIT)
#define FPIT_EFINV_BIT	9		/* enable invalid operand handler */
#define FPIT_EFINV	(1<<FPIT_EFINV_BIT)
#define FPIT_EFDVZ_BIT	8		/* enable divide-by-zero handler */
#define FPIT_EFDVZ	(1<<FPIT_EFDVZ_BIT)
#define FPIT_EFUNF_BIT	7		/* enable underflow handler */
#define FPIT_EFUNF	(1<<FPIT_EFUNF_BIT)
#define FPIT_EFOVF_BIT	6		/* enable overflow handler */
#define FPIT_EFOVF	(1<<FPIT_EFOVF_BIT)
#define FPIT_EFINX_BIT	5		/* enable inexact result handler */
#define FPIT_EFINX	(1<<FPIT_EFINX_BIT)
#define FPIT_DEST_FIELD	5<0>		/* destination register number */
#define FPIT_DEST	0x0000001F

/*
 * FPSR: floating point status register
 */
		/* The following bits are set by software (the kernel). */
#define FPSR_AFINV_BIT	4		/* accumulated invalid op flag */
#define FPSR_AFINV	(1<<FPSR_AFINV_BIT)
#define FPSR_AFDVZ_BIT	3		/* accumulated divide-by-zero flag */
#define FPSR_AFDVZ	(1<<FPSR_AFDVZ_BIT)
#define FPSR_AFUNF_BIT	2		/* accumulated underflow flag */
#define FPSR_AFUNF	(1<<FPSR_AFUNF_BIT)
#define FPSR_AFOVF_BIT	1		/* accumulated overflow flag */
#define FPSR_AFOVF	(1<<FPSR_AFOVF_BIT)

		/* The following bit is set by hardware. */
#define FPSR_AFINX_BIT	0		/* accumulated inexact flag */
#define FPSR_AFINX	(1<<FPSR_AFINX_BIT)

/*
 * FPCR: floating point control register
 */
#define FPCR_RM_FIELD	2<14>		/* floating point rounding mode:
					   00 - round to nearest
					   01 - round towards zero
					   10 - round towards -infinity
					   11 - round towards +infinity */
#define FPCR_RM		0x0000C000
#define FPCR_EFINV_BIT	4		/* enable invalid op handler */
#define FPCR_EFINV	(1<<FPCR_EFINV_BIT)
#define FPCR_EFDVZ_BIT	3		/* enable divide-by-zero handler */
#define FPCR_EFDVZ	(1<<FPCR_EFDVZ_BIT)
#define FPCR_EFUNF_BIT	2		/* enable underflow handler */
#define FPCR_EFUNF	(1<<FPCR_EWFUNF_BIT)
#define FPCR_EFOVF_BIT	1		/* enable overflow handler */
#define FPCR_EFOVF	(1<<FPCR_EFOVF_BIT)
#define FPCR_EFINX_BIT	0		/* enable inexact handler */
#define FPCR_EFINX	(1<<FPCR_EFINX_BIT)
