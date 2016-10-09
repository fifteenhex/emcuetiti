include ../../common.mk

PKGS=glib-2.0 gio-2.0
CFLAGS+=`pkg-config --cflags $(PKGS)`

OUTPUTPREFIX=linux_glib_

$(OUTPUTDIR)/emcuetiti_linux: $(OUTPUTDIR)/emcuetiti_linux_unstripped
	strip -o $@ $(OUTPUTDIR)/emcuetiti_linux_unstripped

$(OUTPUTDIR)/emcuetiti_linux_unstripped: main.c $(OUTPUTDIR)/$(OUTPUTPREFIX)commandline.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)broker.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)remote.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)sys.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)gsocketfuncs.o \
$(OUTPUTDIR)/libemcuetiti.a
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^
	
$(OUTPUTDIR)/$(OUTPUTPREFIX)commandline.o: commandline.c commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<

$(OUTPUTDIR)/$(OUTPUTPREFIX)broker.o: broker.c broker.h commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<
	
$(OUTPUTDIR)/$(OUTPUTPREFIX)remote.o: remote.c remote.h commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<

$(OUTPUTDIR)/$(OUTPUTPREFIX)sys.o: sys.c sys.h commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<
	
$(OUTPUTDIR)/$(OUTPUTPREFIX)gsocketfuncs.o: gsocketfuncs.c gsocketfuncs.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<
