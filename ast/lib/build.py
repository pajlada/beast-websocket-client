from typing import List, Optional

import logging
import os
import sys

import clang.cindex

from .struct import Struct
from .walker import Walker

log = logging.getLogger(__name__)


def add_builtin_include_dirs(include_dirs: List[str]) -> None:
    match sys.platform:
        case "linux":
            include_dirs.append("/usr/include/c++/13.1.1")
            include_dirs.append("/usr/c++/13.1.1/x86_64-pc-linux-gnu")
            include_dirs.append("/usr/include/c++/13.1.1/backward")
            include_dirs.append("/usr/lib/clang/15.0.7/include")
            include_dirs.append("/usr/local/include")
            include_dirs.append("/usr/include")

        case platform:
            log.warn(f"No default includes added for platform {platform}")


def build_structs(filename: str, build_commands: Optional[str] = None) -> List[Struct]:
    if not os.path.isfile(filename):
        raise ValueError(f"Path {filename} is not a file. cwd: {os.getcwd()}")

    parse_args = [
        "-std=c++17",
    ]

    parse_options = (
        clang.cindex.TranslationUnit.PARSE_INCOMPLETE | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
    )

    # include_dirs can be figured out with this command: clang++ -E -x c++ - -v < /dev/null
    include_dirs: List[str] = []

    add_builtin_include_dirs(include_dirs)

    # Append dir of file
    include_dirs.append(os.path.dirname(os.path.realpath(filename)))
    # Append subdir of file
    include_dirs.append(os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(filename)), "..")))

    for include_dir in include_dirs:
        parse_args.append(f"-I{include_dir}")

    # TODO: Use build_commands if available

    index = clang.cindex.Index.create()

    log.debug("Parsing translation unit")
    tu = index.parse(filename, args=parse_args, options=parse_options)
    root = tu.cursor

    for diag in tu.diagnostics:
        log.warn(diag.location)
        log.warn(diag.spelling)
        log.warn(diag.option)

    log.debug("Walking over AST")
    walker = Walker(filename)
    walker.walk(root)

    return walker.structs
