/*
 *  adaptive_mutex.h
 *
 * Copyright (c) 2016 Axinoe. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *  Created by Jean-Charles BERTIN on 5/13/16.
 */

#ifndef __ADAPTIVE_MUTEX__
#define __ADAPTIVE_MUTEX__

#include <pthread.h>
#include <dispatch/dispatch.h>
#include <assert.h>

#if __GNUC__
#define ADAPTIVE_MUTEX_EXPORT extern __attribute__((visibility("default")))
#define ADAPTIVE_MUTEX_LOCAL extern __attribute__((visibility("hidden")))
#define ADAPTIVE_MUTEX_INLINE static __inline__
#define ADAPTIVE_MUTEX_ALWAYS_INLINE __attribute__((__always_inline__))
#  if __clang__ && __clang_major__ < 3
#    define ADAPTIVE_MUTEX_NONNULL
#  else
#    define ADAPTIVE_MUTEX_NONNULL __attribute__((__nonnull__))
#  endif
#define ADAPTIVE_MUTEX_NOTHROW __attribute__((__nothrow__))
#else
#define ADAPTIVE_MUTEX_EXPORT extern
#define ADAPTIVE_MUTEX_LOCAL extern
#define ADAPTIVE_MUTEX_INLINE static inline
#define ADAPTIVE_MUTEX_ALWAYS_INLINE
#define ADAPTIVE_MUTEX_NONNULL
#define ADAPTIVE_MUTEX_NOTHROW
#endif

__BEGIN_DECLS

typedef struct adaptive_mutex {
    pthread_mutex_t _mutex;
    long            _spins;
} adaptive_mutex_t;

typedef int (*adaptive_mutex_lock_t)(adaptive_mutex_t *mutex);

#define ADAPTIVE_MUTEX_INITIALIZER	{ PTHREAD_MUTEX_INITIALIZER, 0 }

ADAPTIVE_MUTEX_EXPORT ADAPTIVE_MUTEX_NONNULL ADAPTIVE_MUTEX_NOTHROW
int
adaptive_mutex_lock(adaptive_mutex_t *mutex);

ADAPTIVE_MUTEX_LOCAL
adaptive_mutex_lock_t
_adaptive_mutex_lock_ptr;

ADAPTIVE_MUTEX_LOCAL
dispatch_once_t
_adaptive_mutex_lock_token;

ADAPTIVE_MUTEX_LOCAL ADAPTIVE_MUTEX_NOTHROW
void
_adaptive_mutex_lock_init(void *context);

ADAPTIVE_MUTEX_INLINE ADAPTIVE_MUTEX_ALWAYS_INLINE ADAPTIVE_MUTEX_NONNULL ADAPTIVE_MUTEX_NOTHROW
int
_adaptive_mutex_lock(adaptive_mutex_t *mutex)
{
    dispatch_once_f(&_adaptive_mutex_lock_token, NULL, _adaptive_mutex_lock_init);
    return _adaptive_mutex_lock_ptr(mutex);
}
#undef adaptive_mutex_lock
#define adaptive_mutex_lock _adaptive_mutex_lock

ADAPTIVE_MUTEX_INLINE ADAPTIVE_MUTEX_ALWAYS_INLINE ADAPTIVE_MUTEX_NONNULL ADAPTIVE_MUTEX_NOTHROW
int
adaptive_mutex_unlock(adaptive_mutex_t *mutex)
{
    assert(mutex);
    return pthread_mutex_unlock(&mutex->_mutex);
}

__END_DECLS

#endif
