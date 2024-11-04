// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <functional>
using namespace std;

/*
 * An Env is an interface used by the leveldb implementation to access
 * operating system functionality like the filesystem etc.  Callers
 * may wish to provide a custom Env object when opening a database to
 * get fine gain control; e.g., to rate limit file system operations.
 *
 * All Env implementations are safe for concurrent access from
 * multiple threads without any external synchronization.
 */

namespace leveldb {

class Env {
public:
    Env();

    Env(const Env&) = delete;
    Env& operator=(const Env&) =  delete;

    virtual ~Env();

    /*
     * Return a default environment suitable for the current operating system.
     * Sophisticated users may wish to provide their own Env implementation
     * instead of relying on this default environment.
     */
    static Env *Default();


    /*
     * Arrange to run "func(arg)" once in a background thread.
     *
     * "funct" may run in an unspecified thread. Multiple functions
     * added to the same Env may run concurrently in different threads, i.e.,
     * the caller may not assume that background work items are serialized.
     */
    virtual void Schedule(function<void(void *)> func, void *arg) = 0;
};

}; // namespace leveldb.