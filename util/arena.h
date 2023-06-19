// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <cstddef>
#include <vector>
using namespace std;

namespace leveldb {


class Arena {
// TODO: not thread safe currently, should check if there is such a need.
public:
    Arena() : _alloc_ptr(nullptr),
              _alloc_bytes_remaining(0),
              _memory_usage(0) {
    }

    ~Arena() {
        for (char *block : _blocks) {
            delete[] block;
        }
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    // Returns a pointer to a newly allocated memory block of "bytes" bytes.
    char *Allocate(size_t bytes);

    // Allocate memory with the normal alignment strategy.
    char *AllocateAligned(size_t bytes);

    // Returns an estimate of the total memory usage of data allocated.
    size_t MemoryUsage() const {
        return _memory_usage;
    }

private:
    char *AllocateFallBack(size_t bytes);
    char *AllocateNewBlock(size_t block_bytes);


    // Total memory usage of the arena.
    size_t _memory_usage;

    // The remaining unused bytes in the current memory block.
    size_t _alloc_bytes_remaining;

    // Allocation state.
    char *_alloc_ptr;

    // Array of new[] allocated memory blocks.
    vector<char *> _blocks;
};

} // namespace leveldb.