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
    size_t _name = 0;
  public:
    Value(Type type): _type(type) {}
    virtual ~Value() {}
    Type type() const { return _type; }

    void set_name(size_t name) { _name = name; }

    void write_arg(std::ostream& stream) const {
      stream << "%" << _name;
    }
  };

  class Inst: public Value {
  private:
    std::vector<Value*> _args;
  public:
    Inst(Type type, const std::vector<Value*> args): Value(type), _args(args) {}

    virtual void write(std::ostream& stream) const = 0;

    void write_args(std::ostream& stream, bool& is_first) const {
      for (Value* arg : _args) {
        if (!is_first) {
          stream << ", ";
        }
        arg->write_arg(stream);
        is_first = false;
      }
    }
  };

  ${insts}

  class Block {
  private:
    std::vector<Inst*> _insts;
  public:
    Block() {};

    void add(Inst* inst) { _insts.push_back(inst); }

    void write(std::ostream& stream) {
      size_t name = 0;
      for (Inst* inst : _insts) {
        inst->set_name(name++);
      }
      for (Inst* inst : _insts) {
        inst->write_arg(stream);
        stream << " = ";
        inst->write(stream);
        stream << '\n';
      }
    }
  };

  class Builder {
  private:
    Block* _block;
  public:
    Builder(Block* block): _block(block) {}
    void insert(Inst* inst) { _block->add(inst); }

    ${builder}
  };
}
