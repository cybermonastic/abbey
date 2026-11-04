#include <iostream>

#include "error.h"
#include "print.h"
#include "tokens.h"

int main(int argc, const char* argv[]) {
  int result = 0;

  result = RunErrorTests(argc, argv) == 0 ? result : 1;
  result = RunPrintTests(argc, argv) == 0 ? result : 1;
  result = RunTokensTests(argc, argv) == 0 ? result : 1;

  return result;
}
