from typing import List, Optional

import logging
import os

import clang.cindex

from .helpers import get_clang_builtin_include_dirs
from .struct import Struct
from .walker import Walker

log = logging.getLogger(__name__)


def build_structs(filename: str, build_commands: Optional[str] = None) -> List[Struct]:
    if not os.path.isfile(filename):
        raise ValueError(f"Path {filename} is not a file. cwd: {os.getcwd()}")

    parse_args = [
        "-std=c++17",
        # Uncomment this if you need to debug where it tries to find headers
        # "-H",
    ]

    parse_options = (
        clang.cindex.TranslationUnit.PARSE_INCOMPLETE | clang.cindex.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
    )

    # include_dirs can be figured out with this command: clang++ -E -x c++ - -v < /dev/null
    include_dirs: List[str] = []

    extra_includes, system_includes = get_clang_builtin_include_dirs()

    for include_dir in system_includes:
        parse_args.append(f"-isystem{include_dir}")

    for extra_includes in system_includes:
        parse_args.append(f"-I{include_dir}")

    # Append dir of file
    file_dir = os.path.dirname(os.path.realpath(filename))
    parse_args.append(f"-I{file_dir}")
    # Append subdir of file
    file_subdir = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(filename)), ".."))
    parse_args.append(f"-I{file_subdir}")

    # TODO: Use build_commands if available

    index = clang.cindex.Index.create()

    log.debug("Parsing translation unit")
    tu = index.parse(filename, args=parse_args, options=parse_options)
    root = tu.cursor

    for diag in tu.diagnostics:
        log.warning(diag.location)
        log.warning(diag.spelling)
        log.warning(diag.option)

    log.debug("Walking over AST")
    walker = Walker(filename)
    walker.walk(root)

    return walker.structs
