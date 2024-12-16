#include <iostream>
#include <cstdio>

#define errf(x, ...) fprintf(stderr, x, ##__VA_ARGS__)

int main() {
  errf("%lu\n", sizeof(double));
  return 0;
}
