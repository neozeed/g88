

# Makefile for building the 188 debug monitor on an XD/88
# Robert Bedichek  3/90


#	Copyright (c) 1990 by Tektronix, Inc.
#
# It may freely be redistributed and modified so long as the copyright
# notices and credit attributions remain intact.

# "$Header: /home/bigbird/Usr.U6/robertb/m88k/src/g88/hmon/RCS/makefile,v 1.4 91/01/13 23:41:56 robertb Exp $";

CPP=/lib/cpp
OCSAS=ocs-m88k-as

AS=m88k-as
CC=gcc
LD=m88k-ld
GCCFLAGS=-B../../lib/m88k-

# Really we should not generate this file with the host
# compiler.  But it works out.

HOSTCC=/bin/cc
HOSTAS=/bin/as

tek188mon: mon.o monlow.o
	$(LD) -T ffe18000 -o tek188mon monlow.o mon.o

clean:
	rm -f mon.o monlow.o temp.s tek188mon

mon.o: mon.c monoffsets.h mon.h psl.h compress.h
	$(CC) $(GCCFLAGS) -S -O -Dinline= mon.c
	ex - <fix.mon mon.s
	$(AS) -o mon.o mon.s

monoffsets.h: genmonoffsets
	./genmonoffsets >monoffsets.h

genmonoffsets: genmonoffsets.c mon.h
	$(HOSTCC) -o genmonoffsets genmonoffsets.c

monlow.o: monlow.s monoffsets.h psl.h
	$(CPP) -P monlow.s | \
        sed -e 's/8\*3/24/' \
            -e 's/8\*1/8/' \
            -e 's/8\*253/2024/' \
            -e 's/8\*254/2032/' > temp.s
	$(AS) -o monlow.o temp.s

conv: conv.c
	$(CC) -o conv conv.c
