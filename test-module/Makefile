
PREFIX=/home/sven/.platformio/packages/toolchain-gccarmnoneeabi-teensy/bin/
INC=-L/home/sven/.platformio/packages/framework-arduinoteensy/cores/teensy4
CFLAGS=-fno-exceptions -felide-constructors -fno-rtti -std=gnu++14 -Wno-error=narrowing -fpermissive -fno-threadsafe-statics -Wall -ffunction-sections -fdata-sections -mthumb -mcpu=cortex-m7 -nostdlib -mfloat-abi=hard -mfpu=fpv5-d16 -O0 -DPLATFORMIO=60107 -D__IMXRT1062__ -DARDUINO_TEENSY41 -DUSB_SERIAL -DARDUINO=10805 -DTEENSYDUINO=158 -DCORE_TEENSY -DF_CPU=600000000 -DLAYOUT_US_ENGLISH -I/home/sven/.platformio/packages/framework-arduinoteensy/cores/teensy4

### Attention: Go back to -O2 when using arduino libs in cod
###      otherwise it will have weird linker errors such as "undefined reference to pulseIn"


all: payload.elf

firmware.elf:
	ln -fs ../host/build/firmware.elf

firmware-dyn.elf.a: firmware.elf
	cd ../host && ./make-test.sh
	ln -fs ../host/build/firmware-dyn.elf.a

payload.o: payload.cpp
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -c payload.cpp
	
payload.a: payload.o
	$(PREFIX)arm-none-eabi-gcc-ar rc payload.a payload.o

payload.elf: payload.a firmware-dyn.elf.a
# using the default linker script, see `arm-none-ebai-ld --verbose` for its content
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -o $@ -T mine.ld -nostartfiles -nodefaultlibs -nostdlib  --static-pie -L../host/build -l:firmware-dyn.elf.a -L. -l:payload.a -g -Wl,-e,test_main payload.a
	
payload.bin: payload.elf
	$(PREFIX)arm-none-eabi-objcopy --dump-section .text=payload.bin  payload.elf

payload.h: payload.bin
	xxd -i $< > $@
#	attention, remove $$NF to $NF when extracting from makefile to regular shell script
	echo "#define ENTRY_POINT $(shell readelf -h payload.elf  | grep "Entry point" | awk '{print $$NF}')" >> $@

clean:
	rm -f *.elf *.a *.o


# what follows are failed experiments.

minimal-payload.elf: payload.a
# the custom linker does not really bring any advantage.
# 32k of zeros is padded in the regular elf, this is what occupies storage.
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -o $@ -nostartfiles -nodefaultlibs -nostdlib  --static-pie -L../host/build -l:firmware-dyn.elf.a -L. -l:payload.a -g -Wl,-e,test_main -Wl,-T,ramloaded_imxrt1062_t41.ld payload.a

relocatable-payload.elf:
# works but certainly not what i want as it needs relocation, i.e. information
# where all the symbols are stored at runtime.
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g payload.cpp
