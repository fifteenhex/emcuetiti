include common.mk

all:
	$(MAKE) -C src/
	$(MAKE) -C ports/
	
.PHONY: clean

clean:
	rm -fv $(OUTPUTDIR)/*
