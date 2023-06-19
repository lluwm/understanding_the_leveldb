// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include "util/arena.h"

namespace leveldb {

template <typename Key, class Comparator>
class SkipList {
public:
    explicit SkipList(Comparator cmp, Arena *arena);


private:
    Arena *const _arena;

};

} // namespace leveldb.