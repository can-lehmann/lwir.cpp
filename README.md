# LWIR.cpp

Lightweight intermediate representation generator.
Do you need a quick intermediate representation, but don't want to setup an entire MLIR dialect?
LWIR might be the solution for you!
(You definitly don't want to use this for anything serious, go use MLIR for that, I hear its really good.)

**Warning** LWIR is still in early development.

IRs are specified in a python script which generates a C++ header:

```python3
lwir(
    template_path = "arith.tmpl.hpp",
    output_path = "arith.hpp",
    ir = IR(
        insts = [
            Inst("Add",
                args=[Arg("a"), Arg("b")],
                type="a->type()",
                type_checks=["a->type() == b->type()"]
            ),
            Inst("ConstInt",
                args=[Arg("value", Type("int"))],
                type="Type::Int" 
            )
        ],
    ),
    plugins = [
        InstPlugin([
            InstConstructorPlugin(),
            InstGetterPlugin(),
            InstWritePlugin()
        ]),
        BuilderPlugin()
    ]
)
```

## License

Copyright 2025 Can Joshua Lehmann

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
