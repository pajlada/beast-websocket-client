#!/usr/bin/env python3

import logging
import os
import sys

from lib import MemberType, build_structs, format_code, init_clang_cindex, init_logging, temporary_file

import clang.cindex
from jinja2 import Environment, FileSystemLoader
from jinja2_workarounds import MultiLineInclude

T = clang.cindex.TypeKind

log = logging.getLogger(__name__)


def init_jinja_env() -> Environment:
    template_paths = os.path.join(os.path.dirname(os.path.realpath(__file__)), "templates")

    log.debug(f"Loading jinja templates from {template_paths}")

    env = Environment(
        loader=FileSystemLoader(template_paths),
        extensions=[MultiLineInclude],
    )
    env.globals["MemberType"] = MemberType

    return env


def main():
    init_logging()

    init_clang_cindex()

    env = init_jinja_env()

    if len(sys.argv) < 2:
        log.error(f"Missing header file argument. Usage: {sys.argv[0]} <path-to-header-file>")
        sys.exit(1)

    filename = sys.argv[1]

    structs = build_structs(filename)

    log.debug("Generate & format definitions")
    definitions = format_code("\n\n".join([struct.try_value_to_definition(env) for struct in structs]))
    log.debug("Generate & format implementations")
    implementations = format_code("\n\n".join([struct.try_value_to_implementation(env) for struct in structs]))

    with temporary_file() as (f, path):
        log.debug(f"Write definitions to {path}")
        f.write(definitions)
        f.write("\n")
        print(path)

    with temporary_file() as (f, path):
        log.debug(f"Write implementations to {path}")
        f.write(implementations)
        f.write("\n")
        print(path)


if __name__ == "__main__":
    main()
