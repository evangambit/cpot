import u64cpot as _u64cpot
import kvcpot as _kvcpot

class KVIndex:
	def __init__(self, path):
		self.index = _kvcpot.newIndex(path)

	def insert(self, token, docid, value = 1):
		_kvcpot.insert(self.index, token, docid, 1)

	def flush(self):
		_kvcpot.flush(self.index)

	def current_memory(self):
		return _kvcpot.currentMemoryUsed(self.index)

	def fetch(self, token, lower_bound=(0, 0), limit = 10):
		docid, value = lower_bound
		return _kvcpot.fetch(self.index, token, docid, value, limit)

	def intersect(self, tokens: list, lower_bound=(0, 0), limit = 10):
		docid, value = lower_bound
		return _kvcpot.intersect(self.index, tokens, docid, value, limit)

class Index:
	def __init__(self, path):
		self.index = _u64cpot.newIndex(path)

	def insert(self, token, docid):
		_u64cpot.insert(self.index, token, docid)

	def flush(self):
		_u64cpot.flush(self.index)

	def current_memory(self):
		return _u64cpot.currentMemoryUsed(self.index)

	def fetch(self, token, lower_bound=0, limit = 10):
		return _u64cpot.fetch(self.index, token, lower_bound, limit)

	def intersect(self, tokens: list, lower_bound=0, limit = 10):
		return _u64cpot.intersect(self.index, tokens, lower_bound, limit)
