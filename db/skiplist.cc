// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "skiplist.h"

#include <cstdlib>
#include <cassert>
using namespace std;

namespace leveldb {

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node *
SkipList<Key, Comparator>::NewNode(const Key& key, int height) const
{
  char* node_memory = _arena->AllocateAligned(sizeof(Node) + sizeof(atomic<Node *>) * (height - 1));
  return new (node_memory) Node(key);
}

template <typename Key, class Comparator>
void
SkipList<Key, Comparator>::Insert(const Key& key)
{
    Node *prev[kMaxHeight];
    Node *node = FindGreaterOrEqual(key, prev);

    // Does not allow duplicate insertion.
    assert(node == nullptr || !Equal(node->_key, key));

    int height = RandomHeight();
    if (height > GetMaxHeight()) {
        for (int i = GetMaxHeight(); i < height; i++) {
            prev[i] = _head;
        }
        _max_height = height;
    }

    node = NewNode(key, height);
    for (int i = 0; i < height; i++) {
        node->SetNext(i, prev[i]->Next(i));
        prev[i]->SetNext(i, node);
    }
}

template <typename Key, class Comparator>
bool
SkipList<Key, Comparator>::Contains(const Key& key) const
{
    Node *node = FindGreaterOrEqual(key, nullptr);
    return node != nullptr && Equal(node->_key, key);
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node *
SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key, Node **prev) const
{
    Node *cur = _head;
    int level = GetMaxHeight() - 1;

    while (true) {
        Node *next = cur->Next(level);
        if (KeyIsAfterNode(key, next)) {
            // key is larger than next node, keep searching in the node list at the current level.
            cur = next;
            if (prev != nullptr) {
                prev[level] = cur;
            }
        } else {
            if (level == 0) {
                // next is the first node greater or equal to key.
                return next;
            }
            level--;
        }
    }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node *
SkipList<Key, Comparator>::FindLessThan(const Key& key) const
{
    Node *cur = _head;
    int level = GetMaxHeight() - 1;
    while (true) {
        Node *next = cur->Next(level);
        if (KeyIsAfterNode(key, next)) {
            // key is larger than next node, keep searching in the node list at the current level.
            cur = next;
        } else {
            if (level == 0) {
                // cur is the last node less than key.
                return cur;
            }
            level--;
        }
    }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node *
SkipList<Key, Comparator>::FindLast() const
{
    Node *cur = _head;
    int level = GetMaxHeight() - 1;

    while (true) {
        Node *next = cur->Next(level);
        if (next != nullptr) {
            cur = next;
            continue;
        }

        if (level == 0) {
            return cur;
        }

        level--;
    }
}

// Returns true with ~'1/n' possbility, false otherwise.
static bool
IsOneIn(int n)
{
    return rand() % n == 0;
}

template <typename Key, class Comparator>
int
SkipList<Key, Comparator>::RandomHeight() {
  // Increase height with probability 1 in kBranching.
  static const int kBranching = 4;
  int height = 1;

  while (height < kMaxHeight && IsOneIn(kBranching)) {
    height++;
  }

  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

} // namespace leveldb.