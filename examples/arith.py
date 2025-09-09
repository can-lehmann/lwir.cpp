import sys, os
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__) + "/.."))

from lwir import *

def binop(name):
    return Inst(name,
        args=[Arg("a"), Arg("b")],
        type="a->type()",
        type_checks=["a->type() == b->type()"]
    )

lwir(
    template_path = "arith.tmpl.hpp",
    output_path = "arith.hpp",
    ir = IR(
        insts = [
            binop("Add"),
            binop("Sub"),
            binop("Mul"),
            Inst("Select",
                args=[Arg("cond"), Arg("a"), Arg("b")],
                type="a->type()",
                type_checks=["cond->type() == Type::Bool", "a->type() == b->type()"]
            ),
            Inst("ConstInt",
                args=[Arg("value", Type("int"))],
                type="Type::Int"
            ),
            Inst("ConstBool",
                args=[Arg("value", Type("bool"))],
                type="Type::Bool"
            )
        ],
    ),
    plugins = [
        InstPlugin([
            InstGetterPlugin(),
            InstWritePlugin()
        ]),
        BuilderPlugin()
    ]
)
