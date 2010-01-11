/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/**
 * Only memory barriers *must* be ported
 * if XCNG (x86-sematics) is provided, spinlocks will be enabled
 */
#ifndef NDB_MT_ASM_H
#define NDB_MT_ASM_H

#if defined(__GNUC__)
/********************
 * GCC
 *******************/
#if defined(__x86_64__) || defined (__i386__)
/* Memory barriers, these definitions are for x64_64. */
#define mb()    asm volatile("mfence":::"memory")
/* According to Intel docs, it does not reorder loads. */
//#define rmb() asm volatile("lfence":::"memory")                               
#define rmb()   asm volatile("" ::: "memory")
#define wmb()   asm volatile("" ::: "memory")
#define read_barrier_depends()  do {} while(0)

#define NDB_HAVE_XCNG
static inline
int
xcng(volatile unsigned * addr, int val)
{
  asm volatile ("xchg %0, %1;" : "+r" (val) , "+m" (*addr));
  return val;
}
static
inline
void
cpu_pause()
{
  asm volatile ("rep;nop");
}

#elif defined(__sparc__)
#define mb()    asm volatile("membar #LoadLoad | #LoadStore | #StoreLoad | #StoreStore":::"memory")
#define rmb()   asm volatile("membar #LoadLoad" ::: "memory")
#define wmb()   asm volatile("membar #StoreStore" ::: "memory")
#define read_barrier_depends()  do {} while(0)

#ifdef HAVE_ATOMIC_H
#include <atomic.h>
#endif

#ifdef HAVE_ATOMIC_SWAP_32
static inline
int
xcng(volatile unsigned * addr, int val)
{
  asm volatile("membar #StoreLoad | #LoadLoad");
  int ret = atomic_swap_32(addr, val);
  asm volatile("membar #StoreLoad | #StoreStore");
  return ret;
}
#define cpu_pause()
#define NDB_HAVE_XCNG
#else
// link error if used incorrectly (i.e wo/ having NDB_HAVE_XCNG)
extern  int xcng(volatile unsigned * addr, int val);
extern void cpu_pause();
#endif

#else
#error "Unsupported architecture (gcc)"
#endif

#elif defined(__sun)
/********************
 * SUN STUDIO
 *******************/

/**
 * TODO check that asm ("") implies a compiler barrier
 *      i.e that it clobbers memory
 */
#if defined(__x86_64__)
#define mb()    asm ("mfence")
/* According to Intel docs, it does not reorder loads. */
//#define rmb() asm ("lfence")
#define rmb()   asm ("")
#define wmb()   asm ("")
#define read_barrier_depends()  do {} while(0)
#elif defined(__sparc)
#define mb() asm ("membar #LoadLoad | #LoadStore | #StoreLoad | #StoreStore")
#define rmb() asm ("membar #LoadLoad")
#define wmb() asm ("membar #StoreStore")
#define read_barrier_depends()  do {} while(0)
#else
#error "Unsupported architecture (sun studio)"
#endif
#if defined(__x86_64__) || defined(__sparc)
/**
 * we should probably use assembler for x86 aswell...
 *   but i'm not really sure how you do this in sun-studio :-(
 */
#ifdef HAVE_ATOMIC_H
#include <atomic.h>
#endif

#ifdef HAVE_ATOMIC_SWAP_32
#define NDB_HAVE_XCNG
#if defined(__sparc)
static inline
int
xcng(volatile unsigned * addr, int val)
{
  asm ("membar #StoreLoad | #LoadLoad");
  int ret = atomic_swap_32(addr, val);
  asm ("membar #StoreLoad | #StoreStore");
  return ret;
}
#define cpu_pause()
#elif defined(__x86_64__)
static inline
int
xcng(volatile unsigned * addr, int val)
{
  /**
   * TODO check that atomic_swap_32 on x86-64 with sun-studio implies
   *  proper barriers
   */
  int ret = atomic_swap_32(addr, val);
  return ret;
}
static
inline
void
cpu_pause()
{
  asm volatile ("rep;nop");
}
#endif
#else
// link error if used incorrectly (i.e wo/ having NDB_HAVE_XCNG)
extern  int xcng(volatile unsigned * addr, int val);
extern void cpu_pause();
#endif
#endif
#elif defined (_MSC_VER)
#include <intrin.h>
/********************
 * Microsoft
 *******************/
// Using instrinsics available on all architectures
#define mb()    _ReadWriteBarrier()
#define rmb()   _ReadBarrier()
#define wmb()   _WriteBarrier()
#define read_barrier_depends()  do {} while(0)

#define NDB_HAVE_XCNG
static inline
int
xcng(volatile unsigned * addr, int val)
{
  return InterlockedExchange((volatile LONG*)addr, val);
}

static
inline
void
cpu_pause()
{
  YieldProcessor();
}
#else
#error "Unsupported compiler"
#endif

#endif
