#ifndef DISK_PAGE_MANAGER_H
#define DISK_PAGE_MANAGER_H

#include "PageManager.h"

template<class Page>
struct DiskPageManager : public PageManager<Page> {
  DiskPageManager() = delete;
  DiskPageManager(const std::string& filename) {
    file = fopen(filename.c_str(), "r+");
    if (file == nullptr) {
      file = fopen(filename.c_str(), "wb");
      fclose(file);
      file = fopen(filename.c_str(), "r+");
    }
    fseek(file, 0, SEEK_END);
    numPages = ftell(file) / sizeof(Page);
  }
  Page const *load_page(PageLoc loc) override {
    return this->_load_page(loc);
  }
  Page *_load_page(PageLoc loc) {
    #ifndef NDEBUG
    if (loc >= numPages) {
      std::cout << "trying to fetch page " << loc << std::endl;
      std::raise(SIGSEGV);
    }
    #endif
    if (!pages.contains(loc)) {
      std::shared_ptr<MemoryBlock<Page>> block = std::make_shared<MemoryBlock<Page>>(loc, 1);
      fseek(file, loc * sizeof(Page), SEEK_SET);
      fread(block->data, sizeof(Page), 1, file);
      pages.insert(std::make_pair(loc, block));
      _currentMemoryUsed += sizeof(Page);
    }
    std::shared_ptr<MemoryBlock<Page>> block = pages.at(loc);
    return &(block->data[loc - block->start]);
  }
  Page *load_and_modify_page(PageLoc loc) override {
    #ifndef NDEBUG
    if (loc >= numPages) {
      std::cout << "trying to fetch page " << loc << std::endl;
      std::raise(SIGSEGV);
    }
    #endif
    Page *page = this->_load_page(loc);
    pages.at(loc)->page_was_modified(loc);
    return page;
  }
  void delete_page(PageLoc loc) override {
    if (loc >= numPages) {
      std::cout << loc << std::endl;
      assert(false);
    }
    // TODO
    // I think all we can do is store that this page is free and use it for the
    // next allocation. Actually deleting the page seems impossible without storing
    // parent pointers.
  }
  Page *new_page(PageLoc *location = nullptr) override {
    const PageLoc loc = numPages++;
    std::shared_ptr<MemoryBlock<Page>> block = std::make_shared<MemoryBlock<Page>>(loc, 1);
    pages.insert(std::make_pair(loc, block));
    _currentMemoryUsed += sizeof(Page);
    block->page_was_modified(loc);
    if (location != nullptr) {
      *location = loc;
    }
    return &(block->data[loc - block->start]);
  }
  void commit() override {
    for (auto it = pages.begin(); it != pages.end(); ++it) {
      it->second->write(file);
    }
  }
  void flush() override {
    this->commit();
    pages.clear();
    _currentMemoryUsed = 0;
  }
  uint64_t currentMemoryUsed() const override {
    return _currentMemoryUsed;
  }
  bool empty() const override {
    return this->numPages == 0;
  }
  ~DiskPageManager() override {
    this->flush();
    fclose(file);
  }
  FILE *file;
  PageLoc numPages;
  uint64_t _currentMemoryUsed;  // in bytes
  std::unordered_map<PageLoc, std::shared_ptr<MemoryBlock<Page>>> pages;
};

#endif  // DISK_PAGE_MANAGER_H