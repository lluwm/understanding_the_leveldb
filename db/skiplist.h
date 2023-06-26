// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include "util/arena.h"
#include "util/random.h"

#include <cassert>
using namespace std;

namespace leveldb {

template <typename Key, class Comparator>
class SkipList {
public:
    const static int kMaxHeight;

    /*
     * Create a new SkipList object that uses 'cmp' for comparing keys,
     * and allocates memory using 'arena'. Objects allocated in the arena
     * must remain allocated for the lifetime of the skiplist object.
     */
    explicit SkipList(Comparator cmp, Arena *arena) : _arena(arena),
                                                      _compare(cmp),
                                                      _head(NewNode(0 /* any key will do */, kMaxHeight)),
                                                      _max_height(1),
                                                      _rnd(0xdeadbeef) {
        for (int i = 0; i < kMaxHeight; i++) {
            _head->SetNext(i, nullptr);
        }
    }

    SkipList(const SkipList&) = delete;

    SkipList& operator=(const SkipList&) = delete;

    ~SkipList() {
    }

    // Insert key into the list.
    void Insert(const Key& key);

    // Returns true iff an entry that compares equal to key is in the list.
    bool Contains(const Key& key) const;


private:
    // Represent a node in a skip list.
    class Node {
        public:
            explicit Node(const Key& k) : _key(k) {}

            Node *Next(int n) {
                assert(n >= 0);
                return _next[n];
            }

            void SetNext(int n, Node *val) {
                assert(n >= 0);
                _next[n] = val;
            }

            const Key& GetKey() const {
                return _key;
            }

        private:
            Key const _key;

            // Array of length equal to the node height. _next[0] is the lowest level link.
            // level is from 0 -> 11.
            atomic<Node *> _next[1];
    };

public:
    // Iteration over the contents of a skip list.
    class Iterator {
        public:
            // Initialize an iterator over the specified list.
            explicit Iterator(const SkipList *list) : _list(list),
                                                      _node(nullptr) {
            }

            // Returns true iff the iterator is positioned at a valid node.
            bool Valid() const {
                return _node != nullptr;
            }

            // Returns the key at the current position.
            const Key& GetKey() const {
                assert(Valid());
                return _node->GetKey();
            }

            // Advances to the next position.
            void Next() {
                assert(Valid());
                _node = _node->Next(0);
            }

            // Adcances to the previous position.
            void Prev() {
                assert(Valid());
                _node = _list->FindLessThan(_node->GetKey());
                if (_node == _list->_head) {
                    _node = nullptr;
                }
            }

            // Adcances to the first entry with a key >= target.
            void Seek(const Key& target) {
                _node = _list->FindGreaterOrEqual(target, nullptr);
            }

            // Position at the first entry in list.
            void SeekToFirst() {
                _node = _list->_head->Next(0);
            }

            // Position at the last entry in list.
            void SeekToLast() {
                _node = _list->FindLast();
                if (_node == _list->_head) {
                    _node = nullptr;
                }
            }

        private:
            const SkipList *const _list;
            Node *_node;
    };

private:
    int GetMaxHeight() const {
        return _max_height;
    }

    bool Equal(const Key& a, const Key& b) const {
        return _compare(a, b) == 0;
    }

    Node *NewNode(const Key& key, int height) const;

    int RandomHeight();

    // Returns true if key is greater or equal to the data stored in 'node'.
    bool KeyIsAfterNode(const Key& key, Node *node) const {
        // If node is nullptr, node is considered infinite.
        return (node != nullptr) && (_compare(node->GetKey(), key) < 0);
    }

    /*
     * Returns the earliest node that comes at or after key,
     *         nullptr if there is no such node.
     *
     * If prev is non-null, fills prev[level] with pointer to previous
     * node at 'level' for every level in [0 ... _max_height - 1].
     */
    Node *FindGreaterOrEqual(const Key& key, Node **prev) const;

    /*
     * Returns the latest node with a key < key,
     *         _head if there is no such node.
     */
    Node *FindLessThan(const Key& key) const;

    // Returns the last node in the list, or _head if list is empty.
    Node *FindLast() const;


    // Immutable after construction.
    Arena *const _arena;
    Comparator const _compare;
    Node *const _head;

    // Modified only by Insert(). Read concurrently by readers, but stale vlaues are okay.
    int _max_height;

    // Read/written only by Insert().
    Random _rnd;
};

template <typename Key, class Comparator>
const int SkipList<Key, Comparator>::kMaxHeight = 12;

// Implementation of member functions.

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node *
SkipList<Key, Comparator>::NewNode(const Key& key, int height) const
{
  char *node_memory = _arena->AllocateAligned(sizeof(Node) + sizeof(atomic<Node *>) * (height - 1));
  return new (node_memory) Node(key);
}

template <typename Key, class Comparator>
void
SkipList<Key, Comparator>::Insert(const Key& key)
{
    Node * prev[kMaxHeight] = { nullptr };
    Node *node = FindGreaterOrEqual(key, prev);
    // Does not allow duplicate insertion.
    assert(node == nullptr || !Equal(node->GetKey(), key));

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
    return node != nullptr && Equal(node->GetKey(), key);
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
        } else {
            if (prev != nullptr) {
                prev[level] = cur;
            }
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

template <typename Key, class Comparator>
int
SkipList<Key, Comparator>::RandomHeight() {
  // Increase height with probability 1 in kBranching.
  static const int kBranching = 4;
  int height = 1;

  while (height < kMaxHeight && _rnd.OneIn(kBranching)) {
    height++;
  }

  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

} // namespace leveldb.