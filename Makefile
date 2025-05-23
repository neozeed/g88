#TEK
#TEK NOTE:  changes made at Tektronix are marked: TEK
#TEK
#TEK $Header: /home/bigbird/Usr.U6/robertb/tar/g88/RCS/Makefile,v 1.55 91/01/14 01:20:20 robertb Exp $
#TEK $Locker:  $

# /bin/cc has been known to fail on readline.c on Vaxen running 4.3.
# If this occurs, use gcc (but see below for compiling with gcc).

# On USG (System V) machines, you must make sure to setup REGEX &
# REGEX1 to point at regex.o and use the USG version of CLIBS.  
# If your system has a broken alloca(), define ALLOCA & ALLOCA1 below.
# Uncomment the SYSV_DEFINE below for USG machines.
# Also, if you compile gdb with a compiler which uses the coff
# encapsulation feature (this is a function of the compiler used, NOT
# of the m-?.h file selected by config.gdb), you must make sure that
# the GNU nm is the one that is used by munch.

# On Sunos 4.0 machines, make sure to compile *without* shared
# libraries if you want to run gdb on itself.  Make sure to compile
# any program on which you want to run gdb without shared libraries.

# On ISI machines, make sure to compile gdb with -DBSD43_ISI40D if you 
# run 4.3BSD.

# If you are compiling with GCC, make sure that either 1) You use the
# -traditional flag, or 2) You have the fixed include files where GCC
# can reach them.  Otherwise the ioctl calls in inflow.c and readline.c 
# will be incorrectly compiled.  The "fixincludes" script in the gcc
# distribution will fix your include files up.

# It is also possible that you will need to add -I/usr/include/sys to the
# CFLAGS section if your system doesn't have fcntl.h in /usr/include (which 
# is where it should be according to Posix).

#CC=gcc -O
# CC=gcc-3.3 -arch ppc
CC=gcc -arch i386
#CC=gcc -traditional
#CC=/bin/cc
#YACC=bison -y -v
YACC=yacc
SHELL=/bin/sh

# Set this up with gcc if you have gnu ld and the loader will print out
# line numbers for undefinded refs.  
#CC-LD=cc -Bstatic
CC-LD=i586-linux-gcc
CC-LD=$(CC)

# -I. for "#include <obstack.h>".  Possibly regex.h also. 
#
# BSD OS X PPC
# HACKFLAGS = -DDG_HACK -DTEK_HACK -DTEK_PROG_HACK -DGHSFORTRAN -DGHS185 -DATTACH_DETACH -DBYTES_BIG_ENDIAN -DBSD -Dm88k -DNON_NATIVE 
HACKFLAGS = -DDG_HACK -DTEK_HACK -DTEK_PROG_HACK -DGHSFORTRAN -DGHS185 -DATTACH_DETACH -DBSD -Dm88k -DNON_NATIVE 
#
#
# HACKFLAGS = -DDG_HACK -DTEK_HACK -DTEK_PROG_HACK -DGHSFORTRAN -DGHS185 -DATTACH_DETACH -DBYTES_BIG_ENDIAN -DSYSV -Dm88k -DNON_NATIVE 
# HACKFLAGS = -DDG_HACK -DTEK_HACK -DTEK_PROG_HACK -DGHSFORTRAN -DGHS185 -DATTACH_DETACH -DBSD -Dm88k -DNON_NATIVE 

# -DUSEDGCOFF  use this only if you build you executables on an Aviion.

CFLAGS = -g -I. -c ${HACKFLAGS}
#CFLAGS = -I. -g -pg
#CFLAGS = -O -g -I.
# LDFLAGS =
LDFLAGS = -g

# define this to be "obstack.o" if you don't have the obstack library installed
# you must at the same time define OBSTACK1 as "obstack.o" 
# so that the dependencies work right.  Similarly with REGEX and "regex.o".
# You must define REGEX and REGEX1 on USG machines.
# If your sysyem is missing alloca(), or, more likely, it's there but
# it doesn't work, define ALLOCA & ALLOCA1
OBSTACK = obstack.o
OBSTACK1 = obstack.o

REGEX = regex.o
REGEX1 = regex.o
# REGEX = 
# REGEX1 =

# rcb ALLOCA = alloca.o
# rcb ALLOCA1 = alloca.o
ALLOCA = 
ALLOCA1 =

#
# define this to be "malloc.o" if you want to use the gnu malloc routine
# (useful for debugging memory allocation problems in gdb).  Otherwise, leave
# it blank.
#GNU_MALLOC = malloc.o
GNU_MALLOC =

# Flags to be used in compiling malloc.o
# Specify range checking for storage allocation.
#MALLOC_FLAGS = ${CFLAGS} -Dbotch
#MALLOC_FLAGS = ${CFLAGS} -Drcheck -Dbotch -DMSTATS
MALLOC_FLAGS = ${CFLAGS} -Drcheck -Dbotch

#
# define this to be SYSV if compiling on a system V or HP machine.
#SYSV_DEFINE = -DSYSV
SYSV_DEFINE = 

#
# Define this null if compiling on HP machines.
#MUNCH_DEFINE =
MUNCH_DEFINE =

# Flags that describe where you can find the termcap library.
# You may need to make other arrangements for USG.
TERMCAP = -lncurses #-ltermcap

#tek define this is you are using a fast malloc other than GNU malloc
#MALLOC_LIB = -lmalloc
MALLOC_LIB = 

ADD_FILES = ${OBSTACK} ${REGEX} ${ALLOCA} ${GNU_MALLOC}
ADD_DEPS = ${OBSTACK1} ${REGEX1} ${ALLOCA1} ${GNU_MALLOC}

# for BSD
CLIBS = ${ADD_FILES} ${TERMCAP}
# for USG
#CLIBS= ${$ADD_FILES} ${TERMCAP} -lPW

CFILES = blockframe.c breakpoint.c coffread.c dbxread.c command.c core.c \
	 environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c \
	 main.c printcmd.c programmer.c decode.c\
         motomode.c remote.c remcmd.c remmem.c remrun.c sim.c\
         source.c stack.c symmisc.c symtab.c \
	 utils.c valarith.c valops.c valprint.c values.c version.c \
	 ui.c watchstub.c

SFILES = $(CFILES) expread.y 

DEPFILES88 = m88k-dep.c
DEPFILES = $(DEPFILES88)

PINSNS88 = m88k-pinsn.c
PINSNS = $(PINSNS88)

HFILES = command.h compress.h defs.h decode.h disasm.h environ.h \
	 expression.h fields88.h format.h frame.h \
	 getpagesize.h \
	 inferior.h montraps.h remote.h symseg.h symtab.h value.h wait.h \
	 a.out.encap.h a.out.gnu.h stab.gnu.h ui.h

OPCODES88 = m88k-opcode.h
OPCODES =

MFILES88 = m-m88k.h
MFILES =

ALLFILES88 = $(DEPFILES88) $(PINSNS88) $(OPCODES88) $(MFILES88)
READLINE =  readline


POSSLIBS = obstack.h obstack.c regex.c regex.h malloc.c alloca.c

TESTS = testbpt.c testfun.c testrec.c testreg.c testregs.c

OTHERS = Makefile createtags munch config.gdb ChangeLog README TAGS \
	 gdb.texinfo .gdbinit COPYING expread.tab.c stab.def \
	 XGDB-README copying.c Projects copying.awk 

TAGFILES = ${SFILES} ${DEPFILES} ${PINSNS} ${HFILES} ${OPCODES} ${MFILES} \
	   ${POSSLIBS} 
TARFILES = ${TAGFILES} ${OTHERS} ${READLINE}

OBS = main.o blockframe.o breakpoint.o findvar.o stack.o source.o\
    values.o eval.o valops.o valarith.o valprint.o printcmd.o decode.o\
    symtab.o symmisc.o coffread.o dbxread.o infcmd.o infrun.o programmer.o\
    motomode.o remote.o remcmd.o remmem.o remrun.o sim.o\
    command.o utils.o expread.o expprint.o pinsn.o environ.o \
    version.o watchstub.o ${READLINEOBS} ui.o

TSOBS = core.o inflow.o dep.o

NTSOBS = standalone.o

NTSSTART = kdb-start.o

RL_LIB = ${READLINE}/libreadline.a

# Avoid funny things that suns make throughs in for us.
.c.o:
	${CC} -c ${CFLAGS} $<

#
# build gdb, no X interface, for testing.
#
g88:  $(OBS) $(TSOBS) $(ADD_FILES) $(TSOBS) init.o norcsid.o ${RL_LIB}
	${CC-LD} $(LDFLAGS) -o g88 norcsid.o $(OBS) $(TSOBS) init.o ${RL_LIB}\
	  $(CLIBS) ${MALLOC_LIB}

run-cscope: FRC
	cscope ${TAGFILES} ${ALLFILES88}

FRC:

#
# init.c is built automatically by "munch". It consists
# of init calls to add commands to gdb at gdb startup.
#
init.c:
	./munch ${MUNCH_DEFINE} $(TSOBS) $(OBS) > init.c


norcsid.o: norcsid.c
	${CC} -c norcsid.c

norcsid.c: FRC
	echo "char _rcsid[] = \"\$$XXX: $$user$$LOGNAME: `date`$$\";" \
		| sed s/XXX/Header/ > norcsid.c

ui.o: ui.c
	${CC} -c ${CFLAGS} -DNOXWINDOWS ui.c

rcsid.c: ${TAGFILES}
	mklog -sexp rcsid.c Makefile ${TAGFILES}
	chmod +w rcsid.c
	(echo /rcsid\\.c,v/s//gdb-Tek/ ; echo wq ) | ex - rcsid.c
	chmod -w rcsid.c

Xrcsid.c: ${TAGFILES} XOBJ/xrcsid.c
	mklog -sexp Xrcsid.c Makefile ${TAGFILES} XOBJ/xrcsid.c
	chmod +w Xrcsid.c
	(echo /_Xrcsid/s//_rcsid/;echo /Xrcsid\\.c,v/s//Xgdb-Tek/;echo wq)\
	  | ex - Xrcsid.c
	chmod -w Xrcsid.c

#TEK
#TEK check out everything from RCS
#TEK

co-all: FRC
	colast ${ALLFILES88}
	colast ${SFILES} ${DEPFILES} ${PINSNS} ${HFILES} ${OPCODES} ${MFILES} \
	${POSSLIBS} COPYING copying.awk config.gdb munch
	cd readline; colast Makefile ; make co-all
	cd sim; colast Makefile; make co-all

Xco-all: co-all
	cd XOBJ; colast Makefile ; make co-all

# If it can figure out the appropriate order, createtags will make sure
# that the proper m-*, *-dep, *-pinsn, and *-opcode files come first
# in the tags list.  It will attempt to do the same for dbxread.c and 
# coffread.c.  This makes using M-. on machine dependent routines much 
# easier.
#
#TAGS: ${TAGFILES}
#	createtags ${TAGFILES}
#tags: TAGS

# Build the tags file so that we can do "vi -t funcname" and
# esc-[ on a function name in vi and have vi find the the source
# file with the specified function and place the cursor on it.
# If there is a simulator directory, go into it and tell its makefile
# to append to ours so that we have all the simulator functions as well

# This will only work for the 88k, if we wanted to do it right, we'd
# test for the gdb that we are building and do a recursive call to make
# defining all the variables that vary from target to target.

tags: ${TAGFILES} $(ALLFILES88)
	ctags -w ${TAGFILES}
	@if [ -d sim ] ;\
    	then \
		cd sim ; make gdbtags;\
	fi ;

lint: $(CFILES) $(DEPFILES88) $(PINSNS88) init.c
	lint $(HACKFLAGS) -I. $(CFILES) $(DEPFILES) $(PINSNS88)  init.c >lint

gdb.tar: ${TARFILES}
	rm -f gdb.tar
	mkdir dist-gdb
	cd dist-gdb ; for i in ${TARFILES} ; do ln -s ../$$i . ; done
	tar chf gdb.tar dist-gdb
	rm -rf dist-gdb

gdb.tar.Z: gdb.tar
	compress gdb.tar

clean:
	rm -f ${OBS} ${TSOBS} ${NTSOBS} ${OBSTACK} ${REGEX} ${GNU_MALLOC}
	rm -f init.c  init.o norcsid.o
	rm -f g88 core gdb.tar gdb.tar.Z make.log
	rm -f gdb[0-9]
	cd ${READLINE} ; make clean
	if [ -d sim ] ;\
	then cd sim ; make clean ;\
	fi

exterminate: clean realclean 
	rm -f ${TAGFILES} munch copying.awk COPYING Makefile
	rm -f config.gdb config.status $(MFILES88) $(OPCODES88) $(HFILES)
	rm -f norcsid.c
	cd ${READLINE} ; make exterminate
	if [ -d sim ] ;\
	then cd sim ; make exterminate ;\
	fi

#
# do tek Xgdb build
#
autobuild:
	make Xexterminate
	make Xco-all
	make Xrelease

install:
	@echo "run install.sh"
	
#
# remove X objects
#
Xclean: clean
	cd XOBJ; make clean

#
# remove X objects/src in preparation for build
#
Xexterminate: exterminate
	cd XOBJ; make exterminate

distclean: clean expread.tab.c TAGS
	rm -f dep.c opcode.h param.h pinsn.c config.status
	rm -f y.output yacc.acts yacc.tmp
	rm -f ${TESTS}

realclean: clean
	rm -f expread.tab.c TAGS
	rm -f dep.c opcode.h param.h pinsn.c config.status

#
# run lint on gdb source
#
lint_msgs: ${SFILES} ${DEPFILES} ${PINSNS} init.c
	rm -f lint_msgs
	lint ${CFLAGS} ${SFILES} ${DEPFILES} ${PINSNS} > lint_msgs

xgdb.o : defs.h param.h symtab.h frame.h

# Make copying.c from COPYING
copying.c : COPYING copying.awk
	awk -f copying.awk < COPYING > copying.c

expread.tab.c : expread.y
	@echo 'Expect 4 shift/reduce conflict.'
	${YACC} expread.y
	mv y.tab.c expread.tab.c

expread.o : expread.tab.c defs.h param.h symtab.h frame.h expression.h
	$(CC) -c ${CFLAGS} expread.tab.c
	mv expread.tab.o expread.o

# readline.o: readline.c readline.h history.h keymaps.c funmap.c
#	${CC} -c ${CFLAGS} ${SYSV_DEFINE} readline.c

# history.o: history.c history.h general.h 
#	${CC} -c ${CFLAGS} ${SYSV_DEFINE} history.c

 readline/libreadline.a : force_update
	cd readline ;\
	  make ${MFLAGS} "SYSV_DEFINE=${SYSV_DEFINE}" "HACKFLAGS=${HACKFLAGS}" libreadline.a

# sim.o contains either a stub of a simulator or the real thing.  If
# there is a directory "sim" we cd into it and make whatever is there.
# We make a symbolic link to sim.o in sim.
# Otherwise, if a directory "sim" doesn't exist, we compile the sub file
# "sim.c".

sim.o: FRC
	@if [ -d sim ] ;\
    	then \
		rm -f sim.o;\
		ln -s sim/sim.o;\
		cd sim ; make sim.o;\
	else \
		cc -c sim.c;\
	fi ;

force_update :

# Just here for dependencies; RH's are included in LH's
keymaps.c: emacs_keymap.c vi_keymap.c
funmap.c: readline.h

# Only useful if you are using the gnu malloc routines.
#
malloc.o : malloc.c
	${CC} -c ${MALLOC_FLAGS} malloc.c

# dep.o depends on config.status in case someone reconfigures gdb out
# from under an already compiled gdb.
dep.o : dep.c config.status defs.h param.h frame.h inferior.h obstack.h \
	a.out.encap.h a.out.gnu.h

# dep.o depends on config.status in case someone reconfigures gdb out
# from under an already compiled gdb.
pinsn.o : pinsn.c config.status defs.h param.h symtab.h obstack.h symseg.h \
	  frame.h

#
# The rest of this is a standard dependencies list (hand edited output of
# cpp -M).  It does not include dependencies of .o files on .c files.
#
blockframe.o : defs.h param.h symtab.h obstack.h symseg.h frame.h 
breakpoint.o : defs.h param.h symtab.h obstack.h symseg.h frame.h
coffread.o : defs.h param.h symtab.h
command.o : command.h defs.h
core.o : defs.h  param.h a.out.encap.h a.out.gnu.h
dbxread.o : param.h defs.h symtab.h obstack.h symseg.h a.out.encap.h \
	    stab.gnu.h a.out.gnu.h
decode.o: decode.h format.h
environ.o : environ.h 
eval.o : defs.h  param.h symtab.h obstack.h symseg.h value.h expression.h 
expprint.o : defs.h symtab.h obstack.h symseg.h param.h expression.h
findvar.o : defs.h inferior.h param.h symtab.h obstack.h symseg.h frame.h \
	    value.h 
infcmd.o : defs.h  param.h symtab.h obstack.h symseg.h frame.h inferior.h \
	   environ.h value.h
inflow.o : defs.h  param.h frame.h inferior.h
infrun.o : defs.h  param.h symtab.h obstack.h symseg.h frame.h inferior.h \
	   wait.h
kdb-start.o : defs.h param.h 
main.o : defs.h  command.h param.h
malloc.o :  getpagesize.h
obstack.o : obstack.h 
printcmd.o :  defs.h param.h frame.h symtab.h obstack.h symseg.h value.h \
	      expression.h 
regex.o : regex.h 
remote.o : defs.h  param.h frame.h inferior.h wait.h remote.h hmon/mon.h
remcmd.o : defs.h  param.h remote.h 
remmem.o : defs.h  param.h remote.h
remrun.o : defs.h  param.h remote.h
motomode.o : defs.h  param.h frame.h inferior.h wait.h remote.h
source.o : defs.h  symtab.h obstack.h symseg.h param.h
stack.o :  defs.h param.h symtab.h obstack.h symseg.h frame.h 
standalone.o : defs.h param.h symtab.h obstack.h symseg.h frame.h \
	       inferior.h wait.h 
symmisc.o : defs.h symtab.h obstack.h symseg.h obstack.h 
symtab.o : defs.h  symtab.h obstack.h symseg.h param.h  obstack.h
utils.o : defs.h  param.h 
valarith.o : defs.h param.h symtab.h obstack.h symseg.h value.h expression.h 
valops.o :  defs.h param.h symtab.h obstack.h symseg.h value.h frame.h \
	    inferior.h
valprint.o :  defs.h param.h symtab.h obstack.h symseg.h value.h 
values.o :  defs.h param.h symtab.h obstack.h symseg.h value.h 

robotussin.h : getpagesize.h   
symtab.h : obstack.h symseg.h 
a.out.encap.h : a.out.gnu.h

