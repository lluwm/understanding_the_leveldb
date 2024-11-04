// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include "gtest/gtest.h"

namespace leveldb {
namespace test {

// Returns the random seed used at the start of the current test run.
inline int RandomSeed() {
  return testing::UnitTest::GetInstance()->random_seed();
}

} // namespace test.
} // namespace leveldb.