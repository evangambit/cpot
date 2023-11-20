from distutils.core import setup, Extension

setup(
  name = "cpot",
  packages=['cpot'],
  ext_modules=[
  Extension(
    "kvcpot",
    sources=["src/mathy/kvcpot.cpp"],
    extra_compile_args=["-std=c++2a"],
  ),
  Extension(
    "u64cpot",
    sources=["src/uint64/u64cpot.cpp"],
    extra_compile_args=["-std=c++2a"],
  ),
])
