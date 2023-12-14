#include <Arduino.h>

#include "imxrt.h" // framework-arduinoteensy/cores/teensy4
#include "teensy_startup.h"

#include "payload.h"

uint8_t* mem;

// Memory alignment in a similar syntax to the linker ". = align(4)"
// example usage: uint8_t* mem = align((uintptr_t)mem, 4)
uintptr_t align(uintptr_t base, uint8_t exp) { return (base + exp - 1) & ~(exp - 1); }
uint8_t*  align(uint8_t*  base, uint8_t exp) { return (uint8_t*)align((uintptr_t)base, exp); }

void setup() {
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }
  Serial.println("Teensy-Ramloder setup() starting.");


  // TODO: Precisely disable memory protection for RAM2.
  // see teensy4/startup.c function configure_cache for proper setting.
  
  // This line works: turn off MPU completely
  // effect: Can execute code in stack and global storage.
  SCB_MPU_CTRL = 0;
  
  // The following lines do not yet work:

  SCB_MPU_RBAR = 0x00000000 | REGION(1); // ITCM
  SCB_MPU_RASR = MEM_NOCACHE | READWRITE | SIZE_512K;

  SCB_MPU_RBAR = 0x20000000 | REGION(4); // DTCM
  SCB_MPU_RASR = MEM_NOCACHE | READWRITE | NOEXEC | SIZE_512K;

  SCB_MPU_RBAR = 0x20200000 | REGION(6); // RAM (AXI bus)
  SCB_MPU_RASR = MEM_CACHE_WBWA | READWRITE | NOEXEC | SIZE_1M;

  // Enabling back the MPU
  //SCB_MPU_CTRL = SCB_MPU_CTRL_ENABLE;

  // recommended to do before and after changing MPU registers
  asm("dsb");
  asm("isb");
}


// Test targets to call back from the ramloaded code.
// They must be part of the symbol table, thus avoid -O2 inlines them.
extern "C" {

void  __attribute__ ((noinline)) call_me_back() {
  Serial.println("this is call_me_back!\n");
}

void  __attribute__ ((noinline)) another_target() {
  call_me_back();
}

} // extern C

// Different ramloader function signatures
typedef void (entry_t)(void);
typedef uint8_t (adder_t)(uint8_t, uint8_t);


// This is bytecode implementing
//    uint8_t adder(uint8_t a, uint8_t b) { return a+b; }
// This is a neat standalone function suitable for whether instruction
// calls work in principle.
// Read out via octeta. Note that readelf always shows words in
// a byte order not suitable for working when embedding them here as
// four word hex codes (0x1234 vs 0x12 0x34).
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

// Demonstration calling custom code from stack-allocated memory.
// This one works, given the above MPU rules.
void call_in_memory_stack_function() {
  uint8_t  stack_storage[100];
  uint8_t* heap_storage = new uint8_t[100];

  uint8_t* stack_program = align(stack_storage, 32);
  //uint8_t* stack_program = align(heap_storage, 32);

  uint32_t* stack_dwords = (uint32_t*) stack_program;
  stack_dwords[0] = 0xf7f8b832; // b.w 68 <call_me_back>

  /// Note this is *NOT* working due to byte order. Mind the objdumps!
  // uint16_t* stack_words = (uint16_t*) stack_program;
  // stack_words[0] = 0x2368; // movs r3, #104 ; 0x68
  // stack_words[1] = 0x4798; // blx  r3
  // stack_words[2] = 0x4770; // bx   lr  ; actually returns

  // assembly variant of func() { call_me_back(); }
  // Again this is not working due to PC-relative bl.
  // Had to relocate here or do it correctly at link time.
  /*
  stack_words[0] = 0xb580; // push    {r7, lr}
  stack_words[1] = 0xaf00; // add     r7, sp, #0
  stack_words[2] = 0xf7f8; // [two words]
  stack_words[3] = 0xf830; // bl      68 <call_me_back>
  stack_words[4] = 0xbf00; // nop
  stack_words[5] = 0xbd80; // pop     {r7, pc}
  */

  // anoher "shellcode": Compiled void any_function() { call_me_back(); }
  // It uses bl instead of blx instructions, calls are pc-relative so this does
  // NOT work.
  uint8_t caller[12] = {
    0x80, 0xb5, 0x00, 0xaf, 0xf8,
    0xf7, 0x1e, 0xf8, 0x00, 0xbf,
    0x80, 0xbd
  };

  memcpy(stack_program, adder_storage, sizeof(adder_storage)); // works!
  // memcpy(stack_program, caller, sizeof(caller));

  // again, this is not working due to wrong byte order:
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
  Serial.printf("My adder computed: %d\n", res);

  Serial.println("call_in_memory_stack_function: done");
}

// Demonstration calling custom code from heap-allocated memory.
// This one does NOT work but crash given the MPU rules.
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

// Disabling the MPU allows for executing code from the initliazed/zeroed variables.
// It is the same memory region as the stack (DTCM on RAM1)
uint8_t global_storage[1024];

// Demonstration calling custom code from the global memory.
// Works given above MPU rules.
void call_in_global_function() {
  uint8_t* stack_program = align(global_storage, 32);
  memcpy(stack_program, adder_storage, sizeof(adder_storage)); // LAEUFT!
  Serial.printf("call_in_global_function: Calling to %x\n", stack_program);
  auto adder = (adder_t*)(stack_program+1);
  auto res = adder(1,2);
  Serial.printf("My adder computed: %d\n", res);
  Serial.println("call_in_global_function: done");
}

// Demonstration calling custom code loaded from the header include and containing
// more fancier code. It works and "only" requires to be linked so that the load_addr
// is exactly the same as in the linker script of the "shellcode".
void call_global_loaded() {
  uint8_t* load_addr = global_storage;
  memcpy(load_addr, payload_bin, payload_bin_len);
  auto entrypoint = (uint8_t*) ENTRY_POINT;
  auto entry = (entry_t*)(ENTRY_POINT); // includes blx selection bit
  for(auto i = (uint8_t*)entrypoint-1; i < (uint8_t*)entrypoint + 10; i++) {
    Serial.printf("Addr %x = %x\n", i, *i);
  }

  Serial.printf("now jumping to %X\n", entrypoint);
  entry();
  Serial.println("Successfully returned");
}

// Demnstration loading custom code from an "arbitrary" memory position. In this case
// the address is at the end of RAM2 where heap evventually grows to. 
// The load_addr computation follows the argument
//   RAM2 starting pos = 0x20200000
// + RAM2 size = 512*1024
// - Arbitrary storage size for code = 50*1024
// = loader addr 0x20273800
// Unfortunately, the MPU disallows running code from that memory region.
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
  Serial.printf("My adder computed: %d\n", res);

  Serial.println("Successfully returned");
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
      case 'g':
        call_in_global_function();
        break;
      case 'l':
        call_global_loaded();
        break;
      case 'f':
        call_fixed_address_loaded();
        break;
    }
  }

  Serial.print("o");
  Serial.flush();
  delay(1000 /*ms*/);
}
