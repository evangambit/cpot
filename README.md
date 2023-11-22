# Cpot

A simple, single-process, zero-dependency inverted index that works for 10 million documents.

## Installation

```
git clone https://github.com/evangambit/cpot.git
rm -rf build
rm -rf dist
python3 setup.py build
python3 -m pip install .
```

## Tests

```
brew install googletest

sh tests/run.sh
```

