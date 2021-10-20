from glob import glob
from setuptools import setup

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir

import sys

__version__ = "0.0.1"

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

include_dirs = [ "/usr/src/jetson_multimedia_api/include"
    , "/usr/src/jetson_multimedia_api/include/libjpeg-8b" ]
        

library_dirs = [ "/usr/local/cuda/lib64"
        , "/usr/lib/aarch64-linux-gnu/tegra"]

libraries = [ "nvargus_socketclient"
    , "nvjpeg"
    , "EGL"
    , "nvbuf_utils" ]


nvidia_source = [ "/usr/src/jetson_multimedia_api/samples/common/classes/NvJpegEncoder.cpp",
        "/usr/src/jetson_multimedia_api/samples/common/classes/NvElement.cpp",
        "/usr/src/jetson_multimedia_api/samples/common/classes/NvElementProfiler.cpp",
        "/usr/src/jetson_multimedia_api/samples/common/classes/NvLogging.cpp",
        ]

sources = list(glob("src/*.cpp"))
sources.extend(nvidia_source)

ext_modules = [
    Pybind11Extension("jepture",
        sources,
        #["src/main.cpp"],
        # Example: passing in the version to the compiled code
        define_macros = [('VERSION_INFO', __version__)],
        include_dirs = include_dirs,
        libraries = libraries,
        library_dirs = library_dirs
        ),
]

setup(
    name="jepture",
    version=__version__,
    author="Mees Delzenne",
    author_email="mees.delzenne@gmail.com",
    # url="https://github.com/pybind/python_example",
    description="A",
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    # Currently, build_ext only provides an optional "highest supported C++
    # level" feature, but in the future it may provide more features.
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
