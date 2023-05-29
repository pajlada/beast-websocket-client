from typing import Generator, Tuple

import contextlib
import logging
import os
from io import TextIOWrapper
from tempfile import mkstemp

import clang.cindex

log = logging.getLogger(__name__)


def init_clang_cindex() -> None:
    # 1. Has the LIBCLANG_LIBRARY_PATH environment variable been set? Use it

    env_path = os.environ.get("LIBCLANG_LIBRARY_PATH")
    if env_path is not None:
        log.debug(f"Setting clang library file from LIBCLANG_LIBRARY_PATH environment variable: {env_path}")
        clang.cindex.Config.set_library_file(env_path)
        return

    import ctypes.util

    autodetected_path = ctypes.util.find_library("clang")
    if autodetected_path is not None:
        log.debug(f"Autodetected clang library file: {autodetected_path}")
        clang.cindex.Config.set_library_file(autodetected_path)
        return


@contextlib.contextmanager
def temporary_file() -> Generator[Tuple[TextIOWrapper, str], None, None]:
    fd, path = mkstemp()
    log.debug(f"Opening file {path}")
    f = os.fdopen(fd, "w")

    yield (f, path)

    f.flush()
    f.close()
    log.debug(f"Closed file {path}")
