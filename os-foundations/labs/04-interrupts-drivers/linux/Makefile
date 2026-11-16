# Makefile - Stage 04: the tickdev module and its reader.
#
# Run in a Linux VM that has the headers for its running kernel
# (Ubuntu: sudo apt-get install build-essential linux-headers-$(uname -r)).
#
#   make            build tickdev.ko and tickread
#   make module     only the module
#   make reader     only the reader
#   make clean
#
# Not run on the machine where this lab was written; see the lab README.

ifneq ($(KERNELRELEASE),)
# Read by kbuild, which the "module" target below starts.
obj-m := tickdev.o
else

KDIR ?= /lib/modules/$(shell uname -r)/build

all: module reader

module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

reader: tickread

tickread: tickread.c tickdev.h
	$(CC) -O2 -Wall -o $@ tickread.c

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f tickread

.PHONY: all module reader clean
endif
