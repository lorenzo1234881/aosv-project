KDIR = /lib/modules/$(shell uname -r)/build
obj-m += ums.o
ums-objs := ums_core.o kworker_thread.o kcompletion_list.o ksched_thread.o ums_proc.o

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
