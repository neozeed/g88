# Generated automatically from Makefile.in by configure.
# This makefile for Readline library documentation is in -*- text -*- mode.
# Emacs likes it that way.
top_srcdir = ..
srcdir = .

prefix = /usr/local
infodir = ${prefix}/info

mandir = ${prefix}/man
man3dir = $(mandir)/man3

RM = rm -f

TEXINPUTDIR = $(srcdir)

MAKEINFO    = makeinfo
TEXI2DVI    = $(srcdir)/texi2dvi
TEXI2HTML   = $(srcdir)/texi2html
QUIETPS     = #set this to -q to shut up dvips
DVIPS       = dvips -D 300 $(QUIETPS) -o $@     # tricky

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

RLSRC = $(srcdir)/rlman.texinfo $(srcdir)/rluser.texinfo \
	$(srcdir)/rltech.texinfo
HISTSRC = $(srcdir)/hist.texinfo $(srcdir)/hsuser.texinfo \
	  $(srcdir)/hstech.texinfo

# This should be a program that converts troff to an ascii-readable format
NROFF       = groff -Tascii

# This should be a program that converts troff to postscript
GROFF       = groff

DVIOBJ = readline.dvi history.dvi
INFOOBJ = readline.info history.info
PSOBJ = readline.ps history.ps
HTMLOBJ = readline.html history.html
TEXTOBJ = readline.0

CREATED_DOCS = $(DVIOBJ) $(INFOOBJ) $(PSOBJ) $(HTMLOBJ) $(TEXTOBJ)

.SUFFIXES:      .0 .3 .ps .txt .dvi

.3.0:
	$(RM) $@
	-${NROFF} -man $< > $@

all: info dvi html ps text
nodvi: info html text

readline.dvi: $(RLSRC)
	TEXINPUTS=.:$(TEXINPUTDIR):$$TEXINPUTS $(TEXI2DVI) $(srcdir)/rlman.texinfo
	mv rlman.dvi readline.dvi

readline.info: $(RLSRC)
	$(MAKEINFO) --no-split -I $(TEXINPUTDIR) -o $@ $(srcdir)/rlman.texinfo

history.dvi: ${HISTSRC}
	TEXINPUTS=.:$(TEXINPUTDIR):$$TEXINPUTS $(TEXI2DVI) $(srcdir)/hist.texinfo
	mv hist.dvi history.dvi

history.info: ${HISTSRC}
	$(MAKEINFO) --no-split -I $(TEXINPUTDIR) -o $@ $(srcdir)/hist.texinfo

readline.ps:	readline.dvi
	$(RM) $@
	$(DVIPS) readline.dvi

history.ps:	history.dvi
	$(RM) $@
	$(DVIPS) history.dvi

readline.html:	${RLSRC}
	$(TEXI2HTML) -I $(TEXINPUTDIR) $(srcdir)/rlman.texinfo
	sed -e 's:rlman.html:readline.html:' -e 's:rlman_toc.html:readline_toc.html:' rlman.html > readline.html
	sed -e 's:rlman.html:readline.html:' -e 's:rlman_toc.html:readline_toc.html:' rlman_toc.html > readline_toc.html
	$(RM) rlman.html rlman_toc.html

history.html:	${HISTSRC}
	$(TEXI2HTML) -I $(TEXINPUTDIR) $(srcdir)/hist.texinfo
	sed -e 's:hist.html:history.html:' -e 's:hist_toc.html:history_toc.html:' hist.html > history.html
	sed -e 's:hist.html:history.html:' -e 's:hist_toc.html:history_toc.html:' hist_toc.html > history_toc.html
	$(RM) hist.html hist_toc.html

info:	$(INFOOBJ)
dvi:	$(DVIOBJ)
ps:	$(PSOBJ)
html:	$(HTMLOBJ)
text:	$(TEXTOBJ)

readline.0: $(srcdir)/readline.3

clean:
	$(RM) *.aux *.cp *.fn *.ky *.log *.pg *.toc *.tp *.vr *.cps *.pgs \
	      *.fns *.kys *.tps *.vrs *.o core

distclean: clean
	$(RM) $(CREATED_DOCS)

mostlyclean: clean

maintainer-clean: clean
	$(RM) $(CREATED_DOCS)

installdirs:	$(top_srcdir)/support/mkdirs
	-$(SHELL) $(top_srcdir)/support/mkdirs $(infodir) $(man3dir)

install:	installdirs info
	${INSTALL_DATA} readline.info $(infodir)/readline.info
	${INSTALL_DATA} history.info $(infodir)/history.info
	if $(SHELL) -c 'install-info --version' >/dev/null 2>&1; then \
		install-info --dir-file=$(infodir)/dir $(infodir)/readline.info ; \
		install-info --dir-file=$(infodir)/dir $(infodir)/history.info ; \
	else true; fi
	-${INSTALL_DATA} $(srcdir)/readline.3 $(man3dir)/readline.3

uninstall:
	$(RM) $(infodir)/readline.info
	$(RM) $(infodir)/history.info
	$(RM) $(man3dir)/readline.3
