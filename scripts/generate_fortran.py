#!/usr/bin/env python
# -* coding: utf-8 -*

import sys
import os

from fortran.parse import FunctionVisitor
from fortran.cenums import write_enums, EnumVisitor
from fortran.cdef import write_cdef
from fortran.ftypes import write_types
from fortran.interface import write_interface
from pycparser import parse_file


def usage():
    print(sys.argv[0] + " path/to/chemharp.h path/to/fortran/binding/dir")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        usage()
        sys.exit(0)
    C_HEADER = sys.argv[1]
    FORTRAN_ROOT = os.path.join(sys.argv[2], "generated")
    libc_path = os.path.join(os.path.dirname(__file__), "libc")
    ast = parse_file(C_HEADER, use_cpp=True, cpp_path="gcc",
                     cpp_args=["-E", "-I" + libc_path])

    evisitor = EnumVisitor()
    evisitor.visit(ast)

    write_enums(os.path.join(FORTRAN_ROOT, "cenums.f90"), evisitor.enums)

    fvisitor = FunctionVisitor()
    functions = fvisitor.visit(ast)

    write_cdef(os.path.join(FORTRAN_ROOT, "cdef.f90"), functions)
    write_types(os.path.join(FORTRAN_ROOT, "ftypes.f90"), functions)
    write_interface(os.path.join(FORTRAN_ROOT, "interface.f90"), functions)