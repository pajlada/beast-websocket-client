#!/usr/bin/env python3

import logging
from tempfile import mkstemp
import sys
import os
import clang.cindex
from walker import Walker, Struct, Member, MemberType
import subprocess

T = clang.cindex.TypeKind

log = logging.getLogger()


def format_code(code: str) -> str:
    proc = subprocess.Popen(
        ["clang-format", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE
    )
    outs, errs = proc.communicate(input=code.encode(), timeout=2)
    if errs is not None:
        log.warn(errs)

    return outs.decode()


def main():
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)

    handler = logging.StreamHandler(sys.stderr)
    handler.setLevel(logging.DEBUG)
    formatter = logging.Formatter(
        "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    handler.setFormatter(formatter)
    root.addHandler(handler)

    clang.cindex.Config.set_library_file("/usr/lib/libclang.so.15.0.7")

    index = clang.cindex.Index.create()
    options = (
        clang.cindex.TranslationUnit.PARSE_INCOMPLETE
        | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
    )
    filename = sys.argv[1]
    log.info(f"Work on file '{filename}")
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
        "./src",
        "../src",  # TMP
    ]

    for include_dir in include_dirs:
        parse_args.append(f"-I{include_dir}")

    log.info("Parsing TU")
    tu = index.parse(filename, args=parse_args, options=options)
    root = tu.cursor

    log.info("Walk over nodes")
    walker = Walker(filename)
    walker.walk(root)

    for diag in tu.diagnostics:
        log.debug(diag.location)
        log.debug(diag.spelling)
        log.debug(diag.option)

    first = True

    definition_file_fd, definition_path = mkstemp()
    definition_file = os.fdopen(definition_file_fd, "w")
    implementation_file_fd, implementation_path = mkstemp()
    implementation_file = os.fdopen(implementation_file_fd, "w")

    for struct in walker.structs:
        # Header file
        definition = format_code(struct.try_value_to_definition())

        if not first:
            definition_file.write("\n")

        definition_file.write(definition)
        definition_file.write("\n")

        # Source file
        implementation = format_code(struct.try_value_to_implementation())

        if not first:
            implementation_file.write("\n")

        implementation_file.write(implementation)
        implementation_file.write("\n")

        first = False

    definition_file.flush()
    definition_file.close()

    implementation_file.flush()
    implementation_file.close()

    print(definition_path)
    print(implementation_path)


if __name__ == "__main__":
    main()
