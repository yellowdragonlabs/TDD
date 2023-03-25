#include "tdd.h"

using namespace dragon;

struct will_work {
	bool works() const { return true; }
	bool add_child(auto) const { return true; }
};

class A : public will_work { private: int n = 14;
	class B { private: int n = 16;
		class C { private: int n = 18; } c;
			using type = bool;
		} b;
};

struct B : will_work {};
struct C : will_work {};
struct D : will_work {};
struct E : will_work {};

class F : public will_work {
	int times2(int n) { return n * 2; }
	constexpr static int g(bool b) { return b ? 14 : 0; }
};

TEST(plus)  { EXPECT(14 + 2 == 16); }

TEST(runtime_test)        { EXPECT(!is_constant_evaluated()); }
CTEST(constant_time_test) { EXPECT( is_constant_evaluated()); }

CRTEST(constant_and_runtime_test) {
	//EXPECT(!is_constant_evaluated());  // compilation error
	//EXPECT( is_constant_evaluated());  // runtime error
}

TEST(test_widget_a) { A a; EXPECT(a.works()); }
TEST(test_widget_b) { B b; EXPECT(b.works()); }
TEST(test_widget_c) { C c; EXPECT(c.works()); }

TESTX(test_widgets1, set<A, B, C>) { X x; EXPECT(x.works()); }    // set is not std::set

using widgets = set<A, B, C>;
TESTX(test_widgets, and_const<widgets>) { X x; EXPECT(x.works()); }    // A, const A, B, const B, C, const C

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

struct Base {};
struct Derived : Base{};

TESTX(test_is_base, parameters<for_each<and_cvref<Base>>,
                               for_each<and_cvref<Derived>>>) {
	EXPECT(is_base_of<Xs...>);
}

TEST(test_n, &A::n) {
	A a;
	EXPECT(prv<0>(a) == 14);
}

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

TEST(test_call, &F::times2, &F::g) {
	F f;
	EXPECT(prv<0>(f)(7) == 14);
	EXPECT(prv<1>( )(true) == 14);    // no parameter with static
}

TEST(test_types, test::type<A::B::type>) {
	EXPECT(is_same<prv_type<0>, bool>);
	prv_type<0> b = true;
}

constexpr bool fail_on_purpose = false;

TEST(test_print) { EXPECT(fail_on_purpose) << "can print"; }

struct S { int a = 14; int b = 16; };

namespace test {
	const printer_t& operator<<(const printer_t& p, const S& s) { return p.print("can print in special format: [%d, %d]\n", s.a, s.b); }
}

TEST(test_custom_print) { EXPECT(fail_on_purpose) << S(); }


TESTX(test_const, constant<14>) {
	EXPECT(X::v == 14);
}

RUN_ALL();
