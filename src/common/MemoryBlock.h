#ifndef MEMORY_BLOCK_H
#define MEMORY_BLOCK_H

#include <cstdint>
#include <fstream>

namespace cpot {

template<class Page>
struct MemoryBlock {
  MemoryBlock() = delete;
  MemoryBlock(uint64_t location) : location(location) {
    data = new Page();
    isModified = false;
  }
  MemoryBlock(const MemoryBlock&) = delete;
  MemoryBlock& operator=(const MemoryBlock&) = delete;
  ~MemoryBlock() {
    delete[] data;
  }

  void page_was_modified() {
    isModified = true;
  }

  void write(FILE *file) {
    if (!isModified) {
      return;
    }
    // TODO: make more efficient by making isModified an array.
    fseek(file, location * sizeof(Page), SEEK_SET);
    fwrite(this->data, sizeof(Page), 1, file);
    isModified = false;
  }

  Page *data;
  bool isModified;
  const uint64_t location;
};

}  // namespace cpot {

#endif  // MEMORY_BLOCK_H