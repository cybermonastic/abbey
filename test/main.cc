#include <iostream>

#include "print.h"

int main(int argc, const char* argv[]) {
  int result = 0;

  result = RunPrintTests(argc, argv) == 0 ? result : 1;

  return result;
}
