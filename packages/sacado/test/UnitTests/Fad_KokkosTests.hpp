// @HEADER
// ***********************************************************************
//
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER
#include "Teuchos_TestingHelpers.hpp"

#include "Kokkos_View.hpp"
#include "Kokkos_Parallel.hpp"
#include "Sacado.hpp"

template <typename FadType1, typename FadType2>
bool checkFads(const FadType1& x, const FadType2& x2,
               Teuchos::FancyOStream& out)
{
  bool success = true;

  // Check sizes match
  TEUCHOS_TEST_EQUALITY(x.size(), x2.size(), out, success);

  // Check values match
  TEUCHOS_TEST_EQUALITY(x.val(), x2.val(), out, success);

  // Check derivatives match
  for (int i=0; i<x.size(); ++i)
    TEUCHOS_TEST_EQUALITY(x.dx(i), x2.dx(i), out, success);

  return success;
}

template <typename fadtype, typename ordinal>
inline
fadtype generate_fad( const ordinal num_rows,
                      const ordinal num_cols,
                      const ordinal fad_size,
                      const ordinal row,
                      const ordinal col )
{
  typedef typename fadtype::value_type scalar;
  fadtype x(fad_size, scalar(0.0));

  const scalar x_row = 100.0 + scalar(num_rows) / scalar(row+1);
  const scalar x_col =  10.0 + scalar(num_cols) / scalar(col+1);
  x.val() = x_row + x_col;
  for (ordinal i=0; i<fad_size; ++i) {
    const scalar x_fad = 1.0 + scalar(fad_size) / scalar(i+1);
    x.fastAccessDx(i) = x_row + x_col + x_fad;
  }
  return x;
}

template <typename DataType, typename LayoutType, typename DeviceType>
struct ApplyView {
  typedef Kokkos::View<DataType,LayoutType,DeviceType> type;
};

struct NoLayout {};
template <typename DataType, typename DeviceType>
struct ApplyView<DataType,NoLayout,DeviceType> {
  typedef Kokkos::View<DataType,DeviceType> type;
};

const int global_num_rows = 11;
const int global_num_cols = 7;
const int global_fad_size = 5;

// Kernel to multiply two views
template <typename ViewType>
struct MultiplyKernel {
  typedef typename ViewType::device_type device_type;
  typedef typename ViewType::size_type size_type;

  const ViewType m_v1, m_v2, m_v3;

  MultiplyKernel(const ViewType v1,
                 const ViewType v2,
                 const ViewType v3) :
    m_v1(v1), m_v2(v2), m_v3(v3) {};

  // Multiply entries for row 'i' with a value
  KOKKOS_INLINE_FUNCTION
  void operator() (const size_type i) const {
    m_v3(i) = m_v1(i)*m_v2(i);
  }

  // Kernel launch
  static void apply(const ViewType v1,
                    const ViewType v2,
                    const ViewType v3) {
    const size_type nrow = v1.dimension_0();
    Kokkos::parallel_for( nrow, MultiplyKernel(v1,v2,v3) );
  }
};

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_View_Fad, DeepCopy, FadType, Layout, Device )
{
  typedef typename ApplyView<FadType**,Layout,Device>::type ViewType;
  typedef typename ViewType::size_type size_type;
  typedef typename ViewType::HostMirror host_view_type;

  const size_type num_rows = global_num_rows;
  const size_type num_cols = global_num_cols;
  const size_type fad_size = global_fad_size;

  // Create and fill view
  ViewType v("view", num_rows, num_cols);
  host_view_type h_v = Kokkos::create_mirror_view(v);
  for (size_type i=0; i<num_rows; ++i)
    for (size_type j=0; j<num_cols; ++j)
      h_v(i,j) = generate_fad<FadType>(num_rows, num_cols, fad_size, i, j);
  Kokkos::deep_copy(v, h_v);

  // Copy back
  host_view_type h_v2 = Kokkos::create_mirror_view(v);
  Kokkos::deep_copy(h_v2, v);

  // Check
  success = true;
  for (size_type i=0; i<num_rows; ++i) {
    for (size_type j=0; j<num_cols; ++j) {
      FadType f = generate_fad<FadType>(num_rows, num_cols, fad_size, i, j);
      success = success && checkFads(f, h_v2(i,j), out);
    }
  }
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_View_Fad, Multiply, FadType, Layout, Device )
{
  typedef typename ApplyView<FadType*,Layout,Device>::type ViewType;
  typedef typename ViewType::size_type size_type;
  typedef typename ViewType::HostMirror host_view_type;

  const size_type num_rows = global_num_rows;
  const size_type fad_size = global_fad_size;

  // Create and fill views
  ViewType v1("view1", num_rows), v2("view2", num_rows);
  host_view_type h_v1 = Kokkos::create_mirror_view(v1);
  host_view_type h_v2 = Kokkos::create_mirror_view(v2);
  for (size_type i=0; i<num_rows; ++i) {
    h_v1(i) = generate_fad<FadType>(
      num_rows, size_type(2), fad_size, i, size_type(0));
    h_v2(i) = generate_fad<FadType>(
      num_rows, size_type(2), fad_size, i, size_type(1));
  }
  Kokkos::deep_copy(v1, h_v1);
  Kokkos::deep_copy(v2, h_v2);

  // Launch kernel
  ViewType v3("view3", num_rows);
  MultiplyKernel<ViewType>::apply(v1,v2,v3);

  // Copy back
  host_view_type h_v3 = Kokkos::create_mirror_view(v3);
  Kokkos::deep_copy(h_v3, v3);

  // Check
  success = true;
  for (size_type i=0; i<num_rows; ++i) {
    FadType f1 =
      generate_fad<FadType>(num_rows, size_type(2), fad_size, i, size_type(0));
    FadType f2 =
      generate_fad<FadType>(num_rows, size_type(2), fad_size, i, size_type(1));
    FadType f3 = f1*f2;
    success = success && checkFads(f3, h_v3(i), out);
  }
}

#define VIEW_FAD_TESTS_FLD( F, L, D )                                   \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Kokkos_View_Fad, DeepCopy, F, L, D ) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT( Kokkos_View_Fad, Multiply, F, L, D )

#define VIEW_FAD_TESTS_FD( F, D )                                       \
  using Kokkos::LayoutLeft;                                             \
  using Kokkos::LayoutRight;                                            \
  VIEW_FAD_TESTS_FLD( F, NoLayout, D)                                   \
  VIEW_FAD_TESTS_FLD( F, LayoutLeft, D)                                 \
  VIEW_FAD_TESTS_FLD( F, LayoutRight, D)

typedef Sacado::Fad::DFad<double> DFadType;
typedef Sacado::Fad::SLFad<double,2*global_fad_size> SLFadType;
typedef Sacado::Fad::SFad<double,global_fad_size> SFadType;

#define VIEW_FAD_TESTS_D( D )                   \
  VIEW_FAD_TESTS_FD( SFadType, D )          \
  VIEW_FAD_TESTS_FD( SLFadType, D )
