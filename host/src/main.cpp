#include <Arduino.h>

//#include "imxrt.h" // framework-arduinoteensy/cores/teensy4
#include "teensy_startup.h" // above is contained here

#include "elf.h"
#include "payload.h"

uint8_t* mem;

// example: uint8_t* mem = align((uintptr_t)mem, 32)
uintptr_t align(uintptr_t base, uint8_t exp) { return (base + exp - 1) & ~(exp - 1); }
uint8_t*  align(uint8_t*  base, uint8_t exp) { return (uint8_t*)align((uintptr_t)base, exp); }



void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }
  Serial.println("Teensy-Ramloder setup() starting.");

/*
  // copy elf from flash to RAM2
  const uint8_t alignment = 32;
  const uint8_t alignment_reserve = alignment;
  mem = new uint8_t[payload_elf_len + alignment_reserve];
  mem = align(mem, 32); // well, aligned new not available...
  memcpy(mem, payload_elf, payload_elf_len);
*/

  // TODO: Precisely disable memory protection for RAM2.
  // see teensy4/startup.c function configure_cache for proper setting.
  SCB_MPU_CTRL = 0; // turn off MPU completely

	SCB_MPU_RBAR = 0x00000000 | REGION(1); // ITCM
	SCB_MPU_RASR = MEM_NOCACHE | READWRITE | SIZE_512K;

	SCB_MPU_RBAR = 0x20000000 | REGION(4); // DTCM
	SCB_MPU_RASR = MEM_NOCACHE | READWRITE | NOEXEC | SIZE_512K;

	SCB_MPU_RBAR = 0x20200000 | REGION(6); // RAM (AXI bus)
	SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1M;

	//SCB_MPU_CTRL = SCB_MPU_CTRL_ENABLE;

	asm("dsb");
	asm("isb");
}

extern "C" {

void  __attribute__ ((noinline)) call_me_back() {
  Serial.println("this is call_me_back!\n");
}

void  __attribute__ ((noinline)) another_target() {
  call_me_back();
}

} // extern C

typedef void (entry_t)(void);
typedef uint8_t (adder_t)(uint8_t, uint8_t);

  // uint8_t adder(uint8_t a, uint8_t b) { return a+b; }
  // AUSGELESEN VON OCTETA, vgl. readelf.
  uint8_t adder_storage[36] = {
    0x80, 0xb4, 0x83, 0xb0, 0x00,
    0xaf, 0x03, 0x46, 0x0a, 0x46,
    0xfb, 0x71, 0x13, 0x46, 0xbb,
    0x71, 0xfa, 0x79, 0xbb, 0x79,
    0x13, 0x44, 0xdb, 0xb2, 0x18,
    0x46, 0x0c, 0x37, 0xbd, 0x46,
    0x5d, 0xf8, 0x04, 0x7b, 0x70,
    0x47
  };

void call_in_memory_stack_function() {
  uint8_t  stack_storage[100];
  uint8_t* heap_storage = new uint8_t[100];

  uint8_t* stack_program = align(stack_storage, 32);
  //uint8_t* stack_program = align(heap_storage, 32);

  uint32_t* stack_dwords = (uint32_t*) stack_program;
  stack_dwords[0] = 0xf7f8b832; // b.w 68 <call_me_back>

  // alternative:
  uint16_t* stack_words = (uint16_t*) stack_program;
  //stack_words[0] = 0x2368; // movs r3, #104 ; 0x68
  //stack_words[1] = 0x4798; // blx  r3
  //stack_words[2] = 0x4770; // bx   lr  ; actually returns

  // assembly variant of func() { call_me_back(); }
  stack_words[0] = 0xb580; // push    {r7, lr}
  stack_words[1] = 0xaf00; // add     r7, sp, #0
  stack_words[2] = 0xf7f8; // [two words]
  stack_words[3] = 0xf830; // bl      68 <call_me_back>
  stack_words[4] = 0xbf00; // nop
  stack_words[5] = 0xbd80; // pop     {r7, pc}



  // mit -O0 kompiliertes void any_function() { call_me_back(); }
  // AUSGELESEN VON OCTETA, vgl. readelf.
  uint8_t caller[12] = {
    0x80, 0xb5, 0x00, 0xaf, 0xf8,
    0xf7, 0x1e, 0xf8, 0x00, 0xbf,
    0x80, 0xbd
  };

  memcpy(stack_program, adder_storage, sizeof(adder_storage)); // LAEUFT!
  // memcpy(stack_program, caller, sizeof(caller));

  // mit umgedrehten bytes: GEHT NICHT
  /*
  stack_words[0] = 0x80b5; // push    {r7, lr}
  stack_words[1] = 0x00af; // add     r7, sp, #0
  stack_words[2] = 0xf8f7; // [two words]
  stack_words[3] = 0x30f8; // bl      68 <call_me_back>
  stack_words[4] = 0x00bf; // nop
  stack_words[5] = 0x80bd; // pop     {r7, pc}
  */

  Serial.println("call_in_memory_stack_function: starting");
  Serial.printf("Calling hexcode %x at addr %x\n", stack_dwords[0], stack_program);
  Serial.printf("That is supposed to b.w to %x\n", call_me_back);
  //entry_t *entry = (entry_t*)(stack_program + 1);
  //entry();
  auto adder = (adder_t*)(stack_program+1);
  auto res = adder(1,2);
  Serial.printf("My adder berechnete: %d\n", res);

  Serial.println("call_in_memory_stack_function: done");
}

void call_in_memory_heap_function() {
  uint8_t* heap_storage = new uint8_t[100];
  uint8_t* stack_program = align(heap_storage, 32);
  memcpy(stack_program, adder_storage, sizeof(adder_storage)); // LAEUFT!
  Serial.printf("call_in_memory_heap_function: Calling to %x\n", stack_program);
  auto adder = (adder_t*)(stack_program+1);
  auto res = adder(1,2);
  Serial.printf("My adder berechnete: %d\n", res);
  Serial.println("call_in_memory_heap_function: done");
}

void call_fixed_address_loaded() {
  auto load_addr = (uint8_t*) 0x20273800; // as in linker script, already aligned
  Serial.printf("call_fixed_address_loaded working at %X\n", load_addr);
  memcpy(load_addr, payload_bin, payload_bin_len);
  auto entrypoint = (uint8_t*) 0x20273800 + 1; // ist der adder
  // auto entry = (entry_t*)(ENTRY_POINT); // shall contain the instruction set selection bit required by blx.

  for(auto i = (uint8_t*)entrypoint-1; i < (uint8_t*)entrypoint + 10; i++) {
    Serial.printf("Addr %x = %x\n", i, *i);
  }

  Serial.printf("now jumping to %X\n", entrypoint);
  // entry();
  auto adder = (adder_t*) entrypoint;
  auto res = adder(1,2);
  Serial.printf("My adder berechnete: %d\n", res);

  Serial.println("Successfully returned");
}

void do_the_shit_manually() {
  auto test_main = (uint8_t*) 0x2027383C;


}

void code() {
  // Read elf headers
  Elf32_Ehdr* hdr = (Elf32_Ehdr*) mem;

  Serial.printf("Entry point from Elf Header: %x\n", hdr->e_entry);

  // note that Thumb instructions always require an offset +1 at loading, this is the
  // instruction set selection bit (section A7.7.19 in armv7 manual)
  uint8_t* entrypoint = mem + hdr->e_entry;

  for(uint8_t* i = entrypoint; i < entrypoint + 30; i++) {
    Serial.printf("Addr %x = %x\n", i, *i);
  }

  Serial.printf("This is where my usb_serial_write lives = %x\n", usb_serial_write);
  Serial.printf("This is where my call_me_back lives = %x\n", call_me_back);

  Serial.printf("Now calling to entrypoint = %x\n", entrypoint);
  entry_t *entry = (entry_t*)(entrypoint);
  entry();
  /*
  typedef int (entry_t)(void);  
  entry_t *entry = (entry_t*)(entrypoint);
  int res = entry();
  Serial.printf("Result is %x=%i\n", res, res);
  */

  Serial.println("And i am back!");

}

void loop() {
  while (Serial.available() > 0) {
    auto byte = Serial.read();

    switch(byte) {
      case 'c':
        if (CrashReport) {    // Make sure Serial is alive and there is a CrashReport stored.
          Serial.println(CrashReport);  // Once called any crash data is cleared
          Serial.println();
          Serial.flush();
        } else {
          Serial.println("No crash report avail.");
        }
        break;
      case 's':
        call_in_memory_stack_function();
        break;
      case 't':
        another_target();
        Serial.println("Well back we are");
        break;
      case 'h':
        call_in_memory_heap_function();
        break;
      case 'f':
        call_fixed_address_loaded();
        break;
      case 'e': 
        code();
        break;
    }
  }

  Serial.print("o");
  Serial.flush();
  delay(1000 /*ms*/);
}
