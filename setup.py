import sys
from distutils.core import setup, Extension

libraries = ["toxcore", "toxav", "toxdns", "sodium", "vpx", "opus"]

if sys.platform != "darwin":
    libraries.append("rt")

setup(
    name         = "pytoxcore",
    version      = "0.2.3",
    description  = "Python binding for ToxCore",
    author       = "Anton Batenev",
    author_email = "antonbatenev@yandex.ru",
    url          = "http://github.com/abbat/pytoxcore",
    license      = "GPL",
    ext_modules  = [
        Extension(
            "pytoxcore",
            sources            = ["pytox.c", "pytoxcore.c", "pytoxav.c", "pytoxdns.c"],
            define_macros      = [],
            include_dirs       = ["/usr/tox/include"],
            library_dirs       = ["/usr/tox/lib"],
            extra_compile_args = ["-Wall", "-Werror", "-Wno-declaration-after-statement"],
            libraries          = libraries
        )
    ]
)
