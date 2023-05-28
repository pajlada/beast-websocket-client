#!/usr/bin/env python3

import sys
import clang.cindex
from walker import Walker, Struct, Member, MemberType

T = clang.cindex.TypeKind


def main():
    clang.cindex.Config.set_library_file("/usr/lib/libclang.so.15.0.7")

    index = clang.cindex.Index.create()
    options = (
        clang.cindex.TranslationUnit.PARSE_INCOMPLETE
        | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
    )
    filename = sys.argv[1]
    parse_args = [
        "-std=c++17",
    ]

    # include_dirs can be figured out with this command: clang++ -E -x c++ - -v < /dev/null
    include_dirs = [
        "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/13.1.1/../../../../include/c++/13.1.1",
        "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/13.1.1/../../../../include/c++/13.1.1/x86_64-pc-linux-gnu",
        "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/13.1.1/../../../../include/c++/13.1.1/backward",
        "/usr/lib/clang/15.0.7/include",
        "/usr/local/include",
        "/usr/include",
    ]

    for include_dir in include_dirs:
        parse_args.append(f"-I{include_dir}")

    tu = index.parse(filename, args=parse_args, options=options)
    root = tu.cursor

    walker = Walker(filename)
    walker.walk(root)

    expected_structs = [
        Struct.with_members(
            "InnerFoo",
            [
                Member(
                    "asd",
                    MemberType.BASIC,
                    "int",
                ),
            ],
        ),
        Struct.with_members(
            "Foo",
            [
                Member(
                    "test",
                    MemberType.BASIC,
                    "int",
                ),
            ],
        ),
        Struct.with_members(
            "S",
            [
                Member(
                    "a",
                    MemberType.BASIC,
                    "Foo",
                ),
                Member(
                    "as",
                    MemberType.VECTOR,
                    "Foo",
                ),
                Member(
                    "ab",
                    MemberType.OPTIONAL,
                    "Foo",
                ),
                Member(
                    "b",
                    MemberType.BASIC,
                    "int",
                ),
            ],
        ),
    ]

    # if walker.structs != expected_structs:
    #     print("ERROR - struct result was bad")
    #     print(f"got {walker.structs}")
    #     print(f"expected {expected_structs}")

    for diag in tu.diagnostics:
        print(diag.location)
        print(diag.spelling)
        print(diag.option)

    for struct in walker.structs:
        print(struct)
        print(struct.try_value_to())


if __name__ == "__main__":
    main()
