/*------------------------------------------------------------------------
 * IOB_defs.h
 *
 * This header file describes the hardware personality of the Blackbird
 * IOB BRD 
 *
 * 
 * Copyright (C) 1987, Tektronix, Inc.
 * All Rights Reserved.
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/IOB_defs.h,v 1.1 89/08/24 15:31:37 robertb Exp $
 *------------------------------------------------------------------------
 */

/* SPECIFIC to I/O Board- relative to FBUS CSR space- use FBUS mapped access */
#define	IOB_LRBAR		0x810	/* IOB local base address reg */
#define	IOB_FTV_BAR		0x818	/* IOB FBUS->VMEbus base address reg */
#define	IOB_GEN_INTR_SR		0x822	/* IOB GEN intr status reg */
#define IOB_VME_INTR_SR		0x832	/* IOB VME intr status reg */
#define IOB_INTR_MSK		0x837	/* IOB Timer/VME intrs msk reg */
#define IOB_LCA_PGM_PORT	0x82b	/* IOB LCA program port */
#define IOB_INTR_EDGE_RST	0x82b	/* IOB edge triggered intr reset reg */
#define	IOB_INTR_SENT_SR	0x83a	/* IOB intr sent status reg */
#define	IOB_INTR_SENT_RST	0x83e	/* IOB intr sent reset reg */
#define IOB_INTR_PRIOR_SR	0x82f	/* IOB intr priority status reg */

#define IOB_BASE_SPACE		0xe0000000  /* base space for mapping IOB's */

/* SPECIFIC to I/O Board- relative to IOB Local Base Address- i.e. mapped */
#define IOB_LAN_MEM_BEGIN	0x100000
#define IOB_LAN_MEM_END		0x140000	/* 1 above end */
#define IOB_LAN_MEM_SIZE	0x40000		/* 256K bytes */
#define IOB_VTF_LEVEL1		0x020000	/* 16Kbyte:1byte entries */
#define IOB_VTF_LEVEL2		0x040000	/* 64K=16Kbyte:4byte entries */
#define IOB_VTF_SRAM_SIZE	0x4000		/* 16K words */
#define IOB_DMAC_B		0x012000
#define IOB_DMAC_A		0x010000
#define IOB_VME_IACK		0x00f000
#define IOB_BAD_PAR		0x00e003
#define IOB_DMA_SNOOP		0x00d003
#define IOB_VME_RESET		0x00c000
#define IOB_VME_INTR_SET	0x00b003		/* lane Z */
#define IOB_CTL			0x00a003		/* IOB control reg */
#define IOB_STAT		0x00a002		/* IOB status reg- */
#define IOB_VME_SLOT_STAT	0x00a001	    /* IOB VME slot status */
#define IOB_FTV_MAP		0x003000
#define IOB_LAN			0x002000  /* lanes Y,Z- struct adds offset */
#define IOB_NVRAM		0x001003		/* lane Z */
#define IOB_TIMER		0x000003		/* lane Z */
#define IOB_DMACA_INTR_ACK	0x011000	/* intr acknowledge decode */	
#define IOB_DMACB_INTR_ACK	0x013000	/* intr acknowledge decode */
#define IOB_SCSI_FLUSH		0x014000
#define IOB_SCCA_RX_FLUSH	0x017000
#define IOB_SCCB_RX_FLUSH	0x016000

#define IOB_LAN_PADDR_MSK	0x0fffff	/* mask LAN physical address */

/* SCSI is on DMACA_CH1 */
#define SCSI_PER_OFFSET		0x000080  /* struct adds byte lane Z offset */
#define IOB_SCSI_BASE		IOB_DMAC_A+SCSI_PER_OFFSET
#define IOB_SCSI_DMA_BASE	IOB_DMAC_A

/* HC is on DMACA_CH2 */
#define HC_PER_OFFSET		0x103		/* lane Z */
#define IOB_HC_SNG_DATA		IOB_DMAC_A+HC_PER_OFFSET
#define IOB_HC_STR_DATA		IOB_DMAC_A+HC_PER_OFFSET+0x20
#define IOB_HC_IPRIME_LATCH	IOB_DMAC_A+HC_PER_OFFSET+0x40  /* write only */
#define IOB_HC_INTR_CLR	        IOB_HC_IPRIME_LATCH            /* read only */
#define IOB_HC_REQDMA_LATCH	IOB_DMAC_A+HC_PER_OFFSET+0x60
#define IOB_HC_STAT_REG		IOB_HC_SNG_DATA

/* SCC_A_Tx:  chan2         SCC_B_Tx:  chan0
 * SCC_A_Rx:  chan3         SCC_B_Rx:  chan1
 */
#define SCC_A_PER_OFFSET	0x43	/* lane Z- for byte access */
#define SCC_B_PER_OFFSET	0x03	/* lane Z- for byte access */
#define IOB_SCC_A_BASE		IOB_DMAC_B+SCC_A_PER_OFFSET
#define IOB_SCC_B_BASE		IOB_DMAC_B+SCC_B_PER_OFFSET
#define SER_CTL_OFFSET		0xb	/* lane Z- for byte access */
#define IOB_SER_CTL		IOB_DMAC_B+SER_CTL_OFFSET

#define IOB_NORESET  0
#define IOB_RESET    1


/*------------------------------------------------------------------------
 * IOB General Interrupt Status Register
 *
 * This 16-bit read-only register provides the current state of the masked
 * general interrupt sources. The interrupt sources are sampled and latched 
 * at the start of the read process to prevent metastable conditions. The Sent
 * Status Register is also clocked at this time.
 *
 *  1 1 1 1 1 1 
 *  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 * | | | | | | | | | | | | | | | | |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *  | | | | | | | | | | | | | | | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---- IS<15..0> Interrupt Status; 0=Present
 *
 *------------------------------------------------------------------------
 */

#define IOB_GEN_ISR15	0x8000
#define IOB_GEN_ISR14	0x4000
#define IOB_GEN_ISR13	0x2000
#define IOB_GEN_ISR12	0x1000
#define IOB_GEN_ISR11	0x0800
#define IOB_GEN_ISR10	0x0400
#define IOB_GEN_ISR9	0x0200
#define IOB_GEN_ISR8	0x0100
#define IOB_GEN_ISR7	0x0080
#define IOB_GEN_ISR6	0x0040
#define IOB_GEN_ISR5	0x0020
#define IOB_GEN_ISR4	0x0010
#define IOB_GEN_ISR3	0x0008
#define IOB_GEN_ISR2	0x0004
#define IOB_GEN_ISR1	0x0002
#define IOB_GEN_ISR0	0x0001

#define IOB_TIMER_ISR	(~IOB_GEN_ISR8&0xffff)	/* Timer */
#define IOB_SCSI_ISR	(~IOB_GEN_ISR7&0xffff)	/* SCSI */
#define IOB_OPT_ISR	(~IOB_GEN_ISR6&0xffff)	/* option slot */
#define IOB_SCC_ISR	(~IOB_GEN_ISR5&0xffff)	/* IOB SCC */
#define IOB_DMACB_ISR	(~IOB_GEN_ISR4&0xffff)	/* serial DMAC */
#define IOB_DMACA_ISR	(~IOB_GEN_ISR3&0xffff)	/* DMACB */
#define IOB_LAN_ISR	(~IOB_GEN_ISR2&0xffff)	/* DMACA */
#define IOB_HC_ISR	(~IOB_GEN_ISR1&0xffff)	/* HC */
#define IOB_PFAIL_ISR	(~IOB_GEN_ISR0&0xffff)	/* from VME ACFAIL */


/*------------------------------------------------------------------------
 * IOB VME Interrupt Status Register
 *
 * This 16-bit read-only register provides the current state of the masked
 * VME interrupt sources. The interrupt sources are sampled and latched 
 * at the start of the read process to prevent metastable conditions. The Sent
 * Status Register is also clocked at this time.
 *
 *  1 1 1 1 1 1 
 *  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 * | | | | | | | | | | | | | | | | |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *  | | | | | | | | | | | | | | | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---- IS<15..0> Interrupt Status; 0=Present
 *
 *------------------------------------------------------------------------
 */

#define IOB_VME_ISR15		0x8000
#define IOB_VME_ISR14		0x4000
#define IOB_VME_ISR13		0x2000
#define IOB_VME_ISR12		0x1000
#define IOB_VME_ISR11		0x0800
#define IOB_VME_ISR10		0x0400
#define IOB_VME_ISR9		0x0200
#define IOB_VME_ISR8		0x0100
#define IOB_VME_ISR7		0x0080
#define IOB_VME_ISR6		0x0040
#define IOB_VME_ISR5		0x0020
#define IOB_VME_ISR4		0x0010
#define IOB_VME_ISR3		0x0008
#define IOB_VME_ISR2		0x0004
#define IOB_VME_ISR1		0x0002
#define IOB_VME_ISR0		0x0001

#define IOB_VME_BERR_ISR	(~IOB_VME_ISR11&0xffff)	/* no intr generated */
#define IOB_VME_SYSFAIL_ISR	(~IOB_VME_ISR10&0xffff)	/* GEMcp fault LED */
#define IOB_VME_SLOTREG_ISR	(~IOB_VME_ISR9&0xffff)
#define IOB_VME_PGFAULT_ISR	(~IOB_VME_ISR8&0xffff)
#define IOB_VME_IRQ1_ISR	(~IOB_VME_ISR0&0xffff)
#define IOB_VME_IRQ2_ISR	(~IOB_VME_ISR1&0xffff)
#define IOB_VME_IRQ3_ISR	(~IOB_VME_ISR2&0xffff)
#define IOB_VME_IRQ4_ISR	(~IOB_VME_ISR3&0xffff)
#define IOB_VME_IRQ5_ISR	(~IOB_VME_ISR4&0xffff)
#define IOB_VME_IRQ6_ISR	(~IOB_VME_ISR5&0xffff)
#define IOB_VME_IRQ7_ISR	(~IOB_VME_ISR6&0xffff)


/*------------------------------------------------------------------------
 * IOB Interrupt Sent Status Register
 *
 * This 16-bit read-only register provides the state of the interrupt sent
 * bits at the time of the last interrupt status read.
 *
 *  1 1 1 1 1 1 
 *  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 * | | | | | | | | | | | | | | | | |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *  | | | | | | | | | | | | | | | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---- SS<15..0> Interrupt Status; 0 = Sent; 
 *                                      1 = Not Sent
 *
 *------------------------------------------------------------------------
 */

#define IOB_INTR_SENT15		0x8000
#define IOB_INTR_SENT14		0x4000
#define IOB_INTR_SENT13		0x2000
#define IOB_INTR_SENT12		0x1000
#define IOB_INTR_SENT11		0x0800
#define IOB_INTR_SENT10		0x0400
#define IOB_INTR_SENT9		0x0200
#define IOB_INTR_SENT8		0x0100
#define IOB_INTR_SENT7		0x0080
#define IOB_INTR_SENT6		0x0040
#define IOB_INTR_SENT5		0x0020
#define IOB_INTR_SENT4		0x0010
#define IOB_INTR_SENT3		0x0008
#define IOB_INTR_SENT2		0x0004
#define IOB_INTR_SENT1		0x0002
#define IOB_INTR_SENT0		0x0001

#define IOB_PFAIL_INTR_SENT		(~IOB_INTR_SENT8&0xffff)/* VME ACFAIL */
#define IOB_TIMER_INTR_SENT		(~IOB_INTR_SENT7&0xffff) /* tod chip */
#define IOB_SCSI_INTR_SENT		(~IOB_INTR_SENT6&0xffff) /* scsi chip */

#define IOB_SCC_INTR_SENT		(~IOB_INTR_SENT5&0xffff)
#define IOB_DMACB_INTR_SENT		(~IOB_INTR_SENT5&0xffff) /* serialdma */

#define IOB_OPT_INTR_SENT		(~IOB_INTR_SENT4&0xffff) /* opt slot */
#define IOB_DMACA_INTR_SENT		(~IOB_INTR_SENT4&0xffff) /* scsidma */
#define IOB_HC_INTR_SENT		(~IOB_INTR_SENT4&0xffff) /* hard copy */

#define IOB_UNUSED_INTR_SENT		(~IOB_INTR_SENT3&0xffff) /* unused */

#define IOB_LAN_INTR_SENT		(~IOB_INTR_SENT2&0xffff) /* lan chip */

#define IOB_VME_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_BERR_INTR_SENT		(~IOB_INTR_SENT1&0xffff) /* stat only */
#define IOB_VME_SYSFAIL_INTR_SENT	(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_SLOTREG_INTR_SENT	(~IOB_INTR_SENT1&0xffff) /* slot_reg */
#define IOB_VME_PGFAULT_INTR_SENT	(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ1_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ2_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ3_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ4_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ5_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ6_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQ7_INTR_SENT		(~IOB_INTR_SENT1&0xffff)
#define IOB_VME_IRQX_INTR_SENT		(~IOB_INTR_SENT1&0xffff)


/*------------------------------------------------------------------------
 * IOB Interrupt Sent Reset Register
 *
 * This 16-bit write-only register provides a means to individually reset
 * sent register bits.
 *
 *  1 1 1 1 1 1 
 *  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 * | | | | | | | | | | | | | | | | |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ 
 *  | | | | | | | | | | | | | | | |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+---- SR<15..0> Interrupt Reset; 1 = Reset; 0 = No Change
 *
 *------------------------------------------------------------------------
 */

#define IOB_INTR_RST15		0x8000
#define IOB_INTR_RST14		0x4000
#define IOB_INTR_RST13		0x2000
#define IOB_INTR_RST12		0x1000
#define IOB_INTR_RST11		0x0800
#define IOB_INTR_RST10		0x0400
#define IOB_INTR_RST9		0x0200
#define IOB_INTR_RST8		0x0100
#define IOB_INTR_RST7		0x0080
#define IOB_INTR_RST6		0x0040
#define IOB_INTR_RST5		0x0020
#define IOB_INTR_RST4		0x0010
#define IOB_INTR_RST3		0x0008
#define IOB_INTR_RST2		0x0004
#define IOB_INTR_RST1		0x0002
#define IOB_INTR_RST0		0x0001

#define IOB_PFAIL_INTR_RST		IOB_INTR_RST8
#define IOB_TIMER_INTR_RST		IOB_INTR_RST7

#define IOB_SCSI_INTR_RST		IOB_INTR_RST6	/* scsi chip */

#define IOB_SCC_INTR_RST		IOB_INTR_RST5	/* serial chip */
#define IOB_DMACB_INTR_RST		IOB_INTR_RST5	/* serial dma */

#define IOB_OPT_INTR_RST		IOB_INTR_RST4	/* option slot */
#define IOB_DMACA_INTR_RST		IOB_INTR_RST4	/* scsi dma */
#define IOB_HC_INTR_RST			IOB_INTR_RST4	/* hard copy */

#define IOB_UNUSED_INTR_RST		IOB_INTR_RST3	/* unused */

#define IOB_LAN_INTR_RST		IOB_INTR_RST2	/* lan chip */

#define IOB_VME_INTR_RST		IOB_INTR_RST1
#define IOB_VME_BERR_INTR_RST		IOB_INTR_RST1	/* status only */
#define IOB_VME_SYSFAIL_INTR_RST	IOB_INTR_RST1
#define IOB_VME_SLOTREG_INTR_RST	IOB_INTR_RST1
#define IOB_VME_PGFAULT_INTR_RST	IOB_INTR_RST1
#define IOB_VME_IRQ1_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ2_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ3_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ4_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ5_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ6_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQ7_INTR_RST		IOB_INTR_RST1
#define IOB_VME_IRQX_INTR_RST		IOB_INTR_RST1

#define IOB_INTR_ALL_RST   0x1ff  /* reset all currently implemented intr's */


/*------------------------------------------------------------------------
 * IOB Edge Triggered Interrupts Reset Register
 *
 * The IOB Edge Interrupts Reset register contains functional bits for
 * reseting the various edge sensitive interrupt flops.
 *
 * This register should only be accessed as a byte.
 *
 *  
 *  7             0
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  +-+-+-+-+-+-+-+--- SR<7..0> Interrupt Reset; 1 = Reset; 0 = No Change
 *
 *------------------------------------------------------------------------
 */

#define IOB_INTR_EDGE_RST7	0x0080
#define IOB_INTR_EDGE_RST6	0x0040
#define IOB_INTR_EDGE_RST5	0x0020
#define IOB_INTR_EDGE_RST4	0x0010
#define IOB_INTR_EDGE_RST3	0x0008
#define IOB_INTR_EDGE_RST2	0x0004
#define IOB_INTR_EDGE_RST1	0x0002
#define IOB_INTR_EDGE_RST0	0x0001

#define IOB_VME_SYSFAIL_ERST	IOB_INTR_EDGE_RST0
#define IOB_VME_PGFAULT_ERST	IOB_INTR_EDGE_RST1
#define IOB_VME_BERR_ERST	IOB_INTR_EDGE_RST2	/* status only */
#define IOB_VME_PFAIL_ERST	IOB_INTR_EDGE_RST3

#define IOB_INTR_ALL_ERST	0x0f	/* reset all current edge intr's */


/*------------------------------------------------------------------------
 * IOB VME_interrupt Generator Register
 *
 * The IOB VME_INTR_GEN register allows software to assert
 * the VME interrupts- VME_INTR1 thru VME_INTR7
 *
 * This register should only be accessed as a byte in lane Z.
 *
 *  7             0
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +--- VME_INTR1_SET
 *  | | | | | | +----- VME_INTR2_SET
 *  | | | | | +------- VME_INTR3_SET
 *  | | | | +--------- VME_INTR4_SET
 *  | | | +----------- VME_INTR5_SET
 *  | | +------------- VME_INTR6_SET
 *  | +--------------- VME_INTR7_SET
 *  +----------------- 
 *
 *------------------------------------------------------------------------
 */

#define IOB_VME_INTR1_SET	0x1
#define IOB_VME_INTR2_SET	0x2
#define IOB_VME_INTR3_SET	0x4
#define IOB_VME_INTR4_SET	0x8
#define IOB_VME_INTR5_SET	0x10
#define IOB_VME_INTR6_SET	0x20
#define IOB_VME_INTR7_SET	0x40


/*------------------------------------------------------------------------
 * IOB Interrupt Mask Register
 *
 * The IOB Interrupt Mask register contains a functional bits
 * for masking the timer interrupt and all VME based interrupts
 *
 * This register should only be accessed as a byte.  Write only.
 *
 *  
 *  7             0
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  +-+-+-+-+-+-+-+--- SR<7..0> Interrupt Mask; 1 = Mask; 0 = Unmask
 *
 *------------------------------------------------------------------------
 */

#define IOB_VME_INTRS_ENA	0x1	/* enable VME interrupts */
#define IOB_TIMER_INTR_ENA	0x40	/* enable Timer interrupts */

#define IOB_SCC_INTR_ENA	0x10  	/* IOB SCC interrupt enable */
#define IOB_HC_INTR_ENA		0x20  	/* IOB hard copy interrupt enable */


/*------------------------------------------------------------------------
 * IOB Control Register
 *
 * The IOB control register contains functional control bits for the IOB 
 * brd including the LED fault light.
 *
 * This register should only be accessed as a byte.
 *
 *  
 *  7             0
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +--- VME_PRIOR: Priority mode for VME controller chip
 *  | | | | | | +----- VME_RWD: VME release when done mode
 *  | | | | | +------- PONCMD: lo true; enable soft power hold_up
 *  | | | | +--------- SFAULT: lo true; turn on IOB brd fault LED
 *  | | | +----------- LCA_PROG: lo true; enable LCA programming
 *  | | +------------- VTF_ENABLE: hi true; enable VME to FBUS translation
 *  | +--------------- FTV_ENABLE: hi true; enable FBUS to VME translation
 *  +----------------- 
 *
 *------------------------------------------------------------------------
 */

/* For VMEbus use release on request, and fixed priority */

#define IOB_VME_PRIOR_ROUND 	0x1	/* round-robin arbitration */
#define IOB_VME_PRIOR_FIXED	0x0	/* fixed priority arbitration */
#define IOB_VME_ROR		0x0	/* release on request mode */
#define IOB_VME_RWD		0x2	/* release when done mode */
#define IOB_PONCMD		0x4	/* soft power hold-up enabled */
#define IOB_LED_OFF		0x8	/* LED off */
#define IOB_LCA_PROG		0x10	
#define IOB_VTF_ENABLE		0x20	
#define IOB_FTV_ENABLE		0x40
#define IOB_INTR_ENABLE		0x80
#define IOB_INTR_DISABLE	0x7F

#define IOB_CTL_DEFAULT    (IOB_VME_PRIOR_FIXED|IOB_VME_ROR)


/*------------------------------------------------------------------------
 * IOB Status Register
 *
 * The IOB status register contains status bits for the IOB 
 * brd including the LCA status bits
 *
 * This register should only be accessed as a byte.
 *
 *  1
 *  5             8
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +--- PONSW
 *  | | | | | | +----- VME_SYSFAIL
 *  | | | | | +------- LCA_DONE
 *  | | | | +--------- LCA_BUSY
 *  | | | +----------- LCA_INIT
 *  | | +------------- NVRAM_DOUT
 *  | +--------------- 
 *  +----------------- SCSI_INTR_STAT
 *
 *------------------------------------------------------------------------
 */

#define IOB_PONSW		0x1
#define IOB_VME_SYSFAIL 	0x2
#define IOB_LCA_DONE		0x4
#define IOB_LCA_BUSY		0x8
#define IOB_LCA_INIT		0x10
#define IOB_NVRAM_DOUT		0x20
#define IOB_SCSI_INTR_STAT      0x80


/*------------------------------------------------------------------------
 * IOB VME Slot Status Register
 *
 * The IOB VME Slot status register contains status bits for 
 * which VME slot the IOB is installed in
 *
 * This register should only be accessed as a byte.
 *
 *  2             1
 *  3             6
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +--- VME_SLOT_BIT0
 *  | | | | | | +----- VME_SLOT_BIT1
 *  | | | | | +------- VME_SLOT_BIT2
 *  | | | | +--------- VME_SLOT_BIT3
 *  | | | +----------- 
 *  | | +-------------
 *  | +--------------- 
 *  +----------------- 
 *
 *------------------------------------------------------------------------
 */

#define	IOB_VME_SLOT_STAT_MSK		0xF



/*------------------------------------------------------------------------
 * IOB NVRAM Register
 *
 * The IOB nvram register contains bits for programming
 * the IOB non-volatile RAM- Xicor-2444
 *
 * This register should only be accessed as a byte in lane Z.
 *
 *  7             0
 * +-+-+-+-+-+-+-+-+
 * | | | | | | | | |
 * +-+-+-+-+-+-+-+-+
 *  | | | | | | | |
 *  | | | | | | | +--- NVRAM_DI
 *  | | | | | | +----- NVRAM_SK
 *  | | | | | +------- NVRAM_CE
 *  | | | | +---------
 *  | | | +-----------
 *  | | +-------------
 *  | +--------------- 
 *  +----------------- 
 *
 *------------------------------------------------------------------------
 */

#define IOB_NVRAM_DI		0x1
#define IOB_NVRAM_SK 		0x2
#define IOB_NVRAM_CE		0x4


/*------------------------------------------------------------------------
 * IOB 8422 DRAM Controller Defines
 *
 * The IOB has an 8422 DRAM controller for the 256K byte LAN buffer
 * memory.  This part must be initialized via a write to a
 * magic address- the data used is immaterial, since the address bits 
 * are used to program the part (address bits applied to R0-R9/C0-C9 
 * of the part).  Initialization takes approximately 60ms- part should
 * not be accessed until this initialization is complete.
 * NOTE- this address is relative to IOB Local Base Address- i.e. mapped 
 *------------------------------------------------------------------------
 */
#define IOB_DRAMC_PROGRAM	0x16F6C0


/*------------------------------------------------------------------------
 * IOB SCC Serial Control Register
 *
 * The IOB has a 4 bit serial control register latch
 * for driving the serial port control lines DSR and CTS
 *
 *------------------------------------------------------------------------
 */
#define IOB_DSRA	0x1
#define IOB_CTSA	0x2
#define IOB_DSRB	0x4
#define IOB_CTSB	0x8


/*------------------------------------------------------------------------
 * IOB DMA SNOOP Control Register
 *
 * The IOB has an 8 bit r/w register which currently implements two
 * control bits (A/B Snoop) for enabling FBUS snooping logic on DMA 
 * accesses over the FBUS by either the A or B DMAC controller.
 *
 *------------------------------------------------------------------------
 */
#define IOB_A_SNOOP	0x80
#define IOB_B_SNOOP	0x40


/*------------------------------------------------------------------------
 * IOB FBUS Bad Parity Generator
 *
 * The IOB has an 8 bit r/w register which currently implements 4
 * control bits for causing bad parity to be generated during FBUS 
 * data phase on each of the 4 byte lanes when the IOB is accessed
 * as a slave.
 *
 *------------------------------------------------------------------------
 */
#define IOB_BPAR_0	0x08
#define IOB_BPAR_1	0x04
#define IOB_BPAR_2	0x02
#define IOB_BPAR_3	0x01
