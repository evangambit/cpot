import cpot


index = cpot.KVIndex('foo')
for docid in range(1, 100_000):
	for token in range(1, 100):
		if docid % token == 0:
			index.insert(token, docid, 1)
index.flush()

# token = 20

# page1 = index.fetch(token=token, limit=10)
# print(page1)

# page2 = index.fetch(token=token, limit=10, lower_bound=(
# 	page1[-1][0],
# 	page1[-1][1] + 1,
# ))
# print(page2)

# print(index.intersect([2, 3, 5, 7], limit=100))
