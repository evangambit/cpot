#ifndef INVERTEDINDEX_H
#define INVERTEDINDEX_H

#include "SkipTree.h"
#include "DiskPageManager.h"

namespace cpot {

// typedef uint32_t RowLoc;
typedef uint64_t Token;

template<class Row>
struct ConstIterator : public IteratorInterface<Row> {
  ConstIterator(Row value) {
    this->currentValue = value;
  }
  Row skip_to(Row row) override {
    return this->currentValue;
  }
  Row next() override {
    return this->currentValue;
  }
};

template<class Row>
struct InvertedIndex {
  // Tokens that are more common than this are stored in their own tree.
  static constexpr uint64_t kRareThreshold = 50;

  struct TokenRow {
    Token token;
    uint64_t count;
    PageLoc root;
    bool operator<(const TokenRow& that) const {
      return this->token < that.token;
    }
    bool operator<=(const TokenRow& that) const {
      return this->token <= that.token;
    }
    bool operator==(const TokenRow& that) const {
      return this->token == that.token;
    }
    static TokenRow smallest() {
      return TokenRow{0, 0, 0};
    }
    static TokenRow largest() {
      return TokenRow{uint64_t(-1), 0, 0};
    }
    friend std::ostream& operator<<(std::ostream& s, TokenRow row) {
      return s << "[TokenRow token:" << row.token << " root:" << row.root << " count:" << row.count << "]";
    }
  };
  struct RareRow {
    Token token;
    Row row;
    bool operator<(const RareRow& that) const {
      if (this->token == that.token) {
        return this->row < that.row;
      }
      return this->token < that.token;
    }
    bool operator<=(const RareRow& that) const {
      if (this->token == that.token) {
        return this->row <= that.row;
      }
      return this->token <= that.token;
    }
    bool operator==(const RareRow& that) const {
      return (this->token == that.token) && (this->row == that.row);
    }
    static RareRow smallest() {
      return RareRow{0, Row::smallest()};
    }
    static RareRow largest() {
      return RareRow{uint64_t(-1), Row::largest()};
    }
    friend std::ostream& operator<<(std::ostream& s, RareRow row) {
      return s << "[RareRow token:" << row.token << " row:" << row.row << "]" << std::endl;
    }
  };
  struct RareToCommonIterator : public IteratorInterface<Row> {
    RareToCommonIterator(uint64_t token, std::shared_ptr<IteratorInterface<RareRow>> it)
    : token_(token), it_(it) {
      this->currentValue = it->currentValue.row;
    }
    Row skip_to(Row row) override {
      it_->skip_to(RareRow{token_, row});
      this->currentValue = it_->currentValue.row;
      return this->currentValue;
    }
    Row next() override {
      it_->next();
      this->currentValue = it_->currentValue.row;
      return this->currentValue;
    }
    uint64_t token_;
    std::shared_ptr<IteratorInterface<RareRow>> it_;
  };
  InvertedIndex(std::string filename)
  : headerPageManager(std::make_shared<DiskPageManager<typename SkipTree<TokenRow>::Node>>(filename + ".header")),
    pageManager(std::make_shared<DiskPageManager<typename SkipTree<Row>::Node>>(filename)),
    rarePageManager(std::make_shared<DiskPageManager<typename SkipTree<RareRow>::Node>>(filename + ".rare")) {
    if (headerPageManager->empty()) {
      this->header = std::make_unique<SkipTree<TokenRow>>(headerPageManager, -1);
      assert(this->header->rootLoc_ == 0);
      rareTree = std::make_shared<SkipTree<RareRow>>(rarePageManager, -1);
      assert(rareTree->rootLoc_ == 0);
    } else {
      this->header = std::make_unique<SkipTree<TokenRow>>(headerPageManager, 0);
      rareTree = std::make_shared<SkipTree<RareRow>>(rarePageManager, 0);
    }
  }

  // For debugging.
  InvertedIndex(
    std::shared_ptr<PageManager<typename SkipTree<Row>::Node>> pageManager,
    std::shared_ptr<PageManager<typename SkipTree<TokenRow>::Node>> headerPageManager,
    std::shared_ptr<PageManager<typename SkipTree<RareRow>::Node>> rarePageManager
  )
  : pageManager(pageManager), headerPageManager(headerPageManager), rarePageManager(rarePageManager) {
    if (headerPageManager->empty()) {
      this->header = std::make_unique<SkipTree<TokenRow>>(headerPageManager, -1);
      assert(this->header->rootLoc_ == 0);
      rareTree = std::make_shared<SkipTree<RareRow>>(rarePageManager, -1);
      assert(rareTree->rootLoc_ == 0);
    } else {
      this->header = std::make_unique<SkipTree<TokenRow>>(headerPageManager, 0);
      rareTree = std::make_shared<SkipTree<RareRow>>(rarePageManager, 0);
    }
  }

  uint64_t currentMemoryUsed() const {
    return headerPageManager->currentMemoryUsed() + pageManager->currentMemoryUsed();
  }

  void insert(Token token, Row row) {
    TokenRow *tokenRow = this->header->find_and_write(TokenRow{token, 0, 0});
    if (tokenRow == nullptr) {
      // Never-before-seen token.
      TokenRow row{token, uint64_t(1), kNullPage};
      this->header->insert(row);
      tokenRow = this->header->find_and_write(row);
      assert(tokenRow != nullptr);
      assert(tokenRow->root == kNullPage);
    } else {
      tokenRow->count += 1;
    }
    assert(tokenRow != nullptr);
    assert(tokenRow->count > 0);

    if (tokenRow->root == kNullPage) {
      RareRow rareRow{token, row};
      rareTree->insert(rareRow);
    } else {
      auto collection = this->collection(token, tokenRow->root);
      collection->insert(row);
    }

    if (tokenRow->count > kRareThreshold && tokenRow->root == kNullPage) {
      // Migrate from rare to common.
      assert(!collections.contains(token));
      auto newTree = std::make_shared<SkipTree<Row>>(this->pageManager, PageLoc(-1));
      tokenRow->root = newTree->rootLoc_;
      collections.insert(std::make_pair(token, newTree));
      std::vector<RareRow> A = rareTree->range(
        RareRow{token, Row::smallest()},
        RareRow{token, Row::largest()}
      );
      for (auto a : A) {
        newTree->insert(a.row);
      }
    }
  }

  bool remove(Token token, Row row) {
    TokenRow *tokenRow = this->header->find_and_write(TokenRow{token, 0, 0});
    if (tokenRow == nullptr) {
      return false;
    }
    if (tokenRow->root == kNullPage) {
      return rareTree->remove(RareRow{token, row});
    }
    auto collection = this->collection(token, tokenRow->root);
    return collection->remove(row);
  }

  std::vector<Row> all(Token token) {
    TokenRow const *tokenRow = this->header->find(TokenRow{token, 0, 0});
    if (tokenRow->root == kNullPage) {
      std::vector<RareRow> A = rareTree->range(
        RareRow{token, Row::smallest()},
        RareRow{token, Row::largest()}
      );
      std::vector<Row> results;
      results.reserve(A.size());
      for (auto it : A) {
        results.push_back(it.row);
      }
      return results;
    }
    return this->collection(token, tokenRow->root)->all();
  }

  // Returns rows on the interval [low, high)
  std::vector<Row> range(Token token, Row low, Row high, uint64_t reserve = uint64_t(-1)) {
    TokenRow const *tokenRow = this->header->find(TokenRow{token, 0, 0});
    if (tokenRow->root == kNullPage) {
      std::vector<RareRow> A = rareTree->range(
        RareRow{token, low},
        RareRow{token, high},
        reserve
      );
      std::vector<Row> results;
      results.reserve(A.size());
      for (auto it : A) {
        results.push_back(it.row);
      }
      return results;
    }
    return this->collection(token, tokenRow->root)->range(low, high, reserve);
  }

  std::shared_ptr<IteratorInterface<Row>> iterator(uint64_t token) {
    TokenRow const *tokenRow = this->header->find(TokenRow{token, 0, 0});
    if (tokenRow->root == kNullPage) {
      std::shared_ptr<IteratorInterface<RareRow>> it = SkipTree<RareRow>::iterator(
        rareTree,
        RareRow{token, Row::smallest()},
        RareRow{token, Row::largest()}
      );
      return std::make_shared<RareToCommonIterator>(token, it);
    } else {
      return SkipTree<Row>::iterator(this->collection(token, tokenRow->root));
    }
  }

  std::shared_ptr<IteratorInterface<Row>> iterator(uint64_t token, Row lowerBound) {
    TokenRow const *tokenRow = this->header->find(TokenRow{token, 0, 0});
    if (tokenRow == nullptr) {
      return std::make_shared<ConstIterator<Row>>(Row::largest());
    }
    if (tokenRow->root == kNullPage) {
      std::shared_ptr<IteratorInterface<RareRow>> it = SkipTree<RareRow>::iterator(
        rareTree,
        RareRow{token, lowerBound},
        RareRow{token, Row::largest()}
      );
      return std::make_shared<RareToCommonIterator>(token, it);
    } else {
      return SkipTree<Row>::iterator(this->collection(token, tokenRow->root), lowerBound, Row::largest());
    }
  }

  // Do *not* flush while you are still using iterators.
  void flush() {
    this->header->flush();
    this->pageManager->flush();
  }

  void commit() {
    this->header->commit();
    this->pageManager->commit();
  }

  uint64_t count(Token token) {
    TokenRow row{token, 0, 0};
    TokenRow const *result = this->header->find(row);
    if (result == nullptr) {
      return 0;
    }
    return result->count;
  }

  std::shared_ptr<SkipTree<Row>> collection(Token token, PageLoc root) {
    assert(root != kNullPage);
    if (!collections.contains(token)) {
      collections.insert(std::make_pair(token, std::make_shared<SkipTree<Row>>(this->pageManager, root)));
    }
    return collections.at(token);
  }

  std::shared_ptr<PageManager<typename SkipTree<TokenRow>::Node>> headerPageManager;
  std::shared_ptr<PageManager<typename SkipTree<Row>::Node>> pageManager;
  std::shared_ptr<PageManager<typename SkipTree<RareRow>::Node>> rarePageManager;

  std::unique_ptr<SkipTree<TokenRow>> header;
  std::shared_ptr<SkipTree<RareRow>> rareTree;

  std::unordered_map<Token, std::shared_ptr<SkipTree<Row>>> collections;
};

}  // namespace cpot

#endif  // INVERTEDINDEX_H
