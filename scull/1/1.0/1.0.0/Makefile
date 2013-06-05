

ifneq ($(KERNELRELEASE),)

obj-m := scull.o
ccflags-y := -g 

else

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD)

endif
