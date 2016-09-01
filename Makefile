PKGS=glib-2.0 gio-2.0
CFLAGS=-Os -ggdb `pkg-config --cflags $(PKGS)` -I ./include -Werror-implicit-function-declaration
LFLAGS=`pkg-config --libs $(PKGS)`

all: emcuetiti_linux testharness_basic

libmqtt.o: libmqtt.c include/libmqtt.h
	$(CC) $(CFLAGS) -c $<

emcuetiti.o: emcuetiti.c include/emcuetiti_config.h include/emcuetiti_priv.h include/emcuetiti.h
	$(CC) $(CFLAGS) -c $<

emcuetiti_port.o: emcuetiti_port.c include/emcuetiti.h 
	$(CC) $(CFLAGS) -c $<

emcuetiti_port_router.o: emcuetiti_port_router.c include/emcuetiti_port_router.h 
	$(CC) $(CFLAGS) -c $<

emcuetiti_port_remote.o: emcuetiti_port_remote.c include/emcuetiti_port_remote.h
	$(CC) $(CFLAGS) -c $<

emcuetiti_linux: main_linux.c libmqtt.o emcuetiti.o emcuetiti_port_router.o emcuetiti_port_remote.o emcuetiti_port.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

testharness_basic: testharness_basic.c testutils.c emcuetiti.o libmqtt.o
	$(CC) $(CFLAGS) -o $@ $^
	
.PHONY: clean

clean:
	rm -f *.o emcuetiti_linux
