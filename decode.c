/*
 * This decodes 88000 instructions
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 * 
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/RCS/decode.c,v 1.3 90/08/02 12:21:00 robertb Exp $
 */

typedef unsigned long u_long;
typedef unsigned char u_char;

#include "decode.h"
#include "format.h"

static
struct instr_info in_tab[] = {
0x70000000, 0xfc000000, "add",      INTRL,
0xf4007000, 0xfc00ffe0, "add",      INTRR,
0xf4007100, 0xfc00ffe0, "add.co",   INTRR,
0xf4007200, 0xfc00ffe0, "add.ci",   INTRR,
0xf4007300, 0xfc00ffe0, "add.cio",  INTRR,

0x60000000, 0xfc000000, "addu",     INTRL,
0xf4006000, 0xfc00ffe0, "addu",     INTRR,
0xf4006100, 0xfc00ffe0, "addu.co",  INTRR,
0xf4006200, 0xfc00ffe0, "addu.ci",  INTRR,
0xf4006300, 0xfc00ffe0, "addu.cio", INTRR,

0x40000000, 0xfc000000, "and",      INTRL,
0x44000000, 0xfc000000, "and.u",    INTRL,
0xf4004000, 0xfc00ffe0, "and",      INTRR,
0xf4004400, 0xfc00ffe0, "and.c",    INTRR,

0xe8400000, 0xffe00000, "bcnd_eq0", CBRANCH,
0xe9a00000, 0xffe00000, "bcnd_ne0", CBRANCH,
0xe8200000, 0xffe00000, "bcnd_gt0", CBRANCH,
0xe9800000, 0xffe00000, "bcnd_lt0", CBRANCH,
0xe8600000, 0xffe00000, "bcnd_ge0", CBRANCH,
0xe9c00000, 0xffe00000, "bcnd_le0", CBRANCH,

0xe8000000, 0xffe00000, "bcnd_0",   CBRANCH,
0xe8800000, 0xffe00000, "bcnd_4",   CBRANCH,
0xe8a00000, 0xffe00000, "bcnd_5",   CBRANCH,
0xe8c00000, 0xffe00000, "bcnd_6",   CBRANCH,
0xe8e00000, 0xffe00000, "bcnd_7",   CBRANCH,
0xe9000000, 0xffe00000, "bcnd_8",   CBRANCH,
0xe9200000, 0xffe00000, "bcnd_9",   CBRANCH,
0xe9400000, 0xffe00000, "bcnd_10",  CBRANCH,
0xe9600000, 0xffe00000, "bcnd_11",  CBRANCH,
0xe9e00000, 0xffe00000, "bcnd_always",CBRANCH,

0xec400000, 0xffe00000, "bcnd_eq0_n", CBRANCH,
0xeda00000, 0xffe00000, "bcnd_ne0_n", CBRANCH,
0xec200000, 0xffe00000, "bcnd_gt0_n", CBRANCH,
0xed800000, 0xffe00000, "bcnd_lt0_n", CBRANCH,
0xec600000, 0xffe00000, "bcnd_ge0_n", CBRANCH,
0xedc00000, 0xffe00000, "bcnd_le0_n", CBRANCH,

0xec000000, 0xffe00000, "bcnd_0_n",   CBRANCH,
0xec800000, 0xffe00000, "bcnd_4_n",   CBRANCH,
0xeca00000, 0xffe00000, "bcnd_5_n",   CBRANCH,
0xecc00000, 0xffe00000, "bcnd_6_n",   CBRANCH,
0xece00000, 0xffe00000, "bcnd_7_n",   CBRANCH,
0xed000000, 0xffe00000, "bcnd_8_n",   CBRANCH,
0xed200000, 0xffe00000, "bcnd_9_n",   CBRANCH,
0xed400000, 0xffe00000, "bcnd_10_n",  CBRANCH,
0xed600000, 0xffe00000, "bcnd_11_n",  CBRANCH,
0xede00000, 0xffe00000, "bcnd_always_n",CBRANCH,

0xc0000000, 0xfc000000, "br",       IPREL,
0xc4000000, 0xfc000000, "br.n",     IPREL,

0xd0000000, 0xffe00000, "bb0_0",    BITBRANCH,
0xd0200000, 0xffe00000, "bb0_1",    BITBRANCH,
0xd0400000, 0xffe00000, "bb0_2",    BITBRANCH,
0xd0600000, 0xffe00000, "bb0_3",    BITBRANCH,
0xd0800000, 0xffe00000, "bb0_4",    BITBRANCH,
0xd0a00000, 0xffe00000, "bb0_5",    BITBRANCH,
0xd0c00000, 0xffe00000, "bb0_6",    BITBRANCH,
0xd0e00000, 0xffe00000, "bb0_7",    BITBRANCH,
0xd1000000, 0xffe00000, "bb0_8",    BITBRANCH,
0xd1200000, 0xffe00000, "bb0_9",    BITBRANCH,
0xd1400000, 0xffe00000, "bb0_10",   BITBRANCH,
0xd1600000, 0xffe00000, "bb0_11",   BITBRANCH,
0xd1800000, 0xffe00000, "bb0_12",   BITBRANCH,
0xd1a00000, 0xffe00000, "bb0_13",   BITBRANCH,
0xd1c00000, 0xffe00000, "bb0_14",   BITBRANCH,
0xd1e00000, 0xffe00000, "bb0_15",   BITBRANCH,
0xd2000000, 0xffe00000, "bb0_16",   BITBRANCH,
0xd2200000, 0xffe00000, "bb0_17",   BITBRANCH,
0xd2400000, 0xffe00000, "bb0_18",   BITBRANCH,
0xd2600000, 0xffe00000, "bb0_19",   BITBRANCH,
0xd2800000, 0xffe00000, "bb0_20",   BITBRANCH,
0xd2a00000, 0xffe00000, "bb0_21",   BITBRANCH,
0xd2c00000, 0xffe00000, "bb0_22",   BITBRANCH,
0xd2e00000, 0xffe00000, "bb0_23",   BITBRANCH,
0xd3000000, 0xffe00000, "bb0_24",   BITBRANCH,
0xd3200000, 0xffe00000, "bb0_25",   BITBRANCH,
0xd3400000, 0xffe00000, "bb0_26",   BITBRANCH,
0xd3600000, 0xffe00000, "bb0_27",   BITBRANCH,
0xd3800000, 0xffe00000, "bb0_28",   BITBRANCH,
0xd3a00000, 0xffe00000, "bb0_29",   BITBRANCH,
0xd3c00000, 0xffe00000, "bb0_30",   BITBRANCH,
0xd3e00000, 0xffe00000, "bb0_31",   BITBRANCH,

0xd8000000, 0xffe00000, "bb1_0",    BITBRANCH,
0xd8200000, 0xffe00000, "bb1_1",    BITBRANCH,
0xd8400000, 0xffe00000, "bb1_2",    BITBRANCH,
0xd8600000, 0xffe00000, "bb1_3",    BITBRANCH,
0xd8800000, 0xffe00000, "bb1_4",    BITBRANCH,
0xd8a00000, 0xffe00000, "bb1_5",    BITBRANCH,
0xd8c00000, 0xffe00000, "bb1_6",    BITBRANCH,
0xd8e00000, 0xffe00000, "bb1_7",    BITBRANCH,
0xd9000000, 0xffe00000, "bb1_8",    BITBRANCH,
0xd9200000, 0xffe00000, "bb1_9",    BITBRANCH,
0xd9400000, 0xffe00000, "bb1_10",   BITBRANCH,
0xd9600000, 0xffe00000, "bb1_11",   BITBRANCH,
0xd9800000, 0xffe00000, "bb1_12",   BITBRANCH,
0xd9a00000, 0xffe00000, "bb1_13",   BITBRANCH,
0xd9c00000, 0xffe00000, "bb1_14",   BITBRANCH,
0xd9e00000, 0xffe00000, "bb1_15",   BITBRANCH,
0xda000000, 0xffe00000, "bb1_16",   BITBRANCH,
0xda200000, 0xffe00000, "bb1_17",   BITBRANCH,
0xda400000, 0xffe00000, "bb1_18",   BITBRANCH,
0xda600000, 0xffe00000, "bb1_19",   BITBRANCH,
0xda800000, 0xffe00000, "bb1_20",   BITBRANCH,
0xdaa00000, 0xffe00000, "bb1_21",   BITBRANCH,
0xdac00000, 0xffe00000, "bb1_22",   BITBRANCH,
0xdae00000, 0xffe00000, "bb1_23",   BITBRANCH,
0xdb000000, 0xffe00000, "bb1_24",   BITBRANCH,
0xdb200000, 0xffe00000, "bb1_25",   BITBRANCH,
0xdb400000, 0xffe00000, "bb1_26",   BITBRANCH,
0xdb600000, 0xffe00000, "bb1_27",   BITBRANCH,
0xdb800000, 0xffe00000, "bb1_28",   BITBRANCH,
0xdba00000, 0xffe00000, "bb1_29",   BITBRANCH,
0xdbc00000, 0xffe00000, "bb1_30",   BITBRANCH,
0xdbe00000, 0xffe00000, "bb1_31",   BITBRANCH,

0xd4000000, 0xffe00000, "bb0_n_0",  BITBRANCH,
0xd4200000, 0xffe00000, "bb0_n_1",  BITBRANCH,
0xd4400000, 0xffe00000, "bb0_n_2",  BITBRANCH,
0xd4600000, 0xffe00000, "bb0_n_3",  BITBRANCH,
0xd4800000, 0xffe00000, "bb0_n_4",  BITBRANCH,
0xd4a00000, 0xffe00000, "bb0_n_5",  BITBRANCH,
0xd4c00000, 0xffe00000, "bb0_n_6",  BITBRANCH,
0xd4e00000, 0xffe00000, "bb0_n_7",  BITBRANCH,
0xd5000000, 0xffe00000, "bb0_n_8",  BITBRANCH,
0xd5200000, 0xffe00000, "bb0_n_9",  BITBRANCH,
0xd5400000, 0xffe00000, "bb0_n_10", BITBRANCH,
0xd5600000, 0xffe00000, "bb0_n_11", BITBRANCH,
0xd5800000, 0xffe00000, "bb0_n_12", BITBRANCH,
0xd5a00000, 0xffe00000, "bb0_n_13", BITBRANCH,
0xd5c00000, 0xffe00000, "bb0_n_14", BITBRANCH,
0xd5e00000, 0xffe00000, "bb0_n_15", BITBRANCH,
0xd6000000, 0xffe00000, "bb0_n_16", BITBRANCH,
0xd6200000, 0xffe00000, "bb0_n_17", BITBRANCH,
0xd6400000, 0xffe00000, "bb0_n_18", BITBRANCH,
0xd6600000, 0xffe00000, "bb0_n_19", BITBRANCH,
0xd6800000, 0xffe00000, "bb0_n_20", BITBRANCH,
0xd6a00000, 0xffe00000, "bb0_n_21", BITBRANCH,
0xd6c00000, 0xffe00000, "bb0_n_22", BITBRANCH,
0xd6e00000, 0xffe00000, "bb0_n_23", BITBRANCH,
0xd7000000, 0xffe00000, "bb0_n_24", BITBRANCH,
0xd7200000, 0xffe00000, "bb0_n_25", BITBRANCH,
0xd7400000, 0xffe00000, "bb0_n_26", BITBRANCH,
0xd7600000, 0xffe00000, "bb0_n_27", BITBRANCH,
0xd7800000, 0xffe00000, "bb0_n_28", BITBRANCH,
0xd7a00000, 0xffe00000, "bb0_n_29", BITBRANCH,
0xd7c00000, 0xffe00000, "bb0_n_30", BITBRANCH,
0xd7e00000, 0xffe00000, "bb0_n_31", BITBRANCH,

0xdc000000, 0xffe00000, "bb1_n_0",  BITBRANCH,
0xdc200000, 0xffe00000, "bb1_n_1",  BITBRANCH,
0xdc400000, 0xffe00000, "bb1_n_2",  BITBRANCH,
0xdc600000, 0xffe00000, "bb1_n_3",  BITBRANCH,
0xdc800000, 0xffe00000, "bb1_n_4",  BITBRANCH,
0xdca00000, 0xffe00000, "bb1_n_5",  BITBRANCH,
0xdcc00000, 0xffe00000, "bb1_n_6",  BITBRANCH,
0xdce00000, 0xffe00000, "bb1_n_7",  BITBRANCH,
0xdd000000, 0xffe00000, "bb1_n_8",  BITBRANCH,
0xdd200000, 0xffe00000, "bb1_n_9",  BITBRANCH,
0xdd400000, 0xffe00000, "bb1_n_10", BITBRANCH,
0xdd600000, 0xffe00000, "bb1_n_11", BITBRANCH,
0xdd800000, 0xffe00000, "bb1_n_12", BITBRANCH,
0xdda00000, 0xffe00000, "bb1_n_13", BITBRANCH,
0xddc00000, 0xffe00000, "bb1_n_14", BITBRANCH,
0xdde00000, 0xffe00000, "bb1_n_15", BITBRANCH,
0xde000000, 0xffe00000, "bb1_n_16", BITBRANCH,
0xde200000, 0xffe00000, "bb1_n_17", BITBRANCH,
0xde400000, 0xffe00000, "bb1_n_18", BITBRANCH,
0xde600000, 0xffe00000, "bb1_n_19", BITBRANCH,
0xde800000, 0xffe00000, "bb1_n_20", BITBRANCH,
0xdea00000, 0xffe00000, "bb1_n_21", BITBRANCH,
0xdec00000, 0xffe00000, "bb1_n_22", BITBRANCH,
0xdee00000, 0xffe00000, "bb1_n_23", BITBRANCH,
0xdf000000, 0xffe00000, "bb1_n_24", BITBRANCH,
0xdf200000, 0xffe00000, "bb1_n_25", BITBRANCH,
0xdf400000, 0xffe00000, "bb1_n_26", BITBRANCH,
0xdf600000, 0xffe00000, "bb1_n_27", BITBRANCH,
0xdf800000, 0xffe00000, "bb1_n_28", BITBRANCH,
0xdfa00000, 0xffe00000, "bb1_n_29", BITBRANCH,
0xdfc00000, 0xffe00000, "bb1_n_30", BITBRANCH,
0xdfe00000, 0xffe00000, "bb1_n_31", BITBRANCH,

0xc8000000, 0xfc000000, "bsr",      IPREL,
0xcc000000, 0xfc000000, "bsr.n",    IPREL,

0xf0008000, 0xfc00fc00, "clr",      BITFIELD,
0xf4008000, 0xfc00ffe0, "clr",      INTRR,

0x7c000000, 0xfc000000, "cmp",      INTRL,
0xf4007c00, 0xfc00fee0, "cmp",      INTRR,

0x78000000, 0xfc000000, "div",      INTRL,
0xf4007800, 0xfc00fee0, "div",      INTRR,

0x68000000, 0xfc000000, "divu",     INTRL,
0xf4006800, 0xfc00fee0, "divu",     INTRR,

0xf0009000, 0xfc00fc00, "ext",      BITFIELD,
0xf4009000, 0xfc00ffe0, "ext",      INTRR,

0xf0009800, 0xfc00fc00, "extu",     BITFIELD,
0xf4009800, 0xfc00ffe0, "extu",     INTRR,

0x84002800, 0xfc00ffe0, "fadd.sss", INTRR,
0x84002880, 0xfc00ffe0, "fadd.ssd", INTRR,
0x84002a00, 0xfc00ffe0, "fadd.sds", INTRR,
0x84002a80, 0xfc00ffe0, "fadd.sdd", INTRR,
0x84002820, 0xfc00ffe0, "fadd.dss", INTRR,
0x840028a0, 0xfc00ffe0, "fadd.dsd", INTRR,
0x84002a20, 0xfc00ffe0, "fadd.dds", INTRR,
0x84002aa0, 0xfc00ffe0, "fadd.ddd", INTRR,

0x84003800, 0xfc00ffe0, "fcmp.sss", INTRR,
0x84003880, 0xfc00ffe0, "fcmp.ssd", INTRR,
0x84003a00, 0xfc00ffe0, "fcmp.sds", INTRR,
0x84003a80, 0xfc00ffe0, "fcmp.sdd", INTRR,

0x84007000, 0xfc00ffe0, "fdiv.sss", INTRR,
0x84007080, 0xfc00ffe0, "fdiv.ssd", INTRR,
0x84007200, 0xfc00ffe0, "fdiv.sds", INTRR,
0x84007280, 0xfc00ffe0, "fdiv.sdd", INTRR,
0x84007020, 0xfc00ffe0, "fdiv.dss", INTRR,
0x840070a0, 0xfc00ffe0, "fdiv.dsd", INTRR,
0x84007220, 0xfc00ffe0, "fdiv.dds", INTRR,
0x840072a0, 0xfc00ffe0, "fdiv.ddd", INTRR,

0xf400ec00, 0xfc1fffe0, "ff0",      INTRR2OP,
0xf400e800, 0xfc1fffe0, "ff1",      INTRR2OP,

0x80004800, 0xfc1ff81f, "fldcr",    FLDCR,

0x84002000, 0xfc1fffe0, "flt.ss",   INTRR2OP,
0x84002020, 0xfc1fffe0, "flt.ds",   INTRR2OP,

0x84000000, 0xfc00ffe0, "fmul.sss", INTRR,
0x84000080, 0xfc00ffe0, "fmul.ssd", INTRR,
0x84000200, 0xfc00ffe0, "fmul.sds", INTRR,
0x84000280, 0xfc00ffe0, "fmul.sdd", INTRR,
0x84000020, 0xfc00ffe0, "fmul.dss", INTRR,
0x840000a0, 0xfc00ffe0, "fmul.dsd", INTRR,
0x84000220, 0xfc00ffe0, "fmul.dds", INTRR,
0x840002a0, 0xfc00ffe0, "fmul.ddd", INTRR,

0x80008800, 0xffe0f800, "fstcr",    FSTCR,

0x84003000, 0xfc00ffe0, "fsub.sss", INTRR,
0x84003080, 0xfc00ffe0, "fsub.ssd", INTRR,
0x84003200, 0xfc00ffe0, "fsub.sds", INTRR,
0x84003280, 0xfc00ffe0, "fsub.sdd", INTRR,
0x84003020, 0xfc00ffe0, "fsub.dss", INTRR,
0x840030a0, 0xfc00ffe0, "fsub.dsd", INTRR,
0x84003220, 0xfc00ffe0, "fsub.dds", INTRR,
0x840032a0, 0xfc00ffe0, "fsub.ddd", INTRR,

0x8000c800, 0xfc00f800, "fxcr",     FXCR,

0x84004800, 0xfc1fffe0, "int.ss",   INTRR2OP,
0x84004880, 0xfc1fffe0, "int.sd",   INTRR2OP,

0xf400c000, 0xffffffe0, "jmp",      JMP,
0xf400c400, 0xffffffe0, "jmp.n",    JMP,

0xf400c800, 0xffffffe0, "jsr",      JMP,
0xf400cc00, 0xffffffe0, "jsr.n",    JMP,

0x10000000, 0xfc000000, "ld.d",     LDLIT,
0x14000000, 0xfc000000, "ld",       LDLIT,
0x18000000, 0xfc000000, "ld.h",     LDLIT,
0x1c000000, 0xfc000000, "ld.b",     LDLIT,
0x00000000, 0xfc000000, "xmem.bu",  LDLIT,
0x04000000, 0xfc000000, "xmem",     LDLIT,
0x08000000, 0xfc000000, "ld.hu",    LDLIT,
0x0c000000, 0xfc000000, "ld.bu",    LDLIT,

0xf4000000, 0xfc00ffe0, "xmem.bu",  LDRO,
0xf4000400, 0xfc00ffe0, "xmem",     LDRO,
0xf4000800, 0xfc00ffe0, "ld.hu",    LDRO,
0xf4000c00, 0xfc00ffe0, "ld.bu",    LDRO,
0xf4001000, 0xfc00ffe0, "ld.d",     LDRO,
0xf4001400, 0xfc00ffe0, "ld",       LDRO,
0xf4001800, 0xfc00ffe0, "ld.h",     LDRO,
0xf4001c00, 0xfc00ffe0, "ld.b",     LDRO,

0xf4000200, 0xfc00ffe0, "xmem.bu",  LDRI,
0xf4000600, 0xfc00ffe0, "xmem",     LDRI,
0xf4000a00, 0xfc00ffe0, "ld.hu",    LDRI,
0xf4000e00, 0xfc00ffe0, "ld.bu",    LDRI,
0xf4001200, 0xfc00ffe0, "ld.d",     LDRI,
0xf4001600, 0xfc00ffe0, "ld",       LDRI,
0xf4001a00, 0xfc00ffe0, "ld.h",     LDRI,
0xf4001e00, 0xfc00ffe0, "ld.b",     LDRI,

0xf4000900, 0xfc00ffe0, "ld.hu.usr",    LDRO,
0xf4000d00, 0xfc00ffe0, "ld.bu.usr",    LDRO,
0xf4001100, 0xfc00ffe0, "ld.d.usr",     LDRO,
0xf4001500, 0xfc00ffe0, "ld.usr",       LDRO,
0xf4001900, 0xfc00ffe0, "ld.h.usr",     LDRO,
0xf4001d00, 0xfc00ffe0, "ld.b.usr",     LDRO,

0xf4000b00, 0xfc00ffe0, "ld.hu.usr",    LDRI,
0xf4000f00, 0xfc00ffe0, "ld.bu.usr",    LDRI,
0xf4001300, 0xfc00ffe0, "ld.d.usr",     LDRI,
0xf4001700, 0xfc00ffe0, "ld.usr",       LDRI,
0xf4001b00, 0xfc00ffe0, "ld.h.usr",     LDRI,
0xf4001f00, 0xfc00ffe0, "ld.b.usr",     LDRI,

0xf4000100, 0xfc00ffe0, "xmem.bu.usr",  LDRO,
0xf4000500, 0xfc00ffe0, "xmem.usr",     LDRO,
0xf4000300, 0xfc00ffe0, "xmem.bu.usr",  LDRI,
0xf4000700, 0xfc00ffe0, "xmem.usr",     LDRI,

/*
 * This is my closest guess on the lda LDLIT and LDRO
 * modes, they aren't documented.
 */
0x30000000, 0xfc000000, "lda.d",    INTRL,
0x34000000, 0xfc000000, "lda",      INTRL,
0x38000000, 0xfc000000, "lda.h",    INTRL,
0x3c000000, 0xfc000000, "lda.b",    INTRL,

0xf4003000, 0xfc00ffe0, "lda.d",    INTRR,
0xf4003400, 0xfc00ffe0, "lda",      INTRR,
0xf4003800, 0xfc00ffe0, "lda.h",    INTRR,
0xf4003c00, 0xfc00ffe0, "lda.b",    INTRR,

0xf4003200, 0xfc00ffe0, "lda.d",    INTRR,
0xf4003600, 0xfc00ffe0, "lda",      INTRR,
0xf4003a00, 0xfc00ffe0, "lda.h",    INTRR,
0xf4003e00, 0xfc00ffe0, "lda.b",    INTRR,

0x80004000, 0xfc1ff81f, "ldcr",     LDCR,

0xf000a000, 0xfc00fc00, "mak",      BITFIELD,
0xf400a000, 0xfc00ffe0, "mak",      INTRR,

0x48000000, 0xfc000000, "mask",     INTRL,
0x4c000000, 0xfc000000, "mask.u",   INTRL,

0x6c000000, 0xfc000000, "mul",      INTRL,
0xf4006c00, 0xfc00fee0, "mul",      INTRR,

0x84005000, 0xfc1fffe0, "nint.ss",  INTRR2OP,
0x84005080, 0xfc1fffe0, "nint.sd",  INTRR2OP,

0x58000000, 0xfc000000, "or",       INTRL,
0x5c000000, 0xfc000000, "or.u",     INTRL,
0xf4005800, 0xfc00ffe0, "or",       INTRR,
0xf4005c00, 0xfc00ffe0, "or.c",     INTRR,

0xf000a800, 0xfc00ffe0, "rot",      ROT,
0xf400a800, 0xfc00ffe0, "rot",      INTRR,

0xf400fc00, 0xffffffff, "rte",      RTE,

0xf0008800, 0xfc00fc00, "set",      BITFIELD,
0xf4008800, 0xfc00ffe0, "set",      INTRR,

0x20000000, 0xfc000000, "st.d",     STLIT,
0x24000000, 0xfc000000, "st",       STLIT,
0x28000000, 0xfc000000, "st.h",     STLIT,
0x2c000000, 0xfc000000, "st.b",     STLIT,

0xf4002000, 0xfc00ffe0, "st.d",     STRO,
0xf4002400, 0xfc00ffe0, "st",       STRO,
0xf4002800, 0xfc00ffe0, "st.h",     STRO,
0xf4002c00, 0xfc00ffe0, "st.b",     STRO,

0xf4002200, 0xfc00ffe0, "st.d",     STRI,
0xf4002600, 0xfc00ffe0, "st",       STRI,
0xf4002a00, 0xfc00ffe0, "st.h",     STRI,
0xf4002e00, 0xfc00ffe0, "st.b",     STRI,

0xf4002100, 0xfc00ffe0, "st.d.usr", STRO,
0xf4002500, 0xfc00ffe0, "st.usr",   STRO,
0xf4002900, 0xfc00ffe0, "st.h.usr", STRO,
0xf4002d00, 0xfc00ffe0, "st.b.usr", STRO,

0xf4002300, 0xfc00ffe0, "st.d.usr", STRI,
0xf4002700, 0xfc00ffe0, "st.usr",   STRI,
0xf4002b00, 0xfc00ffe0, "st.h.usr", STRI,
0xf4002f00, 0xfc00ffe0, "st.b.usr", STRI,

0x80008000, 0xffe0f800, "stcr",     STCR,

0x74000000, 0xfc000000, "sub",      INTRL,
0xf4007400, 0xfc00ffe0, "sub",      INTRR,
0xf4007500, 0xfc00ffe0, "sub.bo",   INTRR,
0xf4007600, 0xfc00ffe0, "sub.bi",   INTRR,
0xf4007700, 0xfc00ffe0, "sub.bio",  INTRR,

0x64000000, 0xfc000000, "subu",     INTRL,
0xf4006400, 0xfc00ffe0, "subu",     INTRR,
0xf4006500, 0xfc00ffe0, "subu.bo",  INTRR,
0xf4006600, 0xfc00ffe0, "subu.bi",  INTRR,
0xf4006700, 0xfc00ffe0, "subu.bio", INTRR,

0xf000d000, 0xfc00fe00, "tb0",      TRAP,
0xf000d800, 0xfc00fe00, "tb1",      TRAP,

0xf8000000, 0xffe00000, "tbnd",     TBND,
0xf400f800, 0xffe0ffe0, "tbnd",     INTRR_S1_S2,

0xf000e800, 0xfc00fe00, "tcnd",     TCND,

0x84005800, 0xfc1fffe0, "trnc.ss",  INTRR2OP,
0x84005880, 0xfc1fffe0, "trnc.sd",  INTRR2OP,

0x8000c000, 0xfc00c000, "xcr",      XCR,

0x50000000, 0xfc000000, "xor",      INTRL,
0x54000000, 0xfc000000, "xor.u",    INTRL,
0xf4005000, 0xfc00ffe0, "xor",      INTRR,
0xf4005400, 0xfc00ffe0, "xor.c",    INTRR,

0x00000000, 0x00000000, "pseudo",   0
};

/*
 * This returns a pointer to the instruction table entry that matches
 * the passed instruction.
 */
struct instr_info *remote_instruction_lookup(i)
    register u_long i;
{
    register struct instr_info *p;

    for (p = &in_tab[0] ; p->opmask != 0 ; p++) {
        if ((p->opmask & i) == p->opcode) {
            /*
             * We do a little adaptive searching here: when
             * we find an instruction we put it at the head of
             * the list.
             */
            return p;
        }
    }
    return (struct instr_info *)0;
}
