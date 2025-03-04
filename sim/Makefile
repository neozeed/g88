
# Makefile to build sim.o, the object file with the 88000 simulator.

CFLAGS= -g 

HEADERS= CE_defs.h DEFS.h FBUS_defs.h IOB_defs.h cmmu.h decode.h \
         io.h sim.h sim_interface.h stacks.h

SOURCES= ce.c cmmu.c console.c simdecode.c simdisasm.c disk.c \
         execute.c gettime.c mvme188.c siminit.c io.c\
         memory.c misc.c simgdb.c duart.c

# Kludge to make ctags work on simulator files from the gdb source
# directory.  There must be a better way.

SLASH_HEADERS= sim/CE_defs.h sim/DEFS.h sim/FBUS_defs.h sim/IOB_defs.h \
               sim/cmmu.h sim/decode.h sim/io.h \
               sim/sim.h sim/sim_interface.h sim/stacks.h
               

SLASH_SOURCES= sim/ce.c sim/cmmu.c sim/console.c sim/simdecode.c sim/simdisasm.c \
               sim/disk.c\
               sim/execute.c sim/gettime.c sim/siminit.c sim/io.c\
               sim/memory.c sim/misc.c \
               sim/simgdb.c sim/duart.c\
	       sim/mvme188.c

OBJECTS= cmmu.o console.o disk.o simdecode.o simdisasm.o execute.o \
	 gettime.o siminit.o io.o memory.o misc.o mvme188.o \
	 simgdb.o duart.o

sim.o: $(OBJECTS)
	ld -r -o sim.o $(OBJECTS)

extern.h: execute.c
	egrep "^L" execute.c > extern.h
	-egrep "^BB" execute.c >> extern.h

disasm.h:
	ln -s ../disasm.h

fields88.h:
	ln -s ../fields88.h

format.h:
	ln -s ../format.h

m-m88k.h:
	ln -s ../m-m88k.h

montraps.h:
	ln -s ../montraps.h

execute.o: execute.c sim.h sim_interface.h fields88.h extern.h montraps.h
	-$(CC) -g -S execute.c
	sed -e 's.\/\*\*\/..g' \
	    -e '/START DELETING HERE/,/STOP DELETING HERE/d' execute.s \
		     >execute1.s
	$(AS) -o execute.o execute1.s

co-all:
	colast $(SOURCES) $(HEADERS)

clean:
	rm -f $(OBJECTS) sim.o extern.h execute1.s execute.s
	rm -f disasm.h format.h fields88.h m-m88k.h montraps.h

exterminate:
	make clean
	rm -f $(SOURCES) $(HEADERS) Makefile

depend: extern.h format.h fields88.h montraps.h disasm.h m-m88k.h
	makedepend $(CDEFINES) $(CINCLUDES) -f Makefile  \
         -s '# DO NOT DELETE THIS LINE' $(SOURCES)

trace: trace.c sim_interface.h
	$(CC) $(CFLAGS) -o trace trace.c

tags: $(HEADERS) $(SOURCES)
	ctags -w $(SOURCES) $(HEADERS)

gdbtags: $(HEADERS) $(SOURCES)
	cd .. ; ctags -w $(SLASH_SOURCES) $(SLASH_HEADERS)


# DO NOT DELETE THIS LINE - makedepend needs it

ce.o: sim.h sim_interface.h io.h CE_defs.h FBUS_defs.h IOB_defs.h
cmmu.o: sim.h sim_interface.h cmmu.h io.h
console.o: sim.h sim_interface.h
simdecode.o: sim.h sim_interface.h decode.h format.h fields88.h extern.h
simdisasm.o: /usr/include/stdio.h fields88.h disasm.h format.h
disk.o: /usr/include/sys/types.h /usr/include/sys/fcntl.h sim.h
disk.o: sim_interface.h
dmac.o: sim.h sim_interface.h dma.h we32104.h
dram_errs.o: sim.h sim_interface.h cmmu.h
dram_mapper.o: sim.h sim_interface.h
execute.o: sim.h sim_interface.h fields88.h montraps.h extern.h
gettime.o: sim.h sim_interface.h /usr/include/sys/time.h
siminit.o: sim.h sim_interface.h /usr/include/signal.h
siminit.o: /usr/include/sys/signal.h
io.o: sim.h sim_interface.h io.h
iob.o: sim.h sim_interface.h io.h FBUS_defs.h IOB_defs.h
lanram.o: sim_interface.h
lcsr.o: sim.h sim_interface.h io.h
led.o: sim.h sim_interface.h led.h
memory.o: sim.h sim_interface.h
misc.o: sim.h sim_interface.h cmmu.h decode.h format.h fields88.h
misc.o: /usr/include/string.h /usr/include/signal.h /usr/include/sys/signal.h
misc.o: /usr/include/stdio.h
newcons.o: /usr/include/errno.h /usr/include/sys/errno.h
newcons.o: /usr/include/sys/file.h /usr/include/sys/types.h
newcons.o: /usr/include/sys/stat.h /usr/include/fcntl.h
newcons.o: /usr/include/sys/fcntl.h /usr/include/signal.h
newcons.o: /usr/include/sys/signal.h /usr/include/sgtty.h
newcons.o: /usr/include/stdio.h sim.h sim_interface.h io.h
rom.o: /usr/include/sys/fcntl.h /usr/include/sys/types.h sim.h
rom.o: sim_interface.h
scc.o: /usr/include/errno.h /usr/include/sys/errno.h /usr/include/sys/types.h
scc.o: /usr/include/sys/stat.h /usr/include/fcntl.h /usr/include/sys/fcntl.h
scc.o: /usr/include/signal.h /usr/include/sys/signal.h /usr/include/stdio.h
scc.o: /usr/include/sys/ioctl.h /usr/include/sys/termio.h sim.h
scc.o: sim_interface.h io.h
simgdb.o: /usr/include/stdio.h sim_interface.h m-m88k.h ../ui.h ../defs.h
simgdb.o: /usr/include/signal.h /usr/include/sys/signal.h
timer.o: /usr/include/sys/time.h /usr/include/signal.h
timer.o: /usr/include/sys/signal.h sim.h sim_interface.h dp8571.h
mvme188.o: sim.h sim_interface.h
