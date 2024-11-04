// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "skiplist.h"
#include "util/testutil.h"
#include "util/hash.h"

#include <atomic>
#include <set>
#include <mutex>
#include <gtest/gtest.h>

using namespace std;

namespace leveldb
{
typedef uint64_t Key;

struct Comparator {
    int operator()(const Key& a, const Key& b) const {
        if (a < b) {
            return -1;
        }
        if (a > b) {
            return 1;
        }
        return 0;
    }
};

TEST(SkipTest, Empty)
{
    Arena arena;
    Comparator cmp;
    SkipList<Key, Comparator> list(cmp, &arena);
    ASSERT_FALSE(list.Contains(10));

    SkipList<Key, Comparator>::Iterator iter(&list);
    ASSERT_FALSE(iter.Valid());

    iter.SeekToFirst();
    ASSERT_FALSE(iter.Valid());

    iter.Seek(100);
    ASSERT_FALSE(iter.Valid());

    iter.SeekToLast();
    ASSERT_FALSE(iter.Valid());
}

TEST(SkipTest, InsertAndLookup)
{
    const int N = 2000;
    const int R = 5000;
    Random rnd(1000);
    set<Key> keys;
    Arena arena;
    Comparator cmp;
    SkipList<Key, Comparator> list(cmp, &arena);

    // Insert same number to a std::set and SkipList.
    for (int i = 0; i < N; i++) {
        Key key = rnd.Uniform(R);
        if (keys.insert(key).second) {
            list.Insert(key);
        }
    }

    // Verifies the correctness of SkipList::Contains.
    for (int i = 0; i < R; i++) {
        if (list.Contains(i)) {
            ASSERT_EQ(keys.count(i), 1);
        } else {
            ASSERT_EQ(keys.count(i), 0);
        }
    }

    // Simple iterator tests.
    {
        SkipList<Key, Comparator>::Iterator iter(&list);
        ASSERT_FALSE(iter.Valid());

        iter.Seek(0);
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.GetKey());

        iter.SeekToFirst();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.begin()), iter.GetKey());

        iter.SeekToLast();
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*(keys.rbegin()), iter.GetKey());
    }

    // Forward iteration test.
    for (int i = 0; i < R; i++) {
        SkipList<Key, Comparator>::Iterator iter(&list);
        iter.Seek(i);

        // Compare against model iterator.
        set<Key>::iterator model_iter = keys.lower_bound(i);
        for (int j = 0; j < 3; j++) {
            if (model_iter == keys.end()) {
                ASSERT_FALSE(iter.Valid());
                break;
            } else {
                ASSERT_TRUE(iter.Valid());
                ASSERT_EQ(*model_iter, iter.GetKey());
                ++model_iter;
                iter.Next();
            }
        }
    }

    // Backward iteration test.
    {
        SkipList<Key, Comparator>::Iterator iter(&list);
        iter.SeekToLast();

        // Compare against model iterator.
        for (set<Key>::reverse_iterator model_iter = keys.rbegin();
             model_iter != keys.rend(); ++model_iter) {
            ASSERT_TRUE(iter.Valid());
            ASSERT_EQ(*model_iter, iter.GetKey());
            iter.Prev();
        }
        ASSERT_TRUE(!iter.Valid());
    }
}

class ConcurrentTest {
public:
    ConcurrentTest() : _list(Comparator(), &_arena) {
    }

    // REQUIRES: External synchronization.
    void
    WriteStep(Random *rnd) {
        const uint32_t k = rnd->Uniform(K);
        const intptr_t g = _current.Get(k) + 1;
        const Key key = MakeKey(k, g);
        _list.Insert(key);
        _current.Set(k, g);
    }

    void
    ReadStep(Random *rnd) {
        // Remember the initial committed state of the skiplist.
        State initial_state;
        for (int k = 0; k < K; k++) {
            initial_state.Set(k, _current.Get(k));
        }

        Key pos = RandomTarget(rnd);
        SkipList<Key, Comparator>::Iterator iter(&_list);

        // Adcances to the first entry with key >= pos.
        iter.Seek(pos);

        while (true) {
            Key current;
            if (!iter.Valid()) {
                current = MakeKey(K, 0);
            } else {
                current = iter.GetKey();
                ASSERT_TRUE(IsValidKey(current)) << current << " is not valid";
            }
            ASSERT_LE(pos, current) << "should not go backwards";

            // Verify that everything in [pos,current) was not present in
            // initial_state.
            while (pos < current) {
                ASSERT_LT(key(pos), K) << pos;

                // Note that generation 0 is never inserted, so it is ok if
                // <*,0,*> is missing.
                ASSERT_TRUE((gen(pos) == 0) ||
                            (gen(pos) > static_cast<Key>(initial_state.Get(key(pos)))))
                    << "key: " << key(pos) << "; gen: " << gen(pos)
                    << "; initgen: " << initial_state.Get(key(pos));

                // Advance to next key in the valid key space
                if (key(pos) < key(current)) {
                    pos = MakeKey(key(pos) + 1, 0);
                } else {
                    pos = MakeKey(key(pos), gen(pos) + 1);
                }
            }

            if (!iter.Valid()) {
                break;
            }

            if (rnd->Uniform(2)) {
                iter.Next();
                pos = MakeKey(key(pos), gen(pos) + 1);
            } else {
                Key new_target = RandomTarget(rnd);
                if (new_target > pos) {
                    pos = new_target;
                    iter.Seek(new_target);
                }
            }
        }
    }

private:
    static const int K = 4;

    static uint64_t key(Key val) {
        return val >> 40;
    }

    static uint64_t gen(Key val) {
        return (val >> 8) & 0x00000000ffffffff;
    }

    static uint64_t hash(Key val) {
        return val & 0x00000000000000ff;
    }

    static uint64_t HashNumbers(uint64_t k, uint64_t g) {
        uint64_t data[2] = {k, g};
        return Hash(reinterpret_cast<char *>(data), sizeof(data), 0);
    }

    static Key MakeKey(uint64_t k, uint64_t g) {
        static_assert(sizeof(Key) == sizeof(uint64_t), "");
        assert(k <= K);  // We sometimes pass K to seek to the end of the skiplist.
        assert(g <= 0x00000000ffffffff);
        return (k << 40) | (g << 8) | (HashNumbers(k, g) & 0xff);
    }

    // Recalculate the hash value and compare it with the stored one.
    static bool IsValidKey(Key k) {
        return hash(k) == (HashNumbers(key(k), gen(k)) & 0x00000000000000ff);
    }

    static Key RandomTarget(Random *rnd) {
        switch (rnd->Uniform(10)) {
        case 0:
            // Seek to beginning.
            return MakeKey(0, 0);
        case 1:
            // Seek to end
            return MakeKey(K, 0);
        default:
            // Seek to middle
            return MakeKey(rnd->Uniform(K), 0);
        }
    }

    // Per-key generation
    struct State {
        atomic<int> generation[K];

        void Set(int k, int v) {
            generation[k] = v;
        }

        int Get(int k) {
            return generation[k];
        }

        State() {
            for (int k = 0; k < K; k++) {
                Set(k, 0);
            }
        }
    };

    // Current state of the test.
    State _current;

    Arena _arena;

    /*
     * SkipList is not protected by lock.
     * We just use a single writer thread to modify it.
     */
    SkipList<Key, Comparator> _list;
};

const int ConcurrentTest::K;

/*
 * Simple test that does single-threaded testing of the ConcurrentTest
 * scaffolding.
 */
TEST(SkipTest, ConcurrentWithoutThreads)
{
    ConcurrentTest test;
    Random rnd(test::RandomSeed());
    for (int i = 0; i < 10000; i++) {
        test.ReadStep(&rnd);
        test.WriteStep(&rnd);
    }
}

class TestState {
public:
    ConcurrentTest _t;
    int _seed;
    atomic<bool> _quit_flag;

    enum ReaderState {
        STARTING,
        RUNNING,
        DONE
    };

    explicit TestState(int s) : _seed(s),
                                _quit_flag(false),
                                _state(STARTING) {
    }

    void Wait(ReaderState s) {
        unique_lock<mutex> lk(_mu);
        while (_state != s) {
            _state_cv.wait(lk);
        }
    }

    void Change(ReaderState s) {
        lock_guard<mutex> lk(_mu);
        _state = s;
        _state_cv.notify_one();
    }

private:
    mutex _mu;
    ReaderState _state;
    condition_variable _state_cv;
};

static void ConcurrentReader(void *arg) {
  TestState *state = reinterpret_cast<TestState *>(arg);
  Random rnd(state->_seed);
  int64_t reads = 0;
  state->Change(TestState::RUNNING);
  while (!state->_quit_flag) {
    state->_t.ReadStep(&rnd);
    reads++;
  }
  state->Change(TestState::DONE);
}

static void RunConcurrent(int run) {
  const int seed = test::RandomSeed() + (run * 100);
  Random rnd(seed);
  const int N = 1000;
  const int kSize = 1000;
  for (int i = 0; i < N; i++) {
    if ((i % 100) == 0) {
      fprintf(stderr, "Run %d of %d\n", i, N);
    }

    TestState state(seed + 1);
    Env::Default()->Schedule(ConcurrentReader, &state);
    state.Wait(TestState::RUNNING);
    for (int i = 0; i < kSize; i++) {
      state._t.WriteStep(&rnd);
    }
    state._quit_flag = true;
    state.Wait(TestState::DONE);
  }
}

TEST(SkipTest, Concurrent1) { RunConcurrent(1); }
TEST(SkipTest, Concurrent2) { RunConcurrent(2); }
TEST(SkipTest, Concurrent3) { RunConcurrent(3); }
TEST(SkipTest, Concurrent4) { RunConcurrent(4); }
TEST(SkipTest, Concurrent5) { RunConcurrent(5); }

} // namespace leveldb.