obj-m := access_bit.o
pagefault-objs = access_bit.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
ccflags-y  := -O2 -DMODULE -D__KERNEL__
KBUILD_EXTRA_SYMBOLS := /home/ww31/Documents/IntelSGX/linux-sgx-driver-master/Module.symvers

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.c .tmp_versions .*o.cmd
