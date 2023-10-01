#ifndef DISK_PAGE_MANAGER_H
#define DISK_PAGE_MANAGER_H

#include "PageManager.h"

namespace cpot {

template<class Page>
struct DiskPageManager : public PageManager<Page> {
  DiskPageManager() = delete;
  DiskPageManager(const std::string& filename)
  : filename_(filename) {
    file_ = fopen(filename.c_str(), "r+");
    if (file_ == nullptr) {
      file_ = fopen(filename.c_str(), "wb");
      fclose(file_);
      file_ = fopen(filename.c_str(), "r+");
    }
    fseek(file_, 0, SEEK_END);
    numPages_ = ftell(file_) / sizeof(Page);

    std::string headerFilename = this->_header_filename();
    FILE *headerFile = fopen(headerFilename.c_str(), "r+");
    if (headerFile == nullptr) {
      headerFile = fopen(headerFilename.c_str(), "wb");
      fclose(headerFile);
      headerFile = fopen(headerFilename.c_str(), "r+");
    }

    fseek(headerFile, 0, SEEK_END);
    long fsize = ftell(headerFile);
    fseek(headerFile, 0, SEEK_SET);
    for (long i = 0; i < fsize; ++i) {
      uint64_t j = i * sizeof(PageLoc);
      uint64_t index;
      fread(&index, 1, sizeof(PageLoc), headerFile);
      availablePages_.push_back(index);
    }
    fclose(headerFile);
  }
  std::string _header_filename() const {
    return filename_ + ".dpm_header";
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
    pages_.erase(loc);
    availablePages_.push_back(loc);
    // TODO
    // I think all we can do is store that this page is free and use it for the
    // next allocation. Actually deleting the page seems impossible without storing
    // parent pointers.
  }
  Page *new_page(PageLoc *location = nullptr) override {
    if (availablePages_.size() > 0) {
      const PageLoc loc = availablePages_.back();
      availablePages_.pop_back();
      std::shared_ptr<MemoryBlock<Page>> block = std::make_shared<MemoryBlock<Page>>(loc, 1);
      pages_.insert(std::make_pair(loc, block));
      _currentMemoryUsed += sizeof(Page);
      block->page_was_modified(loc);
      if (location != nullptr) {
        *location = loc;
      }
      return &(block->data[loc - block->start]);
    }
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
    std::string headerFilename = this->_header_filename();
    FILE *headerFile = fopen(headerFilename.c_str(), "w");
    fseek(headerFile, 0, SEEK_SET);
    fwrite(&availablePages_[0], availablePages_.size(), sizeof(PageLoc), headerFile);
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
  std::vector<uint64_t> availablePages_;
  std::string filename_;
  FILE *file_;
  PageLoc numPages_;
  uint64_t _currentMemoryUsed;  // in bytes
  std::unordered_map<PageLoc, std::shared_ptr<MemoryBlock<Page>>> pages_;
};

}  // namespace cpot

#endif  // DISK_PAGE_MANAGER_H