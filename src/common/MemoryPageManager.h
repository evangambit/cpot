#ifndef MEMORY_PAGE_MANAGER_H
#define MEMORY_PAGE_MANAGER_H

#include "PageManager.h"

#include <unordered_map>

namespace cpot {

template<class Page>
struct MemoryPageManager : public PageManager<Page> {
  MemoryPageManager() {}
  Page const *load_page(PageLoc loc) override {
    if (loc >= pages.size()) {
      throw std::runtime_error("");
    }
    return pages.at(loc);
  }
  Page *load_and_modify_page(PageLoc loc) override {
    if (loc >= pages.size()) {
      throw std::runtime_error("");
    }
    return pages.at(loc);
  }
  void delete_page(PageLoc loc) override {
    if (loc >= pages.size()) {
      throw std::runtime_error("");
    }
    // TODO
  }
  Page *new_page(PageLoc *location = nullptr) override {
    *location = pages.size();
    Page *page = new Page();
    pages.insert(std::make_pair(*location, page));
    return page;
  }
  void commit() override {}
  void flush() override {}
  uint64_t currentMemoryUsed() const override {
    return pages.size() * sizeof(Page);
  }
  bool empty() const override {
    return pages.size() == 0;
  }
  ~MemoryPageManager() override {

  }
  std::unordered_map<PageLoc, Page *> pages;
};

}  // namespace cpot

#endif  // MEMORY_PAGE_MANAGER_H