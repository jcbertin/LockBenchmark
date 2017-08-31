/*
 *  adaptive_mutex.c
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

#include "adaptive_mutex.h"

#include <sys/types.h>
#include <sys/sysctl.h>

#if __GNUC__
#define ADAPTIVE_MUTEX_MIN(a,b)     ({ __typeof__(a) __a = (a); __typeof__(b) __b = (b); __a < __b ? __a : __b; })
#define ADAPTIVE_MUTEX_EXPECT(a,v)  __builtin_expect((long)(a),(v))
#define ADAPTIVE_MUTEX_UNUSED       __attribute__((unused))
#else
#define ADAPTIVE_MUTEX_MIN(a,b)     ((a) < (b) ? (a) : (b))
#define ADAPTIVE_MUTEX_EXPECT(a,v)  (a)
#define ADAPTIVE_MUTEX_UNUSED
#endif

static const long _adaptive_mutex_max_count = 1000;
static const long _adaptive_mutex_off_count = 100;

adaptive_mutex_lock_t _adaptive_mutex_lock_ptr;
dispatch_once_t _adaptive_mutex_lock_token;

static int
_adaptive_mutex_lock(adaptive_mutex_t *mutex)
{
    assert(mutex);
    
    if (pthread_mutex_trylock (&mutex->_mutex) != 0) {
        long count = 0;
        const long max_count = ADAPTIVE_MUTEX_MIN(_adaptive_mutex_max_count, mutex->_spins * 2 + _adaptive_mutex_off_count);
        
        do {
            if (ADAPTIVE_MUTEX_EXPECT(++count >= max_count, 0)) {
                (void) pthread_mutex_lock(&mutex->_mutex);
                break;
            }
            pthread_yield_np();
        }
        while (pthread_mutex_trylock (&mutex->_mutex) != 0);
        
        mutex->_spins += (count - mutex->_spins) / 8;
    }
    return 0;
}

void
_adaptive_mutex_lock_init(ADAPTIVE_MUTEX_UNUSED void *context)
{
    int mib[2] = {CTL_HW, HW_AVAILCPU};
    int32_t ncpu;
    size_t size = sizeof(ncpu);
    if (sysctl(mib, 2, &ncpu, &size, NULL, 0) < 0)
        ncpu = 0;
    _adaptive_mutex_lock_ptr = (ncpu < 2) ? (adaptive_mutex_lock_t)pthread_mutex_lock : _adaptive_mutex_lock;
}
