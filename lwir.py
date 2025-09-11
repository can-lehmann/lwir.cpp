# Copyright 2025 Can Joshua Lehmann
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from enum import Enum

Getter = Enum("Getter", ["Default", "Never", "Always"])

class BaseType:
    def __init__(self):
        pass

class Type(BaseType):
    def __init__(self, name):
        super().__init__()
        self.name = name

    def is_value(self):
        return False

    def format(self, ir):
        return self.name

    def __hash__(self):
        return hash(self.name)
    
    def __eq__(self, other):
        if not isinstance(other, Type):
            return False
        return self.name == other.name

    def __repr__(self):
        return f"Type({self.name})"

class ValueType(BaseType):
    def __init__(self):
        super().__init__()

    def is_value(self):
        return True

    def format(self, ir):
        return ir.value_type + "*"

    def __hash__(self):
        return hash("ValueType")
    
    def __eq__(self, other):
        return isinstance(other, ValueType)

    def __repr__(self):
        return "ValueType()"

class VarargsValueType(BaseType):
    def __init__(self):
        super().__init__()

    def is_value(self):
        return True

    def format(self, ir):
        return "const std::vector<" + ir.value_type + "*>&"

    def __hash__(self):
        return hash("VarargsValueType")
    
    def __eq__(self, other):
        return isinstance(other, VarargsValueType)

    def __repr__(self):
        return "VarargsValueType()"


class Arg:
    def __init__(self, name, type=None, getter=Getter.Default):
        self.name = name
        self.type = type or ValueType()
        self.getter = getter

        assert isinstance(self.type, BaseType)

    def format_formal_arg(self, ir):
        return self.type.format(ir) + " " + self.name

class Inst:
    def __init__(self, name, args, type, type_checks=None, flags=None, base=None):
        self.name = name
        self.args = args
        self.type = type
        self.type_checks = type_checks or []
        self.flags = flags or set()
        self.base = base

    def format_name(self, ir):
        return self.name + ir.inst_suffix

    def format_builder_name(self, ir):
        snake_case = ""
        for it, char in enumerate(self.name):
            if it != 0 and char.isupper():
                snake_case += "_"
            snake_case += char.lower()
        return "build_" + snake_case

    def format_formal_args(self, ir):
        formal_args = []
        for arg in self.args:
            formal_args.append(arg.format_formal_arg(ir))
        return formal_args

class IR:
    def __init__(self, insts, inst_suffix="Inst", inst_base="Inst", value_type="Value"):
        self.insts = insts
        self.inst_suffix = inst_suffix
        self.inst_base = inst_base
        self.value_type = value_type

class InstGetterPlugin:
    def run(self, inst, ir):
        code = ""
        value_index = 0
        for arg in inst.args:
            if arg.type.is_value():
                if arg.getter == Getter.Always:
                    code += f"  {arg.type.format(ir)} {arg.name}() const {{ return arg({value_index}); }}\n"
                value_index += 1
            elif arg.getter != Getter.Never:
                code += f"  {arg.type.format(ir)} {arg.name}() const {{ return _{arg.name}; }}\n"
        return code

class InstWritePlugin:
    def run(self, inst, ir):
        code = ""
        code += f"  void write(std::ostream& stream) const override {{\n"
        code += f"    stream << \"{inst.name}\";\n"
        if len(inst.args) > 0:
            code += f"    stream << \' \';\n"
            code += f"    bool is_first = true;\n"
            code += f"    write_args(stream, is_first);\n"
            for arg in inst.args:
                if not arg.type.is_value():
                    code += f"    if (!is_first) {{ stream << \", \"; }} else {{ is_first = false; }}\n"
                    code += f"    stream << \"{arg.name}=\";\n"
                    code += f"    stream << _{arg.name};\n"
        code += f"  }}\n"
        return code

class InstPlugin:
    def __init__(self, plugins):
        self.plugins = plugins or []

    def run(self, ir):
        code = ""
        for inst in ir.insts:
            name = inst.format_name(ir)
            base = inst.base or ir.inst_base

            code += f"class {name} final: public {base} {{\n"
            code += f"private:\n"

            # Attributes
            for arg in inst.args:
                if not arg.type.is_value():
                    code += f"  {arg.type.format(ir)} _{arg.name};"

            code += f"public:\n"

            # Constructor
            init_list = []
            args_init_list = []
            for arg in inst.args:
                match arg.type:
                    case VarargsValueType():
                        assert len(args_init_list) == 0
                        args_init_list = arg.name
                    case ValueType():
                        assert type(args_init_list) == list
                        args_init_list.append(arg.name)
                    case Type():
                        init_list.append(f"_{arg.name}({arg.name})")
                    case _:
                        assert False, f"Unknown type: {arg.type}"
            if type(args_init_list) == list:
                args_init_list = "{" + ", ".join(args_init_list) + "}"
            init_list = [f"{base}({inst.type}, {args_init_list})"] + init_list
            init_list = ", ".join(init_list)
            
            ctor_args = ", ".join(inst.format_formal_args(ir))
            code += f"  {name}({ctor_args}): {init_list} {{\n"
            for check in inst.type_checks:
                code += f"    assert({check});\n"
            code += f"  }}\n"

            code += "\n"

            # Plugins
            for plugin in self.plugins:
                code += plugin.run(inst, ir)

            code += f"}};\n"
        return {"insts": code}

class BuilderPlugin:
    def __init__(self):
        pass

    def run(self, ir):
        code = ""
        for inst in ir.insts:
            name = inst.format_name(ir)
            formal_args = ", ".join(inst.format_formal_args(ir))
            ctor_args = ", ".join([arg.name for arg in inst.args])

            code += f"{name}* {inst.format_builder_name(ir)}({formal_args}) {{\n"
            code += f"  {name}* inst = new {name}({ctor_args});\n"
            code += f"  insert(inst);\n"
            code += f"  return inst;\n"
            code += f"}}\n"
        return {"builder": code}

class CAPIPlugin:
    def __init__(self, prefix, builder_name, type_substitutions):
        self.prefix = prefix
        self.builder_name = builder_name
        self.type_substitutions = type_substitutions

    def run(self, ir):
        code = ""
        code += f"extern \"C\" {{\n"
        for inst in ir.insts:
            name = inst.format_name(ir)
            formal_args = ["void* builder"]
            build_args = []
            for arg in inst.args:
                assert not isinstance(arg.type, VarargsValueType), "CAPI does not support varargs"
                formal_args.append(f"{self.type_substitutions[arg.type]} {arg.name}")
                build_args.append(f"({arg.type.format(ir)}) {arg.name}")
            formal_args = ", ".join(formal_args)
            build_args = ", ".join(build_args)

            value_type = self.type_substitutions[ValueType()]

            code += f"{value_type} {self.prefix}_{inst.format_builder_name(ir)}({formal_args}) {{\n"
            code += f"  return ({value_type})(({self.builder_name}*)builder)->{inst.format_builder_name(ir)}({build_args});\n"
            code += f"}}\n"
        code += f"}}\n"
        return {"capi": code}

def lwir(template_path, output_path, ir, plugins):
    with open(template_path, "r") as f:
        template = f.read()

    output = template
    for plugin in plugins:
        vars = plugin.run(ir)
        for name, value in vars.items():
            output = output.replace("${" + name + "}", value)

    print(output)
    with open(output_path, "w") as f:
        f.write(output)
