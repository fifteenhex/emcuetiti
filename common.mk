TOP := $(dir $(lastword $(MAKEFILE_LIST)))
INCLUDEDIR=$(TOP)/include
OUTPUTDIR=$(TOP)/build

PKGS=glib-2.0 gio-2.0
CFLAGS=-Os -ggdb `pkg-config --cflags $(PKGS)` -I $(INCLUDEDIR) -Werror-implicit-function-declaration
LFLAGS=`pkg-config --libs $(PKGS)`