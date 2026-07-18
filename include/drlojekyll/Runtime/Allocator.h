// Copyright 2026, Peter Goodman. All rights reserved.

#pragma once

#include <cstddef>
#include <cstdint>

namespace hyde::rt {

// An explicit allocator, passed by value (two pointers wide). Every runtime
// container takes one at construction; nothing in the runtime touches the
// global heap on its own.
struct Allocator {
  void *ctx;
  void *(*alloc_fn)(void *ctx, size_t size, size_t align);
  void (*free_fn)(void *ctx, void *ptr, size_t size, size_t align);

  inline void *Allocate(size_t size, size_t align) const {
    return alloc_fn(ctx, size, align);
  }

  inline void Free(void *ptr, size_t size, size_t align) const {
    free_fn(ctx, ptr, size, align);
  }

  template <typename T>
  T *AllocateArray(size_t count) const {
    return static_cast<T *>(Allocate(sizeof(T) * count, alignof(T)));
  }

  template <typename T>
  void FreeArray(T *ptr, size_t count) const {
    Free(ptr, sizeof(T) * count, alignof(T));
  }
};

// An allocator backed by libc `malloc`/`free`.
Allocator MallocAllocator(void);

// A bump arena: allocations are appended to geometrically growing chunks and
// freed all at once when the arena is destroyed. Individual `Free` calls are
// no-ops.
class Arena {
 public:
  explicit Arena(Allocator backing_);
  ~Arena(void);

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  void *Allocate(size_t size, size_t align);

  // Frees every chunk; the arena is reusable afterward.
  void Reset(void);

 private:
  struct Chunk {
    Chunk *prev;
    size_t size;  // Usable bytes following this header.
    size_t used;
  };

  Chunk *NewChunk(size_t min_size);

  Allocator backing;
  Chunk *head{nullptr};
};

// An `Allocator` view over an arena. The arena must outlive every container
// using the returned allocator.
Allocator ArenaAllocator(Arena &arena);

}  // namespace hyde::rt
