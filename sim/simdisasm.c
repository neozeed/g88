
/*
 * This disassembles the passed M88000 instruction and prints it to the
 * standard output.

 * "$Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/simdisasm.c,v 1.13 90/08/02 12:20:13 robertb Exp $";
 */

#include <stdio.h>

#include "fields88.h"
#include "disasm.h"
#include "format.h"

#define AD (1)

#define pr_dest(i)  {add_to_buf("r%d,", D(i));}
#define pr_s1(i)    {add_to_buf("r%d,", S1(i));}
#define pr_s1_no_comma(i) {add_to_buf("r%d", S1(i));}
#define pr_s2(i)    {add_to_buf("r%d", S2(i));}
#define pr_b5(i)    {add_to_buf("%d,", B5(i));}
#define pr_w5(i)    {add_to_buf("%d", W5(i));}
#define pr_o5(i)    {add_to_buf("<%d>", O5(i));}
#define pr_lit16(i) {add_to_buf("0x%x", LIT16(i));}
#define pr_offset(i) {add_to_buf("0x%x", OFFSET(i));}
#define pr_nm(s)    {add_to_buf("%-9s  ", (s));}
#define pr_fcrs(i)  {add_to_buf("fpcr%d", FCRS(i));}
#define pr_index(i) {add_to_buf("[r%d]", S2(i));}
#define pr_crs(i)   {add_to_buf("cr%d", CRS(i));}
#define pr_vec(i)   {add_to_buf("vector_%d", VEC(i));}
#define pr_sfu(i)   {if (SFU(i) > 0) {add_to_buf("  SFU_%d", SFU(i));}}
static
char *bit_branch_table[] = {
  "", "", "eq", "ne", "gt", "le", "lt", "ge", "hi", "ls", "lo", "hs"};

static
struct instr in_tab[] = {
0x70000000, 0xfc000000, 0, "add",      INTRL,       ONEREG,
0xf4007000, 0xfc00ffe0, 0, "add",      INTRR,       ONEREG,
0xf4007100, 0xfc00ffe0, 0, "add.co",   INTRR,       ONEREG,
0xf4007200, 0xfc00ffe0, 0, "add.ci",   INTRR,       ONEREG,
0xf4007300, 0xfc00ffe0, 0, "add.cio",  INTRR,       ONEREG,

0x60000000, 0xfc000000, 0, "addu",     INTRL,       ONEREG,
0xf4006000, 0xfc00ffe0, 0, "addu",     INTRR,       ONEREG,
0xf4006100, 0xfc00ffe0, 0, "addu.co",  INTRR,       ONEREG,
0xf4006200, 0xfc00ffe0, 0, "addu.ci",  INTRR,       ONEREG,
0xf4006300, 0xfc00ffe0, 0, "addu.cio", INTRR,       ONEREG,

0x40000000, 0xfc000000, 0, "and",      INTRL,       ONEREG,
0x44000000, 0xfc000000, 0, "and.u",    INTRL,       ONEREG,
0xf4004000, 0xfc00ffe0, 0, "and",      INTRR,       ONEREG,
0xf4004400, 0xfc00ffe0, 0, "and.c",    INTRR,       ONEREG,

0xe8000000, 0xfc000000, 0, "bcnd",     CBRANCH, NOREG,
0xec000000, 0xfc000000, 0, "bcnd.n",   CBRANCH, NOREG,

0xc0000000, 0xfc000000, 0, "br",       IPREL,       NOREG,
0xc4000000, 0xfc000000, 0, "br.n",     IPREL,       NOREG,

0xd0000000, 0xfc000000, 0, "bb0",      BITBRANCH,   NOREG,
0xd4000000, 0xfc000000, 0, "bb0.n",    BITBRANCH,   NOREG,

0xd8000000, 0xfc000000, 0, "bb1",      BITBRANCH,   NOREG,
0xdc000000, 0xfc000000, 0, "bb1.n",    BITBRANCH,   NOREG,

0xc8000000, 0xfc000000, 0, "bsr",      IPREL,       NOREG,
0xcc000000, 0xfc000000, 0, "bsr.n",    IPREL,       NOREG,

0xf0008000, 0xfc00fc00, 0, "clr",      BITFIELD,    ONEREG,
0xf4008000, 0xfc00ffe0, 0, "clr",      INTRR,       ONEREG,

0x7c000000, 0xfc000000, 0, "cmp",       INTRL,      ONEREG,
0xf4007c00, 0xfc00fee0, 0, "cmp",      INTRR,       ONEREG,

0x78000000, 0xfc000000, 0, "div",       INTRL,      ONEREG,
0xf4007800, 0xfc00fee0, 0, "div",       INTRR,      ONEREG,

0x68000000, 0xfc000000, 0, "divu",      INTRL,      ONEREG,
0xf4006800, 0xfc00fee0, 0, "divu",      INTRR,      ONEREG,

0xf0009000, 0xfc00fc00, 0, "ext",       BITFIELD,   ONEREG,
0xf4009000, 0xfc00ffe0, 0, "ext",       INTRR,      ONEREG,

0xf0009800, 0xfc00fc00, 0, "extu",      BITFIELD,   ONEREG,
0xf4009800, 0xfc00ffe0, 0, "extu",      INTRR,      ONEREG,

0x84002800, 0xfc00ffe0, 0, "fadd.sss",  INTRR,      ONEREG,
0x84002880, 0xfc00ffe0, 0, "fadd.ssd",  INTRR,      ONEREG,
0x84002a00, 0xfc00ffe0, 0, "fadd.sds",  INTRR,      ONEREG,
0x84002a80, 0xfc00ffe0, 0, "fadd.sdd",  INTRR,      ONEREG,
0x84002820, 0xfc00ffe0, 0, "fadd.dss",  INTRR,      TWOREG,
0x840028a0, 0xfc00ffe0, 0, "fadd.dsd",  INTRR,      TWOREG,
0x84002a20, 0xfc00ffe0, 0, "fadd.dds",  INTRR,      TWOREG,
0x84002aa0, 0xfc00ffe0, 0, "fadd.ddd",  INTRR,      TWOREG,

0x84003800, 0xfc00ffe0, 0, "fcmp.sss",  INTRR,      ONEREG,
0x84003880, 0xfc00ffe0, 0, "fcmp.ssd",  INTRR,      ONEREG,
0x84003a20, 0xfc00ffe0, 0, "fcmp.sds",  INTRR,      ONEREG,
0x84003a80, 0xfc00ffe0, 0, "fcmp.sdd",  INTRR,      ONEREG,

0x84007000, 0xfc00ffe0, 0, "fdiv.sss",  INTRR,      ONEREG,
0x84007080, 0xfc00ffe0, 0, "fdiv.ssd",  INTRR,      ONEREG,
0x84007200, 0xfc00ffe0, 0, "fdiv.sds",  INTRR,      ONEREG,
0x84007280, 0xfc00ffe0, 0, "fdiv.sdd",  INTRR,      ONEREG,
0x84007020, 0xfc00ffe0, 0, "fdiv.dss",  INTRR,      TWOREG,
0x840070a0, 0xfc00ffe0, 0, "fdiv.dsd",  INTRR,      TWOREG,
0x84007220, 0xfc00ffe0, 0, "fdiv.dds",  INTRR,      TWOREG,
0x840072a0, 0xfc00ffe0, 0, "fdiv.ddd",  INTRR,      TWOREG,

0xf400ec00, 0xfc1fffe0, 0, "ff0",       INTRR2OP,   ONEREG,
0xf400e800, 0xfc1fffe0, 0, "ff1",       INTRR2OP,   ONEREG,

0x80004800, 0xfc1ff81f, 0, "fldcr", FLDCR,      ONEREG,

0x84002000, 0xfc1fffe0, 0, "flt.ss",    INTRR2OP,   ONEREG,
0x84002020, 0xfc1fffe0, 0, "flt.ds",    INTRR2OP,   TWOREG,

0x84000000, 0xfc00ffe0, 0, "fmul.sss",  INTRR,      ONEREG,
0x84000080, 0xfc00ffe0, 0, "fmul.ssd",  INTRR,      ONEREG,
0x84000200, 0xfc00ffe0, 0, "fmul.sds",  INTRR,      ONEREG,
0x84000280, 0xfc00ffe0, 0, "fmul.sdd",  INTRR,      ONEREG,
0x84000020, 0xfc00ffe0, 0, "fmul.dss",  INTRR,      TWOREG,
0x840000a0, 0xfc00ffe0, 0, "fmul.dsd",  INTRR,      TWOREG,
0x84000220, 0xfc00ffe0, 0, "fmul.dds",  INTRR,      TWOREG,
0x840002a0, 0xfc00ffe0, 0, "fmul.ddd",  INTRR,      TWOREG,

0x80008800, 0xffe0f800, 0, "fstcr", FSTCR,      NOREG,

0x84003000, 0xfc00ffe0, 0, "fsub.sss",  INTRR,      ONEREG,
0x84003080, 0xfc00ffe0, 0, "fsub.ssd",  INTRR,      ONEREG,
0x84003200, 0xfc00ffe0, 0, "fsub.sds",  INTRR,      ONEREG,
0x84003280, 0xfc00ffe0, 0, "fsub.sdd",  INTRR,      ONEREG,
0x84003020, 0xfc00ffe0, 0, "fsub.dss",  INTRR,      TWOREG,
0x840030a0, 0xfc00ffe0, 0, "fsub.dsd",  INTRR,      TWOREG,
0x84003220, 0xfc00ffe0, 0, "fsub.dds",  INTRR,      TWOREG,
0x840032a0, 0xfc00ffe0, 0, "fsub.ddd",  INTRR,      TWOREG,

0x8000c800, 0xfc00f800, 0, "fxcr",      FXCR,       ONEREG,

0x84004800, 0xfc1fffe0, 0, "int.ss",    INTRR2OP,   ONEREG,
0x84004880, 0xfc1fffe0, 0, "int.sd",    INTRR2OP,   ONEREG,

0xf400c000, 0xffffffe0, 0, "jmp",       JMP,        NOREG,
0xf400c400, 0xffffffe0, 0, "jmp.n", JMP,        NOREG,

0xf400c800, 0xffffffe0, 0, "jsr",       JMP,        NOREG,
0xf400cc00, 0xffffffe0, 0, "jsr.n", JMP,        NOREG,

0x10000000, 0xfc000000, 0, "ld.d",      LDLIT,      TWOREG,
0x14000000, 0xfc000000, 0, "ld",        LDLIT,      ONEREG,
0x18000000, 0xfc000000, 0, "ld.h",      LDLIT,      ONEREG,
0x1c000000, 0xfc000000, 0, "ld.b",      LDLIT,      ONEREG,
0x00000000, 0xfc000000, 0, "xmem.bu",   LDLIT,      ONEREG,
0x04000000, 0xfc000000, 0, "xmem",      LDLIT,      ONEREG,
0x08000000, 0xfc000000, 0, "ld.hu", LDLIT,      ONEREG,
0x0c000000, 0xfc000000, 0, "ld.bu", LDLIT,      ONEREG,

0xf4000000, 0xfc00ffe0, 0, "xmem.bu",   LDRO,       ONEREG,
0xf4000400, 0xfc00ffe0, 0, "xmem",      LDRO,       ONEREG,
0xf4000800, 0xfc00ffe0, 0, "ld.hu", LDRO,       ONEREG,
0xf4000c00, 0xfc00ffe0, 0, "ld.bu", LDRO,       ONEREG,
0xf4001000, 0xfc00ffe0, 0, "ld.d",      LDRO,       TWOREG,
0xf4001400, 0xfc00ffe0, 0, "ld",        LDRO,       ONEREG,
0xf4001800, 0xfc00ffe0, 0, "ld.h",      LDRO,       ONEREG,
0xf4001c00, 0xfc00ffe0, 0, "ld.b",      LDRO,       ONEREG,

0xf4000200, 0xfc00ffe0, 0, "xmem.bu",   LDRI,       ONEREG,
0xf4000600, 0xfc00ffe0, 0, "xmem",      LDRI,       ONEREG,
0xf4000a00, 0xfc00ffe0, 0, "ld.hu", LDRI,       ONEREG,
0xf4000e00, 0xfc00ffe0, 0, "ld.bu", LDRI,       ONEREG,
0xf4001200, 0xfc00ffe0, 0, "ld.d",      LDRI,       TWOREG,
0xf4001600, 0xfc00ffe0, 0, "ld",        LDRI,       ONEREG,
0xf4001a00, 0xfc00ffe0, 0, "ld.h",      LDRI,       ONEREG,
0xf4001e00, 0xfc00ffe0, 0, "ld.b",      LDRI,       ONEREG,

0xf4000900, 0xfc00ffe0, 0, "ld.hu.usr", LDRO,       ONEREG,
0xf4000d00, 0xfc00ffe0, 0, "ld.bu.usr", LDRO,       ONEREG,
0xf4001100, 0xfc00ffe0, 0, "ld.d.usr",      LDRO,       TWOREG,
0xf4001500, 0xfc00ffe0, 0, "ld.usr",        LDRO,       ONEREG,
0xf4001900, 0xfc00ffe0, 0, "ld.h.usr",      LDRO,       ONEREG,
0xf4001d00, 0xfc00ffe0, 0, "ld.b.usr",      LDRO,       ONEREG,

0xf4000b00, 0xfc00ffe0, 0, "ld.hu.usr", LDRI,       ONEREG,
0xf4000f00, 0xfc00ffe0, 0, "ld.bu.usr", LDRI,       ONEREG,
0xf4001300, 0xfc00ffe0, 0, "ld.d.usr",      LDRI,       TWOREG,
0xf4001700, 0xfc00ffe0, 0, "ld.usr",        LDRI,       ONEREG,
0xf4001b00, 0xfc00ffe0, 0, "ld.h.usr",      LDRI,       ONEREG,
0xf4001f00, 0xfc00ffe0, 0, "ld.b.usr",      LDRI,       ONEREG,

0xf4000100, 0xfc00ffe0, 0, "xmem.bu.usr",   LDRO,       ONEREG,
0xf4000500, 0xfc00ffe0, 0, "xmem.usr",      LDRO,       ONEREG,
0xf4000300, 0xfc00ffe0, 0, "xmem.bu.usr",   LDRI,       ONEREG,
0xf4000700, 0xfc00ffe0, 0, "xmem.usr",      LDRI,       ONEREG,

/*
 * This is my closest guess on the lda LDLIT and LDRO
 * modes, they aren't documented.
 */
0x30000000, 0xfc000000, 0, "lda.d", LDLIT,      ONEREG,
0x34000000, 0xfc000000, 0, "lda",       LDLIT,      ONEREG,
0x38000000, 0xfc000000, 0, "lda.h", LDLIT,      ONEREG,
0x3c000000, 0xfc000000, 0, "lda.b", LDLIT,      ONEREG,

0xf4003000, 0xfc00ffe0, 0, "lda.d", LDRO,       ONEREG,
0xf4003400, 0xfc00ffe0, 0, "lda",       LDRO,       ONEREG,
0xf4003800, 0xfc00ffe0, 0, "lda.h", LDRO,       ONEREG,
0xf4003c00, 0xfc00ffe0, 0, "lda.b", LDRO,       ONEREG,

0xf4003200, 0xfc00ffe0, 0, "lda.d", LDRI,       ONEREG,
0xf4003600, 0xfc00ffe0, 0, "lda",       LDRI,       ONEREG,
0xf4003a00, 0xfc00ffe0, 0, "lda.h", LDRI,       ONEREG,
0xf4003e00, 0xfc00ffe0, 0, "lda.b", LDRI,       ONEREG,

0x80004000, 0xfc1ff81f, 0, "ldcr",      LDCR,       ONEREG,

0xf000a000, 0xfc00fc00, 0, "mak",       BITFIELD,   ONEREG,
0xf400a000, 0xfc00ffe0, 0, "mak",       INTRR,      ONEREG,

0x48000000, 0xfc000000, 0, "mask",      INTRL,      ONEREG,
0x4c000000, 0xfc000000, 0, "mask.u",    INTRL,      ONEREG,

0x6c000000, 0xfc000000, 0, "mul",       INTRL,      ONEREG,
0xf4006c00, 0xfc00fee0, 0, "mul",       INTRR,      ONEREG,

0x84005000, 0xfc1fffe0, 0, "nint.ss",   INTRR2OP,   ONEREG,
0x84005080, 0xfc1fffe0, 0, "nint.sd",   INTRR2OP,   ONEREG,

0x58000000, 0xfc000000, 0, "or",        INTRL,      ONEREG,
0x5c000000, 0xfc000000, 0, "or.u",      INTRL,      ONEREG,
0xf4005800, 0xfc00ffe0, 0, "or",        INTRR,      ONEREG,
0xf4005c00, 0xfc00ffe0, 0, "or.c",      INTRR,      ONEREG,

0xf000a800, 0xfc00ffe0, 0, "rot",       ROT,        ONEREG,
0xf400a800, 0xfc00ffe0, 0, "rot",       INTRR,      ONEREG,

0xf400fc00, 0xffffffff, 0, "rte",       RTE,        NOREG,

0xf0008800, 0xfc00fc00, 0, "set",      BITFIELD,    ONEREG,
0xf4008800, 0xfc00ffe0, 0, "set",      INTRR,       ONEREG,

0x20000000, 0xfc000000, 0, "st.d",      STLIT,      NOREG,
0x24000000, 0xfc000000, 0, "st",        STLIT,      NOREG,
0x28000000, 0xfc000000, 0, "st.h",      STLIT,      NOREG,
0x2c000000, 0xfc000000, 0, "st.b",      STLIT,      NOREG,

0xf4002000, 0xfc00ffe0, 0, "st.d",      STRO,       NOREG,
0xf4002400, 0xfc00ffe0, 0, "st",        STRO,       NOREG,
0xf4002800, 0xfc00ffe0, 0, "st.h",      STRO,       NOREG,
0xf4002c00, 0xfc00ffe0, 0, "st.b",      STRO,       NOREG,

0xf4002200, 0xfc00ffe0, 0, "st.d",      STRI,       NOREG,
0xf4002600, 0xfc00ffe0, 0, "st",        STRI,       NOREG,
0xf4002a00, 0xfc00ffe0, 0, "st.h",      STRI,       NOREG,
0xf4002e00, 0xfc00ffe0, 0, "st.b",      STRI,       NOREG,

0xf4002100, 0xfc00ffe0, 0, "st.d.usr",      STRO,       NOREG,
0xf4002500, 0xfc00ffe0, 0, "st.usr",        STRO,       NOREG,
0xf4002900, 0xfc00ffe0, 0, "st.h.usr",      STRO,       NOREG,
0xf4002d00, 0xfc00ffe0, 0, "st.b.usr",      STRO,       NOREG,

0xf4002300, 0xfc00ffe0, 0, "st.d.usr",      STRI,       NOREG,
0xf4002700, 0xfc00ffe0, 0, "st.usr",        STRI,       NOREG,
0xf4002b00, 0xfc00ffe0, 0, "st.h.usr",      STRI,       NOREG,
0xf4002f00, 0xfc00ffe0, 0, "st.b.usr",      STRI,       NOREG,

0x80008000, 0xffe0f800, 0, "stcr",      STCR,       NOREG,

0x74000000, 0xfc000000, 0, "sub",       INTRL,      ONEREG,
0xf4007400, 0xfc00ffe0, 0, "sub",       INTRR,      ONEREG,
0xf4007500, 0xfc00ffe0, 0, "sub.bo",    INTRR,      ONEREG,
0xf4007600, 0xfc00ffe0, 0, "sub.bi",    INTRR,      ONEREG,
0xf4007700, 0xfc00ffe0, 0, "sub.bio",   INTRR,      ONEREG,

0x64000000, 0xfc000000, 0, "subu",      INTRL,      ONEREG,
0xf4006400, 0xfc00ffe0, 0, "subu",      INTRR,      ONEREG,
0xf4006500, 0xfc00ffe0, 0, "subu.bo",   INTRR,      ONEREG,
0xf4006600, 0xfc00ffe0, 0, "subu.bi",   INTRR,      ONEREG,
0xf4006700, 0xfc00ffe0, 0, "subu.bio",  INTRR,      ONEREG,

0xf000d000, 0xfc00fe00, 0, "tb0",       TRAP,       NOREG,
0xf000d800, 0xfc00fe00, 0, "tb1",       TRAP,       NOREG,

0xf8000000, 0xffe00000, 0, "tbnd",      TBND,       NOREG,
0xf400f800, 0xffe0ffe0, 0, "tbnd",      INTRR_S1_S2,    NOREG,

0xf000e800, 0xfc00fe00, 0, "tcnd",      TCND,       NOREG,
/*
 * Manual does not describe the encoding for the .n form
 */
0xf000e800, 0xfc00fe00, 0, "tcnd.n",    TCND,       NOREG,

0x84005800, 0xfc1fffe0, 0, "trnc.ss",   INTRR2OP,   ONEREG,
0x84005880, 0xfc1fffe0, 0, "trnc.sd",   INTRR2OP,   ONEREG,

0x8000c000, 0xfc00c000, 0, "xcr",       XCR,        ONEREG,

0x50000000, 0xfc000000, 0, "xor",       INTRL,      ONEREG,
0x54000000, 0xfc000000, 0, "xor.u", INTRL,      ONEREG,
0xf4005000, 0xfc00ffe0, 0, "xor",       INTRR,      ONEREG,
0xf4005400, 0xfc00ffe0, 0, "xor.c", INTRR,      ONEREG,
};

#define INTAB_SIZE (sizeof(in_tab) / sizeof(struct instr))

static
char *dis_buf;

static struct instr *last_p = &(in_tab[INTAB_SIZE]);

#define DECODECACHESIZE  (64)
#define DECODECACHESHIFT (32 - 6)       /* 32 - log2(DECODECACHESIZE) */
static struct instr decodecache[DECODECACHESIZE];

/*
 * This builds a small lookup table based on the top 6 bits of the
 * instruction.  We use it to speed up sim_instructions_lookup().
 */
static
builddecodetable()
{
    register struct instr *p;
    register struct instr *lp = last_p;
    register i;
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
        for (p = &in_tab[0] ; p < lp ; p++) {
            i = p->opcode >> DECODECACHESHIFT;
            p->link = decodecache[i].link;
            decodecache[i].link = p;
        }
    }
}

/*
 * This returns a pointer to the instruction table entry that matches
 * the passed instruction.
 */
struct instr *
instruction_lookup(i)
    register unsigned i;
{
    register struct instr *p, *prev, *head;
    register struct instr *lp = last_p;
    register int cnt;
    static initialized = 0;

    if (!initialized) {
        builddecodetable();
        initialized = 1;
    }

    prev = head = &decodecache[i >> DECODECACHESHIFT];
    for (p = head->link ; p ; prev = p, p = p->link) {
        if ((p->opmask & i) == p->opcode) {
            /*
             * We do a little adaptive searching here: when
             * we find an instruction we put it at the head of
             * the list.
             */
            prev->link = p->link;
            p->link = head->link;
            head->link = p;
            return p;
        }
    }
    return (struct instr *)0;
}

print_instruction(buf, i, dot)
    char    *buf;
    unsigned int    i;
{
    struct instr *p;

    dis_buf = buf;
    buf[0] = '\0';
    if (p = instruction_lookup(i)) {
        pr_nm(p->mnemonic);
        pr_operands(i, p, dot);
        return;
    }
    add_to_buf("garbage: 0x%x", i);
}

/*
 * This prints the operands.
 */
static
pr_operands(i, p, dot)
    unsigned i;
    struct instr *p;
{
    switch (p->format) {
        case INTRL:
            pr_dest(i);
            pr_s1(i);
            pr_lit16(i);
            break;

        case INTRR:
            pr_dest(i);
            pr_s1(i);
            pr_s2(i);
            break;

        case CBRANCH:
            pr_cond(i);
            pr_s1(i);
            pr_rel16(i, dot);
            break;

        case IPREL:
            pr_rel26(i, dot);
            break;

        case BITBRANCH:
            { int bit_number = B5(i);

                pr_b5(i);
                pr_s1(i);
                pr_rel16(i, dot);

                if (bit_number > 1 && bit_number < 12) {
                    add_to_buf(" %s", bit_branch_table[bit_number]);
                }
            }
            break;

        case BITFIELD:
            pr_dest(i);
            pr_s1(i);
            pr_w5(i);
            pr_o5(i);
            break;

        case INTRR2OP:
            pr_dest(i);
            pr_s2(i);
            break;

        case FLDCR:
            pr_dest(i);
            pr_fcrs(i);
            break;

        case FSTCR:
            pr_s1(i);
            pr_fcrs(i);
            if (S1(i) != S2(i)) {
                add_to_buf("WARNING:  S1=0x%x  S2=0x%x", S1(i), S2(i));
            }
            break;

        case FXCR:
            pr_dest(i);
            pr_s1(i);
            pr_fcrs(i);
            if (S1(i) != S2(i)) {
                add_to_buf("WARNING:  S1=0x%x  S2=0x%x", S1(i), S2(i));
            }
            break;

        case JMP:
            pr_s2(i);
            break;

        case LDLIT: case STLIT:
            pr_dest(i);
            pr_s1(i);
            pr_lit16(i);
            break;

        case LDRO: case STRO:
            pr_dest(i);
            pr_s1(i);
            pr_s2(i);
            break;

        case LDRI: case STRI:
            pr_dest(i);
            pr_s1_no_comma(i);
            pr_index(i);
            break;

        case LDCR:
            pr_dest(i);
            pr_crs(i);
            break;

        case STCR:
            pr_s1(i);
            pr_crs(i);
            break;

        case ROT:
            pr_dest(i);
            pr_s1(i);
            pr_o5(i);
            break;

        case RTE:
            break;

        case TRAP:
            pr_b5(i);
            pr_s1(i);
            pr_vec(i);
            break;

        case TBND:
            pr_s1(i);
            pr_lit16(i);
            break;

        case TCND:
            pr_cond(i);
            pr_s1(i);
            pr_vec(i);
            break;

        case XCR:
            pr_dest(i);
            pr_s1(i);
            pr_crs(i);
            add_to_buf("   ");
            pr_sfu(i);
            if (S1(i) != S2(i)) {
                add_to_buf("WARNING:  S1=0x%x  S2=0x%x", S1(i), S2(i));
            }
            break;

        case INTRR_S1_S2:
            pr_s1(i);
            pr_s2(i);
            break;

        default:
            pr_er("unkwown format %d", p->format);
            break;
    }
}

static
pr_cond(i)
    unsigned i;
{
    int c;
    static
    char *condition_table[] = { 
        "cc0", "gt0", "eq0",  "ge0", 
        "cc4", "cc5", "cc6",  "cc7",
        "cc8", "cc9", "cc10", "cc11",
        "lt0", "ne0", "le0",  "cc15"};

    c = CONDITION(i);
    if (c > 15) {
        add_to_buf("condition_error, cc=%d", c);
    } else {
        add_to_buf("%s,", condition_table[c]);
    }
}

/*
 * Error printing routine.
 */
static
pr_er(fmt, a, b, c, d, e, f)
    char    *fmt;
{
    add_to_buf(fmt, a, b, c, d, e, f);
}

/*
 * This is called just like printf.  It converts the passed
 * parameters to string and then concatenates the string
 * with 'buf'.
 */
static
add_to_buf(fmt, a, b, c, d, e, f, g)
    char    *fmt;
{
    char    s[100];

    sprintf(s, fmt, a, b, c, d, e, f, g);
    strcat(dis_buf, s);
}

#ifndef AD
#include "defs.h"
#include "symbols.h"
#include "mappings.h"
#include "runtime.h"
#endif

/*
 * Print the 26-bit relative field of an instruction (always a
 * branch).
 */
static
pr_rel26(i, dot)
    unsigned i, dot;
{
#ifdef AD
    add_to_buf("0x%x", REL26(i) + (int)dot);
#else
    unsigned addr;
    Symbol func;
    String funcname;

    addr = REL26(i) + dot;
    func = whatfunction(addr);
    funcname = symname(func);
    if (strlen(funcname) > 0) {
        add_to_buf("%s", funcname);
        if (codeloc(func) != addr) {
            add_to_buf("+0x%x", addr - codeloc(func));
        }
    } else {
        if (addr >= dot) {
            add_to_buf("+0x%x", addr - dot);
        } else {
            add_to_buf("-0x%x", dot - addr);
        }
    }
#endif
}

/*
 * Print the 16-bit relative field of an instruction (always a
 * branch).
 */
static
pr_rel16(i, dot)
    unsigned i, dot;
{
#ifdef AD
    add_to_buf("0x%x", REL26(i) + (int)dot);
#else
    unsigned addr;
    Symbol func;
    String funcname;

    addr = REL16(i) + dot;
    func = whatfunction(addr);
    funcname = symname(func);
    if (strlen(funcname) > 0) {
        add_to_buf("%s", funcname);
        if (codeloc(func) != addr) {
            add_to_buf("+0x%x", addr - codeloc(func));
        }
    } else {
        if (addr >= dot) {
            add_to_buf("+0x%x", addr - dot);
        } else {
            add_to_buf("-0x%x", dot - addr);
        }
    }
#endif
}
