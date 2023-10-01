#ifndef MEMORY_PAGE_MANAGER_H
#define MEMORY_PAGE_MANAGER_H

#include "PageManager.h"

#include <unordered_map>

namespace cpot {

template<class Page>
struct MemoryPageManager : public PageManager<Page> {
  MemoryPageManager() : n_(0) {}
  Page const *load_page(PageLoc loc) override {
    if (pages_.count(loc) == 0) {
      throw std::runtime_error("cannot load_page");
    }
    return pages_.at(loc);
  }
  Page *load_and_modify_page(PageLoc loc) override {
    if (pages_.count(loc) == 0) {
      throw std::runtime_error("cannot load_and_modify_page");
    }
    return pages_.at(loc);
  }
  void delete_page(PageLoc loc) override {
    if (pages_.count(loc) == 0) {
      throw std::runtime_error("cannot delete_page");
    }
    delete pages_.at(loc);
    pages_.erase(loc);
  }
  Page *new_page(PageLoc *location = nullptr) override {
    *location = n_++;
    Page *page = new Page();
    pages_.insert(std::make_pair(*location, page));
    return page;
  }
  void commit() override {}
  void flush() override {}
  uint64_t currentMemoryUsed() const override {
    return pages_.size() * sizeof(Page);
  }
  bool empty() const override {
    return pages_.size() == 0;
  }
  ~MemoryPageManager() override {

  }
  uint64_t n_;
  std::unordered_map<PageLoc, Page *> pages_;
};

}  // namespace cpot

#endif  // MEMORY_PAGE_MANAGER_H