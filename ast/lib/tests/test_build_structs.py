from lib import build_structs, init_clang_cindex

import pytest


def init_clang():
    init_clang_cindex()


def test_ok():
    structs = build_structs("lib/tests/resources/simple.hpp")

    assert len(structs) == 1

    s = structs[0]

    assert len(s.members) == 3

    assert s.members[0].name == "a"
    assert s.members[0].type_name == "int"
    assert s.members[1].name == "b"
    assert s.members[1].type_name == "bool"
    assert s.members[2].name == "c"
    assert s.members[2].type_name == "char"


init_clang()
