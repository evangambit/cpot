import cpot

class Index:
	def __init__(self, path):
		self.index = cpot.newIndex(path)

	def insert(self, token, docid, value = 1):
		cpot.insert(self.index, token, docid, 1)

	def flush(self):
		cpot.flush(self.index)

	def current_memory(self):
		return cpot.currentMemoryUsed()

	def fetch(self, token, lower_bound=(0, 0), limit = 10):
		docid, value = lower_bound
		return cpot.fetch(self.index, token, docid, value, limit)

	def intersect(self, tokens: list, lower_bound=(0, 0), limit = 10):
		docid, value = lower_bound
		return cpot.intersect(self.index, tokens, docid, value, limit)

index = Index('foo')
for docid in range(1, 100_000):
	for token in range(1, 100):
		if docid % token == 0:
			index.insert(token, docid, 1)
index.flush()

token = 20

page1 = index.fetch(token=token, limit=10)
print(page1)

page2 = index.fetch(token=token, limit=10, lower_bound=(
	page1[-1][0],
	page1[-1][1] + 1,
))
print(page2)

print(index.intersect([2, 3, 5, 7], limit=100))
