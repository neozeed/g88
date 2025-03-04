/*
 * Data structure for disassembly.
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/disasm.h,v 1.2 88/05/06 10:40:19 robertb Exp $"
 */
struct instr {
	unsigned opcode;		/* Opcode of instruction. 				*/
	unsigned opmask;		/* Mask to use before comparing opcode. */
    struct instr *link;     /* For speeding-up instruction lookup   */
	char	 *mnemonic;		/* String programmers use.				*/
	char	 format;		/* Type of instruction.					*/
	char	 modifies_reg;	/* 1 ==> modifies the reg in dest field */
};

/*
 * Valid values for the 'modifies_reg' field.
 */
#define	NOREG	0
#define	ONEREG	1
#define	TWOREG	2

struct instr *instruction_lookup();
