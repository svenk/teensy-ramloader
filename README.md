# Runtime-loading machine code at teensy4.1 (armv7)

This repository contains a demonstrator code which shows how to load machine code (think of shell code)
at an armv7 microcontroller unit, in particular the [Teensy 4.1 Development board](https://www.pjrc.com/store/teensy41.html).
The code is the result of my initial question for
[Scriptable C++ API / Linkable Loadable Code at Runtime](https://forum.pjrc.com/index.php?threads/scriptable-c-api-linkable-loadable-code-at-runtime.73969/)
in the PJRC/teensy forum.

## The most lightweight module/plugin/extension system for microcontrollers

The motivation for writing this kind of tooling is to allow runtime extension of
"big" codebases running on a microcontroller (MCU). The typical microcontroller layout has a comparatively
small RAM and a big flash ROM. Flashing and resetting the MCU can be nontrivial, and in fact the majority of
value added by the Teensy project to a bare ARM Cortex-M7 (or MIMXRT1062, to be specific) is the dedicated
bootloader chip.

We will refer to the main program running on the MCU as *host code*. Suitable synonyms might be "Arduino sketch",
"upstream code", "main program" or "firmware". We will refer to any loaded code as *payload code*. Suitable
synonyms might be "guest code", "extension", "module" or "plugin". The objective is to allow the payload code
to access all of the host code API.

For implementing a plugin system, I evaluated different approaches:

* Scripting languages such as micropython or Lua: Being interpreted, there is little concern about technicalities
  like compilation, linking and memory protection. The main drawback is the need to expose the API which results
  in manual work (such as writing a bunch of python modules). Furthermore, obviously, the scripting language and
  virtual machine have some overhead, which might be however not so bad, given the enormous manpower put into
  projects like micropython.
* Microcontroller operating systems are lightweight and typically have allow for loading programs (including
  dynamical linking and all that). There are mature solutions out there (typically real time operating systems
  or dedicated to the IoT world) but they also act as frameworks, requiring their tooling.
* Going bare metal question: What should prevent me from running any machine code from RAM, given the large
  and uniform address space of armv7?



## Features and usage

The code in this repository is a minimal [PlatformIO](https://platformio.org/) project for the host code and
something standalone for the guest code. Platformio usage is a simple as installation and running
`pio run -t upload` to compile and upload. It downloads all infrastructure (such as GCC for ARM and the
Teensy/Arduino library) when needed. The guest code Makefile makes use of the PlatformIO installation, therefore
it has no further dependencies.

Currently the payload is provided to the host at compile time with a header file. This can be easily replaced by
receiving the guest machine code at runtime via USB Serial or Ethernet TCP/IP.

To play with the code,

1. Run `cd host && pio run` to build the host code
2. Edit `test-module/payload.cpp` to do something fancy
3. Run `cd test-module && make` to build the guest code
4. Re-run `cd host && pio run` to include the new guest code

The host currently demonstrates different variants where to load the code from.

## About the loader address

There are two relevant armv7 jump instructions: `bl` and `blx`. `bl` allows for PC-relative jumps and is
the standard instruction as it is probably also very fast, given that the immediate relative jump address
is encoded within the 32bit instruction. Given the PC-relative nature, `bl` is not suitable for position
independent code. Typically this is implemented by the global offset table (GOT). However, I could not make
GCC produce a GOT for a static executable. Furthermore, the GOT requires valuable memory.

Therefore, the easiest path is to know in advance at which point the code will live in the memory and link
it for being used exactly at that point (refered to as *load address* in the following). This way, the
PC-relative `bl` jumps just perfectly work.

I would have prefered to tell the linker to use `blx` instructions for accessing host functions. This is
because `blx` loads absolute addresses. However, I did not find a way to tell GCC linker to do so. What
actually happens instead is that it setups trampolines (veneers) to do long jumps with `bl`. It is
definetly possible to edit static linked code and change the trampoline targets at runtime, but to do this
correctly it would require some more tooling which I feel is not available within GCC. Maybe LLVM provides
this infrastructure.

Position independent executables with symbol tables are a thing (see for instance https://github.com/eklitzke/symtab/tree/master)
but it is definetly a niche.

## Memory Protection Unit (MPU) and layout at teensy

In any case, the MPU needs to be patched to make this code running. In
https://github.com/PaulStoffregen/cores/commit/2607e6226f14d87901077092dddd7786853fbad5
the ITCM was patched to be read-only during regular operations for the sake of security, prohibiting arbitrary jumps.

In an ideal world, there are two positions in the teensy memory model suitable for loading code from fixed addresses
in the RAM:

![teensy 4.1 memory laout](https://www.pjrc.com/store/teensy41_memory.png)

The first one is at the end of the stack (within or above the initialized/zeroed variables) and the other one
is at the end of the heap. Unfortunately, I could only get code running in RAM1 but not RAM2. This is an ongoing
fight against the MPU ;-).

## See also

### Other elfloader projects

A good key word for similar projects are [elf-loaders](https://github.com/topics/elf-loader), such as
https://github.com/embedded2014/elf-loader or the derived https://github.com/niicoooo/esp32-elfloader.
However, these projects try to load *dynamically* linked ELF binaries and link them at runtime, with a 
given a symbol table. This again would require me to expose the API at runtime. I want to load *statically*
linked ELFs where I have basically nothing to do expect to jump into the code and let it do it's thing.

### Development and Simulation

Developing code against hardware which always requires a couple of seconds for flashing and crashing is
tedious. Unfortunately, the Teensy has no real debugging facilities.
Therefore I thought about (but not yet did) one of these options:

* QEMU simulation of armv7 on bare metal, see for instance https://stackoverflow.com/questions/49913745/how-to-run-a-bare-metal-elf-file-on-qemu
* Running code within a Process at Linux on a Raspberry Pi, where I can run `gdb` to examine what is going on.

However, both options don't have the memory layout as the Teensy has it. They are better suited for testing things
like libstd++ issues.
