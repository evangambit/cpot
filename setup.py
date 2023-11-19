# from setuptools import Extension, setup

# setup(
#     ext_modules=[
#         Extension(
#             name="foo",  # as it would be imported
#                                # may include packages/namespaces separated by `.`

#             sources=["src/mathy/tmp.cpp"], # all sources are compiled into a single binary file
#         ),
#     ]
# )

from distutils.core import setup, Extension

# the c++ extension module
extension_mod = Extension(
	"cpot",
	sources=["src/mathy/cpot.cpp"],
	extra_compile_args=["-std=c++2a"],
)

setup(name = "cpot", ext_modules=[extension_mod])
