#!/usr/bin/env python3

import logging

from lib import get_clang_builtin_include_dirs, init_logging

log = logging.getLogger(__name__)


def main():
    init_logging()

    quote_includes, angle_includes = get_clang_builtin_include_dirs()

    print(f"Quote includes: {quote_includes}")
    print(f"Angle includes: {angle_includes}")


if __name__ == "__main__":
    main()
