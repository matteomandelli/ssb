// SSB, v0.01 WIP
// (Assert)

#pragma once

#include <assert.h>

template <bool>  struct compile_time_assert_failed;
template <>      struct compile_time_assert_failed <true> { };
template <int x> struct compile_time_assert_test { };

#define compile_time_assert_join2( a, b )		a##b
#define compile_time_assert_join( a, b )		compile_time_assert_join2(a,b)
#define compile_time_assert( x )				typedef compile_time_assert_test<sizeof(compile_time_assert_failed<(bool)(x)>)> compile_time_assert_join(compile_time_assert_typedef_, __LINE__)
#define compile_time_error( x )				    _Pragma(x)

#if defined ( _DEBUG )

#define ASSERT( x )	assert( x )
#define ASSERT_SIZEOF( type, size )	compile_time_assert ( sizeof( type ) == size )

#else

#define ASSERT( x )	{ ( ( void ) 0 ); } 
#define ASSERT_SIZEOF( type, size )

#endif // _DEBUG

#define UNUSED(var) (void)sizeof(var)
