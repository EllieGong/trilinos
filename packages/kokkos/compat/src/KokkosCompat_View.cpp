/*
//@HEADER
// ************************************************************************
//
//                             Kokkos
//         Manycore Performance-Portable Multidimensional Arrays
//
//              Copyright (2012) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions?  Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include "KokkosCompat_View.hpp"
#include "KokkosCompat_View_def.hpp"

#include "Kokkos_Core.hpp"

namespace Kokkos {
  namespace Compat {

#if defined(KOKKOS_HAVE_SERIAL)
#define COMPAT_INSTANT_SERIAL(T) \
    COMPAT_INSTANT(T,Kokkos::Serial)
#else
#  define COMPAT_INSTANT_SERIAL(T)
#endif // defined(KOKKOS_HAVE_SERIAL)

#if defined(KOKKOS_HAVE_PTHREAD)
#define COMPAT_INSTANT_THREADS(T) \
    COMPAT_INSTANT(T,Kokkos::Threads)
#else
#define COMPAT_INSTANT_THREADS(T)
#endif

#if defined(KOKKOS_HAVE_OPENMP)
#define COMPAT_INSTANT_OPENMP(T) \
    COMPAT_INSTANT(T,Kokkos::OpenMP)
#else
#define COMPAT_INSTANT_OPENMP(T)
#endif

#define COMPAT_INSTANT_ALL(T) \
    COMPAT_INSTANT_SERIAL(T) \
    COMPAT_INSTANT_THREADS(T) \
    COMPAT_INSTANT_OPENMP(T) \
    COMPAT_INSTANT(T,Kokkos::HostSpace)

    COMPAT_INSTANT_ALL(float)
    COMPAT_INSTANT_ALL(double)
    COMPAT_INSTANT_ALL(int)
    COMPAT_INSTANT_ALL(long)
    COMPAT_INSTANT_ALL(unsigned)
    COMPAT_INSTANT_ALL(unsigned long)
    COMPAT_INSTANT_ALL(char)
    COMPAT_INSTANT_ALL(short)

  } // namespace Compat
} // namespace Kokkos
