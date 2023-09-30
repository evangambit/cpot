#ifndef DISK_ARRAY_H
#define DISK_ARRAY_H

#include <cstdint>
#include <string>

#include "PageManager.h"

namespace cpot {

// This class is provided for illustration purposes.
template<class T>
struct DiskArray {
  DiskArray(std::shared_ptr<PageManager<T>> pageManager) : pageManager(pageManager) {}
  DiskArray() = delete;

  void append(T val) {
    *pageManager.new_page() = val;
  }

  T operator[](uint64_t idx) {
    return *pageManager.load_page(idx);
  }

  T* operator()(uint64_t idx) {
    return pageManager.load_and_modify_page(idx);
  }

  void save() {
    pageManager.write();
  }

  void load_to_memory() {
    pageManager.load_pages(0, pageManager.numPages);
  }

  uint64_t length() const {
    return pageManager.numPages;
  }

  std::shared_ptr<PageManager<T>> pageManager;
};

}  // namespace cpot

#endif  // DISK_ARRAY_H