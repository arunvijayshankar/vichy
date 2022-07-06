# Comment/uncomment the following line to disable/enable debugging
# DEBUG = y

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG), y)
   DEBFLAGS = -O -g -DVICHY_DEBUG # "-O" is needed to expand inlines
else
   DEBFLAGS = -O2
endif
 
EXTRA_CFLAGS += $(DEBFLAGS) -I$(LDDINC)
 
ifneq ($(KERNELRELEASE),)
   # call from kernel build system
   vichy-objs := vichy_main.o
   obj-m   := vichy.o
else
   KERNELDIR ?= /lib/modules/$(shell uname -r)/build
   PWD       := $(shell pwd)
modules:
	echo $(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules
endif

clean:	
	rm -rf *.o *~ core .depend *.mod.o .*.cmd *.ko *.mod.c *.mod \
	.tmp_versions *.markers *.symvers modules.order a.out vichy_test

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
	include .depend
endif
