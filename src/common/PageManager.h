#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include <cstdint>
#include <errno.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <csignal>

#include "MemoryBlock.h"

namespace cpot {

typedef uint32_t PageLoc;

template<class Page>
struct PageManager {
  virtual Page const *load_page(PageLoc location) = 0;
  virtual Page *load_and_modify_page(PageLoc location) = 0;
  virtual void delete_page(PageLoc location) = 0;
  virtual Page *new_page(PageLoc *location = nullptr) = 0;
  virtual void commit() = 0;
  virtual void flush() = 0;
  virtual uint64_t currentMemoryUsed() const = 0;
  virtual bool empty() const = 0;
  virtual ~PageManager() = default;
};

}  // namespace cpot

#endif  // PAGE_MANAGER_H
