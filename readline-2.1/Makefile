# Generated automatically from Makefile.in by configure.
## -*- text -*- ##
# Master Makefile for the GNU readline library.
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
RL_LIBRARY_VERSION = 2.1
RL_LIBRARY_NAME = readline

srcdir = .
VPATH = .:.
top_srcdir = .
BUILD_DIR = .

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

CC = i586-linux-gcc
LD = i586-linux-ld			# needed when building shared libraries
RANLIB = i586-linux-ranlib
AR = i586-linux-ar
RM = rm -f
CP = cp
MV = mv

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
mandir = ${prefix}/man
includedir = ${prefix}/include

infodir = ${prefix}/info

man3dir = $(mandir)/man3

SHELL = /bin/sh

# Programs to make tags files.
ETAGS = etags -tw
CTAGS = ctags -tw

CFLAGS = -g -O
LOCAL_CFLAGS =  -DRL_LIBRARY_VERSION='"$(RL_LIBRARY_VERSION)"'
CPPFLAGS = 
LDFLAGS =  @LOCAL_LDFLAGS@ -g -O

DEFS = -DHAVE_CONFIG_H
LOCAL_DEFS = 

# For libraries which include headers from other libraries.
INCLUDES = -I. -I$(srcdir) -I$(includedir)

CCFLAGS = $(DEFS) $(LOCAL_DEFS) $(CPPFLAGS) $(INCLUDES) $(LOCAL_CFLAGS) $(CFLAGS)

# these two options need tweaking for compiler/OS versions other than gcc
# and SunOS4
PICFLAG=        -fpic    # -pic for some versions of cc
SHLIB_OPTS=	-assert pure-text -ldl	# -Bshareable for some versions of gcc

MAJOR=		3
# shared library systems like SVR4's do not use minor versions
MINOR=		.0

.SUFFIXES:	.so

.c.o:
	$(CC) -c $(CCFLAGS) $<

.c.so:
	-mv $*.o z$*.o
	$(CC) -c $(PICFLAG) $(CCFLAGS) $< 
	mv $*.o $@
	-mv z$*.o $*.o

# The name of the main library target.
LIBRARY_NAME = libreadline.a
SHARED_READLINE = libreadline.so.$(MAJOR)$(MINOR)
SHARED_HISTORY = libhistory.so.$(MAJOR)$(MINOR)
SHARED_LIBS = $(SHARED_READLINE) $(SHARED_HISTORY)

# The C code source files for this library.
CSOURCES = $(srcdir)/readline.c $(srcdir)/funmap.c $(srcdir)/keymaps.c \
	   $(srcdir)/vi_mode.c $(srcdir)/parens.c $(srcdir)/rltty.c \
	   $(srcdir)/complete.c $(srcdir)/bind.c $(srcdir)/isearch.c \
	   $(srcdir)/display.c $(srcdir)/signals.c $(srcdir)/emacs_keymap.c \
	   $(srcdir)/vi_keymap.c $(srcdir)/util.c $(srcdir)/kill.c \
	   $(srcdir)/undo.c $(srcdir)/macro.c $(srcdir)/input.c \
	   $(srcdir)/callback.c $(srcdir)/terminal.c $(srcdir)/xmalloc.c \
	   $(srcdir)/history.c $(srcdir)/histsearch.c $(srcdir)/histexpand.c \
	   $(srcdir)/histfile.c $(srcdir)/nls.c $(srcdir)/search.c \
	   $(srcdir)/shell.c $(srcdir)/tilde.c

# The header files for this library.
HSOURCES = readline.h rldefs.h chardefs.h keymaps.h history.h histlib.h \
	   posixstat.h posixdir.h posixjmp.h tilde.h rlconf.h rltty.h \
	   ansi_stdlib.h tcap.h

HISTOBJ = history.o histexpand.o histfile.o histsearch.o shell.o
TILDEOBJ = tilde.o
OBJECTS = readline.o vi_mode.o funmap.o keymaps.o parens.o search.o \
	  rltty.o complete.o bind.o isearch.o display.o signals.o \
	  util.o kill.o undo.o macro.o input.o callback.o terminal.o \
	  nls.o xmalloc.o $(HISTOBJ) $(TILDEOBJ)

SHARED_HISTOBJ = history.so histexpand.so histfile.so histsearch.so shell.so
SHARED_TILDEOBJ = tilde.so
SHARED_OBJ = readline.so vi_mode.so funmap.so keymaps.so parens.so search.so \
	  rltty.so complete.so bind.so isearch.so display.so signals.so \
	  util.so kill.so undo.so macro.so input.so callback.so terminal.so \
	  nls.so xmalloc.so $(SHARED_HISTOBJ) $(SHARED_TILDEOBJ)

# The texinfo files which document this library.
DOCSOURCE = doc/rlman.texinfo doc/rltech.texinfo doc/rluser.texinfo
DOCOBJECT = doc/readline.dvi
DOCSUPPORT = doc/Makefile
DOCUMENTATION = $(DOCSOURCE) $(DOCOBJECT) $(DOCSUPPORT)

CREATED_MAKEFILES = Makefile doc/Makefile examples/Makefile

INSTALLED_HEADERS = readline.h chardefs.h keymaps.h history.h tilde.h

##########################################################################

all: libreadline.a libhistory.a
shared: $(SHARED_LIBS)

libreadline.a: $(OBJECTS)
	$(RM) $@
	$(AR) cr $@ $(OBJECTS)
	-test -n "$(RANLIB)" && $(RANLIB) $@

libhistory.a: $(HISTOBJ) xmalloc.o
	$(RM) $@
	$(AR) cr $@ $(HISTOBJ) xmalloc.o
	-test -n "$(RANLIB)" && $(RANLIB) $@

$(SHARED_READLINE):	$(SHARED_OBJ)
	$(RM) $@
	$(LD) ${SHLIB_OPTS} -o $@ $(SHARED_OBJ)

$(SHARED_HISTORY):	$(SHARED_HISTOBJ) xmalloc.so
	$(RM) $@
	$(LD) ${SHLIB_OPTS} -o $@ $(SHARED_HISTOBJ) xmalloc.so

readline: $(OBJECTS) readline.h rldefs.h chardefs.h
	$(CC) $(CCFLAGS) -o $@ ./examples/rl.c ./libreadline.a -ltermcap

Makefile makefile: config.status $(srcdir)/Makefile.in
	CONFIG_FILES=Makefile CONFIG_HEADERS= $(SHELL) ./config.status

Makefiles makefiles: config.status $(srcdir)/Makefile.in
	@for mf in $(CREATED_MAKEFILES); do \
		CONFIG_FILES=$$mf CONFIG_HEADERS= $(SHELL) ./config.status ; \
	done

config.status: configure
	$(SHELL) ./config.status --recheck

config.h:	stamp-h

stamp-h: config.status $(srcdir)/config.h.in
	CONFIG_FILES= CONFIG_HEADERS=config.h ./config.status
	echo > $@

$(srcdir)/configure: $(srcdir)/configure.in	## Comment-me-out in distribution
	cd $(srcdir) && autoconf	## Comment-me-out in distribution

documentation: force
	-test -d doc || mkdir doc
	-( cd doc && $(MAKE) $(MFLAGS) )

force:

install: installdirs libreadline.a
	for f in ${INSTALLED_HEADERS}; do \
		$(INSTALL_DATA) $(srcdir)/$$f $(includedir)/readline ; \
	done
	-$(MV) $(libdir)/libreadline.a $(libdir)/libreadline.old
	$(INSTALL_DATA) libreadline.a $(libdir)/libreadline.a
	-test -n "$(RANLIB)" && -$(RANLIB) -t $(libdir)/libreadline.a
	-( if test -d doc ; then \
		cd doc && \
		${MAKE} ${MFLAGS} infodir=$(infodir) INSTALL_DATA=$(INSTALL_DATA) $@; \
	  fi )

installdirs: $(srcdir)/support/mkdirs
	-$(SHELL) $(srcdir)/support/mkdirs $(includedir) \
		$(includedir)/readline $(libdir) $(infodir) $(man3dir)

uninstall:
	-test -n "$(includedir)" && cd $(includedir)/readline && \
		${RM} ${INSTALLED_HEADERS}
	-test -n "$(libdir)" && cd $(libdir) && \
		${RM} libreadline.a libreadline.old $(SHARED_LIBS)

TAGS:	force
	$(ETAGS) $(CSOURCES) $(HSOURCES)

tags:	force
	$(CTAGS) $(CSOURCES) $(HSOURCES)

clean:	force
	$(RM) $(OBJECTS) *.a
	$(RM) $(SHARED_OBJ) $(SHARED_LIBS)
	-( cd doc && $(MAKE) $(MFLAGS) $@ )

mostlyclean: clean
	-( cd doc && $(MAKE) $(MFLAGS) $@ )

distclean maintainer-clean: clean
	-( cd doc && $(MAKE) $(MFLAGS) $@ )
	$(RM) Makefile
	$(RM) config.status config.h config.cache config.log 
	$(RM) stamp-config stamp-h
	$(RM) TAGS tags

info dvi:
	-( cd doc && $(MAKE) $(MFLAGS) $@ )

install-info:
check:
installcheck:

dist:   force
	@echo Readline distributions are created using $(srcdir)/support/mkdist.
	@echo Here is a sample of the necessary commands:
	@echo bash $(srcdir)/support/mkdist -m $(srcdir)/MANIFEST -s $(srcdir) -r $(RL_LIBRARY_NAME)-$(RL_LIBRARY_VERSION)
	@echo tar cf $(RL_LIBRARY_NAME)-${RL_LIBRARY_VERSION}.tar ${RL_LIBRARY_NAME}-$(RL_LIBRARY_VERSION)
	@echo gzip $(RL_LIBRARY_NAME)-$(RL_LIBRARY_VERSION).tar

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

# Dependencies
bind.o: ansi_stdlib.h posixstat.h
bind.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
bind.o: readline.h keymaps.h chardefs.h tilde.h
bind.o: history.h
callback.o: rlconf.h
callback.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
callback.o: readline.h keymaps.h chardefs.h tilde.h
complete.o: ansi_stdlib.h posixdir.h posixstat.h
complete.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
complete.o: readline.h keymaps.h chardefs.h tilde.h
display.o: ansi_stdlib.h posixstat.h
display.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
display.o: tcap.h
display.o: readline.h keymaps.h chardefs.h tilde.h
display.o: history.h
funmap.o: readline.h keymaps.h chardefs.h tilde.h
funmap.o: rlconf.h ansi_stdlib.h
funmap.o: ${BUILD_DIR}/config.h
histexpand.o: ansi_stdlib.h
histexpand.o: history.h histlib.h
histexpand.o: ${BUILD_DIR}/config.h
histfile.o: ansi_stdlib.h
histfile.o: history.h histlib.h
histfile.o: ${BUILD_DIR}/config.h
history.o: ansi_stdlib.h
history.o: history.h histlib.h
history.o: ${BUILD_DIR}/config.h
histsearch.o: ansi_stdlib.h
histsearch.o: history.h histlib.h
histsearch.o: ${BUILD_DIR}/config.h
input.o: ansi_stdlib.h
input.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
input.o: readline.h keymaps.h chardefs.h tilde.h
isearch.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
isearch.o: readline.h keymaps.h chardefs.h tilde.h
isearch.o: ansi_stdlib.h history.h
keymaps.o: emacs_keymap.c vi_keymap.c
keymaps.o: keymaps.h chardefs.h rlconf.h ansi_stdlib.h
keymaps.o: readline.h keymaps.h chardefs.h tilde.h
keymaps.o: ${BUILD_DIR}/config.h
kill.o: ansi_stdlib.h
kill.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
kill.o: readline.h keymaps.h chardefs.h tilde.h
kill.o: history.h
macro.o: ansi_stdlib.h
macro.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
macro.o: readline.h keymaps.h chardefs.h tilde.h
macro.o: history.h
nls.o: ansi_stdlib.h
nls.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
parens.o: rlconf.h
parens.o: ${BUILD_DIR}/config.h
parens.o: readline.h keymaps.h chardefs.h tilde.h
readline.o: readline.h keymaps.h chardefs.h tilde.h
readline.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
readline.o: history.h
readline.o: posixstat.h ansi_stdlib.h posixjmp.h
rltty.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
rltty.o: rltty.h
rltty.o: readline.h keymaps.h chardefs.h tilde.h
search.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
search.o: readline.h keymaps.h chardefs.h tilde.h
search.o: ansi_stdlib.h history.h
signals.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
signals.o: readline.h keymaps.h chardefs.h tilde.h
signals.o: history.h
terminal.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
terminal.o: tcap.h
terminal.o: readline.h keymaps.h chardefs.h tilde.h
terminal.o: history.h
tilde.o: ansi_stdlib.h
tilde.o: ${BUILD_DIR}/config.h
tilde.o: tilde.h
undo.o: ansi_stdlib.h
undo.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
undo.o: readline.h keymaps.h chardefs.h tilde.h
undo.o: history.h
util.o: posixjmp.h ansi_stdlib.h
util.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
util.o: readline.h keymaps.h chardefs.h tilde.h
vi_mode.o: rldefs.h ${BUILD_DIR}/config.h rlconf.h
vi_mode.o: readline.h keymaps.h chardefs.h tilde.h
vi_mode.o: history.h ansi_stdlib.h
xmalloc.o: ${BUILD_DIR}/config.h
xmalloc.o: ansi_stdlib.h

bind.so: ansi_stdlib.h posixstat.h
bind.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
bind.so: readline.h keymaps.h chardefs.h tilde.h
bind.so: history.h
callback.so: rlconf.h
callback.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
callback.so: readline.h keymaps.h chardefs.h tilde.h
complete.so: ansi_stdlib.h posixdir.h posixstat.h
complete.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
complete.so: readline.h keymaps.h chardefs.h tilde.h
display.so: ansi_stdlib.h posixstat.h
display.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
display.so: tcap.h
display.so: readline.h keymaps.h chardefs.h tilde.h
display.so: history.h
funmap.so: readline.h keymaps.h chardefs.h tilde.h
funmap.so: rlconf.h ansi_stdlib.h
funmap.so: ${BUILD_DIR}/config.h
histexpand.so: ansi_stdlib.h
histexpand.so: history.h histlib.h
histexpand.so: ${BUILD_DIR}/config.h
histfile.so: ansi_stdlib.h
histfile.so: history.h histlib.h
histfile.so: ${BUILD_DIR}/config.h
history.so: ansi_stdlib.h
history.so: history.h histlib.h
history.so: ${BUILD_DIR}/config.h
histsearch.so: ansi_stdlib.h
histsearch.so: history.h histlib.h
histsearch.so: ${BUILD_DIR}/config.h
input.so: ansi_stdlib.h
input.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
input.so: readline.h keymaps.h chardefs.h tilde.h
isearch.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
isearch.so: readline.h keymaps.h chardefs.h tilde.h
isearch.so: ansi_stdlib.h history.h
keymaps.so: emacs_keymap.c vi_keymap.c
keymaps.so: keymaps.h chardefs.h rlconf.h ansi_stdlib.h
keymaps.so: readline.h keymaps.h chardefs.h tilde.h
keymaps.so: ${BUILD_DIR}/config.h
kill.so: ansi_stdlib.h
kill.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
kill.so: readline.h keymaps.h chardefs.h tilde.h
kill.so: history.h
macro.so: ansi_stdlib.h
macro.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
macro.so: readline.h keymaps.h chardefs.h tilde.h
macro.so: history.h
nls.so: ansi_stdlib.h
nls.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
parens.so: rlconf.h
parens.so: ${BUILD_DIR}/config.h
parens.so: readline.h keymaps.h chardefs.h tilde.h
readline.so: readline.h keymaps.h chardefs.h tilde.h
readline.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
readline.so: history.h
readline.so: posixstat.h ansi_stdlib.h posixjmp.h
rltty.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
rltty.so: rltty.h
rltty.so: readline.h keymaps.h chardefs.h tilde.h
search.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
search.so: readline.h keymaps.h chardefs.h tilde.h
search.so: ansi_stdlib.h history.h
signals.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
signals.so: readline.h keymaps.h chardefs.h tilde.h
signals.so: history.h
terminal.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
terminal.so: tcap.h
terminal.so: readline.h keymaps.h chardefs.h tilde.h
terminal.so: history.h
tilde.so: ansi_stdlib.h
tilde.so: ${BUILD_DIR}/config.h
tilde.so: tilde.h
undo.so: ansi_stdlib.h
undo.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
undo.so: readline.h keymaps.h chardefs.h tilde.h
undo.so: history.h
util.so: posixjmp.h ansi_stdlib.h
util.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
util.so: readline.h keymaps.h chardefs.h tilde.h
vi_mode.so: rldefs.h ${BUILD_DIR}/config.h rlconf.h
vi_mode.so: readline.h keymaps.h chardefs.h tilde.h
vi_mode.so: history.h ansi_stdlib.h
xmalloc.so: ${BUILD_DIR}/config.h
xmalloc.so: ansi_stdlib.h

readline.so: $(srcdir)/readline.c
vi_mode.so: $(srcdir)/vi_mode.c
funmap.so: $(srcdir)/funmap.c
keymaps.so: $(srcdir)/keymaps.c
parens.so: $(srcdir)/parens.c
search.so: $(srcdir)/search.c
rltty.so: $(srcdir)/rltty.c
complete.so: $(srcdir)/complete.c
bind.so: $(srcdir)/bind.c
isearch.so: $(srcdir)/isearch.c
display.so: $(srcdir)/display.c
signals.so: $(srcdir)/signals.c
util.so: $(srcdir)/util.c
kill.so: $(srcdir)/kill.c
undo.so: $(srcdir)/undo.c
macro.so: $(srcdir)/macro.c
input.so: $(srcdir)/input.c
callback.so: $(srcdir)/callback.c
terminal.so: $(srcdir)/terminal.c
nls.so: $(srcdir)/nls.c
xmalloc.so: $(srcdir)/xmalloc.c
history.so: $(srcdir)/history.c
histexpand.so: $(srcdir)/histexpand.c
histfile.so: $(srcdir)/histfile.c
histsearch.so: $(srcdir)/histsearch.c
shell.so: $(srcdir)/shell.c
tilde.so: $(srcdir)/tilde.c
