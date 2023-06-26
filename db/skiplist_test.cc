// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "skiplist.h"

#include <set>
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

} // namespace leveldb.