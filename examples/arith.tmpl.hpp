#pragma once

#include <vector>
#include <cassert>

namespace arith {
  enum class Type {
    Int, Bool
  };

  class Value {
  private:
    Type _type;
  public:
    Value(Type type): _type(type) {}
    Type type() const { return _type; }
  };

  class Inst: public Value {
  private:
    std::vector<Value*> _args;
  public:
    Inst(Type type, const std::vector<Value*> args): Value(type), _args(args) {}
  };

  ${insts}

  class Builder {
  public:
    Builder() {}
    void insert(Inst*) {}

    ${builder}
  };
}
