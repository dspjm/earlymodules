ifneq ($(KERNELRELEASE),)
obj-m := scull.o
ccflags-y := -g -Wall
else
KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$$PWD modules
clean:
	make -C $(KDIR) M=$$PWD clean
endif
