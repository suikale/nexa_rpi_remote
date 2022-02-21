MCU=attiny85
AVRDUDEMCU=t85
CC=/usr/bin/avr-gcc
CFLAGS=-Os -Wall -mmcu=$(MCU)
OBJ2HEX=/usr/bin/avr-objcopy
AVRDUDE=/usr/bin/avrdude
TARGET=main
PI_IP=pi@zero
SSH_PREFIX=ssh $(PI_IP)

all :  
	$(CC) $(CFLAGS) $(TARGET).c -o $(TARGET)
	$(OBJ2HEX) -R .eeprom -O ihex $(TARGET) $(TARGET).hex
	rm -f $(TARGET)

install : all
	$(SSH_PREFIX) sudo gpio -g mode 22 out
	$(SSH_PREFIX) sudo gpio -g write 22 0
	scp ./$(TARGET).hex $(PI_IP):/home/pi/bin
	$(SSH_PREFIX) sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U flash:w:/home/pi/bin/$(TARGET).hex
	$(SSH_PREFIX) sudo gpio -g write 22 1
noreset : all
	$(SSH_PREFIX) sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U flash:w:/home/pi/bin/$(TARGET).hex

fuse :
	$(SSH_PREFIX) sudo gpio -g mode 22 out
	$(SSH_PREFIX) sudo gpio -g write 22 0
	$(SSH_PREFIX) sudo $(AVRDUDE) -p $(AVRDUDEMCU) -P /dev/spidev0.0 -c linuxspi -b 10000 -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
	$(SSH_PREFIX) sudo gpio -g write 22 1

clean :
	rm -f *.hex *.obj *.o