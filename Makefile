BASEPATH=~/code/bin/rpi_tools/cross-pi-gcc-10.3.0-0
CC=$(BASEPATH)/bin/arm-linux-gnueabihf-gcc
INCLUDE=-I $(BASEPATH)/include
CFLAGS=-Wall
TARGET=main
PI_IP=pi@192.168.1.199
SSH_PREFIX=ssh $(PI_IP)
DEPENDS=

all :  
	$(CC) $(INCLUDE) $(CFLAGS) $(TARGET).c $(DEPENDS) -o $(TARGET)

install : all
	scp $(TARGET) $(PI_IP):/home/pi/bin

clean :
	rm -f *.hex *.obj *.o
