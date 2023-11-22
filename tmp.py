import cpot

import time

t0 = time.time()
index = cpot.KVIndex('foo')
t1 = time.time()
for docid in range(1, 500_000):
	for token in range(1, 500):
		if docid % token == 0:
			index.insert(token, docid, 1)
index.flush()
t2 = time.time()
index.generalized_intersect([(2, False), (3, True)], limit=10_000)
t3 = time.time()

print('%.3f' % (t1 - t0))
print('%.3f' % (t2 - t1))  # 280k tokens / second
print('%.3f' % (t3 - t2))

# token = 20

# page1 = index.fetch(token=token, limit=10)
# print(page1)

# page2 = index.fetch(token=token, limit=10, lower_bound=(
# 	page1[-1][0],
# 	page1[-1][1] + 1,
# ))
# print(page2)

# print(index.intersect([2, 3, 5, 7], limit=100))
