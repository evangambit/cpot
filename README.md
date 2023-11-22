# Cpot

A simple, single-process, zero-dependency inverted index that works for 10 million documents.

I used to use Sqlite3 for this, but found it's performance was very lacking.

## Installation

Either

```bash
python3 -m pip install git+https://github.com/evangambit/cpot
```

Or

```bash
git clone https://github.com/evangambit/cpot.git
cd cpot
rm -rf build; rm -rf dist
python3 setup.py build
python3 -m pip install .
```

## Example usage:

```python
import cpot

# Create an index.
index = cpot.Index('foo')

# Insert (token, docid) pairs.
for docid in range(1, 500_000):
	for token in range(1, 500):
		if docid % token == 0:
			index.insert(token=token, docid=docid)

# Save to disk and clear RAM.
index.flush()

# Fetch documents with tokens 2 and 3.
# Results are ordered by docid.
# Ignore documents whose docid is less than 1000.
results = index.intersect([2, 3], lower_bound=1000, limit=5)

print(results)  # [1002, 1008, 1014, 1020, 1026]
```

There are two types of indices:

1. `cpot.Index` simply stores (token, docid) pairs.

2. `cpot.KVIndex` stores (token, docid, value) pairs.

You can roughly think of the data structure backing these indices as a BTree of these tuples. When you ask for (e.g.) an intersection of a set of tokens, the results will generally be returned in sorted order.

## Tests

```
brew install googletest

sh tests/run.sh
```

