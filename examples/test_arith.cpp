#include <iostream>

#include "arith.hpp"

int main() {
  using namespace arith;

  Block* block = new Block();
  Builder builder(block);

  builder.build_mul(
    builder.build_add(
      builder.build_const_int(2),
      builder.build_const_int(3)
    ),
    builder.build_const_int(4)
  );

  block->write(std::cout);
}
