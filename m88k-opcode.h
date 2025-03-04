/*
*			Disassembler Instruction Table
*
*	The first field of the table is the opcode field. 
*	The second parameter is a pointer to the
*	instruction mnemonic. Each operand is specified by offset, width,
*	and type. The offset is the bit number of the least significant
*	bit of the operand with bit 0 being the least significant bit of
*	the instruction. The width is the number of bits used to specify
*	the operand. The type specifies the output format to be used for
*	the operand. The valid formats are: register, register indirect,
*	hex constant, and bit field specification.  The last field is a
*	pointer to the next instruction in the linked list.  These pointers
*	are initialized by init_disasm().
*
*				Revision History
*
*	Revision 1.0	11/08/85	Creation date
*		 1.1	02/05/86	Updated instruction mnemonic table MD
*		 1.2	06/16/86	Updated SIM_FLAGS for floating point
*		 1.3	09/20/86	Updated for new encoding
*		 	05/11/89	R. Trawick adapted from Motorola disassembler
*                       09/08/89        P. Gilliam Gutted and raped for easyer use by gdb.

   $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/m88k-opcode.h,v 1.5 90/08/02 12:22:02 robertb Exp $
   $Locker:  $

*/

#include <stdio.h>

/* Hashing Specification */

#define HASHVAL     79

/* Constants and Masks */

#define SFU0       0x80000000
#define SFU1       0x84000000
#define SFU7       0x9c000000
#define RRI10      0xf0000000
#define RRR        0xf4000000
#define SFUMASK    0xfc00ffe0
#define RRRMASK    0xfc00ffe0
#define RRI10MASK  0xfc00fc00
#define DEFMASK    0xfc000000
#define CTRL       0x0000f000
#define CTRLMASK   0xfc00f800

/* Macros */

#define UEXT(src,off,wid)  ((((unsigned int)src)>>off) & ((1<<wid) - 1))
#define SEXT(src,off,wid)  (((((int)src)<<(32-(off+wid))) >>(32-wid)) )

/* Structure templates */

typedef struct {
   unsigned int offset:5;
   unsigned int width:6;
   unsigned int type:5;
} OPSPEC;

/* Operands types */

#define HEX          1    /* hexadecimal */
#define REG          2    /* register */
#define BF           4    /* bit field */
#define REGSC        5    /* scaled register */
#define CRREG        6    /* control register */
#define FCRREG       7    /* floating point control register */
#define PCREL	     8    /* PC relative (branch offset) */
#define CONDMASK     9    /* bcnd mask */
#define CMPRSLT     10	  /* result of cmp instruction (bb0/1, tb0/1) */

typedef struct INSTRUCTAB {
   unsigned int  opcode;
   char          *mnemonic;
   OPSPEC        op1,op2,op3;
   int  effect;
   struct INSTRUCTAB    *next;
} INSTAB;

/* Instruction effects */

#define DONTCARE     0    /* Has no effect that we care about. */
#define ALTERS       1    /* Changes the content of the destination register. */
#define SAVES        2    /* Stores the destination register in memory */
#define GROWS        4    /* Could be used to grow the stack */
#define SHRINKS      8    /* Could be used to shrink the stack */
#define MOVES        16   /* Could be used to move the contents of one register to another */
#define DBL          32   /* Does two words at a time. */


#ifdef GENERATE_TABLE

static INSTAB  instructions[] = {
    
/*   Opcode       Mnemonic         Op 1 Spec         Op 2 Spec      Op 3 Spec      effect             Next*/
                            
  {  0xf400c800,  "jsr         ",  {0,5,REG},        {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xf400cc00,  "jsr.n       ",  {0,5,REG},        {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xf400c000,  "jmp         ",  {0,5,REG},        {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xf400c400,  "jmp.n       ",  {0,5,REG},        {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xc8000000,  "bsr         ",  {0,26,PCREL},     {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xcc000000,  "bsr.n       ",  {0,26,PCREL},     {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xc0000000,  "br          ",  {0,26,PCREL},     {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xc4000000,  "br.n        ",  {0,26,PCREL},     {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0xd0000000,  "bb0         ",  {21,5,CMPRSLT},   {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xd4000000,  "bb0.n       ",  {21,5,CMPRSLT},   {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xd8000000,  "bb1         ",  {21,5,CMPRSLT},   {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xdc000000,  "bb1.n       ",  {21,5,CMPRSLT},   {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xf000d000,  "tb0         ",  {21,5,CMPRSLT},   {16,5,REG},    {0,10,HEX},    DONTCARE,          NULL },
  {  0xf000d800,  "tb1         ",  {21,5,CMPRSLT},   {16,5,REG},    {0,10,HEX},    DONTCARE,          NULL },
  {  0xe8000000,  "bcnd        ",  {21,5,CONDMASK},  {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xec000000,  "bcnd.n      ",  {21,5,CONDMASK},  {16,5,REG},    {0,16,PCREL},  DONTCARE,          NULL },
  {  0xf000e800,  "tcnd        ",  {21,5,CONDMASK},  {16,5,REG},    {0,10,HEX},    DONTCARE,          NULL },
  {  0xf8000000,  "tbnd        ",  {16,5,REG},       {0,16,HEX},    {0,0,0},       DONTCARE,          NULL },
  {  0xf400f800,  "tbnd        ",  {16,5,REG},       {0,5,REG},     {0,0,0},       DONTCARE,          NULL },
  {  0xf400fc00,  "rte         ",  {0,0,0},          {0,0,0},       {0,0,0},       DONTCARE,          NULL },
  {  0x1c000000,  "ld.b        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4001c00,  "ld.b        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001e00,  "ld.b        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x0c000000,  "ld.bu       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000c00,  "ld.bu       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000e00,  "ld.bu       ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x18000000,  "ld.h        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4001800,  "ld.h        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001a00,  "ld.h        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x08000000,  "ld.hu       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000800,  "ld.hu       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000a00,  "ld.hu       ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x14000000,  "ld          ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4001400,  "ld          ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001600,  "ld          ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x10000000,  "ld.d        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS | DBL,      NULL },
  {  0xf4001000,  "ld.d        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0xf4001200,  "ld.d        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS | DBL,      NULL },
  {  0xf4001d00,  "ld.b.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001f00,  "ld.b.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0xf4000d00,  "ld.bu.usr   ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000f00,  "ld.bu.usr   ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0xf4001900,  "ld.h.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001b00,  "ld.h.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0xf4000900,  "ld.hu.usr   ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000b00,  "ld.hu.usr   ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0xf4001500,  "ld.usr      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4001700,  "ld.usr      ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0xf4001100,  "ld.d.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0xf4001300,  "ld.d.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS | DBL,      NULL },
  {  0x2c000000,  "st.b        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002c00,  "st.b        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002e00,  "st.b        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x28000000,  "st.h        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002800,  "st.h        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002a00,  "st.h        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x24000000,  "st          ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    SAVES,             NULL },
  {  0xf4002400,  "st          ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     SAVES,             NULL },
  {  0xf4002600,  "st          ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x20000000,  "st.d        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    SAVES | DBL,       NULL },
  {  0xf4002000,  "st.d        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     SAVES | DBL,       NULL },
  {  0xf4002200,  "st.d        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x2c000100,  "st.b.usr    ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002d00,  "st.b.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002f00,  "st.b.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x28000100,  "st.h.usr    ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002900,  "st.h.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002b00,  "st.h.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x24000100,  "st.usr      ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002500,  "st.usr      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002700,  "st.usr      ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x20000100,  "st.d.usr    ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    DONTCARE,          NULL },
  {  0xf4002100,  "st.d.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     DONTCARE,          NULL },
  {  0xf4002300,  "st.d.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   DONTCARE,          NULL },
  {  0x00000000,  "xmem.bu     ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000000,  "xmem.bu     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000200,  "xmem.bu     ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x04000000,  "xmem        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000400,  "xmem        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000600,  "xmem        ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x00000100,  "xmem.bu.usr ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000100,  "xmem.bu.usr ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000300,  "xmem.bu.usr ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x04000100,  "xmem.usr    ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4000500,  "xmem.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4000700,  "xmem.usr    ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x3c000000,  "lda.b       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4003c00,  "lda.b       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4003e00,  "lda.b       ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x38000000,  "lda.h       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4003800,  "lda.h       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4003a00,  "lda.h       ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x34000000,  "lda         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4003400,  "lda         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4003600,  "lda         ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS,            NULL },
  {  0x30000000,  "lda.d       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS | DBL,      NULL },
  {  0xf4003000,  "lda.d       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0xf4003200,  "lda.d       ",  {21,5,REG},       {16,5,REG},    {0,5,REGSC},   ALTERS | DBL,      NULL },
  {  0x80004000,  "ldcr        ",  {21,5,REG},       {5,6,CRREG},   {0,0,0},       ALTERS,            NULL },
  {  0x80008000,  "stcr        ",  {16,5,REG},       {5,6,CRREG},   {0,0,0},       DONTCARE,          NULL },
  {  0x8000c000,  "xcr         ",  {21,5,REG},       {16,5,REG},    {5,6,CRREG},   ALTERS,            NULL },
  {  0xf4006000,  "addu        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | SHRINKS,  NULL },
  {  0xf4006200,  "addu.ci     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4006100,  "addu.co     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4006300,  "addu.cio    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x60000000,  "addu        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS | SHRINKS,  NULL },
  {  0xf4006400,  "subu        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | GROWS,    NULL },
  {  0xf4006600,  "subu.ci     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4006500,  "subu.co     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4006700,  "subu.cio    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x64000000,  "subu        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS | GROWS,    NULL },

  {  0xf4006900,  "divu        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  /* New divide opcode */
  {  0xf4006800,  "divu        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0x68000000,  "divu        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },

  {  0xf4006d00,  "mul         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  /* New multiply opcode */
  {  0xf4006c00,  "mul         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0x6c000000,  "mul         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4007000,  "add         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007200,  "add.ci      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007100,  "add.co      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007300,  "add.cio     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x70000000,  "add         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4007400,  "sub         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007600,  "sub.ci      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007500,  "sub.co      ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4007700,  "sub.cio     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x74000000,  "sub         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },

  /* Old div opcode -rcb 8/1/90 */
  {  0xf4007900,  "div         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0xf4007800,  "div         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0x78000000,  "div         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },

  /* Old cmp opcode -rcb */
  {  0xf4007d00,  "cmp         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0xf4007c00,  "cmp         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },

  {  0x7c000000,  "cmp         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4004000,  "and         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0xf4004400,  "and.c       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x40000000,  "and         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0x44000000,  "and.u       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0xf4005800,  "or          ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | MOVES,    NULL },
  {  0xf4005c00,  "or.c        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X58000000,  "or          ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0X5C000000,  "or.u        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0XF4005000,  "xor         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF4005400,  "xor.c       ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X50000000,  "xor         ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0X54000000,  "xor.u       ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0X48000000,  "mask        ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0X4C000000,  "mask.u      ",  {21,5,REG},       {16,5,REG},    {0,16,HEX},    ALTERS,            NULL },
  {  0XF400EC00,  "ff0         ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0XF400E800,  "ff1         ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0XF0008000,  "clr         ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF4008000,  "clr         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF0008800,  "set         ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF4008800,  "set         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF0009000,  "ext         ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF4009000,  "ext         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF0009800,  "extu        ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF4009800,  "extu        ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF000A000,  "mak         ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF400A000,  "mak         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0XF000A800,  "rot         ",  {21,5,REG},       {16,5,REG},    {0,10,BF},     ALTERS,            NULL },
  {  0XF400A800,  "rot         ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84002800,  "fadd.sss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84002880,  "fadd.ssd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84002A00,  "fadd.sds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84002A80,  "fadd.sdd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84002820,  "fadd.dss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X840028A0,  "fadd.dsd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84002A20,  "fadd.dds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84002AA0,  "fadd.ddd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84003000,  "fsub.sss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84003080,  "fsub.ssd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84003200,  "fsub.sds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84003280,  "fsub.sdd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84003020,  "fsub.dss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X840030A0,  "fsub.dsd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84003220,  "fsub.dds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X840032A0,  "fsub.ddd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84000000,  "fmul.sss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84000080,  "fmul.ssd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84000200,  "fmul.sds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84000280,  "fmul.sdd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0X84000020,  "fmul.dss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X840000A0,  "fmul.dsd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84000220,  "fmul.dds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X840002A0,  "fmul.ddd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0X84007000,  "fdiv.sss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84007080,  "fdiv.ssd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84007200,  "fdiv.sds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84007280,  "fdiv.sdd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84007020,  "fdiv.dss    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0x840070a0,  "fdiv.dsd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0x84007220,  "fdiv.dds    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0x840072a0,  "fdiv.ddd    ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS | DBL,      NULL },
  {  0x84003800,  "fcmp.ss     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84003880,  "fcmp.sd     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84003a00,  "fcmp.ds     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84003a80,  "fcmp.dd     ",  {21,5,REG},       {16,5,REG},    {0,5,REG},     ALTERS,            NULL },
  {  0x84002000,  "flt.s       ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84002020,  "flt.d       ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84004800,  "int.s       ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84004880,  "int.d       ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84005000,  "nint.s      ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84005080,  "nint.d      ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84005800,  "trnc.s      ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x84005880,  "trnc.d      ",  {21,5,REG},       {0,5,REG},     {0,0,0},       ALTERS,            NULL },
  {  0x80004800,  "fldcr       ",  {21,5,REG},       {5,6,FCRREG},  {0,0,0},       ALTERS,            NULL },
  {  0x80008800,  "fstcr       ",  {16,5,REG},       {5,6,FCRREG},  {0,0,0},       DONTCARE,          NULL },
  {  0x8000c800,  "fxcr        ",  {21,5,REG},       {16,5,REG},    {5,6,FCRREG},  ALTERS,            NULL },
};
#endif /* GENERATE_TABLE */
