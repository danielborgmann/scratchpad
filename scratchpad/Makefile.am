bin_PROGRAMS = scratchpad

INCLUDES = \
	$(SCRATCHPAD_CFLAGS) 					\
	-I../libguniqueapp						\
	-DVERSION=\""$(VERSION)"\"				\
	-DDATADIR=\""$(datadir)"\"				\
	-DSCRATCHPADDIR=\""$(pkgdatadir)"\"		\
	$(NULL)
	
BUILT_SOURCES = scratchpad.stamp

scratchpad_SOURCES = \
	application.c \
	application.h \
	application.vala \
	basewindow.c \
	basewindow.h \
	basewindow.vala \
	bufferlist.c \
	bufferlist.h \
	bufferlist.vala \
	document.c \
	document.h \
	document.vala \
	documentview.c \
	documentview.h \
	documentview.vala \
	documentwindow.c \
	documentwindow.h \
	documentwindow.vala \
	metadata.c \
	metadata.h \
	metadata.vala \
	project.c \
	project.h \
	project.vala \
	projecttree.c \
	projecttree.h \
	projecttree.vala \
	projectwindow.c \
	projectwindow.h \
	projectwindow.vala \
	searchentry.c \
	searchentry.h \
	searchentry.vala \
	settings.c \
	settings.h \
	settings.vala \
	uimanager.c \
	uimanager.h \
	uimanager.vala \
	$(NULL)
	
scratchpad.stamp: $(filter %.vala,$(scratchpad_SOURCES))
	$(VALAC) -C --vapidir $(srcdir)/../vapi --pkg gtk+-2.0 --pkg gtksourceview-2.0 --pkg gconf-2.0 --pkg unique-1.0 --pkg misc-utils $^
	touch $@
	
scratchpad_LDADD = \
	$(SCRATCHPAD_LIBS) 					\
	../libguniqueapp/libguniqueapp.la 	\
	$(NULL)
	
EXTRA_DIST = scratchpad.stamp
