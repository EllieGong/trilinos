/*------------------------------------------------------------------------*/
/*                 Copyright 2011 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stddef.h>                     // for size_t, ptrdiff_t
#include <stdlib.h>                     // for free, malloc, rand, realloc
#include <string.h>                     // for memset
#include <unistd.h>                     // for sbrk
#include <iomanip>                      // for operator<<, setw
#include <iostream>                     // for operator<<, basic_ostream, etc
#include <stk_util/environment/CPUTime.hpp>  // for cpu_time
#include <stk_util/unit_test_support/stk_utest_macros.hpp>



STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_8)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 8;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_16)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 16;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_32)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 32;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_1024)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 1024;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_100x1024)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 1024;

  for (size_t i = 0; i != 100; ++i)  {

    size_t start = malloc_used();

    size_t used = 0;

    char *x = static_cast<char *>(malloc(bytes_to_allocate));

    used += malloc_used();

    free(x);

    size_t end = malloc_used();

    STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
    STKUNIT_EXPECT_LE(start, end);
    std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
  }
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_1M)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 1024*1024;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_1_100M)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 100*1024*1024;

  size_t start = malloc_used();

  size_t used = 0;

  char *x = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  free(x);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_100_32)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 32;

  size_t start = malloc_used();

  char *x[100];
  size_t used = 0;

  for (size_t i = 0; i < 100; ++i)
    x[i] = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  for (size_t i = 0; i < 100; ++i)
    free(x[i]);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate*100, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_100_1024)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 1024;

  size_t start = malloc_used();

  char *x[100];
  size_t used = 0;

  for (size_t i = 0; i < 100; ++i)
    x[i] = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  for (size_t i = 0; i < 100; ++i)
    free(x[i]);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate*100, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

STKUNIT_UNIT_TEST(UnitTestMallocUsed, Malloc_100_1M)
{
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  static const size_t bytes_to_allocate = 1024*1024;

  size_t start = malloc_used();

  char *x[100];
  size_t used = 0;

  for (size_t i = 0; i < 100; ++i)
    x[i] = static_cast<char *>(malloc(bytes_to_allocate));

  used += malloc_used();

  for (size_t i = 0; i < 100; ++i)
    free(x[i]);

  size_t end = malloc_used();

  STKUNIT_EXPECT_LE(bytes_to_allocate*100, used - start);
  STKUNIT_EXPECT_LE(start, end);
  std::cout << "start " << start << ", end " << end << ", used " << used - start << std::endl;
#endif
}

const int MAXP = 4000;
const int SUBP = 200;
const int NPASS = 25;
const int NLOOP = 12;

int lrand()
{
  static unsigned long long next = 0;
  next = next * 0x5deece66dLL + 11;
  return static_cast<int>((next >> 16) & 0x7fffffff);
}

int rsize()
{
  int rv = 8 << (lrand() % 24);
  rv = lrand() & (rv-1);
  return rv;
}

STKUNIT_UNIT_TEST(UnitTestMalloc, DISABLED_Performance)
{
  void *pointers[MAXP];
  int size[MAXP];

  int i = 0, r = 0, loop = 0, pass = 0, subpass = 0;
  size_t absmax=0, curmax=0;
  int realloc_mask = -1;
  size_t allocations = 0;
  size_t allocations_size = 0;
  size_t frees = 0;

  memset(pointers, 0, MAXP*sizeof(pointers[0]));

  double start_time = stk::cpu_time();
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  free(malloc(1));

  ptrdiff_t start_mem = malloc_used();
  ptrdiff_t start_footprint = malloc_footprint();
#else
  ptrdiff_t start_mem = reinterpret_cast<ptrdiff_t>(sbrk(0));
#endif

#if defined SIERRA_PTMALLOC3_ALLOCATOR
  std::cout << "Modified ptmalloc3 allocator: ";
#elif defined SIERRA_PTMALLOC2_ALLOCATOR
  std::cout << "Modified ptmalloc2 allocator: ";
#else
  std::cout << "Default allocator: ";
#endif

#ifndef NDEBUG
  std::cout << "(debug)" << std::endl;
#endif

  std::cout << std::endl;

#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
  std::cout << "Start used " << start_mem << std::endl;
  std::cout << "Start footprint " << start_footprint << std::endl;
#endif

  std::cout << std::endl
            << std::setw(14) << "elapsed" << "       "

#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
            << std::setw(14) << "footprint" << "   "
            << std::setw(14) << "max_footprint" << "   "
#endif
            << std::setw(14) << "used " << "   "
            << std::setw(14) << "curmax" << " " << std::endl;

  for (loop=0; loop<NLOOP; loop++) {
    for (pass=0; pass<NPASS; pass++) {
      for (subpass=0; subpass<SUBP; subpass++) {
	for (i=0; i<MAXP; i++) {
	  int rno = rand();
	  if (rno & 8) {
	    if (pointers[i]) {
	      if (!(rno & realloc_mask)) {
		r = rsize();
		curmax -= size[i];
		curmax += r;
		pointers[i] = realloc(pointers[i], rsize());
		size[i] = r;
		if (absmax < curmax) absmax = curmax;
	      }
	      else {
		curmax -= size[i];
		free(pointers[i]);
		pointers[i] = 0;
                ++frees;
	      }
	    }
	    else {
	      r = rsize();
	      curmax += r;
	      pointers[i] = malloc(r);
	      size[i] = r;
              ++allocations;
              allocations_size += r;
	      if (absmax < curmax) absmax = curmax;
	    }
	  }
	}
      }
    }

    double end_time = stk::cpu_time();
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
    ptrdiff_t end_mem = malloc_used();
    ptrdiff_t end_footprint = malloc_footprint();
#else
    ptrdiff_t end_mem = reinterpret_cast<ptrdiff_t>(sbrk(0));
#endif

    double elapsed = end_time - start_time;
    std::cout << std::setw(14) << elapsed << " ticks "
#if defined SIERRA_PTMALLOC3_ALLOCATOR || defined SIERRA_PTMALLOC2_ALLOCATOR
              << std::setw(14) << (end_footprint - start_footprint)/1024 << " K "
              << std::setw(14) << malloc_max_footprint()/1024 << " K "
#endif
              << std::setw(14) << (end_mem - start_mem)/1024 << " K "
              << std::setw(14) << curmax/1024 << " K" << std::endl;
  }

  std::cout << allocations << " allocations of " << allocations_size/1024 << " K " << std::endl
            << frees << " frees" << std::endl;

  for (i=0; i<MAXP; i++)
    if (pointers[i])
      free(pointers[i]);
}
