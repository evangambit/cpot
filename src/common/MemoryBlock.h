#ifndef MEMORY_BLOCK_H
#define MEMORY_BLOCK_H

#include <cstdint>
#include <fstream>

template<class Page>
struct MemoryBlock {
  MemoryBlock() = delete;
  MemoryBlock(uint64_t start, uint64_t length)
  : start(start), length(length) {
    data = new Page[length];
    isModified = false;
  }
  MemoryBlock(const MemoryBlock&) = delete;
  MemoryBlock& operator=(const MemoryBlock&) = delete;
  ~MemoryBlock() {
    delete[] data;
  }

  void page_was_modified(uint64_t loc) {
    assert(loc >= start);
    assert(loc < start + length);
    isModified = true;
  }

  void write(FILE *file) {
    if (!isModified) {
      return;
    }
    // TODO: make more efficient by making isModified an array.
    fseek(file, start * sizeof(Page), SEEK_SET);
    fwrite(this->data, sizeof(Page), length, file);
    isModified = false;
  }

  Page *data;
  bool isModified;
  const uint64_t start, length;
};

#endif  // MEMORY_BLOCK_H