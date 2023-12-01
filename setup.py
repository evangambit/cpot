from distutils.core import setup, Extension

import os
import numpy as np

headers = []
for path, _, fns in os.walk('src'):
  headers += [os.path.join(path, fn) for fn in fns if fn.endswith('.h')]

setup(
  name = "cpot",
  packages=['cpot'],
  ext_modules=[
    Extension(
      "ccpot",
      sources=["src/ccpot.cpp"],
      extra_compile_args=["-std=c++2a", "-O3"],
      include_dirs=[
        np.get_include(),
      ],
    ),
  ],
  install_requires=[
    'numpy',
  ],
  # Note that "headers" works, but is undocumented, (but would probably be more
  # appropriate if it wasn't).
  package_data = {
    'cpot': headers,
  },
  # headers = headers,
)
