// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef STOKHOS_CRSMATRIX_MP_VECTOR_HPP
#define STOKHOS_CRSMATRIX_MP_VECTOR_HPP

#include "Kokkos_Parallel.hpp"
#include "Kokkos_View.hpp"
#include "Kokkos_CrsArray.hpp"

#include "Stokhos_CrsMatrix.hpp"
#include "Stokhos_Multiply.hpp"
#include "Sacado_MP_Vector.hpp"
#include "Sacado_View_MP_Vector.hpp"

#include "Kokkos_Cuda.hpp" // for determining work range

namespace Stokhos {

/*
 * Compute work range = (begin, end) such that adjacent threads/blocks write to
 * separate cache lines
 */
template <typename scalar_type, typename device_type, typename size_type>
KOKKOS_INLINE_FUNCTION
size_type
compute_work_range( const size_type work_count,
                    const size_type thread_count,
                    const size_type thread_rank,
                    size_type& work_begin)
{
  enum { cache_line =
         Kokkos::Impl::is_same<device_type,Kokkos::Cuda>::value ? 128 : 64 };
  enum { work_align = cache_line / sizeof(scalar_type) };
  enum { work_shift = Kokkos::Impl::power_of_two< work_align >::value };
  enum { work_mask  = work_align - 1 };

  const size_type work_per_thread =
    ( ( ( ( work_count + work_mask ) >> work_shift ) + thread_count - 1 ) /
      thread_count ) << work_shift ;

  work_begin = thread_rank * work_per_thread;
  size_type work_end = work_begin + work_per_thread;
  if (work_begin > work_count)
    work_begin = work_count;
  if (work_end > work_count)
    work_end = work_count;
  return work_end;
}

// Specialization of multiply for CrsMatrix< Sacado::MP::Vector<...>, ... >
// using overloaded MP::Vector arithmetic operations.
template <typename MatrixStorage,
          typename MatrixDevice,
          typename MatrixLayout,
          typename InputVectorType,
          typename OutputVectorType>
class Multiply< CrsMatrix<Sacado::MP::Vector<MatrixStorage>,
                          MatrixDevice,
                          MatrixLayout>,
                InputVectorType,
                OutputVectorType,
                void,
                IntegralRank<1>,
                DefaultMultiply >
{
public:
  typedef Sacado::MP::Vector<MatrixStorage> MatrixValue;
  typedef typename InputVectorType::value_type InputVectorValue;
  typedef typename OutputVectorType::value_type OutputVectorValue;

  typedef MatrixDevice device_type;
  typedef typename device_type::size_type size_type;

  typedef CrsMatrix< MatrixValue, device_type, MatrixLayout > matrix_type;
  typedef typename matrix_type::values_type matrix_values_type;
  typedef InputVectorType input_vector_type;
  typedef OutputVectorType output_vector_type;

  typedef Kokkos::View< typename matrix_values_type::data_type,
                        typename matrix_values_type::array_layout,
                        device_type,
                        Kokkos::MemoryUnmanaged > matrix_local_view_type;
  typedef Kokkos::View< typename input_vector_type::data_type,
                        typename input_vector_type::array_layout,
                        device_type,
                        Kokkos::MemoryUnmanaged > input_local_view_type;
  typedef Kokkos::View< typename output_vector_type::data_type,
                        typename output_vector_type::array_layout,
                        device_type,
                        Kokkos::MemoryUnmanaged > output_local_view_type;
  typedef typename matrix_local_view_type::Partition matrix_partition_type;
  typedef typename input_local_view_type::Partition input_partition_type;
  typedef typename output_local_view_type::Partition output_partition_type;

  typedef OutputVectorValue scalar_type;

  const matrix_type  m_A;
  const input_vector_type  m_x;
  output_vector_type  m_y;

  Multiply( const matrix_type & A,
            const input_vector_type & x,
            output_vector_type & y )
    : m_A( A )
    , m_x( x )
    , m_y( y )
    {}

  KOKKOS_INLINE_FUNCTION
  void operator()( device_type dev ) const
  {
    // 2-D distribution of threads: num_vector_threads x num_row_threads
    // where the x-dimension are vector threads and the y dimension are
    // row threads
    const size_type num_vector_threads = m_A.dev_config.block_dim.x;
    const size_type num_row_threads = m_A.dev_config.block_dim.y;
    const size_type vector_rank = dev.team_rank() % num_vector_threads;
    const size_type row_rank = dev.team_rank() / num_vector_threads;

    // Create local views with corresponding offset into the vector
    // dimension based on vector_rank
    matrix_partition_type matrix_partition(vector_rank, num_vector_threads);
    input_partition_type input_partition(vector_rank, num_vector_threads);
    output_partition_type output_partition(vector_rank, num_vector_threads);
    const matrix_local_view_type A(m_A.values, matrix_partition);
    const input_local_view_type x(m_x, input_partition);
    const output_local_view_type y(m_y, output_partition);

    // Compute range of rows processed for each thread block
    const size_type row_count = m_A.graph.row_map.dimension_0()-1;
    size_type work_begin;
    const size_type work_end =
      compute_work_range<typename scalar_type::value_type,device_type,size_type>(
        row_count, dev.league_size(), dev.league_rank(), work_begin);

    // To make better use of L1 cache on the CPU/MIC, we move through the
    // row range where each thread processes a cache-line's worth of rows,
    // with adjacent threads processing adjacent cache-lines.
    // For Cuda, adjacent threads process adjacent rows.
    const size_type cache_line =
      Kokkos::Impl::is_same<device_type,Kokkos::Cuda>::value ? 1 : 64;
    const size_type scalar_size = sizeof(scalar_type);
    const size_type rows_per_thread = (cache_line+scalar_size-1)/scalar_size;
    const size_type row_block_size = rows_per_thread * num_row_threads;

    scalar_type sum;

    // Loop over rows in blocks of row_block_size
    for (size_type iBlockRow=work_begin+row_rank*rows_per_thread;
         iBlockRow<work_end; iBlockRow+=row_block_size) {

      // Loop over rows within block
      const size_type row_end = iBlockRow+rows_per_thread <= work_end ?
        rows_per_thread : work_end - iBlockRow;
      for (size_type row=0; row<row_end; ++row) {
        const size_type iRow = iBlockRow + row;

        // Compute mat-vec for this row
        const size_type iEntryBegin = m_A.graph.row_map[iRow];
        const size_type iEntryEnd   = m_A.graph.row_map[iRow+1];
        sum = 0.0;
        for (size_type iEntry = iEntryBegin; iEntry < iEntryEnd; ++iEntry) {
          size_type iCol = m_A.graph.entries(iEntry);
          sum += A(iEntry) * x(iCol);
        }
        y(iRow) = sum;

      } // row loop

    } // block row loop

  } // operator()

  static void apply( const matrix_type & A,
                     const input_vector_type & x,
                     output_vector_type & y )
  {
    const size_type league_size = A.dev_config.num_blocks;
    const size_type team_size = A.dev_config.num_threads_per_block;
    Kokkos::ParallelWorkRequest config(league_size, team_size);
    Kokkos::parallel_for( config, Multiply(A,x,y) );
  }
};

// Specialization of multiply for CrsMatrix< Sacado::MP::Vector<...>, ... >
// that uses the 2-D view directly.
// Currently this only works for statically sized MP::Vector
//
// Note:  This appears to give the wrong answer with Cuda and LayoutLeft
// sometimes, and not others, so there is probably a race-condition somewhere.
struct EnsembleMultiply {};
template <typename MatrixStorage,
          typename MatrixDevice,
          typename MatrixLayout,
          typename InputVectorType,
          typename OutputVectorType>
class Multiply< CrsMatrix<Sacado::MP::Vector<MatrixStorage>,
                          MatrixDevice,
                          MatrixLayout>,
                InputVectorType,
                OutputVectorType,
                void,
                IntegralRank<1>,
                EnsembleMultiply >
{
public:
  typedef Sacado::MP::Vector<MatrixStorage> MatrixValue;
  typedef typename InputVectorType::value_type InputVectorValue;
  typedef typename OutputVectorType::value_type OutputVectorValue;

  typedef MatrixDevice device_type;
  typedef typename device_type::size_type size_type;

  typedef CrsMatrix< MatrixValue, device_type, MatrixLayout > matrix_type;
  typedef typename matrix_type::values_type matrix_values_type;
  typedef InputVectorType input_vector_type;
  typedef OutputVectorType output_vector_type;

  typedef typename OutputVectorValue::value_type scalar_type;
  typedef typename InputVectorValue::value_type x_scalar_type;
  typedef typename OutputVectorValue::value_type y_scalar_type;
  typedef typename MatrixValue::value_type A_scalar_type;
  static const size_type NumPerThread = MatrixStorage::static_size;

  const matrix_type  m_A;
  const input_vector_type  m_x;
  output_vector_type  m_y;

  const typename matrix_values_type::array_type m_Avals ;
  const typename input_vector_type::array_type  m_Xvals ;
  const typename output_vector_type::array_type m_Yvals ;

  Multiply( const matrix_type & A,
            const input_vector_type & x,
            output_vector_type & y )
    : m_A( A )
    , m_x( x )
    , m_y( y )
    , m_Avals( A.values )
    , m_Xvals( x )
    , m_Yvals( y )
    {}

  KOKKOS_INLINE_FUNCTION
  void operator()( device_type dev ) const
  {
    // 2-D distribution of threads: num_vector_threads x num_row_threads
    // where the x-dimension are vector threads and the y dimension are
    // row threads
    const size_type num_vector_threads = m_A.dev_config.block_dim.x;
    const size_type num_row_threads = m_A.dev_config.block_dim.y;
    const size_type vector_rank = dev.team_rank() % num_vector_threads;
    const size_type row_rank = dev.team_rank() / num_vector_threads;

    // We have to extract pointers to the A, x, and y views below
    // because the Intel compiler does not appear to be able to vectorize
    // through them.  Thus we need to compute the correct stride for
    // LayoutLeft.
    using Kokkos::Impl::is_same;
    using Kokkos::LayoutRight;
    typedef typename matrix_values_type::array_layout matrix_layout;
    typedef typename input_vector_type::array_layout input_layout;
    typedef typename output_vector_type::array_layout output_layout;
    size_type As[2], xs[2], ys[2];
    m_A.values.stride(As);
    m_x.stride(xs);
    m_y.stride(ys);
    const bool is_cuda = is_same<device_type, Kokkos::Cuda>::value;
    const size_type stride_one =
      is_cuda ? num_vector_threads : size_type(1);
    const size_type A_stride =
      is_same<matrix_layout, LayoutRight>::value ? stride_one : As[1];
    const size_type x_stride =
      is_same<input_layout,  LayoutRight>::value ? stride_one : xs[1];
    const size_type y_stride =
      is_same<output_layout, LayoutRight>::value ? stride_one : ys[1];

    // Compute range of rows processed for each thread block
    const size_type row_count = m_A.graph.row_map.dimension_0()-1;
    size_type work_begin;
    const size_type work_end =
      compute_work_range<scalar_type,device_type,size_type>(
        row_count, dev.league_size(), dev.league_rank(), work_begin);
    const size_type vector_offset =
      is_cuda ? vector_rank : vector_rank*NumPerThread;

    // To make better use of L1 cache on the CPU/MIC, we move through the
    // row range where each thread processes a cache-line's worth of rows,
    // with adjacent threads processing adjacent cache-lines.
    // For Cuda, adjacent threads process adjacent rows.
    const size_type cache_line =
      Kokkos::Impl::is_same<device_type,Kokkos::Cuda>::value ? 1 : 64;
    const size_type scalar_size = sizeof(scalar_type);
    const size_type rows_per_thread = (cache_line+scalar_size-1)/scalar_size;
    const size_type row_block_size = rows_per_thread * num_row_threads;

    scalar_type sum[NumPerThread];

    // Loop over rows in blocks of row_block_size
    for (size_type iBlockRow=work_begin+row_rank*rows_per_thread;
         iBlockRow<work_end; iBlockRow+=row_block_size) {

      // Loop over rows within block
      const size_type row_end = iBlockRow+rows_per_thread <= work_end ?
        rows_per_thread : work_end - iBlockRow;
      for (size_type row=0; row<row_end; ++row) {
        const size_type iRow = iBlockRow + row;

        // Compute mat-vec for this row
        const size_type iEntryBegin = m_A.graph.row_map[iRow];
        const size_type iEntryEnd   = m_A.graph.row_map[iRow+1];

        // y_scalar_type * const y = &m_y(iRow,vector_offset);
        y_scalar_type * const y = &m_Yvals(iRow,vector_offset);

        for (size_type e=0; e<NumPerThread; ++e)
          sum[e] = 0;

        for ( size_type iEntry = iEntryBegin; iEntry < iEntryEnd; ++iEntry ) {
          size_type iCol = m_A.graph.entries(iEntry);

          // const A_scalar_type * const A = &m_A.values(iEntry,vector_offset);
          // const x_scalar_type * const x = &m_x(iCol,vector_offset);
          const A_scalar_type * const A = &m_Avals(iEntry,vector_offset);
          const x_scalar_type * const x = &m_Xvals(iCol,vector_offset);

          for (size_type e=0; e<NumPerThread; ++e)
            sum[e] += A[e*A_stride] * x[e*x_stride];
        }

        for (size_type e=0; e<NumPerThread; ++e)
          y[e*y_stride] = sum[e];

      } // row loop

    } // block row loop

  } // operator()

  static void apply( const matrix_type & A,
                     const input_vector_type & x,
                     output_vector_type & y )
  {
    const size_type league_size = A.dev_config.num_blocks;
    const size_type team_size = A.dev_config.num_threads_per_block;
    Kokkos::ParallelWorkRequest config(league_size, team_size);
    Kokkos::parallel_for( config, Multiply(A,x,y) );
  }
};

template <typename MatrixType,
          typename InputVectorType,
          typename OutputVectorType>
void multiply(const MatrixType& A,
              const InputVectorType& x,
              OutputVectorType& y,
              EnsembleMultiply tag) {
  typedef Multiply<MatrixType,InputVectorType,OutputVectorType,void,typename ViewRank<InputVectorType>::type,EnsembleMultiply> multiply_type;
  multiply_type::apply( A, x, y );
}

}

#endif /* #ifndefSTOKHOS_CRSMATRIX_MP_VECTOR_HPP */
