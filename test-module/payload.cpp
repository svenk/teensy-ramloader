#include <Arduino.h>
#include <cstdint>

// This file contains the code which is evventually loaded "in RAM"
// at runtime from the armv7 microprocessor.
//
// It can link to any code which is part of the base image (firmware,
// arduino programm, whatever you want to call it), thus can also share
// the same header files.
// 
// There are some caveats, such as symbols in headers might not exist
// because they were stripped or inlined at the compilation/linking stage
// from the base code.

// One left weirdness, why is this needed? Probably for above reason.
void * operator new(size_t size) { return malloc(size); }

// Testing the C++ kitchen sink. Virtual destructors do not work, but everything
// else does. Templating is a no-brainer since it is not related to linking.
struct Test {
  int a;
  
  virtual void f() { Serial.printf("Test::f, a=%d\n",a );  }
  
  Test(int _a) : a(_a) { Serial.printf("Test::Test(%d)\n", _a); }
  void dump() { Serial.printf("Test::bar() prints a=%d\n",a); }
  ~Test() { Serial.printf("Test::Test~(a was %d)\n", a); }
};

struct Hansi : public Test {
  Hansi(int a) : Test(a) { Serial.println("Hansi::Hansi"); }
  void f() override { Serial.printf("Hansi::f, a=%d\n", a); }
};

int  x = 17;
Test *h;
const char* z = "Hallo Welt\n";


extern "C" { // for startup symbol
void call_me_back(); // Defined in main program
}

uint8_t adder(uint8_t a, uint8_t b) {
  return a+b;
}

void somewhere() {
  Serial.println("Somewhere starting");
  Test t(456);
  t.f();
  Serial.println("Somewhere finishing");
}

void test_main_actual() {
  Serial.println("test_main starting");
  Serial.printf("global *x=%x, value  x=%d\n", &x, x);
  Serial.print(z);
  call_me_back();
  somewhere();
  h = new Hansi(77);
  h->f();
  Serial.println("test_main");
}

extern "C" { // for startup symbol
  void test_main() { test_main_actual(); }
}
