// Copyright 2026, Peter Goodman. All rights reserved.

#include <drlojekyll/Runtime/Allocator.h>

#include <cstdlib>
#include <new>

namespace hyde::rt {

namespace {

void *MallocAlloc(void *, size_t size, size_t align) {
  if (align <= alignof(std::max_align_t)) {
    return std::malloc(size);
  }
  return std::aligned_alloc(align, (size + align - 1u) / align * align);
}

void MallocFree(void *, void *ptr, size_t, size_t) {
  std::free(ptr);
}

void *ArenaAlloc(void *ctx, size_t size, size_t align) {
  return static_cast<Arena *>(ctx)->Allocate(size, align);
}

void ArenaFree(void *, void *, size_t, size_t) {}

}  // namespace

Allocator MallocAllocator(void) {
  return {nullptr, MallocAlloc, MallocFree};
}

Arena::Arena(Allocator backing_) : backing(backing_) {}

Arena::~Arena(void) {
  Reset();
}

Arena::Chunk *Arena::NewChunk(size_t min_size) {
  static constexpr size_t kDefaultChunkSize = 64u * 1024u;
  size_t chunk_size = head ? head->size * 2u : kDefaultChunkSize;
  if (chunk_size < min_size) {
    chunk_size = min_size;
  }
  auto chunk = static_cast<Chunk *>(backing.Allocate(
      sizeof(Chunk) + chunk_size, alignof(std::max_align_t)));
  chunk->prev = head;
  chunk->size = chunk_size;
  chunk->used = 0u;
  head = chunk;
  return chunk;
}

void *Arena::Allocate(size_t size, size_t align) {
  Chunk *chunk = head;
  if (chunk) {
    const auto base = reinterpret_cast<uintptr_t>(chunk + 1u);
    const auto aligned = (base + chunk->used + align - 1u) & ~(align - 1u);
    const auto new_used = (aligned - base) + size;
    if (new_used <= chunk->size) {
      chunk->used = new_used;
      return reinterpret_cast<void *>(aligned);
    }
  }
  chunk = NewChunk(size + align);
  const auto base = reinterpret_cast<uintptr_t>(chunk + 1u);
  const auto aligned = (base + align - 1u) & ~(align - 1u);
  chunk->used = (aligned - base) + size;
  return reinterpret_cast<void *>(aligned);
}

void Arena::Reset(void) {
  while (head) {
    Chunk *prev = head->prev;
    backing.Free(head, sizeof(Chunk) + head->size, alignof(std::max_align_t));
    head = prev;
  }
}

Allocator ArenaAllocator(Arena &arena) {
  return {&arena, ArenaAlloc, ArenaFree};
}

}  // namespace hyde::rt
