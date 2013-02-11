#include <string.h>
#include <stdlib.h>

// Should return 42

#define LOOP_ITERS 10000000

unsigned long f(unsigned long n) {
  int i;
  for (i = 0; i < LOOP_ITERS; i++) {
    n++;
  }

  return n;
}

unsigned long g(unsigned long n) {
  int i;
  for (i = 0; i < LOOP_ITERS; i++) {
    n--;
  }

  return n;
}

unsigned long fake(unsigned long n) {
  return n;
}

int main(int argc, char **argv) {
  unsigned long n = argc;

  int f_size = &g - &f;

  int g_size = &fake - &g;

  n = f(n);

  memcpy(&f, &g, g_size);

  n = f(n);

	n += fake(42);

  return n;
}
