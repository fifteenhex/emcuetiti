include ../common.mk

LIBMQTTHEADERS=$(INCLUDEDIR)/libmqtt.h $(INCLUDEDIR)/libmqtt_priv.h $(INCLUDEDIR)/buffers.h
GLOBALHEADERS=$(INCLUDEDIR)/emcuetiti_types.h $(INCLUDEDIR)/emcuetiti_config.h

$(OUTPUTDIR)/libemcuetiti.a: \
$(OUTPUTDIR)/buffers.o \
$(OUTPUTDIR)/libmqtt.o \
$(OUTPUTDIR)/libmqtt_readpkt.o \
$(OUTPUTDIR)/libmqtt_writepkt.o \
$(OUTPUTDIR)/emcuetiti_broker.o \
$(OUTPUTDIR)/emcuetiti_topic.o \
$(OUTPUTDIR)/emcuetiti_port.o \
$(OUTPUTDIR)/emcuetiti_client.o \
$(OUTPUTDIR)/emcuetiti_port_router.o \
$(OUTPUTDIR)/emcuetiti_port_remote.o \
$(OUTPUTDIR)/emcuetiti_port_sys.o \
$(OUTPUTDIR)/emcuetiti_log.o 
	$(AR) cr $@ $^

#$(OUTPUTDIR)/sparsepointerarray.o \

$(OUTPUTDIR)/buffers.o: buffers.c $(INCLUDEDIR)/buffers.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/libmqtt.o: libmqtt.c $(LIBMQTTHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/libmqtt_readpkt.o: libmqtt_readpkt.c $(INCLUDEDIR)/libmqtt_readpkt.h $(LIBMQTTHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/libmqtt_writepkt.o: libmqtt_writepkt.c $(INCLUDEDIR)/libmqtt_writepkt.h $(LIBMQTTHEADERS) 
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_broker.o: emcuetiti_broker.c $(INCLUDEDIR)/emcuetiti_broker.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_topic.o: emcuetiti_topic.c $(INCLUDEDIR)/emcuetiti_topic.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port.o: emcuetiti_port.c $(INCLUDEDIR)/emcuetiti_port.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port_router.o: emcuetiti_port_router.c $(INCLUDEDIR)/emcuetiti_port_router.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port_remote.o: emcuetiti_port_remote.c $(INCLUDEDIR)/emcuetiti_port_remote.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUTDIR)/emcuetiti_port_sys.o: emcuetiti_port_sys.c $(INCLUDEDIR)/emcuetiti_port_sys.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
	
$(OUTPUTDIR)/emcuetiti_client.o: emcuetiti_client.c $(INCLUDEDIR)/emcuetiti_client.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
	
$(OUTPUTDIR)/emcuetiti_log.o: emcuetiti_log.c $(INCLUDEDIR)/emcuetiti_log.h $(GLOBALHEADERS)
	$(CC) $(CFLAGS) -c $< -o $@