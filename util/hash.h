// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <cstddef>
#include <cstdint>

namespace leveldb {

uint32_t Hash(const char *data, size_t n, uint32_t seed);

}  // namespace leveldb.