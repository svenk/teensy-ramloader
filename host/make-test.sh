#!/bin/bash -x

PREFIX=/home/sven/.platformio/packages/toolchain-gccarmnoneeabi-teensy/bin/

additional="-Wl,--export-dynamic -Wl,--out-implib .pio/build/teensy41/firmware-dyn.elf.a"
	
${PREFIX}arm-none-eabi-g++ -o .pio/build/teensy41/firmware-dyn.elf -T imxrt1062_t41.ld  ${additional} -Wl,--gc-sections,--relax -mthumb -mcpu=cortex-m7 -Wl,--defsym=__rtc_localtime=1702253338 -mfloat-abi=hard -mfpu=fpv5-d16 -O2 .pio/build/teensy41/src/main.cpp.o -L/home/sven/.platformio/packages/framework-arduinoteensy/cores/teensy4 -L.pio/build/teensy41 -Wl,--start-group .pio/build/teensy41/libFrameworkArduino.a -larm_cortexM7lfsp_math -lm -lstdc++ -Wl,--end-group 
