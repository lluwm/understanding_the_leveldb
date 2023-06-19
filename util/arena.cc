// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "arena.h"

#include <cassert>
using namespace std;

namespace leveldb {

static const int kBlockSize = 4096; // 4KB.

char *
Arena::Allocate(size_t bytes) {
    assert(bytes > 0);

    if (bytes <= _alloc_bytes_remaining) {
        char *result = _alloc_ptr;
        _alloc_ptr += bytes;
        _alloc_bytes_remaining -= bytes;
        return result;
    }
    return AllocateFallBack(bytes);
}

char *
Arena::AllocateFallBack(size_t bytes) {
    if (bytes > kBlockSize / 4) {
        /*
         * Object is more than a quarter of block size. Allocate it separately
         * to avoid wasting too much space of current block.
         */
        return AllocateNewBlock(bytes);
    }

    // Move to a new block and waste the remaining space in current block.
    _alloc_ptr = AllocateNewBlock(kBlockSize);
    _alloc_bytes_remaining = kBlockSize;

    char *result = _alloc_ptr;
    _alloc_ptr += bytes;
    _alloc_bytes_remaining -= bytes;
    return result;
}

/*
 * Allocate memory space with the normal alignment gurantees.
 */
char *
Arena::AllocateAligned(size_t bytes) {
    // 8 bytes alignment by default.
    const int align = sizeof(void *) > 8 ? sizeof(void *) : 8;
    static_assert((align & (align - 1)) == 0, "pointer size should be a power of 2");

    size_t offset = reinterpret_cast<uintptr_t>(_alloc_ptr) & (align - 1);
    size_t padding = offset == 0 ? 0 : align - offset;
    size_t actual_bytes = bytes + padding;

    char *result = nullptr;
    if (actual_bytes <= _alloc_bytes_remaining) {
        // Adjust returned virtual memory address so that it is aligned.
        result = _alloc_ptr + padding;
        _alloc_ptr += actual_bytes;
        _alloc_bytes_remaining -= actual_bytes;
    } else {
        result = AllocateFallBack(bytes);
    }

    // verify that the returned memory address is aligned.
    assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
    return result;
}

/*
 * Allocate a continuous virtual memory space of 'block_bytes'.
 */
char *
Arena::AllocateNewBlock(size_t block_bytes) {
    char *result = new char[block_bytes];
    _blocks.push_back(result);
    _memory_usage += block_bytes + sizeof(char *);
    return result;
}

} // namespace leveldb.