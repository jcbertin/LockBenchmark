//
//  main.cpp
//  LockBenchmark
//
//  Created by Jean-Charles BERTIN on 4/30/16.
//  Copyright © 2016 Axinoe. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following
//  conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
//

#include <iostream>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <dispatch/dispatch.h>

#include <list>

#define LOOPS 100000000
#define SLEEP_MAX 300
#define SLEEP_FACTOR_1 0ull
#define SLEEP_OFFSET_1 0ull
#define SLEEP_FACTOR_2 1ull
#define SLEEP_OFFSET_2 0ull

using namespace std;

list<int> the_list;

#define USE_SPINLOCK    1
#define USE_SEMAPHORE   2
#define USE_MUTEX       3
#define USE_UNFAIR_LOCK 4
#define USE_ADAPTIVE_1  5
#define USE_ADAPTIVE_2  6
#define USE_ADAPTIVE_3  7

#define LOCK_KIND       USE_ADAPTIVE_3
#define ADD_LATENCY     0

struct thread_data_t {
    int         _tid;
    uint64_t    _min_latency;
    uint64_t    _max_latency;
    uint64_t    _cumul_latency;
    uint64_t    _count;
#if ADD_LATENCY
    long        _inter_delay[LOOPS];
    long        _intra_delay[LOOPS];
#endif
};

#if LOCK_KIND == USE_SPINLOCK
#import <libkern/OSAtomic.h>

OSSpinLock _lock = (OSSpinLock) OS_SPINLOCK_INIT;
#define LOCK()			OSSpinLockLock(&_lock)
#define UNLOCK()		OSSpinLockUnlock(&_lock)

#elif LOCK_KIND == USE_SEMAPHORE
dispatch_semaphore_t _lock;
#define LOCK()			dispatch_semaphore_wait(_lock, DISPATCH_TIME_FOREVER)
#define UNLOCK()		dispatch_semaphore_signal(_lock)
static __attribute__ ((constructor)) void _initSemaphore()
{
    _lock = dispatch_semaphore_create(1);
}

#elif LOCK_KIND == USE_MUTEX
pthread_mutex_t _lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
#define LOCK()			pthread_mutex_lock(&_lock)
#define UNLOCK()		pthread_mutex_unlock(&_lock)

#elif LOCK_KIND == USE_UNFAIR_LOCK
#import <os/lock.h>

os_unfair_lock _lock = OS_UNFAIR_LOCK_INIT;
#define LOCK()          os_unfair_lock_lock(&_lock)
#define UNLOCK()        os_unfair_lock_unlock(&_lock)

#elif LOCK_KIND == USE_ADAPTIVE_1
#include <sys/types.h>
#include <sys/sysctl.h>

typedef struct adaptive_mutex {
    pthread_mutex_t _mutex;
    int64_t         _spins;
    uint64_t        _spinsCount;
    uint64_t        _locksCount;
} adaptive_mutex_t;

#define ADAPTIVE_MUTEX_INITIALIZER	{ PTHREAD_MUTEX_INITIALIZER, 0, 0, 0 }

static const uint64_t kAdaptiveMaximumCount = 1000ll;
static const uint64_t kAdaptiveOffsetCount = 100ll;

static int              _is_smp;
static dispatch_once_t  _is_smp_token;

static void _is_smp_init(void *context)
{
#pragma unused(context)
    int mib[2] = {CTL_HW, HW_AVAILCPU};
    int32_t ncpu;
    size_t size = sizeof(ncpu);
    if (sysctl(mib, 2, &ncpu, &size, NULL, 0) < 0)
        ncpu = 0;
    _is_smp = (ncpu < 2);
}

static int adaptive_mutex_lock(adaptive_mutex_t *mutex)
{
    assert(mutex != NULL);
    
    dispatch_once_f(&_is_smp_token, NULL, _is_smp_init);
    if (_is_smp)
        return pthread_mutex_lock(&mutex->_mutex);
    
    if (pthread_mutex_trylock (&mutex->_mutex) != 0) {
        int64_t count = 0;
        const int64_t max_count = MIN(kAdaptiveMaximumCount, mutex->_spins * 2ll + kAdaptiveOffsetCount);
        
        do {
            if (__builtin_expect(++count >= max_count, 0)) {
                (void) pthread_mutex_lock(&mutex->_mutex);
                mutex->_locksCount ++;
                break;
            }
            pthread_yield_np();
            
            (void) __sync_fetch_and_add(&mutex->_spinsCount, 1);
        }
        while (pthread_mutex_trylock (&mutex->_mutex) != 0);
        
        mutex->_spins += (count - mutex->_spins) / 8;
    }
    return 0;
}

static int adaptive_mutex_unlock(adaptive_mutex_t *mutex)
{
    assert(mutex != NULL);
    
    return pthread_mutex_unlock(&mutex->_mutex);
}

adaptive_mutex_t _lock = (adaptive_mutex_t) ADAPTIVE_MUTEX_INITIALIZER;
#define LOCK()			adaptive_mutex_lock(&_lock)
#define UNLOCK()		adaptive_mutex_unlock(&_lock)

#elif LOCK_KIND == USE_ADAPTIVE_2
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

typedef struct adaptive_mutex {
    pthread_mutex_t _mutex;
    uint64_t        _spins;
    uint64_t        _spinsCount;
    uint64_t        _locksCount;
} adaptive_mutex_t;

#define ADAPTIVE_MUTEX_INITIALIZER	{ PTHREAD_MUTEX_INITIALIZER, 0, 0, 0 }

#if TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
static const uint64_t kAdaptiveMaximumTimeout = 30ull * NSEC_PER_USEC;
#else
static const uint64_t kAdaptiveMaximumTimeout = 10ull * NSEC_PER_USEC;
#endif
static const uint64_t kAdaptiveOffsetTimeout = 100ull;

static int              _is_smp;
static dispatch_once_t  _is_smp_token;

static void _is_smp_init(void *context)
{
#pragma unused(context)
    int mib[2] = {CTL_HW, HW_AVAILCPU};
    int32_t ncpu;
    size_t size = sizeof(ncpu);
    if (sysctl(mib, 2, &ncpu, &size, NULL, 0) < 0)
        ncpu = 0;
    _is_smp = (ncpu < 2);
}

#if defined(__i386__) || defined(__x86_64__)
// x86 currently implements mach time in nanoseconds
// this is NOT likely to change
static const uint64_t   _max_timeout = kAdaptiveMaximumTimeout;
static const uint64_t   _off_timeout = kAdaptiveOffsetTimeout;
#else
static uint64_t         _max_timeout;
static uint64_t         _off_timeout;
static dispatch_once_t  _max_timeout_token;

static void _max_timeout_init(void *context)
{
#pragma unused(context)
    mach_timebase_info_data_t tbi;
    (void) mach_timebase_info(&tbi);
    if (LIKELY((tbi.numer != tbi.denom))) {
        _max_timeout = (kAdaptiveMaximumTimeout * tbi.denom) / tbi.numer;
        _off_timeout = (kAdaptiveOffsetTimeout  * tbi.denom) / tbi.numer;
    } else {
        _max_timeout = kAdaptiveMaximumTimeout;
        _off_timeout = kAdaptiveOffsetTimeout;
    }
}
#endif

static int adaptive_mutex_lock(adaptive_mutex_t *mutex)
{
    assert(mutex != NULL);
    
    dispatch_once_f(&_is_smp_token, NULL, _is_smp_init);
    if (_is_smp)
        return pthread_mutex_lock(&mutex->_mutex);
    
    if (pthread_mutex_trylock (&mutex->_mutex) != 0) {
#if !defined(__i386__) && !defined(__x86_64__)
        dispatch_once_f(&_max_timeout_token, NULL, _max_timeout_init);
#endif
        const uint64_t start = mach_absolute_time();
        const uint64_t end = start + MIN(_max_timeout, mutex->_spins * 2ull + _off_timeout);
        
        do {
            pthread_yield_np();
            
            const uint64_t now = mach_absolute_time();
            if (__builtin_expect(now > end, 0)) {
                (void) pthread_mutex_lock(&mutex->_mutex);
                mutex->_locksCount ++;
                break;
            }
            
            mutex->_spinsCount ++;
        }
        while (pthread_mutex_trylock (&mutex->_mutex) != 0);
        
        const uint64_t elapsed = mach_absolute_time() - start;
        if (mutex->_spins < elapsed)
            mutex->_spins = elapsed;
    }
    return 0;
}

static int adaptive_mutex_unlock(adaptive_mutex_t *mutex)
{
    assert(mutex != NULL);
    
    return pthread_mutex_unlock(&mutex->_mutex);
}

adaptive_mutex_t _lock = (adaptive_mutex_t) ADAPTIVE_MUTEX_INITIALIZER;
#define LOCK()			adaptive_mutex_lock(&_lock)
#define UNLOCK()		adaptive_mutex_unlock(&_lock)

#elif LOCK_KIND == USE_ADAPTIVE_3
#include "adaptive_mutex.h"

adaptive_mutex_t _lock = (adaptive_mutex_t) ADAPTIVE_MUTEX_INITIALIZER;
#define LOCK()			adaptive_mutex_lock(&_lock)
#define UNLOCK()		adaptive_mutex_unlock(&_lock)
#endif

void *consumer(void *ptr)
{
    thread_data_t* data = (thread_data_t*)ptr;
    int i;
#if ADD_LATENCY
    struct timespec time = {0,0};
    int rc;
#endif
    
    while (1)
    {
        const uint64_t start = dispatch_time(DISPATCH_TIME_NOW, 0);
        LOCK();
        uint64_t elapsed = dispatch_time(DISPATCH_TIME_NOW, 0) - start;
        if (data->_min_latency > elapsed)
            data->_min_latency = elapsed;
        if (data->_max_latency < elapsed)
            data->_max_latency = elapsed;
        data->_cumul_latency += elapsed;
        data->_count ++;
        
        if (the_list.empty())
        {
            UNLOCK();
            break;
        }
        
#if ADD_LATENCY
        time.tv_nsec = data->_intra_delay[data->_count];
        if (time.tv_nsec != 0) {
            rc = nanosleep(&time, NULL);
            assert(rc == 0);
        }
#endif
        
        i = the_list.front();
        the_list.pop_front();
        
        UNLOCK();
        
#if ADD_LATENCY
        time.tv_nsec = data->_inter_delay[data->_count];
        if (time.tv_nsec != 0) {
            rc = nanosleep(&time, NULL);
            assert(rc == 0);
        }
#endif
    }
    
    return NULL;
}

int main(int argc, const char * argv[])
{
    int i;
    pthread_t thr1, thr2;
    struct timeval tv1, tv2;
    thread_data_t *data;
    
    data = (thread_data_t *) calloc(2, sizeof(thread_data_t));
    data[0]._tid = 1;
    data[0]._min_latency = UINT64_MAX;
    data[0]._max_latency = 0;
    data[0]._cumul_latency = 0;
    data[0]._count = 0;
    
    data[1]._tid = 2;
    data[1]._min_latency = UINT64_MAX;
    data[1]._max_latency = 0;
    data[1]._cumul_latency = 0;
    data[1]._count = 0;
    
    // Creating the list content...
    for (i = 0; i < LOOPS; i++) {
        the_list.push_back(i);
#if ADD_LATENCY
        uint64_t r = arc4random_uniform(SLEEP_MAX);
        data[0]._inter_delay[i] = r * SLEEP_FACTOR_1 + SLEEP_OFFSET_1;
        r = arc4random_uniform(SLEEP_MAX);
        data[0]._intra_delay[i] = r * SLEEP_FACTOR_1 + SLEEP_OFFSET_1;
        r = arc4random_uniform(SLEEP_MAX);
        data[1]._inter_delay[i] = r * SLEEP_FACTOR_2 + SLEEP_OFFSET_2;
        r = arc4random_uniform(SLEEP_MAX);
        data[1]._intra_delay[i] = r * SLEEP_FACTOR_2 + SLEEP_OFFSET_2;
#endif
    }
    
    // Measuring time before starting the threads...
    gettimeofday(&tv1, NULL);
    
    pthread_create(&thr1, NULL, consumer, &data[0]);
    pthread_create(&thr2, NULL, consumer, &data[1]);
    
    pthread_join(thr1, NULL);
    pthread_join(thr2, NULL);
    
    // Measuring time after threads finished...
    gettimeofday(&tv2, NULL);
    
    if (tv1.tv_usec > tv2.tv_usec) {
        tv2.tv_sec--;
        tv2.tv_usec += 1000000;
    }
    
    printf("Thread #%d: min latency = %llu\n", data[0]._tid, data[0]._min_latency);
    printf("Thread #%d: max latency = %llu\n", data[0]._tid, data[0]._max_latency);
    printf("Thread #%d: mean latency = %f, count = %llu\n", data[0]._tid, (double)data[0]._cumul_latency / (double)data[0]._count, data[0]._count);
    
    printf("Thread #%d: min latency = %llu\n", data[1]._tid, data[1]._min_latency);
    printf("Thread #%d: max latency = %llu\n", data[1]._tid, data[1]._max_latency);
    printf("Thread #%d: mean latency = %f, count = %llu\n", data[1]._tid, (double)data[1]._cumul_latency / (double)data[1]._count, data[1]._count);
    
#if LOCK_KIND == USE_ADAPTIVE_1 || LOCK_KIND == USE_ADAPTIVE_2
    printf("Lock: spins = %lld, spins count = %llu, locks count = %llu\n", _lock._spins, _lock._spinsCount, _lock._locksCount);
#elif LOCK_KIND == USE_ADAPTIVE_3
    printf("Lock: spins = %ld\n", _lock._spins);
#endif
    
    printf("Result - %ld.%06d\n", tv2.tv_sec - tv1.tv_sec,
           tv2.tv_usec - tv1.tv_usec);
    
    return 0;
}
