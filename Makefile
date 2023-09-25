obj-m += dynmodule.o

KDIR = /lib/modules/6.2.0-33-generic/build

all:
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" modules

install:
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" modules_install
	depmod -A
	
clean:
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)" clean
