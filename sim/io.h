/*
 * Header file for IO part of 88000 simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/io.h,v 1.22 90/09/29 21:12:43 robertb Exp $
 */

struct io_dev {
    u_long    l_addr;         /* Lowest address device responds to    */
    u_long    size;           /* 1, 2, 4, or 8.  Size of IO operation.*/
    u_long    restore;        /* boolean, 1 => call after checkpoint  */
    u_long    cnt;            /* Number of IO transactions performed  */
    char      *name;          /* Name of IO device.                   */
    void      (*init)();      /* function called at initialization.   */
    int       (*operation)(); /* function called for each IO operation*/
    void      (*print)();     /* function called to print dev state.  */
};

struct io_trace {
    u_long cnt;               /* Number of this transaction.          */
    u_long addr;              /* IO address transaction is done with. */
    struct io_dev *io_ptr;    /* Points to IO device structure.       */
    u_long ip;                /* Instruction pointer at time of IO.   */
    u_long value;             /* Word that is loaded or stored to IO. */
    u_char mem_op_type;       /* Type of IO operation (LD, ST, LD_U..)*/
    u_char size;              /* Size of IO operation (BYTE, HALF, ..)*/
    short  pad; };            /* Make this word aligned.              */

extern struct io_dev io_dev_tab[];
extern u_long cmmus;

#define DECL(name)  extern void name##_init();      \
                    extern int  name##_operation(); \
                    extern void name##_print()

DECL(data_cmmu_0); DECL(data_cmmu_1); DECL(data_cmmu_2); DECL(data_cmmu_3);
DECL(code_cmmu_0); DECL(code_cmmu_1); DECL(code_cmmu_2); DECL(code_cmmu_3);

DECL(console);     DECL(clrint);      DECL(duart);        DECL(whoami);
DECL(disk1);       DECL(disk2);       DECL(disk3);       DECL(disk4);       
DECL(special);     DECL(gettime);     DECL(trivial_timer);                  
DECL(ien0);	   DECL(cio);
DECL(ien1);        DECL(ien2);        DECL(ien3);        DECL(ien);
DECL(ist);         DECL(setswi);      DECL(clrswi);      DECL(istate);


#undef DECL
