obj-m = stopwatch.o 
CC := arm-none-linux-gnueabi-gcc

KDIR	:=/work/achroimx_kernel
PWD := $(shell pwd)

all: driver

driver : 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

app:
	arm-none-linux-gnueabi-gcc -static -o app app.c
	
clean : 
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.o
	rm -rf $(APP)
	rm -rf Module.symvers
	rm -rf modules.order
	rm -rf .stopwatch*
	rm -rf .tmp*
