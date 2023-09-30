# Implementation Details

## SkipTree

```
template<class Row>
class SkipTree {
 public:
  SkipTree(std::shared_ptr<PageManager<Node>> pageManager, PageLoc rootLoc);

  // Finds and returns a row that is equal to the query.
  Row const *find(Row query);

  bool insert(Row row);

  bool remove(Row row);

  // ** USE WITH CAUTION **
  // This lets you mutate a row in-place. This is powerful, but dangerous.
  // Your new value should be considered equal to the old value, otherwise
  // you risk invalidating the tree.
  Row *find_and_write(Row query);

  // Convenience method for retrieving all tokens between low (inclusive) and
  // high (exclusive).
  std::vector<Row> range(Row low, Row high, uint64_t reserve = uint64_t(-1));

  static std::shared_ptr<IteratorInterface<Row>> iterator(std::shared_ptr<SkipTree> tree, Row low, Row high);

  void commit();

  void flush();

  PageLoc rootLoc_;
  std::shared_ptr<PageManager<Node>> pageManager_;
};
```

The SkipTree here


## PageManager

```
template<class Page>
class PageManager {
 public:
  // Loads a page from memory.
  Page const *load_page(PageLoc location);

  // Loads a page from memory and marks it as mutated.
  Page *load_and_modify_page(PageLoc location);

  // Deletes a page.
  void delete_page(PageLoc location);

  // Requests a new page.
  Page *new_page(PageLoc *location);

  // Persists the changes.
  void commit();

  // Persists the changes and flushes the cache.
  void flush();

  // Returns the number of bytes currently used by the cache.
  uint64_t currentMemoryUsed() const;

  // Whether the PageManager has zero pages.
  bool empty() const;
};
```

PageManager is an abstraction requesting memory. The real-world implementation is the DiskPageManager which is responsible for fetching pages off of disk and writing them back (if they are actually modified).

