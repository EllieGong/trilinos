/*
//@HEADER
// ************************************************************************
// 
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
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
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov) 
// 
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_ARRAYVIEWDEFAULT_HPP
#define KOKKOS_ARRAYVIEWDEFAULT_HPP

#include <Kokkos_Macros.hpp>
#include <Kokkos_View.hpp>
#include <impl/Kokkos_Shape.hpp>

namespace Kokkos {
namespace Impl {

struct LayoutEmbedArray {};

template< typename ScalarType , unsigned Count ,
          class Rank , class RankDynamic ,
          class MemorySpace , class MemoryTraits >
struct ViewSpecialize< ScalarType ,
                       Array< ScalarType , Count , void > ,
                       LayoutLeft , Rank , RankDynamic ,
                       MemorySpace , MemoryTraits >
{ typedef LayoutEmbedArray type ; };

template< typename ScalarType , unsigned Count ,
          class Rank , class RankDynamic ,
          class MemorySpace , class MemoryTraits >
struct ViewSpecialize< ScalarType ,
                       Array< ScalarType , Count , void > ,
                       LayoutRight , Rank , RankDynamic ,
                       MemorySpace , MemoryTraits >
{ typedef LayoutEmbedArray type ; };

//-----------------------------------------------------------------------------

template<>
struct ViewAssignment<LayoutEmbedArray,void,void>
{
  typedef LayoutEmbedArray Specialize ;

private:

  template< class T , class L , class D , class M >
  inline
  void allocate( View<T,L,D,M,Specialize> & dst , const std::string & label )
  {
    typedef View<T,L,D,M,Specialize> DstViewType ;
    typedef typename DstViewType::scalar_type   scalar_type ;
    typedef typename DstViewType::memory_space  memory_space ;

    const size_t cap = Impl::capacity( dst.m_shape , dst.m_stride );

    dst.m_ptr_on_device = (scalar_type *)
      memory_space::allocate( label , typeid(scalar_type) , sizeof(scalar_type) , cap );

    ViewInitialize< DstViewType > init( dst );
  }

public:

  template< class T , class L , class D , class M >
  inline
  ViewAssignment( View<T,L,D,M,Specialize> & dst ,
                  const typename enable_if< ViewTraits<T,L,D,M>::is_managed , std::string >::type & label ,
                  const size_t n0 = 0 ,
                  const size_t n1 = 0 ,
                  const size_t n2 = 0 ,
                  const size_t n3 = 0 ,
                  const size_t n4 = 0 ,
                  const size_t n5 = 0 ,
                  const size_t n6 = 0 ,
                  const size_t n7 = 0 )
  {
    typedef View<T,L,D,M,Specialize> DstViewType ;
    typedef typename DstViewType::scalar_type   scalar_type ;
    typedef typename DstViewType::shape_type    shape_type ;
    typedef typename DstViewType::memory_space  memory_space ;
    typedef typename DstViewType::stride_type   stride_type ;

    ViewTracking< DstViewType >::decrement( dst.m_ptr_on_device );

    shape_type ::assign( dst.m_shape, n0, n1, n2, n3, n4, n5, n6, n7 );
    stride_type::assign( dst.m_stride , dst.m_shape );

    allocate( dst , label );
  }

  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM >
  ViewAssignment(       View<DT,DL,DD,DM,Specialize> & dst ,
                  const View<ST,SL,SD,SM,Specialize> & src ,
                  typename enable_if<
                    is_same< View<DT,DL,DD,DM,Specialize> ,
                             typename View<ST,SL,SD,SM,Specialize>::HostMirror >::value
                  >::type * = 0 )
  {
    dst.m_shape  = src.m_shape ;
    dst.m_stride = src.m_stride ;
    allocate( dst , "mirror" );
  }
};

template<>
struct ViewAssignment< LayoutEmbedArray , LayoutEmbedArray , void >
{
  typedef LayoutEmbedArray Specialize ;

  /** \brief Assign compatible views */

  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM >
  KOKKOS_INLINE_FUNCTION
  ViewAssignment(       View<DT,DL,DD,DM,Specialize> & dst ,
                  const View<ST,SL,SD,SM,Specialize> & src ,
                  const typename enable_if<(
                    ViewAssignable< ViewTraits<DT,DL,DD,DM> , ViewTraits<ST,SL,SD,SM> >::value
                  )>::type * = 0 )
  {
    typedef View<DT,DL,DD,DM,Specialize> DstViewType ;
    typedef typename DstViewType::shape_type    shape_type ;
    typedef typename DstViewType::memory_space  memory_space ;
    typedef typename DstViewType::memory_traits memory_traits ;
    typedef typename DstViewType::stride_type   stride_type ;

    ViewTracking< DstViewType >::decrement( dst.m_ptr_on_device );

    shape_type::assign( dst.m_shape,
                        src.m_shape.N0 , src.m_shape.N1 , src.m_shape.N2 , src.m_shape.N3 ,
                        src.m_shape.N4 , src.m_shape.N5 , src.m_shape.N6 );

    stride_type::assign( dst.m_stride , src.m_stride.value );

    dst.m_ptr_on_device = src.m_ptr_on_device ;

    ViewTracking< DstViewType >::increment( dst.m_ptr_on_device );
  }

  //------------------------------------

  /** \brief  Extract LayoutRight Rank-N array from range of LayoutRight Rank-N array */
  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM ,
            typename iType >
  KOKKOS_INLINE_FUNCTION
  ViewAssignment(       View<DT,DL,DD,DM,Specialize> & dst ,
                  const View<ST,SL,SD,SM,Specialize> & src ,
                  const std::pair<iType,iType> & range ,
                  typename enable_if< (
                    ViewAssignable< ViewTraits<DT,DL,DD,DM> , ViewTraits<ST,SL,SD,SM> >::value
                    &&
                    Impl::is_same< typename ViewTraits<DT,DL,DD,DM>::array_layout , LayoutRight >::value
                    &&
                    ( ViewTraits<ST,SL,SD,SM>::rank > 1 )
                    &&
                    ( ViewTraits<DT,DL,DD,DM>::rank_dynamic > 0 )
                  )>::type * = 0 )
  {
    typedef ViewTraits<DT,DL,DD,DM> traits_type ;
    typedef typename traits_type::shape_type   shape_type ;
    typedef typename View<DT,DL,DD,DM,Specialize>::stride_type  stride_type ;

    ViewTracking< traits_type >::decrement( dst.m_ptr_on_device );

    shape_type ::assign( dst.m_shape, 0, 0, 0, 0, 0, 0, 0, 0 );
    stride_type::assign( dst.m_stride , 0 );
    dst.m_ptr_on_device = 0 ;

    if ( range.first < range.second ) {
      assert_shape_bounds( src.m_shape , 8 , range.first ,      0,0,0,0,0,0,0);
      assert_shape_bounds( src.m_shape , 8 , range.second - 1 , 0,0,0,0,0,0,0);

      shape_type::assign( dst.m_shape, range.second - range.first ,
                          src.m_shape.N1 , src.m_shape.N2 , src.m_shape.N3 ,
                          src.m_shape.N4 , src.m_shape.N5 , src.m_shape.N6 , src.m_shape.N7 );

      stride_type::assign( dst.m_stride , src.m_stride.value );

      dst.m_ptr_on_device = src.m_ptr_on_device + range.first * src.m_stride.value ;

      ViewTracking< traits_type >::increment( dst.m_ptr_on_device );
    }
  }

  //------------------------------------
  /** \brief  Deep copy data from compatible value type, layout, rank, and specialization.
   *          Check the dimensions and allocation lengths at runtime.
   */
  template< class DT , class DL , class DD , class DM ,
            class ST , class SL , class SD , class SM >
  inline static
  void deep_copy( const View<DT,DL,DD,DM,Specialize> & dst ,
                  const View<ST,SL,SD,SM,Specialize> & src ,
                  const typename Impl::enable_if<(
                    Impl::is_same< typename ViewTraits<DT,DL,DD,DM>::scalar_type ,
                                   typename ViewTraits<ST,SL,SD,SM>::non_const_scalar_type >::value
                    &&
                    Impl::is_same< typename ViewTraits<DT,DL,DD,DM>::array_layout ,
                                   typename ViewTraits<ST,SL,SD,SM>::array_layout >::value
                    &&
                    ( unsigned(ViewTraits<DT,DL,DD,DM>::rank) == unsigned(ViewTraits<ST,SL,SD,SM>::rank) )
                  )>::type * = 0 )
  {
    typedef ViewTraits<DT,DL,DD,DM> dst_traits ;
    typedef ViewTraits<ST,SL,SD,SM> src_traits ;

    enum { is_right = Impl::is_same<typename dst_traits::array_layout,LayoutRight>::value };

    if ( dst.m_ptr_on_device != src.m_ptr_on_device ) {

      Impl::assert_shapes_are_equal( dst.m_shape , src.m_shape );

      const size_t nbytes = dst.m_shape.scalar_size *
        ( 1 == dst_traits::rank ? dst.m_shape.N0 : (
          is_right  ? dst.m_shape.N0 * dst.m_stride.value : (
                      dst.m_stride.value  * dst.m_shape.N1 * dst.m_shape.N2 * dst.m_shape.N3 *
                      dst.m_shape.N4 * dst.m_shape.N5 * dst.m_shape.N6 * dst.m_shape.N7 )));

      DeepCopy< typename dst_traits::memory_space ,
                typename src_traits::memory_space >( dst.m_ptr_on_device , src.m_ptr_on_device , nbytes );
    }
  }
};

//----------------------------------------------------------------------------

template< class Traits , class Layout , unsigned Rank , unsigned RankOper ,
          typename iType0 = int , typename iType1 = int ,
          typename iType2 = int , typename iType3 = int ,
          typename iType4 = int , typename iType5 = int ,
          typename iType6 = int , typename iType7 = int ,
          int RankDiff =
            ( is_same< Layout , typename Traits::array_layout >::value &&
              Rank == Traits::rank &&
              iType0(0) == 0 && iType1(0) == 0 && iType2(0) == 0 && iType3(0) == 0 &&
              iType4(0) == 0 && iType5(0) == 0 && iType6(0) == 0 && iType7(0) == 0 &&
              Rank >= RankOper )
            ? int(Rank - RankOper) : -1 >
struct EnableArrayViewOper ;

template< class Traits , class Layout , unsigned Rank , unsigned RankOper ,
          typename iType0 , typename iType1 ,
          typename iType2 , typename iType3 ,
          typename iType4 , typename iType5 ,
          typename iType6 , typename iType7 >
struct EnableArrayViewOper<
  Traits , Layout , Rank , RankOper ,
  iType0 , iType1 , iType2 , iType3 ,
  iType4 , iType5 , iType6 , iType7 ,
  0 >
{
  typedef typename Traits::scalar_type type ;
};

template< class Traits ,
          unsigned Rank , unsigned RankOper ,
          typename iType0 , typename iType1 ,
          typename iType2 , typename iType3 ,
          typename iType4 , typename iType5 ,
          typename iType6 , typename iType7 >
struct EnableArrayViewOper<
  Traits , LayoutLeft , Rank , RankOper ,
  iType0 , iType1 , iType2 , iType3 ,
  iType4 , iType5 , iType6 , iType7 ,
  1 >
{
  typedef Array< typename Traits::scalar_type ,
                 ArrayTypeCount< typename Traits::value_type >::value ,
                 ArrayProxyStrided > type ;
};

template< class Traits ,
          unsigned Rank , unsigned RankOper ,
          typename iType0 , typename iType1 ,
          typename iType2 , typename iType3 ,
          typename iType4 , typename iType5 ,
          typename iType6 , typename iType7 >
struct EnableArrayViewOper<
  Traits , LayoutRight , Rank , RankOper ,
  iType0 , iType1 , iType2 , iType3 ,
  iType4 , iType5 , iType6 , iType7 ,
  1 >
{
  typedef Array< typename Traits::scalar_type ,
                 ArrayTypeCount< typename Traits::value_type >::value ,
                 ArrayProxyContiguous > type ;
};

} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {

template< class DataType , class Arg1Type , class Arg2Type , class Arg3Type >
class View< DataType , Arg1Type , Arg2Type , Arg3Type , Impl::LayoutEmbedArray >
  : public ViewTraits< DataType , Arg1Type , Arg2Type , Arg3Type >
{
private:

  template< class , class , class > friend class Impl::ViewAssignment ;

  typedef ViewTraits< DataType , Arg1Type , Arg2Type , Arg3Type > traits ;

  typedef Impl::ViewAssignment<Impl::LayoutEmbedArray> alloc ;
  typedef Impl::ViewAssignment<Impl::LayoutEmbedArray,Impl::LayoutEmbedArray> assign ;
  typedef Impl::LayoutStride< typename traits::shape_type ,
                              typename traits::array_layout > stride_type ;

  typedef Array< typename traits::scalar_type ,
                 ArrayTypeCount< typename traits::value_type >::value ,
                 ArrayProxyStrided > ArrayProxyLeftType ;

  typedef Array< typename traits::scalar_type ,
                 ArrayTypeCount< typename traits::value_type >::value ,
                 ArrayProxyContiguous > ArrayProxyRightType ;

  typename traits::scalar_type * m_ptr_on_device ;
  typename traits::shape_type    m_shape ;
  stride_type                    m_stride ;

public:

  typedef Impl::LayoutEmbedArray specialize ;

  typedef View< typename traits::const_data_type ,
                typename traits::array_layout ,
                typename traits::device_type ,
                typename traits::memory_traits > const_type ;

  typedef View< typename traits::non_const_data_type ,
                typename traits::array_layout ,
                Host > HostMirror ;

  enum { Rank = traits::rank };

  KOKKOS_INLINE_FUNCTION typename traits::shape_type shape() const { return m_shape ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_0() const { return m_shape.N0 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_1() const { return m_shape.N1 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_2() const { return m_shape.N2 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_3() const { return m_shape.N3 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_4() const { return m_shape.N4 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_5() const { return m_shape.N5 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_6() const { return m_shape.N6 ; }
  KOKKOS_INLINE_FUNCTION typename traits::size_type dimension_7() const { return m_shape.N7 ; }

  template< typename iType >
  KOKKOS_INLINE_FUNCTION
  typename traits::size_type dimension( const iType & i ) const
    { return Impl::dimension( m_shape , i ); }

  KOKKOS_INLINE_FUNCTION
  bool is_null() const { return 0 == m_ptr_on_device ; }

  KOKKOS_INLINE_FUNCTION
  View() : m_ptr_on_device(0)
    {
      traits::shape_type::assign(m_shape,0,0,0,0,0,0,0,0);
      stride_type::assign( m_stride , 0 );
    }

  KOKKOS_INLINE_FUNCTION
  ~View() { Impl::ViewTracking< traits >::decrement( m_ptr_on_device ); }

  KOKKOS_INLINE_FUNCTION
  View( const View & rhs ) : m_ptr_on_device(0) { assign( *this , rhs ); }

  KOKKOS_INLINE_FUNCTION
  View & operator = ( const View & rhs ) { assign( *this , rhs ); return *this ; }

  //------------------------------------

  template< class RT , class RL , class RD , class RM >
  KOKKOS_INLINE_FUNCTION
  View( const View<RT,RL,RD,RM,specialize> & rhs )
    : m_ptr_on_device(0) { assign( *this , rhs ); }

  template< class RT , class RL , class RD , class RM >
  KOKKOS_INLINE_FUNCTION
  View & operator = ( const View<RT,RL,RD,RM,specialize> & rhs )
    { assign( *this , rhs ); return *this ; }

  //------------------------------------

  explicit
  View( const std::string & label ,
        const size_t n0 = 0 ,
        const size_t n1 = 0 ,
        const size_t n2 = 0 ,
        const size_t n3 = 0 ,
        const size_t n4 = 0 ,
        const size_t n5 = 0 ,
        const size_t n6 = 0 ,
        const size_t n7 = 0 )
    : m_ptr_on_device(0)
    { alloc( *this, label, n0, n1, n2, n3, n4, n5, n6, n7 ); }


  //------------------------------------
  // LayoutRight rank 1:

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 1, 1, iType0
  >::type & operator()
    ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_1( m_shape, i0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 ];
    }

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 1, 1, iType0
  >::type & operator()
    ( const iType0 & i0 , const int     , const int = 0 , const int = 0 ,
      const int = 0     , const int = 0 , const int = 0 , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_1( m_shape, i0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 ];
    }

  // LayoutRight rank 2:

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 2, 2, iType0, iType1
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,i1 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i1 + i0 * m_stride.value ];
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 2, 2, iType0, iType1
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const int     , const int = 0 ,
      const int = 0     , const int = 0     , const int = 0 , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,i1 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i1 + i0 * m_stride.value ];
    }

  // LayoutRight rank 3:

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 3, 3, iType0, iType1, iType2
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,i2 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i2 + m_shape.N2 * (
                              i1 ) + i0 * m_stride.value ];
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 3, 3, iType0, iType1, iType2
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const int ,
      const int = 0     , const int = 0     , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,i2 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i2 + m_shape.N2 * (
                              i1 ) + i0 * m_stride.value ];
    }

  // LayoutRight rank 4:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 4, 4, iType0, iType1, iType2, iType3
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,i3 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 )) + i0 * m_stride.value ];
    }

  // LayoutRight rank 5:
  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 4, 4, iType0, iType1, iType2, iType3
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const int         , const int = 0     , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,i3 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 )) + i0 * m_stride.value ];
    }

  // LayoutRight rank 5:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 5, 5, iType0, iType1, iType2, iType3, iType4 
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,i4 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 ))) + i0 * m_stride.value ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 5, 5, iType0, iType1, iType2, iType3, iType4 
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const int         , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,i4 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 ))) + i0 * m_stride.value ];
    }

  // LayoutRight rank 6:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 6, 6, iType0, iType1, iType2, iType3, iType4, iType5
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,i5 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i5 + m_shape.N5 * (
                              i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 )))) + i0 * m_stride.value ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 6, 6, iType0, iType1, iType2, iType3, iType4, iType5
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const int         , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,i5 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i5 + m_shape.N5 * (
                              i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 )))) + i0 * m_stride.value ];
    }

  // LayoutRight rank 7:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 7, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,i6 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i6 + m_shape.N6 * (
                              i5 + m_shape.N5 * (
                              i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 ))))) + i0 * m_stride.value ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 7, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 , const int ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,i6 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i6 + m_shape.N6 * (
                              i5 + m_shape.N5 * (
                              i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 ))))) + i0 * m_stride.value ];
    }

  // LayoutRight rank 8:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 , typename iType7 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper<
    traits, LayoutRight, 8, 8, iType0, iType1, iType2, iType3, iType4, iType5, iType6, iType7
  >::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 , const iType7 & i7 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0,i1,i2,i3,i4,i5,i6,i7 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i7 + m_shape.N7 * (
                              i6 + m_shape.N6 * (
                              i5 + m_shape.N5 * (
                              i4 + m_shape.N4 * (
                              i3 + m_shape.N3 * (
                              i2 + m_shape.N2 * (
                              i1 )))))) + i0 * m_stride.value ];
    }

  //------------------------------------
  // LayoutLeft rank 1:

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 1, 1, iType0 >
    ::type & operator()
    ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_1( m_shape, i0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 ];
    }

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 1, 1, iType0 >
    ::type & operator()
    ( const iType0 & i0 , const int     , const int = 0 , const int = 0 ,
      const int = 0 ,     const int = 0 , const int = 0 , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_1( m_shape, i0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 ];
    }

  // LayoutLeft rank 2:

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 2, 2, iType0, iType1 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,i1 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * i1 ];
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 2, 2, iType0, iType1 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const int     , const int = 0 ,
      const int = 0     , const int = 0     , const int = 0 , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,i1 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * i1 ];
    }

  // LayoutLeft rank 3:

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 3, 3, iType0, iType1, iType2 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,i2 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * i2 ) ];
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 3, 3, iType0, iType1, iType2 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const int ,
      const int = 0     , const int = 0     , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,i2 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * i2 ) ];
    }

  // LayoutLeft rank 4:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 4, 4, iType0, iType1, iType2, iType3 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,i3 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * i3 )) ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 4, 4, iType0, iType1, iType2, iType3 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const int         , const int = 0     , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,i3 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * i3 )) ];
    }

  // LayoutLeft rank 5:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 5, 5, iType0, iType1, iType2, iType3, iType4 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,i4 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * i4 ))) ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 5, 5, iType0, iType1, iType2, iType3, iType4 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const int         , const int = 0     , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,i4 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * i4 ))) ];
    }

  // LayoutLeft rank 6:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 6, 6, iType0, iType1, iType2, iType3, iType4, iType5 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,i5 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * (
                              i4 + m_shape.N4 * i5 )))) ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 6, 6, iType0, iType1, iType2, iType3, iType4, iType5 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const int         , const int = 0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,i5 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * (
                              i4 + m_shape.N4 * i5 )))) ];
    }

  // LayoutLeft rank 7:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 7, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,i6 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * (
                              i4 + m_shape.N4 * (
                              i5 + m_shape.N5 * i6 ))))) ];
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 7, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 , const int ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,i6 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * (
                              i4 + m_shape.N4 * (
                              i5 + m_shape.N5 * i6 ))))) ];
    }

  // LayoutLeft rank 8:

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 , typename iType7 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 8, 8, iType0, iType1, iType2, iType3, iType4, iType5, iType6, iType7 >
    ::type & operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 , const iType7 & i7 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0,i1,i2,i3,i4,i5,i6,i7 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return m_ptr_on_device[ i0 + m_stride.value * (
                              i1 + m_shape.N1 * (
                              i2 + m_shape.N2 * (
                              i3 + m_shape.N3 * (
                              i4 + m_shape.N4 * (
                              i5 + m_shape.N5 * (
                              i6 + m_shape.N6 * i7 )))))) ];
    }

  //------------------------------------

  KOKKOS_INLINE_FUNCTION
  ArrayProxyRightType operator * () const
    {
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device );
    }

  //------------------------------------

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 2, 1, iType0
    >::type operator ()
    ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device + m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 3, 2, iType0, iType1
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device + m_shape.N2 * i1 + m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 4, 3, iType0, iType1, iType2
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device +
                                  m_shape.N3 * ( i2 +
                                  m_shape.N2 * ( i1 )) +
                                  m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 5, 4, iType0, iType1, iType2, iType3
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device +
                                  m_shape.N4 * ( i3 +
                                  m_shape.N3 * ( i2 +
                                  m_shape.N2 * ( i1 ))) +
                                  m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 6, 5, iType0, iType1, iType2, iType3, iType4
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device +
                                  m_shape.N5 * ( i4 +
                                  m_shape.N4 * ( i3 +
                                  m_shape.N3 * ( i2 +
                                  m_shape.N2 * ( i1 )))) +
                                  m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 7, 6, iType0, iType1, iType2, iType3, iType4, iType5
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device +
                                  m_shape.N6 * ( i5 +
                                  m_shape.N5 * ( i4 +
                                  m_shape.N4 * ( i3 +
                                  m_shape.N3 * ( i2 +
                                  m_shape.N2 * ( i1 ))))) +
                                  m_stride.value * i0 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename
    Impl::EnableArrayViewOper<
      traits, LayoutRight, 8, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6
    >::type operator ()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0,i1,i2,i3,i4,i5,i6,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyRightType( m_ptr_on_device +
                                  m_shape.N7 * ( i6 +
                                  m_shape.N6 * ( i5 +
                                  m_shape.N5 * ( i4 +
                                  m_shape.N4 * ( i3 +
                                  m_shape.N3 * ( i2 +
                                  m_shape.N2 * ( i1 )))))) +
                                  m_stride.value * i0 );
    }

  //------------------------------------

  template< typename iType0 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 2, 1, iType0 >
    ::type operator()
    ( const iType0 & i0 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_2( m_shape, i0,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device + i0 , m_stride.value );
    }

  template< typename iType0 , typename iType1 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 3, 2, iType0 , iType1 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_3( m_shape, i0,i1,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * ( i1 ) ,
                                 m_stride.value   * m_shape.N1 );
    }

  template< typename iType0 , typename iType1 , typename iType2 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 4, 3, iType0, iType1, iType2 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_4( m_shape, i0,i1,i2,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * (
                                 i1 + m_shape.N1 * ( i2 )) ,
                                 m_stride.value   * m_shape.N1 *
                                 m_shape.N2 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 5, 4, iType0, iType1, iType2, iType3 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_5( m_shape, i0,i1,i2,i3,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * (
                                 i1 + m_shape.N1 * (
                                 i2 + m_shape.N2 * ( i3 ))) ,
                                 m_stride.value   * m_shape.N1 *
                                 m_shape.N2 * m_shape.N3 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 6, 5, iType0, iType1, iType2, iType3, iType4 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_6( m_shape, i0,i1,i2,i3,i4,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * (
                                 i1 + m_shape.N1 * (
                                 i2 + m_shape.N2 * (
                                 i3 + m_shape.N3 * ( i4 )))) ,
                                 m_stride.value   * m_shape.N1 *
                                 m_shape.N2 * m_shape.N3 *
                                 m_shape.N4 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 7, 6, iType0, iType1, iType2, iType3, iType4, iType5 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_7( m_shape, i0,i1,i2,i3,i4,i5,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * (
                                 i1 + m_shape.N1 * (
                                 i2 + m_shape.N2 * (
                                 i3 + m_shape.N3 * (
                                 i4 + m_shape.N4 * ( i5 ))))) ,
                                 m_stride.value   * m_shape.N1 *
                                 m_shape.N2 * m_shape.N3 *
                                 m_shape.N4 * m_shape.N5 );
    }

  template< typename iType0 , typename iType1 , typename iType2 , typename iType3 ,
            typename iType4 , typename iType5 , typename iType6 >
  KOKKOS_INLINE_FUNCTION
  typename Impl::EnableArrayViewOper< traits, LayoutLeft, 8, 7, iType0, iType1, iType2, iType3, iType4, iType5, iType6 >
    ::type operator()
    ( const iType0 & i0 , const iType1 & i1 , const iType2 & i2 , const iType3 & i3 ,
      const iType4 & i4 , const iType5 & i5 , const iType6 & i6 ) const
    {
      KOKKOS_ASSERT_SHAPE_BOUNDS_8( m_shape, i0,i1,i2,i3,i4,i5,i6,0 );
      KOKKOS_RESTRICT_EXECUTION_TO_DATA( typename traits::memory_space , m_ptr_on_device );

      return ArrayProxyLeftType( m_ptr_on_device +
                                 i0 + m_stride.value * (
                                 i1 + m_shape.N1 * (
                                 i2 + m_shape.N2 * (
                                 i3 + m_shape.N3 * (
                                 i4 + m_shape.N4 * (
                                 i5 + m_shape.N5 * ( i6 )))))) ,
                                 m_stride.value   * m_shape.N1 *
                                 m_shape.N2 * m_shape.N3 *
                                 m_shape.N4 * m_shape.N5 * m_shape.N6 );
    }

  //------------------------------------

  KOKKOS_INLINE_FUNCTION
  typename traits::scalar_type * ptr_on_device() const { return m_ptr_on_device ; }

  KOKKOS_INLINE_FUNCTION
  typename traits::size_type capacity() const
  { return Impl::capacity( m_shape , m_stride ); }
};

} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif /* #ifndef KOKKOS_ARRAYVIEWDEFAULT_HPP */
