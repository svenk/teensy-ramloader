
https://github.com/niicoooo/esp32-elfloader/blob/master/example-payload/Makefile

https://github.com/embedded2014/elf-loader/tree/master

https://github.com/topics/elf-loader

https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html
and man ld

link against symtab:
https://github.com/eklitzke/symtab/blob/master/Makefile


levine linkers and loaders
https://www.amazon.de/Linkers-Loaders-John-R-Levine/dp/1558604960

/home/sven/.platformio/packages/framework-arduinoteensy/cores/teensy4/imxrt1062_t41.ld


QEMU simulation armv7 bare metal:

https://stackoverflow.com/questions/49913745/how-to-run-a-bare-metal-elf-file-on-qemu

koennte schon reichen fuer primitiver test des loadings



Backreference to https://forum.pjrc.com/index.php?threads/scriptable-c-api-linkable-loadable-code-at-runtime.73969/post-334470

> You would need to at least reprogram the MPU since the ITCM (the RAM where executable code lives) _was recently patched_ to be read-only during regular operation.

----> https://github.com/PaulStoffregen/cores/commit/2607e6226f14d87901077092dddd7786853fbad5 <---



register:



    fp[-0] saved pc, where we stored this frame.
    fp[-1] saved lr, the return address for this function.
    fp[-2] previous sp, before this function eats stack.
    fp[-3] previous fp, the last stack frame.
