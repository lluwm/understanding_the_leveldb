// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include "util/arena.h"

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
    explicit SkipList(Comparator cmp, Arena *arena) : _compare(cmp),
                                                      _arena(arena),
                                                      _max_height(1) {
        _head = NewNode(0, kMaxHeight);
        for (int i = 0; i < kMaxHeight; i++) {
            _head->SetNext(i, nullptr);
        }
    }

    SkipList(const SkipList&) = delete;

    SkipList& operator=(const SkipList&) = delete;

    ~SkipList();

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

        private:
            Key const _key;

            // Array of length equal to the node height. _next[0] is the lowest level link.
            // level is from 0 -> 11.
            atomic<Node *> _next[1];
    };

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
                return _node->_key;
            }

            // Advances to the next position.
            void Next() {
                assert(Valid());
                _node = _node->Next(0);
            }

            // Adcances to the previous position.
            void Prev() {
                assert(Valid());
                _node = _list->FindLessThan(_node->_key);
                if (_node == _list->_head) {
                    _node = nullptr;
                }
            }

            // Adcances to the fist entry with a key >= target.
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

    int GetMaxHeight() const {
        return _max_height;
    }

    bool Equal(const Key& a, const Key& b) {
        return _compare(a, b) == 0;
    }

    Node *NewNode(const Key& key, int height) const;

    int RandomHeight();

    // Returns true if key is greater or equal to the data stored in 'node'.
    bool KeyIsAfterNode(const Key& key, Node *node) const {
        // If node is nullptr, node is considered infinite.
        return (node != nullptr) && (_compare(node->_key, key) < 0);
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
};

template <typename Key, class Comparator>
const int SkipList<Key, Comparator>::kMaxHeight = 12;

} // namespace leveldb.