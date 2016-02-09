CFLAGS=-ggdb

all: emcuetiti_linux testharness_basic

libmqtt.o: libmqtt.c libmqtt.h
	$(CC) $(CFLAGS) -c $<

emcuetiti.o: emcuetiti.c emcuetiti_config.h emcuetiti_priv.h emcuetiti.h
	$(CC) $(CFLAGS) -c $<

emcuetiti_linux: main_linux.c emcuetiti.o libmqtt.o
	$(CC) $(CFLAGS) -o $@ $^

testharness_basic: testharness_basic.c testutils.c emcuetiti.o libmqtt.o
	$(CC) $(CFLAGS) -o $@ $^
	
.PHONY: clean

clean:
	rm -f *.o
