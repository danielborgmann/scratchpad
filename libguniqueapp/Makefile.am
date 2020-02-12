noinst_LTLIBRARIES = libguniqueapp.la


guniqueapp_headers =		\
	guniqueapp.h		\
	guniqueapp-base.h	\
	guniqueapp-bacon.h
	
BUILT_SOURCES = 		\
	guniqueapp-marshal.c	\
	guniqueapp-marshal.h
	
libguniqueapp_la_SOURCES = 	\
	guniqueapp.c		\
	guniqueapp-base.c	\
	guniqueapp-bacon.c
	
	
if HAVE_DBUS 
libguniqueapp_la_SOURCES += guniqueapp-dbus.c	
BUILT_SOURCES += 		\
	guniqueapp-bindings.h	\
	guniqueapp-glue.h
guniqueapp_headers += guniqueapp-dbus.h
endif

libguniqueapp_la_SOURCES += 	\
	$(BUILT_SOURCES)	\
	$(guniqueapp_headers)

	
guniqueapp-marshal.h: guniqueapp-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=guniqueapp_marshal > $@

guniqueapp-marshal.c: guniqueapp-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"guniqueapp-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=guniqueapp_marshal >> $@	
	
guniqueapp-bindings.h: guniqueapp.xml $(DBUS_BINDING_TOOL)
	$(DBUS_BINDING_TOOL) --mode=glib-server --output=$@ --prefix=guniqueapp $<
	
guniqueapp-glue.h: guniqueapp.xml $(DBUS_BINDING_TOOL)
	$(DBUS_BINDING_TOOL) --mode=glib-client --output=$@ --prefix=guniqueapp $<  
	
EXTRA_DIST      = 		\
	guniqueapp-marshal.list	\
	guniqueapp.xml
			
	
dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
	
CLEANFILES      = $(BUILT_SOURCES)
INCLUDES        = @SCRATCHPAD_CFLAGS@	
LDADD           = @SCRATCHPAD_LIBS@	

if HAVE_DBUS 
INCLUDES += @DBUS_CFLAGS@
LDADD += @DBUS_LIBS@
endif
