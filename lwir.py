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

class ValueType(BaseType):
    def __init__(self):
        super().__init__()

    def is_value(self):
        return True

    def format(self, ir):
        return ir.value_type + "*"

class Arg:
    def __init__(self, name, type=None):
        self.name = name
        self.type = type or ValueType()

class Inst:
    def __init__(self, name, args, type, type_checks):
        self.name = name
        self.args = args
        self.type = type
        self.type_checks = type_checks

    def format_name(self, ir):
        return self.name + ir.inst_suffix

    def format_builder_name(self, ir):
        return "build_" + self.name.lower()

class IR:
    def __init__(self, insts, inst_suffix="Inst", inst_base="Inst", value_type="Value"):
        self.insts = insts
        self.inst_suffix = inst_suffix
        self.inst_base = inst_base
        self.value_type = value_type

class InstPlugin:
    def __init__(self):
        pass

    def run(self, ir):
        code = ""
        for inst in ir.insts:
            name = inst.name + ir.inst_suffix

            code += f"class {name} final: public {ir.inst_base} {{\n"
            code += f"private:\n"

            # Attributes
            for arg in inst.args:
                if not arg.type.is_value():
                    code += f"  {arg.type.format(ir)} _{arg.name};"

            code += f"public:\n"

            # Constructor
            ctor_args = []
            init_list = []
            args_init_list = []
            for arg in inst.args:
                ctor_args.append(f"{arg.type.format(ir)} {arg.name}")
                if arg.type.is_value():
                    args_init_list.append(arg.name)
                else:
                    ctor_args.append(f"_{arg.name}({arg.name})")
            args_init_list = ", ".join(args_init_list)
            init_list = [f"{ir.inst_base}({inst.type}, {{{args_init_list}}})"] + init_list
            ctor_args = ", ".join(ctor_args)
            init_list = ", ".join(init_list)
            code += f"  {name}({ctor_args}): {init_list} {{\n"
            for check in inst.type_checks:
                code += f"    assert({check});\n"
            code += f"  }}\n"

            code += "\n"

            # Getters
            for arg in inst.args:
                if not arg.type.is_value():
                    code += f"  {arg.type.format(ir)} {arg.name}() const {{ return _{arg.name}; }}\n"
            code += f"}};\n"
        return {"insts": code}

class BuilderPlugin:
    def __init__(self):
        pass

    def run(self, ir):
        code = ""
        for inst in ir.insts:
            name = inst.format_name(ir)
            formal_args = []
            ctor_args = []
            for arg in inst.args:
                formal_args.append(f"{arg.type.format(ir)} {arg.name}")
                ctor_args.append(arg.name)
            formal_args = ", ".join(formal_args)
            ctor_args = ", ".join(ctor_args)

            code += f"{name}* {inst.format_builder_name(ir)}({formal_args}) {{\n"
            code += f"  {name}* inst = new {name}({ctor_args});\n"
            code += f"  insert(inst);\n"
            code += f"  return inst;\n"
            code += f"}}\n"
        return {"builder": code}

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
