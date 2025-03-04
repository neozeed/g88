
/*
 * Memory manipulation routines for the 88000 simulator.
 *
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/memory.c,v 1.18 90/04/29 19:25:19 robertb Exp $
 */

#include "sim.h"

extern sim_not_decoded();
extern sim_end_of_page();

/*
 * This initializes the decoded part of a page.
 */
void
clean_decoded_part(d_ptr)
    struct decoded_part *d_ptr;
{
    register struct decoded_i *d, *lastd;

    if (!d_ptr) {
        return;
    }

    /*
     * Set all of the entry addresses in the page to point to the
     * not-decoded-yet entry point.  This entry point will decode
     * the instruction and then execute it.
     */
    d = &(d_ptr->decoded_i[0]);
    lastd = &(d_ptr->decoded_i[PAGESIZE/4]);
    while (d < lastd) {
        d->norm_e_addr = sim_not_decoded;
        d++;
    }

    /*
     * Install pointer to the end-of-page entry point so that when
     * the ip crosses a page boundary by being incremented, we requalify
     * the decoded instruction pointer.
     */
    lastd->norm_e_addr = sim_end_of_page;
}

/*
 * This zeros the data part of a memory page.  If the page has
 * a decoded part, it is re-initialized.
 */
void
clean_page(page_ptr)
    struct page *page_ptr;
{
    bzero(&page_ptr->values[0], PAGESIZE);
    clean_decoded_part(page_ptr->decoded_part);
}

/*
 * This allocates a page of simulated-88000 memory.
 */
struct page *
allocate_page()
{
    struct page *p;

    p = (struct page *)sbrk(sizeof(struct page) + 8);
    if (p == (struct page *)0) {
        sim_printf("allocate_page(): no memory left.\n");
        exit(1);
    }

    /*
     * Make the value part of a page be double-word aligned.
     */
    return (struct page *)(((u_long)p + 7) & ~7);
}

/*
 * This allocates and initializes the second part of a memory page.
 */
struct decoded_part *
allocate_decoded_part()
{
    struct decoded_part *d_ptr;

    d_ptr = (struct decoded_part *)sbrk(sizeof(struct decoded_part));
    if (!d_ptr) {
        sim_printf("allocate_decoded_part: unable to get memory with sbrk\n");
        exit(1);
    }
    clean_decoded_part(d_ptr);
    return d_ptr;
}

/*
 * Read a single byte of simulated memory.
 */
u_char
read_sim_b(sim_addr)
    u_long  sim_addr;
{
    if (sim_addr >= memory_size) {
        u_long value;

        if (io_operation(sim_addr, &value, BYTE, LD_U, 1) != E_NONE) {
            sim_errno = 1;
            return 0;
        }
        return value;
    }
    sim_errno = 0;
    return read_sim_w(sim_addr) >> 24;
}

/*
 * Read a halfword (16 bits) of simulated memory.
 */
u_short
read_sim_h(sim_addr)
    u_long  sim_addr;
{
    if (sim_addr & 1) {
        sim_errno = 1;
        return 0;
    }

    if (sim_addr >= memory_size) {
        u_long value;

        if (io_operation(sim_addr, &value, HALF, LD_U, 1) != E_NONE) {
            sim_errno = 1;
            return 0;
        }
        sim_errno = 0;
        return value;
    }

    sim_errno = 0;
    return read_sim_w(sim_addr) >> 16;
}

/*
 * This returns the value of one location of simulated memory.  
 * The parameter specifies the physical address of this location.
 */
u_long
read_sim_w(sim_addr)
    u_long  sim_addr;
{
    u_long  page;
    u_long  page_offset;
    struct page *page_ptr;
    u_long  w1, w2;

    sim_errno = 0;
    if (sim_addr >= memory_size) {
        u_long value;

        if (io_operation(sim_addr, &value, WORD, LD_U, 1) != E_NONE) {
            sim_errno = 1;
            return 0;
        }
        return value;
    }

    page = sim_addr / PAGESIZE;
    page_ptr = page_table[page];
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(sim_addr & ~PAGEMASK);
        page_table[page] = page_ptr;
    }
    page_offset = poff(sim_addr);

    w1 = page_ptr->values[page_offset / 4];

    switch (sim_addr & 3) {
        case 0:
            return w1;

        case 1:
            w2 = read_sim_w((sim_addr & ~3) + 4);
            return (w1 << 8) | (w2 >> 24);

        case 2:
            w2 = read_sim_w((sim_addr & ~3) + 4);
            return (w1 << 16) | (w2 >> 16);

        case 3:
            w2 = read_sim_w((sim_addr & ~3) + 4);
            return (w1 << 24) | (w2 >> 8);

        default:
            sim_printf("case error in read_sim_w(0x%x)\n", sim_addr);
    }
    sim_errno = 1;
    return 0;
}

/*
 * Write the passed value into a byte of simulated memory.
 */
int
write_sim_b(sim_addr, value)
    u_long  sim_addr;
    u_long  value;
{
    u_long  page;
    u_long  word_index;
    struct page *page_ptr;
    u_long  *word_ptr;
    u_char *byte_ptr;
    struct decoded_part *d_ptr;

    /*
     * First see if the address is outside the range of physcial
     * memory addresses.  If so, let the IO system have a chance
     * at it.
     */
    sim_errno = 0;
    if (sim_addr >= memory_size) {
        if (io_operation(sim_addr, &value, BYTE, ST, 1) != E_NONE) {
            sim_errno = 1;
        }
        return 0;
    }

    /*
     * Its a memory operation.  Find the physical page.  If its not
     * allocated yet, then allocate it and put it in the page table.
     */
    page = sim_addr / PAGESIZE;
    page_ptr = page_table[page];
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(sim_addr & ~PAGEMASK);
        page_table[page] = page_ptr;
    }
    word_index = poff(sim_addr) / 4;

    /*
     * Caclculate the offset within the page of the byte to modify.
     */
    word_ptr = &page_ptr->values[word_index];
    byte_ptr = (u_char *)word_ptr + (sim_addr & 3);
    *byte_ptr = (u_char)value;

    /*
     * Invalidate the decoded instruction that corresponds to this
     * location.
     */
    if (d_ptr = page_ptr->decoded_part) {
        d_ptr->decoded_i[word_index].norm_e_addr = sim_not_decoded;
    }
    return 0;
}

/*
 * Write the passed halfword (16 bits) into simulated memory.
 */
int
write_sim_h(sim_addr, value)
    u_long  sim_addr;
    u_long  value;
{
    sim_errno = 0;
    if (sim_addr >= memory_size) {
        if (io_operation(sim_addr, &value, HALF, ST, 1) != E_NONE) {
            sim_errno = 1;
            return -1;
        }
        return 0;
    }

    if (write_sim_b(sim_addr    , (value >> 8) & 0xff) == -1 ||
        write_sim_b(sim_addr + 1, (value     ) & 0xff) == -1) {
        sim_errno = 1;
        return -1;
    } else {
        return 0;
    }
}

/*
 * Write the passed word into simulator physical memory.
 */
int
write_sim_w(sim_addr, value)
    u_long  sim_addr;
    u_long  value;
{
    u_long  page;
    u_long  page_offset;
    u_long  word_index;
    struct page *page_ptr;
    struct decoded_part *d_ptr;
    u_long  a1;

    sim_errno = 0;
    if (sim_addr >= memory_size) {
        if (io_operation(sim_addr, &value, WORD, ST, 1) != E_NONE) {
            sim_errno = 1;
            return -1;
        }
        return 0;
    }

    page = sim_addr / PAGESIZE;
    page_ptr = page_table[page];
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(sim_addr & ~PAGEMASK);
        page_table[page] = page_ptr;
    }
    page_offset = poff(sim_addr);

    word_index = page_offset / 4;
    if ((sim_addr & 3) == 0) {
        page_ptr->values[word_index] = value;
        /*
         * Invalidate the decoded instruction that corresponds to the
         * word we are changing.
         */
        if (d_ptr = page_ptr->decoded_part) {
            d_ptr->decoded_i[word_index].norm_e_addr = sim_not_decoded;
        }

        return 0;
    }

    if (write_sim_b(sim_addr,     (value >> 24) & 0xff) == -1 ||
        write_sim_b(sim_addr + 1, (value >> 16) & 0xff) == -1 ||
        write_sim_b(sim_addr + 2, (value >>  8) & 0xff) == -1 ||
        write_sim_b(sim_addr + 3, (value      ) & 0xff) == -1) {
        sim_errno = 1;
        return -1;
    } else {
        return 0;
    }
}

/*
 * This takes a 88k physical address and a length in bytes.  It
 * returns a pointer to the simulation memory for the 88000 physical
 * address.  If a buffer starting at this address and extending for
 * for the length specified by the second parameter doesn't fit
 * in continguous host memory a 0 is returned instead.
 */
u_char *sim_ptr(sim_addr, length)
    u_long sim_addr;
    u_long length;
{
    u_long  page;
    u_long  page_offset;
    struct page *page_ptr;

    if (sim_addr >= memory_size) {
        return 0;
    }

    page_offset = poff(sim_addr);
    if (page_offset + length > PAGESIZE) {
        return 0;
    }

    page = sim_addr / PAGESIZE;
    page_ptr = page_table[page];
    if (page_ptr == (struct page *)0) {
        page_ptr = allocate_page(sim_addr & ~PAGEMASK);
        page_table[page] = page_ptr;
    }

    return (u_char *)&page_ptr->values[0] + page_offset;
}

/*
 * This reads part or all of the passed file into simulation memory.
 * It returns the number of bytes actually read.
 */
int sim_readfile(fd, sim_addr, len)
    int fd;
    u_long sim_addr;
    u_long len;
{
    u_long bytestoread, bytesread, actual = 0;
    u_char *buf, *malloc();

    while (len > 0) {
        bytestoread = PAGESIZE - poff(sim_addr);
        if (bytestoread > len) {
            bytestoread = len;
        }
        buf = sim_ptr(sim_addr, bytestoread);

        /*
         * If we can't get a pointer to this piece of memory, then
         * read the data into a buffer and stuff it into memory a 
         * word at a time.  This allows us to download the
         * ROM.
         */
        if (buf == 0) {
            int i;

            buf = malloc(bytestoread);
            if (!buf) {
                sim_printf("sim_readfile: unable to malloc buffer\n");
                return actual;
            }
            bytesread = read(fd, buf, bytestoread);
            for (i = 0 ; i < bytesread ; i += sizeof(u_long *)) {
                write_sim_w(sim_addr + i, *(u_long *)&buf[i]);
                if (sim_errno) {
                    sim_printf("sim_readfile: unable to write address 0x%x\n", 
                                                                  sim_addr);
                    return actual;
                }
            }
            free(buf);
        } else {
            bytesread = read(fd, buf, bytestoread);
        }
        actual += bytesread;
        if (bytesread != bytestoread) {
            break;
        }
        sim_addr += bytesread;
        len -= bytesread;
    }
    return actual;
}

/*
 * This writes part or all of the passed file into simulation memory.
 * It returns the number of bytes actually written.
 */
int sim_writefile(fd, sim_addr, len)
    int fd;
    u_long sim_addr;
    u_long len;
{
    u_long bytestowrite, byteswritten, actual = 0;
    u_char *buf;

    while (len > 0) {
        bytestowrite = PAGESIZE - poff(sim_addr);
        if (bytestowrite > len) {
            bytestowrite = len;
        }
        buf = sim_ptr(sim_addr, bytestowrite);
        if (buf == 0) {
            break;
        }
        byteswritten = write(fd, buf, bytestowrite);
        actual += byteswritten;
        if (byteswritten != bytestowrite) {
            break;
        }
        sim_addr += byteswritten;
        len -= byteswritten;
    }
    return actual;
}

/*
 * This zeros simulation memory.  It returns 0 if it was able to
 * zero all of the memory it was told to zero.  It returns -1 otherwise.
 */
int sim_zeromem(sim_addr, len)
    u_long sim_addr;
    u_long len;
{
    u_long bytestozero;
    u_char *buf;

    while (len > 0) {
        bytestozero = PAGESIZE - poff(sim_addr);
        if (bytestozero > len) {
            bytestozero = len;
        }
        buf = sim_ptr(sim_addr, bytestozero);
        if (buf == 0) {
            int i;

            for (i = 0 ; i < bytestozero ; i += sizeof(u_long *)) {
                write_sim_w(sim_addr + i, 0);
                if (sim_errno) {
                    sim_printf("sim_zeromem: unable to write address 0x%x\n", 
                                                                  sim_addr);
                    return -1;
                }
            }
        } else {
            bzero(buf, bytestozero);
        }
        sim_addr += bytestozero;
        len -= bytestozero;
    }
    return 0;
}
