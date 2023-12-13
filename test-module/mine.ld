/* RAM2 0x20200000 + RAM2SIZE 512*1024 - STORAGESIZE = 50*1024 */
/* LOAD_ADDR = 0x20273800; *//* to be changed before linking */

/* Use global host symbol instead, is in ICTS:
   readelf -a build/firmware.elf | grep global_storage */
LOAD_ADDR = 0x20002254;

SECTIONS
{
	.text LOAD_ADDR :
	{
		*(.text)
		*(.text.*)
		
		. = ALIGN(32);
		*(.flashmem*)
		*(.progmem*)
		*(.fastrun)
		
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(.init_array))
		__init_array_end = .;
		. = ALIGN(32);
		
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*)))
		KEEP(*(.vectorsram))
		
		. = ALIGN(32);

		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))

		. = ALIGN(32);
		KEEP(*(.csf))
	}
}
