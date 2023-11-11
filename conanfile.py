from conan import ConanFile
from conan.tools.files import copy
from os import path
import shutil

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

BOOST_DISABLED_OPTIONS = [opt for opt in BOOST_ALL_OPTIONS if opt not in BOOST_ENABLED_OPTIONS]


class Eventsub(ConanFile):
    name = "Eventsub"
    requires = [
        "boost/[~1.81]",
        "openssl/[~3]",
    ]
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "openssl*:shared": True,
    }
    default_options.update({f"boost*:without_{opt}": True for opt in BOOST_DISABLED_OPTIONS})

    options = {}
    generators = "CMakeDeps", "CMakeToolchain"

    def layout(self):
        self.cpp.build.libdirs = ["lib"]
        self.cpp.build.bindirs = ["bin"]

    def requirements(self):
        self.output.warning(BOOST_DISABLED_OPTIONS)
        self.output.warning(self.default_options)

    def generate(self):
        for dep in self.dependencies.values():
            try:
                # macOS
                copy(
                    self,
                    "*.dylib",
                    dep.cpp_info.libdirs[0],
                    path.join(self.build_folder, self.cpp.build.libdirs[0]),
                    keep_path=False,
                )
                # Windows
                copy(
                    self,
                    "*.lib",
                    dep.cpp_info.libdirs[0],
                    path.join(self.build_folder, self.cpp.build.libdirs[0]),
                    keep_path=False,
                )
                copy(
                    self,
                    "*.dll",
                    dep.cpp_info.bindirs[0],
                    path.join(self.build_folder, self.cpp.build.bindirs[0]),
                    keep_path=False,
                )
                # Linux
                copy(
                    self,
                    "*.so*",
                    dep.cpp_info.libdirs[0],
                    path.join(self.build_folder, self.cpp.build.libdirs[0]),
                    keep_path=False,
                )
            except shutil.SameFileError:
                # Ignore 'same file' errors
                pass
