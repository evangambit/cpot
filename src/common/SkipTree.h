#ifndef BTREE_H
#define BTREE_H

#include "PageManager.h"
#include "Iterator.h"

#include <cstdint>

#include <algorithm>
#include <sstream>
#include <cmath>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define ASSERT(condition, message) do { } while (false)
#endif

namespace cpot {

typedef uint16_t RowLoc;

constexpr PageLoc kNullPage = PageLoc(-1);

template<class Row>
struct SkipTree {
  // TODO: what should these be?
  // static constexpr int kLeafSize = 8;
  // static constexpr int kNodeSize = 8;
  // static constexpr int kMinLeafSize = 4;
  // static constexpr int kMinNodeSize = 4;
  static constexpr int kLeafSize = 32;
  static constexpr int kNodeSize = 32;
  static constexpr int kMinLeafSize = 16;
  static constexpr int kMinNodeSize = 16;

  struct Leaf {
    Row rows[kLeafSize];
  };

  struct InternalNode {
    Row rows[kNodeSize];  // the smallest value of each child
    PageLoc children[kNodeSize];
  };

  union NodeValue {
    constexpr NodeValue() {}
    Leaf leaf;
    InternalNode internal;
  };

  struct Node {
    uint16_t depth;  // 0 is a leaf
    uint16_t length;
    PageLoc self;
    PageLoc next;
    NodeValue value;
    Node() {}
    inline void assert_alive() const {
      assert(self != kNullPage);
    }
    inline bool is_leaf() const {
      return depth == 0;
    }
    Row max() const {
      if (this->is_leaf()) {
        return this->value.leaf.rows[length - 1];
      } else {
        return this->value.internal.rows[length - 1];
      }
    }
    Row min() const {
      if (this->is_leaf()) {
        return this->value.leaf.rows[0];
      } else {
        return this->value.internal.rows[0];
      }
    }
    Row const * get_row(size_t idx) const {
      assert(idx < this->length);
      if (this->is_leaf()) {
        assert(idx < kLeafSize);
        return &(this->value.leaf.rows[idx]);
      } else {
        assert(idx < kNodeSize);
        return &(this->value.internal.rows[idx]);
      }
    }
    void set_row(size_t idx, Row const *val) {
      assert(idx < this->length);
      if (this->is_leaf()) {
        assert(idx < kLeafSize);
        this->value.leaf.rows[idx] = *val;
      } else {
        assert(idx < kNodeSize);
        this->value.internal.rows[idx] = *val;
      }
    }
    inline bool is_full() const {
      return length >= (this->is_leaf() ? kLeafSize : kNodeSize);
    }
    inline bool is_too_small() const {
      return length < (this->is_leaf() ? kMinLeafSize : kMinNodeSize);
    }
  };

  SkipTree(std::shared_ptr<PageManager<Node>> pageManager, PageLoc rootLoc)
  : rootLoc_(rootLoc), pageManager_(pageManager) {
    if (rootLoc_ == kNullPage) {
      rootLoc_ = this->_create_node(kNullPage, 0)->self;
    }
  }

  ~SkipTree() {}

  void print(std::ostream& stream) {
    Node const *root = pageManager_->load_page(rootLoc_);
    for (size_t i = root->depth; i < root->depth + 1; --i) {
      this->_print_depth(stream, i);
    }
  }
  void _print_depth(std::ostream& stream, size_t depth) {
    Node const *node = pageManager_->load_page(rootLoc_);
    while (depth < node->depth) {
      assert(node->length >= 1);
      node = pageManager_->load_page(node->value.internal.children[0]);
    }

    std::unordered_set<PageLoc> locs;

    stream << "depth " << depth << std::endl;
    while (true) {
      locs.insert(node->self);
      this->_print_node(stream, node);
      if (node->next == kNullPage) {
        break;
      }
      PageLoc nextLoc = node->next;
      node = pageManager_->load_page(node->next);
      if (node->self != nextLoc) {
        std::cout << "node->self is wrong! (should be " << nextLoc << " but is " << node->self << ")" << std::endl;
        this->_print_node(stream, node);
        assert(false);
      }
      if (locs.contains(node->self)) {
        std::cout << "Cycle! " << node->self << std::endl;
        assert(false);
      }
    }
  }
  void _print_node(std::ostream& stream, Node const *node) {
    node->assert_alive();
    stream << node << "[loc=" << node->self << " next=" << node->next << " len=" << node->length << " depth=" << unsigned(node->depth) << " data:";
    if (node->is_leaf()) {
      this->_print_leaf(stream, node);
    } else {
      this->_print_inode(stream, node);
    }
  }
  void _print_leaf(std::ostream& stream, Node const *node) {
    assert(node->is_leaf());
    for (size_t i = 0; i < node->length; ++i) {
      stream << " " << node->value.leaf.rows[i];
    }
    stream << "]" << std::endl;
  }
  void _print_inode(std::ostream& stream, Node const *node) {
    assert(!node->is_leaf());
    const InternalNode& iNode = node->value.internal;
    for (size_t i = 0; i < node->length; ++i) {
      stream << " [" << iNode.rows[i] << "|" << iNode.children[i] << "]";
    }
    stream << "]" << std::endl;
  }

  Row const *find(Row query) {
    std::pair<Node const *, uint16_t> result = this->_lower_bound(query);
    if (result.first == nullptr) {
      return nullptr;
    }
    assert(result.second < result.first->length);
    Row const *row = &(result.first->value.leaf.rows[result.second]);
    return (query == *row) ? row : nullptr;
  }
  // ** USE WITH CAUTION **
  // This lets you mutate a row in-place. This is powerful, but dangerous.
  // Your new value should be considered equal to the old value, otherwise
  // you risk invalidating the tree (since the row's leaf's parents will
  // not be updated).
  Row *find_and_write(Row query) {
    std::pair<Node const *, uint16_t> result = this->_lower_bound(query);
    if (result.first == nullptr) {
      return nullptr;
    }
    if (query != result.first->value.leaf.rows[result.second]) {
      return nullptr;
    }
    Node *node = pageManager_->load_and_modify_page(result.first->self);
    return &(node->value.leaf.rows[result.second]);
  }

  std::pair<Node const *, uint16_t> _lower_bound(const Row& query) {
    assert(rootLoc_ != kNullPage);
    Node const *kRoot = pageManager_->load_page(rootLoc_);
    return this->_lower_bound(kRoot, query);
  }

  // `first->value.leaf.rows[second]` is the smallest row that is >= query
  // if no row satisifies this, then node is nullptr
  std::pair<Node const *, uint16_t> _lower_bound(Node const *node, const Row& query) {
    Row const *vals = (node->is_leaf() ? node->value.leaf.rows : node->value.internal.rows);
    Row const *end = vals + node->length;
    Row const *it = std::lower_bound(vals, end, query);
    if (node->is_leaf()) {
      if (it < end) {
        return std::make_pair(node, it - vals);
      }
      // TODO: does the fact that we need this suggest a bug in our implementation?
      if (vals[0] < query && node->next != kNullPage) {
        node = pageManager_->load_page(node->next);
        vals = (node->is_leaf() ? node->value.leaf.rows : node->value.internal.rows);
        end = vals + node->length;
        it = std::lower_bound(vals, end, query);
        if (it < end) {
          return std::make_pair(node, it - vals);
        }
      }
      return std::pair<Node const *, uint16_t>(nullptr, 0);
    }

    Node const *child;
    if (it < end) {
      if (it == vals || node->value.internal.rows[it - vals] == query) {
        child = pageManager_->load_page(node->value.internal.children[it - vals]);
      } else {
        child = pageManager_->load_page(node->value.internal.children[it - vals - 1]);
      }
    } else {
      child = pageManager_->load_page(node->value.internal.children[node->length - 1]);
    }

    std::pair<Node const *, uint16_t> r = this->_lower_bound(child, query);

    return r;
  }

  // Returns rows on the interval [low, high)
  std::vector<Row> range(Row low, Row high, uint64_t reserve = uint64_t(-1)) {
    std::vector<Row> r;
    std::pair<Node const *, uint16_t> it = this->_lower_bound(low);
    if (it.first == nullptr) {
      return r;
    }
    if (reserve != uint64_t(-1)) {
      r.reserve(reserve);
    }
    while (it.first->value.leaf.rows[0] < high) {
      for (uint16_t i = it.second; i < it.first->length; ++i) {
        if (high <= it.first->value.leaf.rows[i]) {
          break;
        }
        r.push_back(it.first->value.leaf.rows[i]);
      }
      if (it.first->next == kNullPage) {
        break;
      }
      it.first = pageManager_->load_page(it.first->next);
      it.second = 0;
    }
    return r;
  }

  // Populates "result" with the smallest rows that are
  // greater than "row". Returns the end of the array.
  Row *next(Row row, Row *result, uint64_t limit) {
    Row *end = result + limit;
    std::pair<Node const *, uint16_t> it = this->_lower_bound(row);
    if (it.first == nullptr) {
      return result;
    }
    while (result < end) {
      for (uint16_t i = it.second; i < it.first->length; ++i) {
        *(result++) = it.first->value.leaf.rows[i];
        if (result >= end) {
          break;
        }
      }
      if (it.first->next == kNullPage) {
        break;
      }
      it.first = pageManager_->load_page(it.first->next);
      it.second = 0;
    }
    return result;
  }

  std::vector<Row> all(uint64_t reserve = uint64_t(-1)) {
    std::vector<Row> r;
    if (reserve != uint64_t(-1)) {
      r.reserve(reserve);
    }

    Node const *node = pageManager_->load_page(rootLoc_);
    while (!node->is_leaf()) {
      node = pageManager_->load_page(node->value.internal.children[0]);
    }
    while (true) {
      assert(node->is_leaf());
      for (size_t i = 0; i < node->length; ++i) {
        r.push_back(node->value.leaf.rows[i]);
      }
      if (node->next == kNullPage) {
        break;
      }
      node = pageManager_->load_page(node->next);
    }
    return r;
  }

  bool insert(Row row) {
    // leaves/nodes need at least 2 children when they're too small
    // so that _handle_too_small_child can rebalance two children.
    assert(kMinLeafSize > 2);
    assert(kMinNodeSize > 2);

    Node const * const kRoot = pageManager_->load_page(rootLoc_);
    assert(kRoot->depth < 20);
    bool result = this->_insert(kRoot, row);
    assert(kRoot->length > 0);
    if (!kRoot->is_leaf()) {
      assert(kRoot->length > 1);
    }
    assert(kRoot->depth < 20);
    if (kRoot->is_full()) {
      Node *root = pageManager_->load_and_modify_page(kRoot->self);
      assert(root == kRoot);
      Node *child = this->_create_node(root->self, root->depth);

      // Copy root into a child.
      if (root->is_leaf()) {
        std::memcpy(child->value.leaf.rows, root->value.leaf.rows, sizeof(Row) * root->length);
      } else {
        std::memcpy(child->value.internal.rows, root->value.internal.rows, sizeof(Row) * root->length);
        std::memcpy(child->value.internal.children, root->value.internal.children, sizeof(PageLoc) * root->length);
        // We don't actually have to update our children's parent property
        // since _split should handle that.
      }
      child->length = root->length;

      // Clear root and add new child.
      std::fill_n(root->value.internal.children, root->length, kNullPage);
      std::fill_n(root->value.internal.rows, root->length, Row::largest());
      root->depth += 1;
      root->length = 1;
      root->value.internal.children[0] = child->self;

      this->_split(root, child, 0);

      assert(kRoot->length == 2);
    }
    if (kRoot->is_leaf()) {
      assert(kRoot->length < kLeafSize);
    } else {
      assert(kRoot->length < kNodeSize);
    }
    assert(kRoot->depth < 20);
    return result;

  }

  bool _insert(Node const *knode, Row row) {
    if (knode->is_leaf()) {
      assert(knode->depth < 20);
      bool r = this->_insert_into_leaf(knode, row);
      assert(knode->depth < 20);
      return r;
    }

    Row const *vals = knode->value.internal.rows;
    Row const *end = vals + knode->length;
    Row const *it = std::lower_bound(vals, end, row);
    size_t idx = it - vals;
    if (it > vals && (it >= end || *it != row)) {
      idx--;
    }

    Node const *kChild = pageManager_->load_page(knode->value.internal.children[idx]);

    assert(knode->depth < 20);
    bool result = this->_insert(kChild, row);
    assert(knode->depth < 20);

    if (kChild->get_row(0) != knode->get_row(idx)) {
      Node *parent = pageManager_->load_and_modify_page(knode->self);
      assert(knode->depth < 20);
      assert(parent->depth < 20);
      parent->set_row(idx, kChild->get_row(0));
      assert(knode->depth < 20);
      assert(parent->depth < 20);
    }

    if (kChild->is_full()) {
      Node *parent = pageManager_->load_and_modify_page(knode->self);
      Node *child = pageManager_->load_and_modify_page(kChild->self);
      assert(knode->depth < 20);
      assert(parent->depth < 20);
      this->_split(parent, child, idx);
      assert(knode->depth < 20);
      assert(parent->depth < 20);
    }
    assert(knode->depth < 20);
    return result;
  }

  void _split(Node *parent, Node *child, const size_t idx) {
    parent->assert_alive();
    child->assert_alive();

    if (parent->self != rootLoc_) {
      if (parent->is_leaf()) {
        assert(parent->length >= kMinLeafSize);
      } else {
        assert(parent->length >= kMinNodeSize);
      }
    }
    if (child->is_leaf()) {
      ASSERT(child->length >= kMinLeafSize, std::to_string(child->length));
    } else {
      ASSERT(child->length >= kMinNodeSize, std::to_string(child->length));
    }

    assert(child->self == parent->value.internal.children[idx]);
    Node *newChild = this->_create_node(parent->self, child->depth);

    newChild->assert_alive();

    // Split child.
    const size_t leftN = child->length / 2;
    const size_t rightN = child->length - leftN;
    assert(leftN > 0);
    assert(rightN > 0);

    if (child->is_leaf()) {
      assert(leftN < kLeafSize);
      assert(rightN < kLeafSize);
      assert(leftN >= kMinLeafSize);
      assert(rightN >= kMinLeafSize);
      std::memcpy(newChild->value.leaf.rows, child->value.leaf.rows + leftN, sizeof(Row) * rightN);
      std::fill_n(child->value.leaf.rows + leftN, rightN, Row::largest());
    } else {
      assert(leftN < kNodeSize);
      assert(rightN < kNodeSize);
      assert(leftN >= kMinNodeSize);
      assert(rightN >= kMinNodeSize);
      std::memcpy(newChild->value.internal.rows, child->value.internal.rows + leftN, sizeof(Row) * rightN);
      std::memcpy(newChild->value.internal.children, child->value.internal.children + leftN, sizeof(PageLoc) * rightN);
      std::fill_n(child->value.internal.rows + leftN, rightN, Row::largest());
      std::fill_n(child->value.internal.children + leftN, rightN, kNullPage);
    }
    child->length = leftN;
    newChild->length = rightN;
    newChild->next = child->next;
    child->next = newChild->self;

    assert(!child->is_too_small());
    assert(!newChild->is_too_small());

    // Insert new child into parent
    assert(parent->value.internal.children[idx] == child->self);
    for (size_t i = parent->length - 1; i > idx; --i) {
      parent->value.internal.children[i + 1] = parent->value.internal.children[i];
      parent->value.internal.rows[i + 1] = parent->value.internal.rows[i];
    }

    // Should be unnecessary.
    parent->value.internal.children[idx] = child->self;
    parent->value.internal.rows[idx] = child->value.leaf.rows[0];

    parent->value.internal.children[idx + 1] = newChild->self;
    parent->value.internal.rows[idx + 1] = newChild->value.leaf.rows[0];
    parent->length += 1;
  }

  bool _insert_into_leaf(Node const *knode, Row row) {
    knode->assert_alive();
    Node *node = pageManager_->load_and_modify_page(knode->self);
    assert(node == knode);
    assert(node->length + 1 <= kLeafSize);
    Row *start = node->value.leaf.rows;
    Row *end = start + node->length;
    Row *it = std::lower_bound(start, end, row);
    if (it < end && *it == row) {
      *it = row;
      return false;
    } else {
      // TODO: avoid sorting here; just shift everything manually.
      assert(node->length < kLeafSize);
      node->value.leaf.rows[node->length++] = row;
      std::sort(node->value.leaf.rows, node->value.leaf.rows + node->length);
      return true;
    }
  }

  bool remove(Row row) {
    return this->remove(row, false);
  }

  /**
   * Removes a row from the tree.
   * Returns true iff the row was found and deleted.
   */
  bool remove(Row row, bool debug) {
    Node const *kRoot = pageManager_->load_page(rootLoc_);
    kRoot->assert_alive();
    bool result = this->_remove(kRoot, row, debug);

    if (kRoot->length == 1 && !kRoot->is_leaf()) {
      Node *root = pageManager_->load_and_modify_page(kRoot->self);
      Node *child = pageManager_->load_and_modify_page(kRoot->value.internal.children[0]);

      root->length = child->length;
      root->depth = child->depth;
      root->assert_alive();
      child->assert_alive();
      if (child->is_leaf()) {
        std::memcpy(root->value.leaf.rows, child->value.leaf.rows, sizeof(Row) * child->length);
      } else {
        std::memcpy(root->value.internal.rows, child->value.internal.rows, sizeof(Row) * child->length);
        std::memcpy(root->value.internal.children, child->value.internal.children, sizeof(PageLoc) * child->length);
      }
      pageManager_->delete_page(child->self);
      child = nullptr;
    }
    return result;
  }

  bool _remove(Node const *knode, Row row, bool debug) {
    assert(!knode->is_too_small() || knode->self == rootLoc_);
    Row const *vals = knode->value.internal.rows;
    Row const *end = vals + knode->length;
    Row const *it = std::lower_bound(vals, end, row);
    size_t idx = it - vals;

    if (knode->is_leaf()) {
      if (it >= end || *it != row) {
        return false;
      }
      Node *node = pageManager_->load_and_modify_page(knode->self);
      // TODO: avoid sorting here; just shift everything manually.
      assert(node->length - 1 >= 0);
      assert(node->length - 1 < kLeafSize);
      node->value.leaf.rows[idx] = node->value.leaf.rows[--node->length];
      std::sort(node->value.leaf.rows, node->value.leaf.rows + node->length);
      return true;
    }

    if (it > vals && (it >= end || *it != row)) {
      idx--;
    }

    Node const *kChild = pageManager_->load_page(knode->value.internal.children[idx]);
    assert(*(kChild->get_row(0)) == knode->value.internal.rows[idx]);
    bool result = this->_remove(kChild, row, debug);

    if (kChild->is_too_small()) {
      assert(knode->length >= 2);
      Node *parent = pageManager_->load_and_modify_page(knode->self);
      Node *child = pageManager_->load_and_modify_page(kChild->self);
      this->_handle_too_small_child(parent, child, idx, debug);
    } else if (kChild->get_row(0) != knode->get_row(idx)) {
      Node *parent = pageManager_->load_and_modify_page(knode->self);
      parent->set_row(idx, kChild->get_row(0));
    }

    return result;
  }

  void _handle_too_small_child(Node *parent, Node *child, size_t idx, bool debug) {
    assert(!parent->is_leaf());
    assert(child->is_too_small());
    assert(parent->value.internal.children[idx] == child->self);

    Node *left = nullptr;
    Node *right = nullptr;
    if (idx != 0) {
      idx -= 1;
      left = pageManager_->load_and_modify_page(parent->value.internal.children[idx]);
      right = child;
    } else {
      left = child;
      right = pageManager_->load_and_modify_page(parent->value.internal.children[idx + 1]);
    }

    bool shouldMerge;
    if (left->is_leaf()) {
      shouldMerge = left->length + right->length < 2 * kMinLeafSize;
    } else {
      shouldMerge = left->length + right->length < 2 * kMinNodeSize;
    }

    if (shouldMerge) {
      this->_merge(parent, left, right, idx);
    } else {
      this->_rebalance(parent, left, right, idx);
    }
  }

  void _merge(Node *parent, Node *left, Node *right, size_t leftIdx) {
    assert(left->is_leaf() == right->is_leaf());
    assert(parent->value.internal.children[leftIdx] == left->self);
    if (left->is_leaf()) {
      assert(left->length < kLeafSize);
      assert(right->length < kLeafSize);
      assert(left->length + right->length < kLeafSize);
      assert(left->length + right->length >= kMinLeafSize);
      for (size_t i = 0; i < right->length; ++i) {
        left->value.leaf.rows[left->length++] = right->value.leaf.rows[i];
      }
      left->next = right->next;
      for (size_t i = leftIdx + 1; i < parent->length; ++i) {
        parent->value.internal.rows[i] = parent->value.internal.rows[i + 1];
        parent->value.internal.children[i] = parent->value.internal.children[i + 1];
      }
      parent->length -= 1;
      pageManager_->delete_page(right->self);
      right = nullptr;
    } else {
      assert(left->length + right->length < kNodeSize);
      assert(left->length + right->length >= kMinNodeSize);
      assert(left->length < kNodeSize);
      assert(right->length < kNodeSize);
      for (size_t i = 0; i < right->length; ++i) {
        left->value.internal.rows[left->length] = right->value.internal.rows[i];
        left->value.internal.children[left->length] = right->value.internal.children[i];
        left->length += 1;
      }
      left->next = right->next;
      for (size_t i = leftIdx + 1; i < parent->length; ++i) {
        parent->value.internal.rows[i] = parent->value.internal.rows[i + 1];
        parent->value.internal.children[i] = parent->value.internal.children[i + 1];
      }
      parent->length -= 1;
      pageManager_->delete_page(right->self);
      right = nullptr;
    }

    parent->value.internal.rows[leftIdx] = *(left->get_row(0));
  }

  void _rebalance(Node *parent, Node *left, Node *right, size_t leftIdx) {
    assert(left->is_leaf() == right->is_leaf());
    assert(left->next == right->self);
    const bool isLeaf = left->is_leaf();
    assert(parent->value.internal.children[leftIdx] == left->self);
    if (isLeaf) {
      assert(left->length + right->length >= 2 * kMinLeafSize);
      assert(std::min(left->length, right->length) == kMinLeafSize - 1);
      assert(std::max(left->length, right->length) >= kMinLeafSize);
    } else {
      assert(left->length + right->length >= 2 * kMinNodeSize);
      assert(std::min(left->length, right->length) == kMinNodeSize - 1);
      assert(std::max(left->length, right->length) >= kMinNodeSize);
    }

    if (left->length < right->length) {
      size_t delta = (right->length - left->length) / 2;
      assert(delta > 0);
      assert(delta < right->length);
      if (isLeaf) {
        assert(right->length - delta >= kMinLeafSize);
        assert(left->length + delta < kLeafSize);
        for (size_t i = 0; i < delta; ++i) {
          left->value.leaf.rows[left->length++] = right->value.leaf.rows[i];
          right->value.leaf.rows[i] = right->value.leaf.rows[i + delta];
        }
        for (size_t i = delta; i < right->length - delta; ++i) {
          right->value.leaf.rows[i] = right->value.leaf.rows[i + delta];
        }
        right->length -= delta;
      } else {
        assert(right->length - delta >= kMinNodeSize);
        assert(left->length + delta < kNodeSize);
        for (size_t i = 0; i < delta; ++i) {
          left->value.internal.rows[left->length] = right->value.internal.rows[i];
          left->value.internal.children[left->length] = right->value.internal.children[i];
          right->value.internal.rows[i] = right->value.internal.rows[i + delta];
          right->value.internal.children[i] = right->value.internal.children[i + delta];
          left->length += 1;
        }

        for (size_t i = delta; i < right->length - delta; ++i) {
          right->value.internal.rows[i] = right->value.internal.rows[i + delta];
          right->value.internal.children[i] = right->value.internal.children[i + delta];
        }
        right->length -= delta;
      }
    } else {
      size_t delta = (left->length - right->length) / 2;
      assert(delta > 0);
      assert(delta < left->length);
      if (isLeaf) {
        assert(left->length - delta >= kMinLeafSize);
        assert(right->length + delta < kLeafSize);
        // Make space in right leaf.
        for (size_t i = right->length - 1; i < right->length; --i) {
          right->value.leaf.rows[i + delta] = right->value.leaf.rows[i];
        }
        right->length += delta;
        // Move values from end of left leaf to beginning of right leaf.
        for (size_t i = delta - 1; i < delta; --i) {
          right->value.leaf.rows[i] = left->value.leaf.rows[--left->length];
        }
      } else {
        assert(left->length - delta >= kMinNodeSize);
        assert(right->length + delta < kNodeSize);
        // Make space in right node.
        for (size_t i = right->length - 1; i < right->length; --i) {
          right->value.internal.rows[i + delta] = right->value.internal.rows[i];
          right->value.internal.children[i + delta] = right->value.internal.children[i];
        }
        right->length += delta;
        // Move values from end of left node to beginning of right node.
        for (size_t i = delta - 1; i < delta; --i) {
          --left->length;
          right->value.internal.rows[i] = left->value.internal.rows[left->length];
          right->value.internal.children[i] = left->value.internal.children[left->length];
        }
      }
    }
    parent->value.internal.rows[leftIdx] = *(left->get_row(0));
    parent->value.internal.rows[leftIdx + 1] = *(right->get_row(0));
  }

  struct Iterator : public IteratorInterface<Row> {
    Iterator(std::shared_ptr<SkipTree> tree, Row low, Row high)
    : low(low), high(high), tree_(tree) {
      this->skip_to(low);
    }
    // Returns the smallest value that is greater than or equal to val
    Row skip_to(Row val) override {
      loc_ = tree_->_lower_bound(val);
      if (loc_.first != nullptr && loc_.first->value.leaf.rows[loc_.second] < high) {
        this->currentValue = loc_.first->value.leaf.rows[loc_.second];
      } else {
        this->currentValue = Row::largest();
      }
      return this->currentValue;
    }
    Row next() override {
      if (loc_.first == nullptr) {
        this->currentValue = Row::largest();
        return this->currentValue;
      }
      loc_.second++;
      if (loc_.second >= loc_.first->length) {
        if (loc_.first->next == kNullPage) {
          loc_.first = nullptr;
          this->currentValue = Row::largest();
          return this->currentValue;
        }
        loc_.first = tree_->pageManager_->load_page(loc_.first->next);
        loc_.second = 0;
      }
      if (loc_.first->value.leaf.rows[loc_.second] < high) {
        this->currentValue = loc_.first->value.leaf.rows[loc_.second];
      } else {
        this->currentValue = Row::largest();
      }
      return this->currentValue;
    }
    Row low, high;
    std::shared_ptr<SkipTree> tree_;
    std::pair<Node const *, uint16_t> loc_;
  };

  static std::shared_ptr<IteratorInterface<Row>> iterator(std::shared_ptr<SkipTree> tree) {
    return std::make_shared<Iterator>(tree, Row::smallest(), Row::largest());
  }

  static std::shared_ptr<IteratorInterface<Row>> iterator(std::shared_ptr<SkipTree> tree, Row low, Row high) {
    return std::make_shared<Iterator>(tree, low, high);
  }

  void commit() {
    pageManager_->commit();
  }

  void flush() {
    pageManager_->flush();
  }

  Node *_create_node(PageLoc parent, uint8_t depth) {
    PageLoc loc;
    Node *node = pageManager_->new_page(&loc);
    node->depth = depth;
    node->self = loc;
    node->next = kNullPage;
    node->length = 0;
    if (depth == 0) {
      std::fill_n(node->value.leaf.rows, kLeafSize, Row::smallest());
    } else {
      std::fill_n(node->value.internal.rows, kNodeSize, Row::smallest());
      std::fill_n(node->value.internal.children, kNodeSize, kNullPage);
    }
    return node;
  }

  PageLoc rootLoc_;
  std::shared_ptr<PageManager<Node>> pageManager_;
};

}  // namespace cpot

#endif  // BTREE_H