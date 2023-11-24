import ccpot as _cpot

from enum import IntEnum

class IndexType(IntEnum):
  UInt64 = 1
  Mathy = 2

  @staticmethod
  def assert_valid(value):
    assert value in iter(IndexType)

class BaseIndex:
  def __init__(self, indexType, path):
    self.assert_valid(indexType)
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
    self.assert_valid_row(lower_bound)
    assert isinstance(limit, int)
    return _cpot.generalized_intersect(self.indexType, self.index, tokens, lower_bound, limit)

class KVIndex(BaseIndex):
  def __init__(self, path):
    super().__init__(indexType=Mathy, path=path)

  @staticmethod
  def assert_valid_row(row):
    assert len(row) == 2
    assert isinstance(row[0], int)
    assert isinstance(row[1], int)

  @staticmethod
  def smallest_row():
    return (0, 0)

class IntIndex(BaseIndex):
  def __init__(self, path):
    super().__init__(indexType=UInt64, path=path)

  @staticmethod
  def assert_valid_row(row):
    assert isinstance(row, int)

  @staticmethod
  def smallest_row():
    return 0
