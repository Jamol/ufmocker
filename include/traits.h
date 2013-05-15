// copy from boost and vs2012 lib

#ifndef __TRAITS_H__
#define __TRAITS_H__

namespace ufmock{

// TEMPLATE CLASS integral_constant
template<class _Ty,
    _Ty _Val>
struct integral_constant
{  // convenient template for integral constant types
    static const _Ty value = _Val;

    typedef _Ty value_type;
    typedef integral_constant<_Ty, _Val> type;

    operator value_type() const
    {	// return stored value
        return (value);
    }
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

// TEMPLATE CLASS _Cat_base
template<bool>
struct _Cat_base
    : false_type
{	// base class for type predicates
};

template<>
struct _Cat_base<true>
    : true_type
{	// base class for type predicates
};

//////////////////////////////////////////////////////////////////////////
typedef char yes_type;
typedef int no_type;

template<typename T>
struct is_class_imp{
    template <typename U> static yes_type is_calss_tester(void(U::*)(void));
    template <typename U> static no_type is_calss_tester(...);

    static const bool value = (sizeof(is_calss_tester<T>(0)) == sizeof(yes_type));
};

template<typename T>
struct is_class : is_class_imp<T>{};

//////////////////////////////////////////////////////////////////////////
template<bool b_>  
struct bool_type{  
    static const bool value = b_;  
};  

template<bool b_>  
const bool bool_type<b_>::value;  

template<typename T>  
struct is_array : bool_type<false>{  
};  

template<typename T>  
struct is_array<T[]>: bool_type<true>{  
};  

template<typename T, unsigned int N>  
struct is_array<T[N]> : bool_type<true>{  
}; 

//////////////////////////////////////////////////////////////////////////
template<typename T>  
struct is_float : bool_type<false>{};  

#define DEF_IS_FLOAT(T) template<> struct is_float<T> : bool_type<true>{};\
    template<> struct is_float<const T> : bool_type<true>{};\
    template<> struct is_float<volatile T> : bool_type<true>{};\
    template<> struct is_float<const volatile T> : bool_type<true>{};

DEF_IS_FLOAT(float);  
DEF_IS_FLOAT(double);  
DEF_IS_FLOAT(long double);

//////////////////////////////////////////////////////////////////////////
#if _HAS_CPP0X
// TEMPLATE CLASS is_lvalue_reference
template<class _Ty>
struct is_lvalue_reference
    : false_type
{	// determine whether _Ty is an lvalue reference
};

template<class _Ty>
struct is_lvalue_reference<_Ty&>
    : true_type
{	// determine whether _Ty is an lvalue reference
};

// TEMPLATE CLASS is_rvalue_reference
template<class _Ty>
struct is_rvalue_reference
    : false_type
{	// determine whether _Ty is an rvalue reference
};

template<class _Ty>
struct is_rvalue_reference<_Ty&&>
    : true_type
{	// determine whether _Ty is an rvalue reference
};

// TEMPLATE CLASS is_reference -- retained
template<class _Ty>
struct is_reference
    : _Cat_base<is_lvalue_reference<_Ty>::value
    || is_rvalue_reference<_Ty>::value>
{	// determine whether _Ty is a reference
};

#else /* _HAS_CPP0X */
// TEMPLATE CLASS is_reference
template<class _Ty>
struct is_reference
    : false_type
{	// determine whether _Ty is a reference
};

template<class _Ty>
struct is_reference<_Ty&>
    : true_type
{	// determine whether _Ty is a reference
};
#endif /* _HAS_CPP0X */

}

#endif
