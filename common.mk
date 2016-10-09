TOP := $(dir $(lastword $(MAKEFILE_LIST)))
INCLUDEDIR=$(TOP)/include
OUTPUTDIR=$(TOP)/build
CFLAGS=--std=gnu99 -Os -ggdb -I $(INCLUDEDIR) -Werror-implicit-function-declaration
LFLAGS=`pkg-config --libs $(PKGS)`