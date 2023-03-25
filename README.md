<img src="https://codeberg.org/yellowdragon/pages/raw/branch/master/tdd.png">

TDD: Testing in pristine C++
============================

See while you code. TDD is a zero dependency test system that is 23x faster than doctest and 60x faster than GoogleTest.

- **Fast:** So fast that tests can run automatically every time you save.
- **Zero setup:** Start immediately. No CMake, no standard library needed.
- **Test private members** without changing your code.
- **Powerful:** Generates tests even more powerfully than templates.

Despite the name, TDD is not specifically intended for "test driven development" but serves all testing purposes.

Tested with GCC and Clang.


Index
-----

- [Quick Start](#quick-start)                                    
- [Performance](#performance)                                    
- [Constant time, Runtime, or Both](#constant-time-runtime-or-both)
- [Generate Multiple Tests](#generate-multiple-tests)            
- [Access private members](#access-private-members)              
- [Print](#print)                                                
- [Test automatically](#test-automatically)                      
- [Tips](#tips)                                                  
- [Note](#note)                                                  
- [Credits](#credits)


Quick Start
-----------

1. Download `tdd.h` and `tdd.cpp` into your test directory.

	```
	wget https://codeberg.org/yellowdragon/tdd/raw/branch/master/tdd.h https://codeberg.org/yellowdragon/tdd/raw/branch/master/tdd.cpp
	```
2. Create a cpp:

	```
	#include "tdd.h"

	TEST(plus)  { EXPECT(14 + 2 == 16); }

	RUN_ALL();
	```
3. `clang++ *.cpp -std=c++20 -o t && ./t`

C++20 is required. There is almost no output in the absence of errors.


Performance
-----------

TDD compiles even faster than `#include <vector>` and has essentially zero runtime overhead. <br>

|         | TDD   | `#include <vector>` | doctest | googletest |
|    ---: | :---  | :---                | :---    | :---       |
| clang++ | 132ms | 235ms               | 1734ms  | 3807ms     |
|     g++ |  97ms | 190ms               | 2310ms  | 5884ms     |


It is nice to have [tests running automatically](#test-automatically) as quickly as saving a file.


Constant time, Runtime, or Both
-------------------------------

```
TEST(runtime_test)        { EXPECT(!is_constant_evaluated()); }
CTEST(constant_time_test) { EXPECT( is_constant_evaluated()); }

CRTEST(constant_and_runtime_test) {
	//EXPECT(!is_constant_evaluated());  // compilation error
	//EXPECT( is_constant_evaluated());  // runtime error
}
```

Generate Multiple Tests
-----------------------

Suppose:
```
TEST(test_widget_a) { A a; EXPECT(a.works()); }
TEST(test_widget_b) { B b; EXPECT(b.works()); }
TEST(test_widget_c) { C c; EXPECT(c.works()); }
```
The same can be written like this:
```
TESTX(test_widgets, set<A, B, C>) { X x; EXPECT(x.works()); }    // set is not std::set
```
But does it also work for `const`?
```
using widgets = set<A, B, C>;
TESTX(test_widgets, and_const<widgets>) { X x; EXPECT(x.works()); }    // A, const A, B, const B, C, const C
```
That is 6 tests in one. Multiple parameters have to be wrapped:
```
TESTX(test_child_widgets_set, parameters<set<A, B, C>, set<D, E, F>>) {
	// A, D
	// B, E
	// C, F
	EXPECT(sizeof...(Xs) == 2);
}

TESTX(test_child_widgets, parameters<for_each<A, B, C>, set<D, E, F>>) {
	// A, D
	// A, E
	// A, F
	// B, D
	// B, E
	// B, F
	// C, D
	// C, E
	// C, F
	X parent;
	nth<1, Xs...> child;
	EXPECT(parent.add_child(&child));
}
```
144 tests in one:
```
TESTX(test_is_base, parameters<for_each<and_cvref<Base>>,
                               for_each<and_cvref<Derived>>>) {
	EXPECT(is_base_of<Xs...>);
}
```

Access private members
----------------------

There is no reason to hinder testing, ever. TDD provides direct access to private members with near zero runtime overhead.
Simply pass member pointers to the test and use `prv`:

```
class A { private: int n = 14;
	class B { private: int n = 16;
		class C { private: int n = 18; } c;
		using type = bool;
	} b;
};

TEST(test_n, &A::n) {
	A a;
	EXPECT(prv<0>(a) == 14);
}
```
`prv` gives direct references (except in the case of functions). To have an alias use `prv_ref`, which is just a macro for `decltype(auto)`:

```
TEST(test_all_n,
     &A::b,
     &A::B::c,
     &A::n,
     &A::B::n,
     &A::B::C::n) {
	A a;
	prv_ref b   = prv<0>(a);
	prv_ref c   = prv<1>(b);
	prv_ref a_n = prv<2>(a);
	prv_ref b_n = prv<3>(b);
	prv_ref c_n = prv<4>(c);

	EXPECT(a_n == 14);  a_n += 100;  EXPECT(a_n == 114);
	EXPECT(b_n == 16);  b_n += 100;  EXPECT(b_n == 116);
	EXPECT(c_n == 18);  c_n += 100;  EXPECT(c_n == 118);

	EXPECT(&a_n == &prv<      2>(a));
	EXPECT(&b_n == &prv<0,    3>(a));
	EXPECT(&c_n == &prv<0, 1, 4>(a));  // a.b.c.n
}

```

Private Functions
-----------------

Same syntax:

```
class F {
	int times2(int n) { return n * 2; }
	constexpr static int g(bool b) { return b ? 14 : 0; }
};

TEST(test_call, &F::times2, &F::g) {
	F f;
	EXPECT(prv<0>(f)(7) == 14);
	EXPECT(prv<1>( )(true) == 14);    // no parameter with static
}
```

Private Types
-------------

```
TEST(test_types, test::type<A::B::type>) {
	EXPECT(is_same<prv_type<0>, bool>);
	prv_type<0> b = true;
}
```

Print
-----
`EXPECT()` can print, GoogleTest style:
```
TEST(test_print) { EXPECT(0) << "can print"; }
```
Adding another format:
```
struct S { int a = 14; int b = 16; };

namespace test {
    const printer_t& operator<<(const printer_t& p, const S& s) { return p.print("[%d, %d]\n", s.a, s.b); }
}

TEST(test_custom_print) { EXPECT(0) << S(); }
```

Test automatically
------------------

[`run`](https://codeberg.org/yellowdragon/tdd/raw/branch/master/run) automatically runs all tests every time something changes in the parent directory.
Any arguments are passed to the compiler. In the absence of arguments, it uses clang++ to compile `*cpp`.

```
$ ./run

compiling... 0:00.73
31 tests, 0 errors.
0:00.00s (2944kb)

compiling... 0:00.71
31 tests, 0 errors.
0:00.00s (2672kb)

$ ./run *cpp -DEXHAUSTIVE

 compiling...0:01.46
 52 tests, 0 errors.
 0:00.00s (3072kb)
```
The script is easily adaptable.


Tips
----

- A good habit is to keep heavier within `#ifdef EXHAUSTIVE` and then, occasionally, add `-DEXHAUSTIVE` to `run`.
- If your template takes non-type parameters, `dragon::constant<>` can be of use:

```
	TESTX(test_const, constant<14>) {
		EXPECT(X::v == 14);
	}
```

Note
----

- Test names must be unique across all translation units.
- `RUN_ALL()` must be the last statement.
- All test code runs within static initialization. Care must be taken to avoid the [Static Initialization Order Fiasco](https://en.cppreference.com/w/cpp/language/siof). [`sample.cpp`](https://codeberg.org/yellowdragon/tdd/raw/branch/master/sample.cpp) illustrates how to initialize before TDD.
- Defining `TDD_INIT_IOS` is a quick way to include iostream and initialize `std::ios_base`. Otherwise, using `std::cout` and similar may segfault.
- Define `TDD_MAX_ERRORS` to limit the maximum number of errors.
- [An old C++ standard issue](https://www.open-std.org/jtc1/sc22/wg21/docs/cwg_active.html#2118) says friend injection should be made ill-formed, but that it is "as yet undetermined" how to prohibit this.
	There is no reason for this, and I do not see how it could be accomplished without breaking a ton of C++ code. Friend functions and CRTP'd friends are everywhere.
- GCC warns about undefined inline functions. We await the [option to supress this](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66918).
- Functions accessed with `prv()` do not have default arguments.
- TDD is thread-safe.


Credits
-------

The included script uses [entr](https://github.com/eradman/entr) to monitor file changes.
