obj-m = stopwatch.o 
CC := arm-none-linux-gnueabi-gcc

KDIR	:=/work/achroimx_kernel
PWD := $(shell pwd)

all: app

driver : 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

app:
	arm-none-linux-gnueabi-gcc -static -o app app.c
	
clean : 
	rm -rf app
