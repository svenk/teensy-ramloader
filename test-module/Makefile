

# typically something like $HOME/.platformio
PLATFORMIO_CORE_DIR=$(shell pio system info --json-output | jq -r '.core_dir.value')

# the CFLAGS have to match how the host code was built. For instance, mixing -O2 and -O0 is
# a bad idea (as it is also for two compilation units in a simpler setup) and typically results
# in weird linker errors.
PREFIX=$(PLATFORMIO_CORE_DIR)/packages/toolchain-gccarmnoneeabi-teensy/bin/
INC=-L$(PLATFORMIO_CORE_DIR)/packages/framework-arduinoteensy/cores/teensy4
CFLAGS=-fno-exceptions -felide-constructors -fno-rtti -std=gnu++14 -Wno-error=narrowing -fpermissive -fno-threadsafe-statics -fno-use-cxa-atexit -Wall -ffunction-sections -fdata-sections -mthumb -mcpu=cortex-m7 -nostdlib -mfloat-abi=hard -mfpu=fpv5-d16 -O2 -DPLATFORMIO=60107 -D__IMXRT1062__ -DARDUINO_TEENSY41 -DUSB_SERIAL -DARDUINO=10805 -DTEENSYDUINO=158 -DCORE_TEENSY -DF_CPU=600000000 -DLAYOUT_US_ENGLISH -I$(PLATFORMIO_CORE_DIR)/packages/framework-arduinoteensy/cores/teensy4

# See also `arm-none-ebai-ld --verbose` for the default linker script
# as well as the Teensy one at https://github.com/PaulStoffregen/cores/blob/master/teensy4/imxrt1062_t41.ld
ldscript=ramload.ld

# Just for the nice tooling, we automatically read the appropriate code load address
# and entry point from the respective sources. Note that Makefiles variables are lazy
# evaluated, so files are always there at variable evaluation time.
# Also note, remove $$NF to $NF when extracting from makefile to regular shell script
LOAD_ADDR=0x$(shell nm -gC firmware.elf | grep global_storage | cut -f1 -d' ')
ENTRY_POINT=$(shell readelf -h payload.elf  | grep "Entry point" | awk '{print $$NF}')

all: payload.h

# Host code which is flashed on the MCU
firmware.elf:
	ln -fs ../host/build/firmware.elf

# This variant contains linkable symbols (i.e. non-stripped).
firmware-dyn.elf.a: firmware.elf
	cd ../host && ./make-test.sh
	ln -fs ../host/build/firmware-dyn.elf.a

# going the .o -> .a way only for making the linker entry point calm.
payload.o: payload.cpp
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -c payload.cpp
	
payload.a: payload.o
	$(PREFIX)arm-none-eabi-gcc-ar rc payload.a payload.o

payload.elf: payload.a firmware-dyn.elf.a
	sed -i "s/^LOAD_ADDR.*/LOAD_ADDR = $(LOAD_ADDR);/" $(ldscript)
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -o $@ -T $(ldscript) -nostartfiles -nodefaultlibs -nostdlib  --static-pie -L../host/build -l:firmware-dyn.elf.a -L. -l:payload.a -g -Wl,-e,test_main payload.a
#	$(PREFIX)arm-none-eabi-strip --strip-unneeded  payload.elf

# the contents of this file are actually ramloaded
payload.bin: payload.elf
	$(PREFIX)arm-none-eabi-objcopy --dump-section .text=payload.bin  payload.elf

payload.h: payload.bin
	xxd -i $< > $@
	echo "#define LOAD_ADDR $(LOAD_ADDR)" >> $@
	echo "#define ENTRY_POINT $(ENTRY_POINT)" >> $@

clean:
	rm -f *.elf *.a *.o

# show size statistics:
#    nm --print-size --size-sort --radix=d payload.elf
#    arm-none-eabi-size --common payload.elf

# what follows are failed experiments.

minimal-payload.elf: payload.a
# the custom linker does not really bring any advantage.
# 32k of zeros is padded in the regular elf, this is what occupies storage.
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -o $@ -nostartfiles -nodefaultlibs -nostdlib  --static-pie -L../host/build -l:firmware-dyn.elf.a -L. -l:payload.a -g -Wl,-e,test_main -Wl,-T,ramloaded_imxrt1062_t41.ld payload.a

relocatable-payload.elf:
# works but certainly not what i want as it needs relocation, i.e. information
# where all the symbols are stored at runtime.
	$(PREFIX)arm-none-eabi-gcc $(CFLAGS) $(INC) -Wl,-r -nostartfiles -nodefaultlibs -nostdlib -g payload.cpp
