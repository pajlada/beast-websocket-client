from conan import ConanFile
from conan.tools.files import copy
from os import path
from os import listdir

BOOST_ALL_OPTIONS = [
    "atomic",
    "chrono",
    "container",
    "context",
    "contract",
    "coroutine",
    "date_time",
    "exception",
    "fiber",
    "filesystem",
    "graph",
    "graph_parallel",
    "iostreams",
    "json",
    "locale",
    "log",
    "math",
    "mpi",
    "nowide",
    "program_options",
    "python",
    "random",
    "regex",
    "serialization",
    "stacktrace",
    "system",
    "test",
    "thread",
    "timer",
    "type_erasure",
    "url",
    "wave",
]

BOOST_ENABLED_OPTIONS = {
    "container",
    "json",
    "system",
}

BOOST_DISABLED_OPTIONS = [
    opt for opt in BOOST_ALL_OPTIONS if opt not in BOOST_ENABLED_OPTIONS
]


class BeastWebsocketClient(ConanFile):
    name = "BeastWebsocketClient"
    requires = "boost/1.81.0"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "with_openssl3": False,
        "openssl*:shared": True,
    }
    default_options.update(
        {f"boost*:without_{opt}": True for opt in BOOST_DISABLED_OPTIONS}
    )

    options = {
        # Qt is built with OpenSSL 3 from version 6.5.0 onwards
        "with_openssl3": [True, False],
    }
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.output.warning(BOOST_DISABLED_OPTIONS)
        self.output.warning(self.default_options)
        if self.options.get_safe("with_openssl3", False):
            self.requires("openssl/3.1.0")
        else:
            self.requires("openssl/1.1.1t")

    def generate(self):
        copy_bin = lambda dep, selector, subdir: copy(
            self,
            selector,
            dep.cpp_info.bindirs[0],
            path.join(self.build_folder, subdir),
            keep_path=False,
        )
        for dep in self.dependencies.values():
            print(listdir(path.join(self.builder_folder, "bin")))
            print(listdir())
            # macOS
            copy_bin(dep, "*.dylib", "bin")
            # Windows
            copy_bin(dep, "*.dll", "bin")
            # Linux
            copy(
                self,
                "*.so*",
                dep.cpp_info.libdirs[0],
                path.join(self.build_folder, "bin"),
                keep_path=False,
            )
