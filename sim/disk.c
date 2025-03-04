/*
 * 88000 disk simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 * 
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/disk.c,v 1.19 91/01/13 23:58:26 robertb Exp $
 */

#include <sys/types.h>
#include <sys/fcntl.h>
#include "sim.h"

#define DISKS	4

char *filenames[4] = { "simdisk1", "simdisk2", "simdisk3", "simdisk4" };

static 
struct disk_regs {
    u_long physical_address;
    u_long block;
    u_long count;
    u_long command;
    char *filename;
    int fd;
} disk_regs[DISKS];

#define	DC_READ		(1)
#define	DC_WRITE	(2)

#define	SD_BLKSIZE	(512)

#define SIMDISK1   (0)
#define SIMDISK2   (1)
#define SIMDISK3   (2)
#define SIMDISK4   (2)

disk1_init(restore) { disk_init(SIMDISK1, restore); }
disk2_init(restore) { disk_init(SIMDISK2, restore); }
disk3_init(restore) { disk_init(SIMDISK3, restore); }
disk4_init(restore) { disk_init(SIMDISK4, restore); }

disk_init(diskno, restore)
    u_long diskno;
{
    struct disk_regs *p = &disk_regs[diskno];

    if (!restore) {
        p->physical_address = 0;
        p->block = 0;
        p->count = 0;
        p->command = 0;
	if (p->fd > 0) {
            if (close(p->fd) != 0) {
                sim_printf("unabled to close %d\n", p->filename);
            }
        }
        p->fd = 0;

        p->filename = filenames[diskno];

        if (!p->filename) {
            p->filename = "(none)";
            p->fd = -1;
        }
    }
    if (p->fd != -1) {
        p->fd = open(p->filename, O_RDWR, 0);
    }
}

int
disk1_operation(address_offset, reg_ptr, size, mem_op_type)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
{
    return disk_operation(address_offset, reg_ptr, size, mem_op_type, SIMDISK1);
}

int
disk2_operation(address_offset, reg_ptr, size, mem_op_type)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
{
    return disk_operation(address_offset, reg_ptr, size, mem_op_type, SIMDISK2);
}

int
disk3_operation(address_offset, reg_ptr, size, mem_op_type)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
{
    return disk_operation(address_offset, reg_ptr, size, mem_op_type, SIMDISK3);
}

int
disk4_operation(address_offset, reg_ptr, size, mem_op_type)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
{
    return disk_operation(address_offset, reg_ptr, size, mem_op_type, SIMDISK4);
}

static
int
do_disk_command(p)
    struct disk_regs *p;
{
    u_long offset = p->block * SD_BLKSIZE;
    u_long cnt = p->count * SD_BLKSIZE;

    if (lseek(p->fd, offset, 0) != offset) {
        sim_printf("simdisk: Error seeking to byte offset 0x%x in %s\n",
                                                 offset, p->filename);
        return E_DACC;
    }
    switch (p->command) {
        case DC_READ:
            if (sim_readfile(p->fd, p->physical_address, cnt) != cnt) {
                sim_printf("simdisk: error reading %d bytes\n", cnt);
                return E_DACC;
            }
            break;

        case DC_WRITE:
            if (sim_writefile(p->fd, p->physical_address, cnt) != cnt) {
                sim_printf("simdisk: error reading %d bytes\n", cnt);
                return E_DACC;
            }
            break;

        default:
            sim_printf("simdisk: unknown command: %d\n", p->command);
            return E_DACC;
            break;
    }
    return E_NONE;
}

disk_operation(address_offset, reg_ptr, size, mem_op_type, diskno)
    u_long address_offset;
    u_long *reg_ptr;
    u_long size;
    u_long mem_op_type;
    u_long diskno;
{
    struct disk_regs *p = &disk_regs[diskno];

    if (p->fd < 0) {
        sim_printf("simdisk: unable to open %s.\n", p->filename);
        return E_DACC;
    }

    if (size != WORD) {
        sim_printf("simdisk: must use word operations.\n");
        return E_DACC;
    }

    if (mem_op_type != ST) {
        sim_printf("simdisk: registers are write-only.\n");
        return E_DACC;
    }

    switch (address_offset) {
        case 0:
            /*
             * Physical address of buffer. 
             */
            p->physical_address = *reg_ptr;       
            break;

        case 4:
            /*
             * Disk block number.  If the block number is zero,
             * close and reopen the file.
             */
            p->block = *reg_ptr;       
            if (p->block == 0 && p->fd != -1) {
                close(p->fd);
                p->fd = open(p->filename, O_RDWR, 0);
            }
                
            break;

        case 8:
            /*
             * Number of blocks to read or write.
             */
            p->count = *reg_ptr;       
            break;

        case 12:
            /*
             * Command to read or write.
             */
            p->command = *reg_ptr;       
            return do_disk_command(p);
            break;

        default:
            sim_printf("simdisk: address offset not 0, 4, 8, or 12\n");
            return E_DACC;
    }
    return E_NONE;
}

static
char *
d_comname(command)
    u_long command;
{
    switch (command) {
        case DC_READ: return "read";
        case DC_WRITE: return "write";
        default: return "undefined";
    }
}

void
disk_print(diskno)
    u_long diskno;
{
    struct disk_regs *p = &disk_regs[diskno];

    sim_printf("disk %d: \n", diskno);
    sim_printf("              uses: %s\n", p->filename);
    sim_printf("  physical address: 0x%x\n", p->physical_address);
    sim_printf("        disk block: 0x%x\n", p->block);
    sim_printf("             count: 0x%x\n", p->count);
    sim_printf("           command: %s\n", d_comname(p->command));
}

void disk1_print() { disk_print(SIMDISK1); }
void disk2_print() { disk_print(SIMDISK2); }
void disk3_print() { disk_print(SIMDISK3); }
void disk4_print() { disk_print(SIMDISK4); }
