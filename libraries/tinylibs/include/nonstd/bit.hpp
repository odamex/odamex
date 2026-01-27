//
// Copyright (c) 2020-2025 Martin Moene
//
// https://github.com/martinmoene/bit-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef NONSTD_BIT_LITE_HPP
#define NONSTD_BIT_LITE_HPP

#define bit_lite_MAJOR  1
#define bit_lite_MINOR  2
#define bit_lite_PATCH  0

#define bit_lite_VERSION  bit_STRINGIFY(bit_lite_MAJOR) "." bit_STRINGIFY(bit_lite_MINOR) "." bit_STRINGIFY(bit_lite_PATCH)

#define bit_STRINGIFY(  x )  bit_STRINGIFY_( x )
#define bit_STRINGIFY_( x )  #x

// bit-lite configuration:

#define bit_BIT_DEFAULT  0
#define bit_BIT_NONSTD   1
#define bit_BIT_STD      2

#if !defined( bit_CONFIG_SELECT_BIT )
# define bit_CONFIG_SELECT_BIT  ( bit_HAVE_STD_BIT ? bit_BIT_STD : bit_BIT_NONSTD )
#endif

#if !defined( bit_CONFIG_STRICT )
# define bit_CONFIG_STRICT  0
#endif

// C++ language version detection (C++23 is speculative):
// Note: VC14.0/1900 (VS2015) lacks too much from C++14.

#ifndef   bit_CPLUSPLUS
# if defined(_MSVC_LANG ) && !defined(__clang__)
#  define bit_CPLUSPLUS  (_MSC_VER == 1900 ? 201103L : _MSVC_LANG )
# else
#  define bit_CPLUSPLUS  __cplusplus
# endif
#endif

#define bit_CPP98_OR_GREATER  ( bit_CPLUSPLUS >= 199711L )
#define bit_CPP11_OR_GREATER  ( bit_CPLUSPLUS >= 201103L )
#define bit_CPP14_OR_GREATER  ( bit_CPLUSPLUS >= 201402L )
#define bit_CPP17_OR_GREATER  ( bit_CPLUSPLUS >= 201703L )
#define bit_CPP20_OR_GREATER  ( bit_CPLUSPLUS >= 202002L )
#define bit_CPP23_OR_GREATER  ( bit_CPLUSPLUS >= 202300L )

// Use C++20 std bit operations if available and requested:

#if bit_CPP20_OR_GREATER && defined(__has_include )
# if __has_include( <bit> )
#  define bit_HAVE_STD_BIT  1
# else
#  define bit_HAVE_STD_BIT  0
# endif
#else
# define  bit_HAVE_STD_BIT  0
#endif

#define  bit_USES_STD_BIT  ( (bit_CONFIG_SELECT_BIT == bit_BIT_STD) || ((bit_CONFIG_SELECT_BIT == bit_BIT_DEFAULT) && bit_HAVE_STD_BIT) )

//
// Using std <bit>:
//

#if bit_USES_STD_BIT

#include <bit>

#if bit_CPP20_OR_GREATER
# include <version>
# define bit_HAVE_BYTESWAP  __cpp_lib_byteswap
#else
# define bit_HAVE_BYTESWAP  0
#endif

namespace nonstd
{
    using std::bit_cast;

    using std::has_single_bit;
    using std::bit_ceil;
    using std::bit_floor;
    using std::bit_width;

    using std::rotl;
    using std::rotr;

    using std::countl_zero;
    using std::countl_one;
    using std::countr_zero;
    using std::countr_one;
    using std::popcount;

#if bit_HAVE_BYTESWAP || bit_CPP23_OR_GREATER
    using std::byteswap;
#endif

    using std::endian;
}

#else // bit_USES_STD_BIT

// half-open range [lo..hi):
#define bit_BETWEEN( v, lo, hi ) ( (lo) <= (v) && (v) < (hi) )

// Compiler versions:
//
// MSVC++  6.0  _MSC_VER == 1200  bit_COMPILER_MSVC_VERSION ==  60  (Visual Studio 6.0)
// MSVC++  7.0  _MSC_VER == 1300  bit_COMPILER_MSVC_VERSION ==  70  (Visual Studio .NET 2002)
// MSVC++  7.1  _MSC_VER == 1310  bit_COMPILER_MSVC_VERSION ==  71  (Visual Studio .NET 2003)
// MSVC++  8.0  _MSC_VER == 1400  bit_COMPILER_MSVC_VERSION ==  80  (Visual Studio 2005)
// MSVC++  9.0  _MSC_VER == 1500  bit_COMPILER_MSVC_VERSION ==  90  (Visual Studio 2008)
// MSVC++ 10.0  _MSC_VER == 1600  bit_COMPILER_MSVC_VERSION == 100  (Visual Studio 2010)
// MSVC++ 11.0  _MSC_VER == 1700  bit_COMPILER_MSVC_VERSION == 110  (Visual Studio 2012)
// MSVC++ 12.0  _MSC_VER == 1800  bit_COMPILER_MSVC_VERSION == 120  (Visual Studio 2013)
// MSVC++ 14.0  _MSC_VER == 1900  bit_COMPILER_MSVC_VERSION == 140  (Visual Studio 2015)
// MSVC++ 14.1  _MSC_VER >= 1910  bit_COMPILER_MSVC_VERSION == 141  (Visual Studio 2017)
// MSVC++ 14.2  _MSC_VER >= 1920  bit_COMPILER_MSVC_VERSION == 142  (Visual Studio 2019)

#if defined(_MSC_VER ) && !defined(__clang__)
# define bit_COMPILER_MSVC_VER      (_MSC_VER )
# define bit_COMPILER_MSVC_VERSION  (_MSC_VER / 10 - 10 * ( 5 + (_MSC_VER < 1900 ) ) )
#else
# define bit_COMPILER_MSVC_VER      0
# define bit_COMPILER_MSVC_VERSION  0
#endif

// Courtesy of https://github.com/gsl-lite/gsl-lite
// AppleClang  7.0.0  __apple_build_version__ ==  7000172  bit_COMPILER_APPLECLANG_VERSION ==  700  (Xcode 7.0, 7.0.1)          (LLVM 3.7.0)
// AppleClang  7.0.0  __apple_build_version__ ==  7000176  bit_COMPILER_APPLECLANG_VERSION ==  700  (Xcode 7.1)                 (LLVM 3.7.0)
// AppleClang  7.0.2  __apple_build_version__ ==  7000181  bit_COMPILER_APPLECLANG_VERSION ==  702  (Xcode 7.2, 7.2.1)          (LLVM 3.7.0)
// AppleClang  7.3.0  __apple_build_version__ ==  7030029  bit_COMPILER_APPLECLANG_VERSION ==  730  (Xcode 7.3)                 (LLVM 3.8.0)
// AppleClang  7.3.0  __apple_build_version__ ==  7030031  bit_COMPILER_APPLECLANG_VERSION ==  730  (Xcode 7.3.1)               (LLVM 3.8.0)
// AppleClang  8.0.0  __apple_build_version__ ==  8000038  bit_COMPILER_APPLECLANG_VERSION ==  800  (Xcode 8.0)                 (LLVM 3.9.0)
// AppleClang  8.0.0  __apple_build_version__ ==  8000042  bit_COMPILER_APPLECLANG_VERSION ==  800  (Xcode 8.1, 8.2, 8.2.1)     (LLVM 3.9.0)
// AppleClang  8.1.0  __apple_build_version__ ==  8020038  bit_COMPILER_APPLECLANG_VERSION ==  810  (Xcode 8.3)                 (LLVM 3.9.0)
// AppleClang  8.1.0  __apple_build_version__ ==  8020041  bit_COMPILER_APPLECLANG_VERSION ==  810  (Xcode 8.3.1)               (LLVM 3.9.0)
// AppleClang  8.1.0  __apple_build_version__ ==  8020042  bit_COMPILER_APPLECLANG_VERSION ==  810  (Xcode 8.3.2, 8.3.3)        (LLVM 3.9.0)
// AppleClang  9.0.0  __apple_build_version__ ==  9000037  bit_COMPILER_APPLECLANG_VERSION ==  900  (Xcode 9.0)                 (LLVM 4.0.0?)
// AppleClang  9.0.0  __apple_build_version__ ==  9000038  bit_COMPILER_APPLECLANG_VERSION ==  900  (Xcode 9.1)                 (LLVM 4.0.0?)
// AppleClang  9.0.0  __apple_build_version__ ==  9000039  bit_COMPILER_APPLECLANG_VERSION ==  900  (Xcode 9.2)                 (LLVM 4.0.0?)
// AppleClang  9.1.0  __apple_build_version__ ==  9020039  bit_COMPILER_APPLECLANG_VERSION ==  910  (Xcode 9.3, 9.3.1)          (LLVM 5.0.2?)
// AppleClang  9.1.0  __apple_build_version__ ==  9020039  bit_COMPILER_APPLECLANG_VERSION ==  910  (Xcode 9.4, 9.4.1)          (LLVM 5.0.2?)
// AppleClang 10.0.0  __apple_build_version__ == 10001145  bit_COMPILER_APPLECLANG_VERSION == 1000  (Xcode 10.0, 10.1)          (LLVM 6.0.1?)
// AppleClang 10.0.1  __apple_build_version__ == 10010046  bit_COMPILER_APPLECLANG_VERSION == 1001  (Xcode 10.2, 10.2.1, 10.3)  (LLVM 7.0.0?)
// AppleClang 11.0.0  __apple_build_version__ == 11000033  bit_COMPILER_APPLECLANG_VERSION == 1100  (Xcode 11.1, 11.2, 11.3)    (LLVM 8.0.0?)

#define bit_COMPILER_VERSION( major, minor, patch )  ( 10 * ( 10 * (major) + (minor) ) + (patch) )

#if defined( __apple_build_version__ )
# define bit_COMPILER_APPLECLANG_VERSION bit_COMPILER_VERSION( __clang_major__, __clang_minor__, __clang_patchlevel__ )
# define bit_COMPILER_CLANG_VERSION 0
#elif defined( __clang__ )
# define bit_COMPILER_APPLECLANG_VERSION 0
# define bit_COMPILER_CLANG_VERSION bit_COMPILER_VERSION( __clang_major__, __clang_minor__, __clang_patchlevel__ )
#else
# define bit_COMPILER_APPLECLANG_VERSION 0
# define bit_COMPILER_CLANG_VERSION 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
# define bit_COMPILER_GNUC_VERSION  bit_COMPILER_VERSION(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
# define bit_COMPILER_GNUC_VERSION  0
#endif

// Presence of language and library features:

#define bit_HAVE( feature )  ( bit_HAVE_##feature )

#ifdef _HAS_CPP0X
# define bit_HAS_CPP0X  _HAS_CPP0X
#else
# define bit_HAS_CPP0X  0
#endif

#define bit_CPP11_90   (bit_CPP11_OR_GREATER || bit_COMPILER_MSVC_VER >= 1500)
#define bit_CPP11_100  (bit_CPP11_OR_GREATER || bit_COMPILER_MSVC_VER >= 1600)
#define bit_CPP11_110  (bit_CPP11_OR_GREATER || bit_COMPILER_MSVC_VER >= 1700)
#define bit_CPP11_120  (bit_CPP11_OR_GREATER || bit_COMPILER_MSVC_VER >= 1800)
#define bit_CPP11_140  (bit_CPP11_OR_GREATER || bit_COMPILER_MSVC_VER >= 1900)

#define bit_CPP14_000  (bit_CPP14_OR_GREATER)
#define bit_CPP17_000  (bit_CPP17_OR_GREATER)

// Presence of C++11 language features:

#define bit_HAVE_CONSTEXPR_11           bit_CPP11_140
#define bit_HAVE_ENUM_CLASS             bit_CPP11_110
#define bit_HAVE_NOEXCEPT               bit_CPP11_140
#define bit_HAVE_DEFAULT_FUNCTION_TEMPLATE_ARG  bit_CPP11_120
#define bit_HAVE_STATIC_ASSERT          bit_CPP11_90

// Presence of C++11 library features:

#define bit_HAVE_IS_TRIVIAL             bit_CPP11_110
#define bit_HAVE_IS_TRIVIALLY_COPYABLE  bit_CPP11_110 && !bit_BETWEEN(bit_COMPILER_GNUC_VERSION, 1, 500) // GCC >= 5
#define bit_HAVE_IS_COPY_CONSTRUCTIBLE  bit_CPP11_110
#define bit_HAVE_IS_MOVE_CONSTRUCTIBLE  bit_CPP11_110

#define bit_HAVE_TYPE_TRAITS            bit_CPP11_90
#define bit_HAVE_TR1_TYPE_TRAITS        (!! bit_COMPILER_GNUC_VERSION )

#define bit_HAVE_IS_UNSIGNED            bit_HAVE_TYPE_TRAITS
#define bit_HAVE_IS_SAME                bit_HAVE_TYPE_TRAITS
#define bit_HAVE_IS_SAME_TR1            bit_HAVE_TR1_TYPE_TRAITS

#define bit_HAVE_CSTDINT                bit_CPP11_90

// Presence of C++14 language features:

#define bit_HAVE_CONSTEXPR_14           bit_CPP14_000

// Presence of C++17 language features:

#define bit_HAVE_NODISCARD              bit_CPP17_000

// Presence of C++23 library features:

#define bit_HAVE_BYTESWAP               1  // self-supplied

// Presence of C++ language features:

#if bit_HAVE_CONSTEXPR_11
# define bit_constexpr constexpr
#else
# define bit_constexpr /*constexpr*/
#endif

#if bit_HAVE_CONSTEXPR_14
# define bit_constexpr14 constexpr
#else
# define bit_constexpr14 /*constexpr*/
#endif

#if bit_HAVE_NOEXCEPT
# define bit_noexcept noexcept
#else
# define bit_noexcept /*noexcept*/
#endif

#if bit_HAVE_NODISCARD
# define bit_nodiscard [[nodiscard]]
#else
# define bit_nodiscard /*[[nodiscard]]*/
#endif

// Additional includes:

#include <cstring>      // std::memcpy()
#include <climits>      // CHAR_BIT
#include <limits>       // std::numeric_limits<>

#if bit_HAVE_TYPE_TRAITS
# include <type_traits>
#elif bit_HAVE_TR1_TYPE_TRAITS
# include <tr1/type_traits>
#endif

#ifdef  _MSC_VER
# include <cstdlib>
# define bit_byteswap16  _byteswap_ushort
# define bit_byteswap32  _byteswap_ulong
# define bit_byteswap64  _byteswap_uint64
#else
# define bit_byteswap16  __builtin_bswap16
# define bit_byteswap32  __builtin_bswap32
# define bit_byteswap64  __builtin_bswap64
#endif

#if bit_HAVE( CSTDINT )
# include <cstdint>
#endif

// Method enabling (return type):

#if bit_HAVE( TYPE_TRAITS )
# define bit_ENABLE_IF_R_(R, VA)  typename std::enable_if< (VA), R >::type
#else
# define bit_ENABLE_IF_R_(R, VA)  R
#endif

// Method enabling (function template argument):

#if bit_HAVE( TYPE_TRAITS ) && bit_HAVE( DEFAULT_FUNCTION_TEMPLATE_ARG )
// VS 2013 seems to have trouble with SFINAE for default non-type arguments:
# if !bit_BETWEEN( bit_COMPILER_MSVC_VERSION, 1, 140 )
#  define bit_ENABLE_IF_(VA) , typename std::enable_if< ( VA ), int >::type = 0
# else
#  define bit_ENABLE_IF_(VA) , typename = typename std::enable_if< ( VA ), ::nonstd::bit::enabler >::type
# endif
#else
# define  bit_ENABLE_IF_(VA)
#endif

namespace nonstd {
namespace bit {

// for bit_ENABLE_IF_():

/*enum*/ class enabler{};

template< typename T >
bit_constexpr T bitmask( int i )
{
#if bit_CPP11_OR_GREATER
    return static_cast<T>( T{1} << i );
#else
    return static_cast<T>( T(1) << i );
#endif
}

// C++11 emulation:

namespace std11 {

#if bit_HAVE( CSTDINT )
    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;
#else
    typedef unsigned char      uint8_t;
    typedef unsigned short int uint16_t;
    typedef unsigned int       uint32_t;
# if bit_CPP11_OR_GREATER
    typedef unsigned long long uint64_t;
# endif
#endif

template< class T, T v > struct integral_constant { enum { value = v }; };
typedef integral_constant< bool, true  > true_type;
typedef integral_constant< bool, false > false_type;

#if bit_HAVE( IS_TRIVIAL )
    using std::is_trivial;
#else
    template< class T > struct is_trivial : std11::true_type{};
#endif

#if bit_HAVE( IS_TRIVIALLY_COPYABLE )
    using std::is_trivially_copyable;
#else
    template< class T > struct is_trivially_copyable : std11::true_type{};
#endif

#if bit_HAVE( IS_COPY_CONSTRUCTIBLE )
    using std::is_copy_constructible;
#else
    template< class T > struct is_copy_constructible : std11::true_type{};
#endif

#if bit_HAVE( IS_MOVE_CONSTRUCTIBLE )
    using std::is_move_constructible;
#else
    template< class T > struct is_move_constructible : std11::true_type{};
#endif

#if bit_HAVE( IS_UNSIGNED )
    using std::is_unsigned;
#else
    template< class T > struct is_unsigned : std11::true_type{};
#endif

#if bit_HAVE( IS_SAME )
    using std::is_same;
#elif bit_HAVE( IS_SAME_TR1 )
    using std::tr1::is_same;
#else
    template< class T, class U > struct is_same : std11::true_type{};
#endif

} // namespace std11

// C++20 emulation:

namespace std20 {

template< class T, class U >
struct same_as : std11::integral_constant<bool, std11::is_same<T,U>::value && std11::is_same<U,T>::value> {};

} // namespace std20

// Other detail:

// make sure all unsigned types are covered, see
// http://ithare.com/c-on-using-int_t-as-overload-and-template-parameters/

template< size_t N > struct uint_by_size;
template<> struct uint_by_size< 8> { typedef std11::uint8_t  type; };
template<> struct uint_by_size<16> { typedef std11::uint16_t type; };
template<> struct uint_by_size<32> { typedef std11::uint32_t type; };
#if bit_CPP11_OR_GREATER
template<> struct uint_by_size<64> { typedef std11::uint64_t type; };
#endif

template< typename T >
struct normalized_uint_type
{
    typedef typename uint_by_size< CHAR_BIT * sizeof( T ) >::type type;

#if bit_HAVE( STATIC_ASSERT )
    static_assert( std::is_integral<T>::value, "integral type required.");
    static_assert( std11::is_unsigned<type>::value, "unsigned type result expected.");
    static_assert( sizeof( type ) == sizeof( T ), "size of determined type differs from type derived from.");
#endif
};

//
// For reference:
//

#if 0

// 26.5.3, bit_cast

template< class To, class From > constexpr To bit_cast( From const & from ) noexcept;

// 26.5.4, byteswap
template< class T > constexpr T byteswap(T value) noexcept;

// 26.5.5, integral powers of 2

template< class T > constexpr bool has_single_bit(T x) noexcept;
template< class T > constexpr T bit_ceil(T x);
template< class T > constexpr T bit_floor(T x) noexcept;
template< class T > constexpr T bit_width(T x) noexcept;

// 26.5.6, rotating

template< class T > [[nodiscard]] constexpr T rotl(T x, int s) noexcept;
template< class T > [[nodiscard]] constexpr T rotr(T x, int s) noexcept;

// 26.5.7, counting

template< class T > constexpr int countl_zero(T x) noexcept;
template< class T > constexpr int countl_one(T x) noexcept;
template< class T > constexpr int countr_zero(T x) noexcept;
template< class T > constexpr int countr_one(T x) noexcept;
template< class T > constexpr int popcount(T x) noexcept;

#endif // 0: For reference

//
// Implementation:
//

// 26.5.3, bit_cast

// constexpr support needs compiler magic

template< class To, class From >
/*constexpr*/
bit_ENABLE_IF_R_(
    To,
    ( (sizeof(To) == sizeof(From))
        && std11::is_trivially_copyable<From>::value
        && std11::is_trivial<To>::value
        && (std11::is_copy_constructible<To>::value || std11::is_move_constructible<To>::value)
    )
)
bit_cast( From const & src ) bit_noexcept
{
    To dst;
    std::memcpy( &dst, &src, sizeof(To) );
    return dst;
}

// 26.5.4, byteswap (C++23, p1272)

inline bit_constexpr std11::uint8_t byteswap_( std11::uint8_t value ) bit_noexcept
{
    return value;
}

inline /*bit_constexpr*/ std11::uint16_t byteswap_( std11::uint16_t value ) bit_noexcept
{
    return bit_byteswap16( value );
}

inline /*bit_constexpr*/ std11::uint32_t byteswap_( std11::uint32_t value ) bit_noexcept
{
    return bit_byteswap32( value );
}

#if bit_CPP11_OR_GREATER

inline /*bit_constexpr*/ std11::uint64_t byteswap_( std11::uint64_t value ) bit_noexcept
{
    return bit_byteswap64( value );
}

#endif

template< typename T >
inline /*bit_constexpr*/ T byteswap( T v ) bit_noexcept
{
    return static_cast<T>( byteswap_( static_cast< typename normalized_uint_type<T>::type >( v ) ) );
}

// 26.5.6, rotating

// clang 3.5 - 3.8: Infinite recursive template instantiation when using Clang while GCC works fine?
// https://stackoverflow.com/questions/37931284/infinite-recursive-template-instantiation-when-using-clang-while-gcc-works-fine

#if bit_BETWEEN( bit_COMPILER_CLANG_VERSION, 1, 390 ) || bit_BETWEEN( bit_COMPILER_APPLECLANG_VERSION, 1, 900 )
# define bit_constexpr_rot  /*constexpr*/
#else
# define bit_constexpr_rot  bit_constexpr14
#endif

template< class T >
bit_nodiscard bit_constexpr_rot T rotr_impl(T x, int s) bit_noexcept;

template< class T >
bit_nodiscard bit_constexpr_rot T rotl_impl(T x, int s) bit_noexcept
{
    bit_constexpr14 int N = std::numeric_limits<T>::digits;
    const int r = s % N;

    if ( r == 0 )
        return x;
    else if ( r > 0 )
        return static_cast<T>( (x << r) | (x >> (N - r)) );
    else /*if ( r < 0 )*/
        return rotr_impl( x, -r );
}

template< class T >
bit_nodiscard bit_constexpr_rot T rotr_impl(T x, int s) bit_noexcept
{
    bit_constexpr14 int N = std::numeric_limits<T>::digits;
    const int r = s % N;

    if ( r == 0 )
        return x;
    else if ( r > 0 )
        return static_cast<T>( (x >> r) | (x << (N - r)) );
    else /*if ( r < 0 )*/
        return rotl_impl( x, -r );
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_nodiscard bit_constexpr14 T rotl(T x, int s) bit_noexcept
{
    return rotl_impl( x, s );
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_nodiscard bit_constexpr14 T rotr(T x, int s) bit_noexcept
{
    return rotr_impl( x, s );
}

// 26.5.7, counting

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr14 int countl_zero(T x) bit_noexcept
{
    bit_constexpr14 int N1 = CHAR_BIT * sizeof(T) - 1;

    int result = 0;
    for( int i = N1; i >= 0; --i, ++result )
    {
        if ( 0 != (x & bitmask<T>(i)) )
            break;
    }
    return result;
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr14 int countl_one(T x) bit_noexcept
{
    bit_constexpr14 int N1 = CHAR_BIT * sizeof(T) - 1;

    int result = 0;
    for( int i = N1; i >= 0; --i, ++result )
    {
        if ( 0 == (x & bitmask<T>(i)) )
            break;
    }
    return result;
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr14 int countr_zero(T x) bit_noexcept
{
    bit_constexpr14 int N = CHAR_BIT * sizeof(T);

    int result = 0;
    for( int i = 0; i < N; ++i, ++result )
    {
        if ( 0 != (x & bitmask<T>(i)) )
            break;
    }
    return result;
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr14 int countr_one(T x) bit_noexcept
{
    bit_constexpr14 int N = CHAR_BIT * sizeof(T);

    int result = 0;
    for( int i = 0; i < N; ++i, ++result )
    {
        if ( 0 == (x & bitmask<T>(i)) )
            break;
    }
    return result;
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr14 int popcount(T x) bit_noexcept
{
    bit_constexpr14 int N = CHAR_BIT * sizeof(T);

    int result = 0;
    for( int i = 0; i < N; ++i )
    {
        if ( 0 != (x & bitmask<T>(i)) )
            ++result;
    }
    return result;
}

// 26.5.5, integral powers of 2

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr bool has_single_bit(T x) bit_noexcept
{
    return x != 0 && ( x & (x - 1) ) == 0;
    // return std::popcount(x) == 1;
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr T bit_width(T x) bit_noexcept
{
    return static_cast<T>(std::numeric_limits<T>::digits - countl_zero(x) );
}

template< class T >
bit_constexpr T bit_ceil_impl( T x, std11::true_type /*case: same type*/)
{
#if bit_CPP11_OR_GREATER
    return T{1} << bit_width( T{x - 1} );
#else
    return T(1) << bit_width( T(x - 1) );
#endif
}

template< class T >
bit_constexpr14 T bit_ceil_impl( T x, std11::false_type /*case: integral promotion*/ )
{
    bit_constexpr T offset_for_ub =
        static_cast<T>( std::numeric_limits<unsigned>::digits - std::numeric_limits<T>::digits );

#if 0 // bit_CPP14_OR_GREATER
    return T{ 1u << ( bit_width(T{x - 1}) + offset_for_ub ) >> offset_for_ub };
#else
    return T( 1u << ( bit_width(T(x - 1)) + offset_for_ub ) >> offset_for_ub );
#endif
}

// ToDo: pre-C++11 behaviour for types subject to integral promotion.

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr T bit_ceil(T x)
{
    return ( x <= 1u )
#if bit_CPP11_OR_GREATER
        ? T{1} : bit_ceil_impl( x, std20::same_as<T, decltype(+x)>{} );
#else
        ? T(1) : bit_ceil_impl( x, std11::true_type() );
#endif
}

template< class T
    bit_ENABLE_IF_(
        std11::is_unsigned<T>::value
    )
>
bit_constexpr T bit_floor(T x) bit_noexcept
{
    return (x != 0)
#if bit_CPP11_OR_GREATER
        ? T{1} << (bit_width(x) - 1)
#else
        ? T(1) << (bit_width(x) - 1)
#endif
        : 0;
}

// 26.5.8, endian

#if bit_HAVE( ENUM_CLASS )

enum class endian
{
#ifdef _WIN32
    little = 0,
    big    = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};

#else // enum class

class endian
{
public:
    enum endian_
    {
#ifdef _WIN32
        little = 0,
        big    = 1,
        native = little
#else
        little = __ORDER_LITTLE_ENDIAN__,
        big    = __ORDER_BIG_ENDIAN__,
        native = __BYTE_ORDER__
#endif
    };

    endian( endian_ v )
        : value( v ) {}

    friend inline bool operator==( endian a, endian b ) { return a.value == b.value; }
    friend inline bool operator!=( endian a, endian b ) { return a.value != b.value; }

private:
    endian_ value;
};

#endif

} // namespace bit
} // namespace nonstd

//
// Extensions: endian conversions
//

#if !bit_CONFIG_STRICT

namespace nonstd {
namespace bit {

// endianness selection types:

typedef std11::integral_constant<int, static_cast<int>(endian::big   )> big_endian_type;
typedef std11::integral_constant<int, static_cast<int>(endian::little)> little_endian_type;
typedef std11::integral_constant<int, static_cast<int>(endian::native)> native_endian_type;

// to big endian (implementation):

inline std11::uint8_t to_big_endian_( std11::uint8_t v, little_endian_type ) bit_noexcept
{
    return v;
}

inline std11::uint16_t to_big_endian_( std11::uint16_t v, little_endian_type ) bit_noexcept
{
    return bit_byteswap16( v );
}

inline std11::uint32_t to_big_endian_( std11::uint32_t v, little_endian_type ) bit_noexcept
{
    return bit_byteswap32( v );
}

#if bit_CPP11_OR_GREATER

inline std11::uint64_t to_big_endian_( std11::uint64_t v, little_endian_type ) bit_noexcept
{
    return bit_byteswap64( v );
}

#endif

template< typename T >
inline T to_big_endian_( T v, big_endian_type ) bit_noexcept
{
    return v;
}

// to little endian (implementation):

inline std11::uint8_t to_little_endian_( std11::uint8_t v, big_endian_type ) bit_noexcept
{
    return v;
}

inline std11::uint16_t to_little_endian_( std11::uint16_t v, big_endian_type ) bit_noexcept
{
    return bit_byteswap16( v );
}

inline std11::uint32_t to_little_endian_( std11::uint32_t v, big_endian_type ) bit_noexcept
{
    return bit_byteswap32( v );
}

#if bit_CPP11_OR_GREATER

inline std11::uint64_t to_little_endian_( std11::uint64_t v, big_endian_type ) bit_noexcept
{
    return bit_byteswap64( v );
}

#endif

template< typename T >
inline T to_little_endian_( T v, little_endian_type ) bit_noexcept
{
    return v;
}

// to native endian (implementation):

template< typename T >
inline T to_native_endian_( T v, native_endian_type ) bit_noexcept
{
    return v;
}

template< typename T, typename EN >
inline T to_native_endian_( T v, EN ) bit_noexcept
{
    // force conversion:
    return to_big_endian_( v, little_endian_type() );
}

//
// to_{endian}: convert unconditionally (default), or depending in given endianness.
// Note: avoid C++11 default function template arguments.
//

template< typename T >
inline T to_big_endian( T v ) bit_noexcept
{
    return to_big_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), little_endian_type() );
}

template< typename T, typename EN >
inline T to_big_endian( T v, EN ) bit_noexcept
{
    return to_big_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), EN() );
}

template< typename T >
inline T to_little_endian( T v ) bit_noexcept
{
    return to_little_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), big_endian_type() );
}

template< typename T, typename EN >
inline T to_little_endian( T v, EN ) bit_noexcept
{
    return to_little_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), EN() );
}

template< typename T >
inline T to_native_endian( T v ) bit_noexcept
{
    return to_native_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), native_endian_type() );
}

template< typename T, typename EN >
inline T to_native_endian( T v, EN ) bit_noexcept
{
    return to_native_endian_( static_cast< typename normalized_uint_type<T>::type >( v ), EN() );
}

//
// as_{endian}: convert if different from native_endian_type.
//

template< typename T >
inline T as_big_endian( T v ) bit_noexcept
{
    return to_big_endian( v, native_endian_type() );
}

template< typename T >
inline T as_little_endian( T v ) bit_noexcept
{
    return to_little_endian( v, native_endian_type() );
}

template< typename T >
inline T as_native_endian( T v ) bit_noexcept
{
    return to_native_endian( v, native_endian_type() );
}

}} // namespace nonstd::bit

#endif // !bit_CONFIG_STRICT

//
// Make type available in namespace nonstd:
//

namespace nonstd
{
    using bit::bit_cast;

    using bit::has_single_bit;
    using bit::bit_ceil;
    using bit::bit_floor;
    using bit::bit_width;

    using bit::rotl;
    using bit::rotr;

    using bit::countl_zero;
    using bit::countl_one;
    using bit::countr_zero;
    using bit::countr_one;
    using bit::popcount;

    using bit::byteswap;

    using bit::endian;
}

#if !bit_CONFIG_STRICT

namespace nonstd
{
    using bit::big_endian_type;
    using bit::little_endian_type;
    using bit::native_endian_type;

    using bit::to_big_endian;
    using bit::to_little_endian;
    using bit::to_native_endian;

    using bit::as_big_endian;
    using bit::as_little_endian;
    using bit::as_native_endian;
}

#endif // !bit_CONFIG_STRICT

#endif // bit_USES_STD_BIT

#endif // NONSTD_BIT_LITE_HPP
