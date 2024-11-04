// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

/**
 * Endian-neutral encoding:
 * 1. Fixed-length numbers are encoded with least-significant byte first.
 * 2. In addition we support variable length "varint" encoding.
 * 3. Strings are encoded prefixed by their length in varint format.
 */

#pragma once
#include <cstdint>

namespace leveldb {

// Lower-level versions of Get... that read directly from a character buffer
// without any bounds checking.

inline uint32_t DecodeFixed32(const char *ptr) {
  const uint8_t * const buffer = reinterpret_cast<const uint8_t *>(ptr);

  // Recent clang and gcc optimize this to a single mov / ldr instruction.
  return (static_cast<uint32_t>(buffer[0])) |
         (static_cast<uint32_t>(buffer[1]) << 8) |
         (static_cast<uint32_t>(buffer[2]) << 16) |
         (static_cast<uint32_t>(buffer[3]) << 24);
}
} // namespace leveldb.