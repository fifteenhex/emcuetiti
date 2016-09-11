include ../../common.mk

OUTPUTPREFIX=linux_glib_

$(OUTPUTDIR)/emcuetiti_linux: main.c $(OUTPUTDIR)/$(OUTPUTPREFIX)commandline.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)broker.o \
$(OUTPUTDIR)/$(OUTPUTPREFIX)remote.o \
$(OUTPUTDIR)/libemcuetiti.a
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^
	
$(OUTPUTDIR)/$(OUTPUTPREFIX)commandline.o: commandline.c commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<

$(OUTPUTDIR)/$(OUTPUTPREFIX)broker.o: broker.c broker.h commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<
	
$(OUTPUTDIR)/$(OUTPUTPREFIX)remote.o: remote.c remote.h commandline.h
	$(CC) $(CFLAGS) $(LFLAGS) -c -o $@ $<