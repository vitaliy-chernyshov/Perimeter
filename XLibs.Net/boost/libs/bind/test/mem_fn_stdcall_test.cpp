#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  mem_fn_stdcall_test.cpp - a test for mem_fn.hpp + __stdcall
//
//  Copyright (c) 2001 Peter Dimov and Multi Media Ltd.
//
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.
//

#define BOOST_MEM_FN_ENABLE_STDCALL

#include <boost/mem_fn.hpp>
#include <boost/shared_ptr.hpp>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(push, 3)
#endif

#include <iostream>

#if defined(BOOST_MSVC) && (BOOST_MSVC < 1300)
#pragma warning(pop)
#endif


struct X
{
    mutable unsigned int hash;

    X(): hash(0) {}

    int __stdcall f0() { f1(17); return 0; }
    int __stdcall g0() const { g1(17); return 0; }

    int __stdcall f1(int a1) { hash = (hash * 17041 + a1) % 32768; return 0; }
    int __stdcall g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; return 0; }

    int __stdcall f2(int a1, int a2) { f1(a1); f1(a2); return 0; }
    int __stdcall g2(int a1, int a2) const { g1(a1); g1(a2); return 0; }

    int __stdcall f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); return 0; }
    int __stdcall g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); return 0; }

    int __stdcall f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); return 0; }
    int __stdcall g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); return 0; }

    int __stdcall f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); return 0; }
    int __stdcall g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); return 0; }

    int __stdcall f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); return 0; }
    int __stdcall g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); return 0; }

    int __stdcall f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); return 0; }
    int __stdcall g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); return 0; }

    int __stdcall f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); return 0; }
    int __stdcall g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); return 0; }
};

int detect_errors(bool x)
{
    if(x)
    {
        std::cerr << "no errors detected.\n";
        return 0;
    }
    else
    {
        std::cerr << "test failed.\n";
        return 1;
    }
}

int main()
{
    using boost::mem_fn;

    X x;

    X const & rcx = x;
    X const * pcx = &x;

    boost::shared_ptr<X> sp(new X);

    mem_fn(&X::f0)(x);
    mem_fn(&X::f0)(&x);
    mem_fn(&X::f0)(sp);

    mem_fn(&X::g0)(x);
    mem_fn(&X::g0)(rcx);
    mem_fn(&X::g0)(&x);
    mem_fn(&X::g0)(pcx);
    mem_fn(&X::g0)(sp);

    mem_fn(&X::f1)(x, 1);
    mem_fn(&X::f1)(&x, 1);
    mem_fn(&X::f1)(sp, 1);

    mem_fn(&X::g1)(x, 1);
    mem_fn(&X::g1)(rcx, 1);
    mem_fn(&X::g1)(&x, 1);
    mem_fn(&X::g1)(pcx, 1);
    mem_fn(&X::g1)(sp, 1);

    mem_fn(&X::f2)(x, 1, 2);
    mem_fn(&X::f2)(&x, 1, 2);
    mem_fn(&X::f2)(sp, 1, 2);

    mem_fn(&X::g2)(x, 1, 2);
    mem_fn(&X::g2)(rcx, 1, 2);
    mem_fn(&X::g2)(&x, 1, 2);
    mem_fn(&X::g2)(pcx, 1, 2);
    mem_fn(&X::g2)(sp, 1, 2);

    mem_fn(&X::f3)(x, 1, 2, 3);
    mem_fn(&X::f3)(&x, 1, 2, 3);
    mem_fn(&X::f3)(sp, 1, 2, 3);

    mem_fn(&X::g3)(x, 1, 2, 3);
    mem_fn(&X::g3)(rcx, 1, 2, 3);
    mem_fn(&X::g3)(&x, 1, 2, 3);
    mem_fn(&X::g3)(pcx, 1, 2, 3);
    mem_fn(&X::g3)(sp, 1, 2, 3);

    mem_fn(&X::f4)(x, 1, 2, 3, 4);
    mem_fn(&X::f4)(&x, 1, 2, 3, 4);
    mem_fn(&X::f4)(sp, 1, 2, 3, 4);

    mem_fn(&X::g4)(x, 1, 2, 3, 4);
    mem_fn(&X::g4)(rcx, 1, 2, 3, 4);
    mem_fn(&X::g4)(&x, 1, 2, 3, 4);
    mem_fn(&X::g4)(pcx, 1, 2, 3, 4);
    mem_fn(&X::g4)(sp, 1, 2, 3, 4);

    mem_fn(&X::f5)(x, 1, 2, 3, 4, 5);
    mem_fn(&X::f5)(&x, 1, 2, 3, 4, 5);
    mem_fn(&X::f5)(sp, 1, 2, 3, 4, 5);

    mem_fn(&X::g5)(x, 1, 2, 3, 4, 5);
    mem_fn(&X::g5)(rcx, 1, 2, 3, 4, 5);
    mem_fn(&X::g5)(&x, 1, 2, 3, 4, 5);
    mem_fn(&X::g5)(pcx, 1, 2, 3, 4, 5);
    mem_fn(&X::g5)(sp, 1, 2, 3, 4, 5);

    mem_fn(&X::f6)(x, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::f6)(&x, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::f6)(sp, 1, 2, 3, 4, 5, 6);

    mem_fn(&X::g6)(x, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::g6)(rcx, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::g6)(&x, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::g6)(pcx, 1, 2, 3, 4, 5, 6);
    mem_fn(&X::g6)(sp, 1, 2, 3, 4, 5, 6);

    mem_fn(&X::f7)(x, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::f7)(&x, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::f7)(sp, 1, 2, 3, 4, 5, 6, 7);

    mem_fn(&X::g7)(x, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::g7)(rcx, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::g7)(&x, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::g7)(pcx, 1, 2, 3, 4, 5, 6, 7);
    mem_fn(&X::g7)(sp, 1, 2, 3, 4, 5, 6, 7);

    mem_fn(&X::f8)(x, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::f8)(&x, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::f8)(sp, 1, 2, 3, 4, 5, 6, 7, 8);

    mem_fn(&X::g8)(x, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::g8)(rcx, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::g8)(&x, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::g8)(pcx, 1, 2, 3, 4, 5, 6, 7, 8);
    mem_fn(&X::g8)(sp, 1, 2, 3, 4, 5, 6, 7, 8);

    return detect_errors(x.hash == 17610 && sp->hash == 2155);
}
