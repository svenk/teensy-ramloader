#include <Arduino.h>
#include <cstdint>

extern "C" { // for startup symbol
  
/*
void test_main() {
  Serial.println("Hello World.");
}
*/

void call_me_back(); // Defined in main program

uint8_t adder(uint8_t a, uint8_t b) {
  return a+b;
}

void any_function() {
  call_me_back();
}

void third_function() {
  call_me_back();
}

void test_main() {
  any_function();
  Serial.println("test_main");
}

void fourth_function() {
  any_function();
}


}
