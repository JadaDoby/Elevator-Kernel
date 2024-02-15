obj-m +=my_timer.o

KDIR:=/LIB/MODULES/$(shell uname -r)/build

all:
	$(MAKE)-c $(KDIR) M=$(PWD) modules 

clean:
 	 $(MAKE) -C $(KDIR) M=$(PWD) clean