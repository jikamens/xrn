bin_PROGRAMS = xrn
xrn_CFLAGS = $(XRN_CFLAGS)
xrn_LDADD = $(XRN_LIBS)

xrn_SOURCES = avl.c compose.c cursor.c dialogs.c \
       refile.c mesg.c error_hnds.c newsrcfile.c resources.c    \
       internals.c save.c server.c utils.c xrn.c \
       xthelper.c cancel.c vprintf.c tempnam.c strstr.c \
       strtok.c mesg_strings.c XrnText.c ngMode.c artMode.c allMode.c \
       addMode.c snapshot.c artstruct.c Buttons.c InfoLine.c ButtonBox.c \
       busyCursor.c varfile.c activecache.c sort.c hash.c getdate.y \
       killfile.c InfoDialog.c Frame.c Xmisc.c file_cache.c rcyacc.y rclex.l \
	ButtonBox.h COMMON-PROBLMS COPYRIGHT CREDITS Frame.h InfoDialog.h \
	InfoLine.h Text.h XRn.src Xresources.sam activecache.h addMode.h \
	allMode.h artMode.h artstruct.h avl.h busyCursor.h butdefs.h \
	butexpl.h buttons.h cache.h cancel.h clientlib.h codes.h compose.h \
	config.h copyright.h cursor.h dialogs.h error_hnds.h file_cache.h \
	hash.h internals.h killfile.h mesg.h mesg_str.awk modes.h news.h \
	newsrcfile.h ngMode.h refile.h resources.h save.h server.h snapshot.h \
	sort.h utils.h varfile.h xmisc.h xrn-man.src xrn-man.sym xrn.h \
	xthelper.h xrn.icon

xrn_SOURCES += clientlib.c

EXTRA_DIST = xrn.spec
nodist_xrn_SOURCES = mesg_strings.h
BUILT_SOURCES = mesg_strings.h rcyacc.h xrn.spec
AM_YFLAGS = -d
CLEANFILES = mesg_strings.h rclex.c rcyacc.c rcyacc.h getdate.c getdate.h \
	XRn.ad XRn xrn-man.c xrn-man.cpp xrn-man.sed xrn.man

mesg_strings.h: mesg_strings.c mesg_str.awk
	-rm -f mesg_str.tmp
	$(AWK) -f $(srcdir)/mesg_str.awk $(srcdir)/mesg_strings.c > mesg_str.tmp
	@set -x; if cmp -s mesg_str.tmp mesg_strings.h; then \
		rm -f mesg_str.tmp; \
	else \
		rm -f mesg_strings.h; \
		mv mesg_str.tmp mesg_strings.h; \
	fi

appdefaultdir = $(sysconfdir)/X11/app-defaults
DEFS += -DXRNAPPDIR=\"$(appdefaultdir)\"

appdefault_DATA = XRn

% : %.ad
	cp $< $@

TKSED=	-e 's/XAW//' -e '/MOTIF/d'
TKSED2=	-e 's/XawSet()/set()/' -e 's/XawNotify()/notify()/' \
	-e 's/XawUnSet()/unset()/'

XRn.ad: XRn.src
	-rm -f XRn.tmp XRn.ad
	sed -e 's/LANG_english//' -e '/LANG_/d' $(TKSED) $(TKSED2) \
		-e 's/xAPP_CLASSx/XRn/' \
		-e 's/xPACKAGE_VERSIONx/$(PACKAGE_VERSION)/' $< > XRn.tmp
	mv XRn.tmp XRn.ad

xrn-man.c: xrn-man.sym config.h
	-rm -f xrn-man.tmp xrn-man.c
	$(AWK) 'BEGIN {printf("#include \"config.h\"\n");} {printf("#ifdef %s\nx%sx value %s value\n#else\nx%sx\n#endif\n",$$0,$$0,$$0,$$0);}' \
		$(srcdir)/xrn-man.sym > xrn-man.tmp
	mv xrn-man.tmp xrn-man.c

xrn-man.cpp: xrn-man.c
	-rm -f xrn-man.tmp xrn-man.cpp
	$(CPP) $(DEFAULT_INCLUDES) $(DEFS) xrn-man.c > xrn-man.tmp
	mv xrn-man.tmp xrn-man.cpp

xrn-man.sed: xrn-man.cpp
	-rm -f xrn-man.tmp xrn-man.sed
	sed -e 's|^x\(.*\)x$$|/x\1_STARTx/,/x\1_ENDx/d|' \
	    -e 's|^x\(.*\)x value [ 	]*"\(.*\)"[ 	]* value.*|s,x\1x,\2,|' \
	    -e 's|^x\(.*\)x value [ 	]*\([^ 	].*[^ 	]\)[ 	]* value.*|s,x\1x,\2,|' \
	    -e 's|^x\(.*\)x value [ 	]*\([^ 	]\)[ 	]* value.*|s,x\1x,\2,|' \
	    -e 's|^x\(.*\)x.*||' \
	    -e '/^#/d' \
		xrn-man.cpp > xrn-man.tmp
	echo '/^x.*_STARTx/d' >> xrn-man.tmp
	echo '/^x.*_ENDx/d' >> xrn-man.tmp
	mv xrn-man.tmp xrn-man.sed

man1_MANS = xrn.man

xrn.man:: xrn-man.src xrn-man.sed
	-rm -f xrn-man.tmp xrn.man
	sed -f xrn-man.sed $(srcdir)/xrn-man.src > xrn-man.tmp
	mv xrn-man.tmp xrn.man

xrn.spec:: xrn.spec.in
	rm -f $@.tmp $@
	sed -e 's/xPACKAGE_VERSIONx/$(PACKAGE_VERSION)/' $< > $@.tmp
	mv $@.tmp $@
