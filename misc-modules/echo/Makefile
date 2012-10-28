obj-m := echo.o
CFLAGS_echo.o := -DDEBUG -Wall -Werror -Wmissing-prototypes \
	-Wstrict-prototypes -Wunused-variable -O2 \
	-Wunused-function -g

modules:
	make -C /lib/modules/`uname -r`/build M=`pwd`
clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean
