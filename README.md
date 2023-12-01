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
index = cpot.IntPairIndex('foo')

for page in pages:
	for token in page['tokens']:
		index.insert(token=token, obj=(page['rank'], page['id']))

# Save to disk and clear RAM.
index.flush()

# Fetch documents with tokens 2 and 3.
# Results are ordered low-to-high by (rank, id).
# Skip documents whose rank is less than 100.
results = index.intersect([2, 3], lower_bound=(100, 0), limit=5)

print(results)  # [1002, 1008, 1014, 1020, 1026]
```

You can roughly think of indices as an index as a B-tree (or, equivalently, as a B-tree
for every token). When you make queries / create iterators, results are sorted in ascending order.

There are two types of indices:

1. `cpot.IntIndex` stores (token, docid) pairs.

2. `cpot.IntPairIndex` stores (token, rank, docid) tuples. This is useful when your documents have a standard order (e.g. a timestamp, page rank, etc.). Remember, results returned lowest to highest, so a *low* rank will be returned first.

## Tests

```
brew install googletest

sh tests/run.sh
```

