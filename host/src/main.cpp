#include <Arduino.h>

#include "imxrt.h" // framework-arduinoteensy/cores/teensy4

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

  // copy elf from flash to RAM2
  const uint8_t alignment = 32;
  const uint8_t alignment_reserve = alignment;
  mem = new uint8_t[payload_elf_len + alignment_reserve];
  mem = align(mem, 32); // well, aligned new not available...
  memcpy(mem, payload_elf, payload_elf_len);

  // TODO: Precisely disable memory protection for RAM2.
  // see teensy4/startup.c function configure_cache for proper setting.
  SCB_MPU_CTRL = 0; // turn off MPU completely
	asm("dsb");
	asm("isb");
}

extern "C" {

void  __attribute__ ((noinline)) call_me_back() {
  Serial.println("this is call_me_back!\n");
}

} // extern C

void code() {
  // Read elf headers
  Elf32_Ehdr* hdr = (Elf32_Ehdr*) mem;

  Serial.printf("Entry point from Elf Header: %x\n", hdr->e_entry);
  uint8_t* entrypoint = mem + 0x8000; // for whatever reason, hdr->e_entry == 0x8001 which is wrong.

  for(uint8_t* i = entrypoint; i < entrypoint + 30; i++) {
    Serial.printf("Addr %x = %x\n", i, *i);
  }

  Serial.printf("This is where my usb_serial_write lives = %x\n", usb_serial_write);
  Serial.printf("This is where my call_me_back lives = %x\n", call_me_back);

  entrypoint = 0x68 + 1;

  Serial.println("Calling to call_me_back not as blx but regular:");
  call_me_back();
  Serial.println("And we are back.");

  Serial.printf("Now calling to entrypoint = %x\n", entrypoint);
  typedef void (entry_t)(void);
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
        code();
        break;
    }
  }

  Serial.print("o");
  Serial.flush();
  delay(1000 /*ms*/);
}
