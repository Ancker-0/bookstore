#include "ci.h"
#include "error.h"

#include <iostream>
#include <cstdio>

using namespace ci;

int main() {
  errf("%lu\n", sizeof(double));
  while (true) {
    Ci &ci = Ci::getInstance();
    // Tokenized tk = ci.process_one();
    try {
      ci.process_one();
    } catch (const Error &e) {
      errf("error - %s\n", e.msg.c_str());
    }
    errf("");
  }
  return 0;
}
