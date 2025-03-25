#TEK $Header: /am/bigbird/home/bigbird/Usr.U6/robertb/m88k/src/g88/readline/RCS/Makefile,v 1.9 90/08/19 21:50:30 robertb Exp $
#TEK $Locker:  $
## -*- text -*- ####################################################
#								   #
# Makefile for readline and history libraries.			   #
#								   #
####################################################################

# Here is a rule for making .o files from .c files that doesn't force
# the type of the machine (like -sun3) into the flags.
.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

# Destination installation directory.  The libraries are copied here if
# you do a `make install'.  The directory name does NOT end in a slash.
DESTDIR = /usr/gnu/lib

# This is expected to be overruled from outside this Makefile on SYSV 
# Systems.
SYSV_DEFINE =

# GCC_SUNOS4_FLAG = -Bstatic
DEBUG_FLAGS = -g
# LDFLAGS = $(DEBUG_FLAGS) 
#TEK CFLAGS = $(DEBUG_FLAGS)
CFLAGS = $(DEBUG_FLAGS) $(SYSV_DEFINE) $(HACKFLAGS) -DBSD

# If you don't have Gcc use cc.
# CC = cc
CC = gcc -g
AR = ar
RANLIB = /usr/bin/ranlib
#RANLIB = ar qs
RM = rm

CSOURCES = readline.c history.c funmap.c emacs_keymap.c vi_keymap.c keymaps.c
HSOURCES = readline.h history.h
SOURCES  = $(CSOURCES) $(HSOURCES)

DOCUMENTATION = readline.texinfo inc-readline.texinfo \
		history.texinfo inc-history.texinfo

SUPPORT = COPYING Makefile $(DOCUMENTATION) ChangeLog

THINGS_TO_TAR = $(SOURCES) $(SUPPORT)

##########################################################################

all: libreadline.a

libreadline.a : history.o readline.o 
	$(RM) -f libreadline.a
	$(AR) clq libreadline.a readline.o history.o
	$(RANLIB) libreadline.a

readline.o:	readline.h readline.c keymaps.c funmap.c history.h
history.o:	history.c history.h

keymaps.c:	emacs_keymap.c vi_keymap.c
			touch keymaps.c

libtest:	libreadline.a libtest.c
		$(CC) -o libtest $(CFLAGS) -I. -L. libtest.c libreadline.a -ltermcap

readline: readline.c history.c keymaps.c funmap.c readline.h libhistory.a
	$(CC) $(CFLAGS)  -DTEST -o readline readline.c $(TERMCAP) libhistory.a

readline.tar:	$(THINGS_TO_TAR)
		tar cf readline.tar $(THINGS_TO_TAR)

readline.tar.Z:	readline.tar
		compress -f readline.tar

install:	$(DESTDIR)/libreadline.a

$(DESTDIR)/libreadline.a: libreadline.a
		-mv $(DESTDIR)/libreadline.a $(DESTDIR)/libreadline.old
		cp libreadline.a $(DESTDIR)/libreadline.a
		$(RANLIB) -t $(DESTDIR)/libreadline.a

clean:
		rm -f readline.o history.o libreadline.a

exterminate:  clean
		rm -f ${SOURCES} Makefile

co-all:
		colast ${SOURCES}
