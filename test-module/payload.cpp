#include <Arduino.h>

extern "C" { // for startup symbol
  
/*
void test_main() {
  Serial.println("Hello World.");
}
*/

void call_me_back(); // Defined in main program

void test_main() {
  call_me_back();
  Serial.println("test_main");
}


}
