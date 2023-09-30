#ifndef DISK_PAGE_MANAGER_H
#define DISK_PAGE_MANAGER_H

#include "PageManager.h"

namespace cpot {

template<class Page>
struct DiskPageManager : public PageManager<Page> {
  DiskPageManager() = delete;
  DiskPageManager(const std::string& filename) {
    file_ = fopen(filename.c_str(), "r+");
    if (file_ == nullptr) {
      file_ = fopen(filename.c_str(), "wb");
      fclose(file_);
      file_ = fopen(filename.c_str(), "r+");
    }
    fseek(file_, 0, SEEK_END);
    numPages_ = ftell(file_) / sizeof(Page);
  }
  Page const *load_page(PageLoc loc) override {
    return this->_load_page(loc);
  }
  Page *_load_page(PageLoc loc) {
    #ifndef NDEBUG
    if (loc >= numPages_) {
      std::cout << "trying to fetch page " << loc << std::endl;
      std::raise(SIGSEGV);
    }
    #endif
    if (!pages_.contains(loc)) {
      std::shared_ptr<MemoryBlock<Page>> block = std::make_shared<MemoryBlock<Page>>(loc, 1);
      fseek(file_, loc * sizeof(Page), SEEK_SET);
      fread(block->data, sizeof(Page), 1, file_);
      pages_.insert(std::make_pair(loc, block));
      _currentMemoryUsed += sizeof(Page);
    }
    std::shared_ptr<MemoryBlock<Page>> block = pages_.at(loc);
    return &(block->data[loc - block->start]);
  }
  Page *load_and_modify_page(PageLoc loc) override {
    #ifndef NDEBUG
    if (loc >= numPages_) {
      std::cout << "trying to fetch page " << loc << std::endl;
      std::raise(SIGSEGV);
    }
    #endif
    Page *page = this->_load_page(loc);
    pages_.at(loc)->page_was_modified(loc);
    return page;
  }
  void delete_page(PageLoc loc) override {
    if (loc >= numPages_) {
      std::cout << loc << std::endl;
      assert(false);
    }
    // TODO
    // I think all we can do is store that this page is free and use it for the
    // next allocation. Actually deleting the page seems impossible without storing
    // parent pointers.
  }
  Page *new_page(PageLoc *location = nullptr) override {
    const PageLoc loc = numPages_++;
    std::shared_ptr<MemoryBlock<Page>> block = std::make_shared<MemoryBlock<Page>>(loc, 1);
    pages_.insert(std::make_pair(loc, block));
    _currentMemoryUsed += sizeof(Page);
    block->page_was_modified(loc);
    if (location != nullptr) {
      *location = loc;
    }
    return &(block->data[loc - block->start]);
  }
  void commit() override {
    for (auto it = pages_.begin(); it != pages_.end(); ++it) {
      it->second->write(file_);
    }
  }
  void flush() override {
    this->commit();
    pages_.clear();
    _currentMemoryUsed = 0;
  }
  uint64_t currentMemoryUsed() const override {
    return _currentMemoryUsed;
  }
  bool empty() const override {
    return this->numPages_ == 0;
  }
  ~DiskPageManager() override {
    this->flush();
    fclose(file_);
  }
  FILE *file_;
  PageLoc numPages_;
  uint64_t _currentMemoryUsed;  // in bytes
  std::unordered_map<PageLoc, std::shared_ptr<MemoryBlock<Page>>> pages_;
};

}  // namespace cpot

#endif  // DISK_PAGE_MANAGER_H