CC=./build_tools/gcc
CFLAGS=-Wall
TARGET=main
PI_IP=pi@192.168.1.199
SSH_PREFIX=ssh $(PI_IP)
DEPENDS=

all :  
	$(CC) $(CFLAGS) $(TARGET).c $(DEPENDS) -o $(TARGET)

install : all
	scp $(TARGET) $(PI_IP):/home/pi/bin

clean :
	rm -f *.hex *.obj *.o