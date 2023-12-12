/*
 * Teensy ramloaded -- things are relocatable anyway but here, we suggest
 * that things start at ORIGIN=0.
 *
 * This is  basically the imxrt1062_t41.ld but everything going to the "virtual"
 * memory section RAMLOADED.
 *
 *
 */

MEMORY
{
	RAMLOADED (rwx):  ORIGIN = 0x00000000, LENGTH = 256K /* teensy has 2x 512K */
}

SECTIONS
{
	.text.code : {
		KEEP(*(.startup))
		*(.flashmem*)
		. = ALIGN(4);
		KEEP(*(.init))
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(.init_array))
		__init_array_end = .;
		. = ALIGN(4);
	} > RAMLOADED

	.text.progmem : {
		*(.progmem*)
		. = ALIGN(4);
	} > RAMLOADED

	.text.itcm : {
		*(.fastrun)
		*(.text*)
		. = ALIGN(16);
	} > RAMLOADED

	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .ARM.extab.text* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > RAMLOADED

	.data : {
		*(.endpoint_queue)   
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*)))
		KEEP(*(.vectorsram))
	} > RAMLOADED
	
	.bss ALIGN(4) : {
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))
		. = ALIGN(32);
		. = . + 32; /* MPU to trap stack overflow */
	} > RAMLOADED

	.bss.dma (NOLOAD) : {
		*(.hab_log)
		*(.dmabuffers)
		. = ALIGN(32);
	} > RAMLOADED

	.bss.extram (NOLOAD) : {
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.externalram)))
		. = ALIGN(32);
	} > RAMLOADED

	.text.csf : {
		/* FILL(0xFF) */
		. = ALIGN(1024);
		KEEP(*(.csf))
		__text_csf_end = .;
	} > RAMLOADED

	_stext = ADDR(.text.itcm);
	_etext = ADDR(.text.itcm) + SIZEOF(.text.itcm) + SIZEOF(.ARM.exidx);
	_stextload = LOADADDR(.text.itcm);

	_sdata = ADDR(.data);
	_edata = ADDR(.data) + SIZEOF(.data);
	_sdataload = LOADADDR(.data);

	_sbss = ADDR(.bss);
	_ebss = ADDR(.bss) + SIZEOF(.bss);


	_extram_start = ADDR(.bss.extram);
	_extram_end = ADDR(.bss.extram) + SIZEOF(.bss.extram);

	_teensy_model_identifier = 0x25;

	.debug_info     0 : { *(.debug_info) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }

}
