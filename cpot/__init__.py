import ccpot as _cpot

from enum import IntEnum

class IndexType(IntEnum):
  UInt64Index = 1
  UInt32PairIndex = 2
  UInt64KeyValueIndex = 3

  @staticmethod
  def assert_valid(value):
    assert value in iter(IndexType)

class BaseIndex:
  def __init__(self, indexType, path):
    IndexType.assert_valid(indexType)
    self.indexType = indexType
    self.index = _cpot.newIndex(self.indexType, path)

  @staticmethod
  def assert_valid_row(row):
    assert False

  @staticmethod
  def smallest_row():
    assert False

  def insert(self, token, obj):
    _cpot.insert(self.indexType, self.index, token, obj)

  def remove(self, token, obj):
    return _cpot.remove(self.indexType, self.index, token, obj)

  def count(self, token):
    return _cpot.count(self.indexType, self.index, token)

  def flush(self):
    _cpot.flush(self.indexType, self.index)

  def current_memory(self):
    return _cpot.currentMemoryUsed(self.indexType, self.index)

  def intersect(self, tokens: list, lower_bound=None, limit = 10):
    if lower_bound is None:
      lower_bound = self.smallest_row()
    for token in tokens:
      assert isinstance(token, int)
    self.assert_valid_row(lower_bound)
    assert isinstance(limit, int)
    return _cpot.intersect(self.indexType, self.index, tokens, lower_bound, limit)

  def generalized_intersect(self, tokens: list, lower_bound=None, limit = 10):
    if lower_bound is None:
      lower_bound = self.smallest_row()
    for token in tokens:
      assert len(token) == 2
      assert isinstance(token[0], int)
      assert isinstance(token[1], bool)
    assert sum(1 - t[1] for t in tokens) > 0, 'Must have at least one non-negated token'
    self.assert_valid_row(lower_bound)
    assert isinstance(limit, int)
    return _cpot.generalized_intersect(self.indexType, self.index, tokens, lower_bound, limit)

  def token_iterator(self, token, lower_bound = None):
    if lower_bound is None:
      lower_bound = self.smallest_row()
    assert isinstance(token, int)
    self.assert_valid_row(lower_bound)
    return _cpot.token_iterator(self.indexType, self.index, token, lower_bound)

  def generalized_intersection_iterator(self, iterators):
    assert len(iterators) > 0
    for iterator in iterators:
      assert isinstance(iterator, tuple)
      assert isinstance(iterator[1], bool)
    assert sum(1 - t[1] for t in iterators) > 0, 'Must have at least one non-negated token'
    return _cpot.generalized_intersection_iterator(self.indexType, iterators)

  def union_iterator(self, iterators):
    return _cpot.union_iterator(self.indexType, iterators)

  def empty_iterator(self):
    return _cpot.empty_iterator(self.indexType)

  def fetch_many(self, iterator, limit: int):
    assert isinstance(limit, int)
    return _cpot.fetch_many(self.indexType, iterator, limit)

class UInt32PairIndex(BaseIndex):
  def __init__(self, path):
    super().__init__(indexType=IndexType.UInt32PairIndex, path=path)

  @staticmethod
  def assert_valid_row(row):
    assert len(row) == 2
    assert isinstance(row[0], int)
    assert isinstance(row[1], int)

  @staticmethod
  def smallest_row():
    return (0, 0)

class UInt64KeyValueIndex(BaseIndex):
  def __init__(self, path):
    super().__init__(indexType=IndexType.UInt64KeyValueIndex, path=path)

  @staticmethod
  def assert_valid_row(row):
    assert len(row) == 2
    assert isinstance(row[0], int)
    assert isinstance(row[1], int)

  @staticmethod
  def smallest_row():
    return (0, 0)

  def kv_union(self, tokens):
    return _cpot.kv_union(self.indexType, self.index, tokens)

class UInt64Index(BaseIndex):
  def __init__(self, path):
    super().__init__(indexType=IndexType.UInt64Index, path=path)

  @staticmethod
  def assert_valid_row(row):
    assert isinstance(row, int)

  @staticmethod
  def smallest_row():
    return 0
