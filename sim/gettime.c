/*
 * 88000 gettimeofday (server) simulator.
 * 
 * Copyright (c) 1987, 1988, Tektronix Inc.
 * All Rights Reserved
 *
 * $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/sim/RCS/gettime.c,v 1.1 88/04/11 17:08:59 robertb Exp $
 */

#include "sim.h"
#include <sys/time.h>

struct {
    struct timeval tp;
    struct timezone tzp;
} tu;

void
gettime_init()
{}

int
gettime_operation(address_offset, reg_ptr, size, mem_op_type, override)
	unsigned address_offset;
	unsigned *reg_ptr;
	unsigned size;
	unsigned mem_op_type;
	int      override;
{
        if (address_offset == 0) {
            gettimeofday(&tu.tp, &tu.tzp);
        }
        do_mem_op(reg_ptr, (char *)&tu + address_offset, size, mem_op_type);
	return E_NONE;
}

gettime_print()
{
    sim_printf("gettime: tv_sec = %d tv_usec = %d\n", 
                                  tu.tp.tv_sec, tu.tp.tv_usec);

    sim_printf("gettime: tz_minuteswest = %d tz_dsttime = %d\n", 
                    tu.tzp.tz_minuteswest, tu.tzp.tz_dsttime);
}
