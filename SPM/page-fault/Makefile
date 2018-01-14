obj-m := my_page_fault.o
pagefault-objs = my_page_fault.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
KBUILD_EXTRA_SYMBOLS := /home/ww31/Documents/IntelSGX/linux-sgx-driver-master/Module.symvers

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -rf *.o *.ko *.mod.c .tmp_versions .*o.cmd
