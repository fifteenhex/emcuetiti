include ../common.mk

$(OUTPUTDIR)/libemcuetiti.a: $(OUTPUTDIR)/libmqtt.o $(OUTPUTDIR)/emcuetiti.o $(OUTPUTDIR)/emcuetiti_port.o $(OUTPUTDIR)/emcuetiti_port_router.o $(OUTPUTDIR)/emcuetiti_port_remote.o 
	$(AR) cr $@ $^

$(OUTPUTDIR)/libmqtt.o: libmqtt.c $(INCLUDEDIR)/libmqtt.h $(INCLUDEDIR)/libmqtt_priv.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti.o: emcuetiti.c $(INCLUDEDIR)/emcuetiti_config.h $(INCLUDEDIR)/emcuetiti_priv.h $(INCLUDEDIR)/emcuetiti.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port.o: emcuetiti_port.c $(INCLUDEDIR)/emcuetiti.h 
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port_router.o: emcuetiti_port_router.c $(INCLUDEDIR)/emcuetiti_port_router.h 
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port_remote.o: emcuetiti_port_remote.c $(INCLUDEDIR)/emcuetiti_port_remote.h
	$(CC) $(CFLAGS) -c $< -o $@