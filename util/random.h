// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <cstdint>

namespace leveldb {


// A simple random number generator.
class Random {
public:
    explicit Random(uint32_t seed) : _seed(seed & 0x7fffffffu) {
        // Avoid bad seeds.
        if (_seed == 0 || _seed == 0x7fffffffu) {
        _seed = 1;
        }
    }

    // Returns a uniformly distributed value in the range [0..n-1]
    uint32_t Uniform(int n) {
        return Next() % n;
    }

    // Randomly returns true ~"1/n" of the time, and false otherwise.
    bool OneIn(int n) {
        return Next() % n == 0;
    }

private:
    uint32_t Next() {
        // 2^31-1.
        static const uint32_t M = 2147483647L;

        // bits 14, 8, 7, 5, 2, 1, 0
        static const uint64_t A = 16807;

        /*
         * We are computing:
         *      _seed = (_seed * A) % M,    where M = 2^31-1
         *
         * _seed must not be zero or M, or else all subsequent computed values
         * will be zero or M respectively. For all other values, seed_ will end
         * up cycling through every number in [1, M-1].
         */
         uint64_t product = _seed * A;

        // Compute (product % M) using the fact that ((x << 31) % M) == x.
        _seed = static_cast<uint32_t>((product >> 31) + (product & M));

        /*
         * The first reduction may overflow by 1 bit, so we may need to
         * repeat.  mod == M is not possible; using > allows the faster
         * sign-bit-based test.
         */
        if (_seed > M) {
            _seed -= M;
        }
        return _seed;
    }

    uint32_t    _seed;
};

} // namespace leveldb.