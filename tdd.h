/*
 * Copyright (c) 2023 Philipp Roesch
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#if !defined(__ATOMIC_RELAXED) || !__has_builtin(__atomic_fetch_add)
	#error "tdd.h needs __atomic builtins"
#endif

#ifndef TDD_MAX_ERRORS
#define TDD_MAX_ERRORS 0
#endif

#ifdef TDD_INIT_IOS
#include <iostream>
namespace test::_internal {
	static std::ios_base::Init init_ios{};
}
#endif

// requisites {{{
namespace dragon {
	using size_t = decltype(sizeof(int));

	struct NAT {};

	template<class... T> struct type_list { static constexpr size_t size = sizeof...(T); };
	template<template<class...> class... TT> struct template_list { constexpr static size_t size = sizeof...(TT); };

	// constant {{{
	template<auto value>
	struct constant {
		using type = constant<value>;
		using value_type = decltype(value);
		constexpr static value_type v = value;
		constexpr operator value_type() const { return v; }
	};

	using true_t = constant<true>;
	using false_t = constant<false>;
	// }}}
	// Modifiers {{{
	template<class T> struct add_const_t    { using type = const T; };
	template<class T> struct add_volatile_t { using type = volatile T; };
	template<class T> struct add_pointer_t  { using type = T*; };
	template<class T> struct add_lref_t     { using type = T&; };
	template<class T> struct add_rref_t     { using type = T&&; };

	template<class T> struct remove_const_t    { using type = T; }; template<class T> struct remove_const_t<const T>       { using type = T; };
	template<class T> struct remove_volatile_t { using type = T; }; template<class T> struct remove_volatile_t<volatile T> { using type = T; };
	template<class T> struct remove_pointer_t  { using type = T; }; template<class T> struct remove_pointer_t<T*>          { using type = T; };
	template<class T> struct remove_lref_t     { using type = T; }; template<class T> struct remove_lref_t<T&>             { using type = T; };
	template<class T> struct remove_rref_t     { using type = T; }; template<class T> struct remove_rref_t<T&&>            { using type = T; };

	template<class T> struct remove_ref_t : remove_lref_t<typename remove_rref_t<T>::type> {};
	template<class T> struct remove_cv_t : remove_const_t<typename remove_volatile_t<T>::type> {};
	template<class T> struct remove_cvref_t : remove_cv_t<typename remove_ref_t<T>::type> {};
	// }}}
	// declval {{{
	namespace _internal::declval_aux {
		template<class T, class U = T&&>
		U f(int);

		template<class T>
		T f(long);
	}

	template<class T>
	decltype(_internal::declval_aux::f<T>(0)) declval() noexcept;
	// }}}
	// naked: remove cvref and pointers. {{{
	template<class T> struct naked_t                   { using type = T; };
	template<class T> struct naked_t<T&>               { using type = typename naked_t<T>::type; };
	template<class T> struct naked_t<T&&>              { using type = typename naked_t<T>::type; };
	template<class T> struct naked_t<T*>               { using type = typename naked_t<T>::type; };
	template<class T> struct naked_t<const T>          { using type = typename naked_t<T>::type; };
	template<class T> struct naked_t<volatile T>       { using type = typename naked_t<T>::type; };
	template<class T> struct naked_t<volatile const T> { using type = typename naked_t<T>::type; };
	// }}}
	// is_same {{{
	template<class, class> struct is_same_t : false_t {};
	template<class T> struct is_same_t<T, T> : true_t {};
	// }}}
	// are_same / is_any_of {{{
	namespace _internal {
		template<bool And, class A, class... B>
		struct type_comparison {
			using type = A;
			constexpr static bool v = And ? (is_same_t<A, B>::v && ...) : (is_same_t<A, B>::v || ...);
		};
	}

	template<class... T> using are_same_t = _internal::type_comparison<true, T...>;
	template<class... T> using is_any_of_t = _internal::type_comparison<false, T...>;
	// }}}
	// is_const / is_function, etc... {{{
	template<class T> struct is_const_t : false_t {};
	template<class T> struct is_const_t<const T> : true_t {};

	template<class T> struct is_volatile_t : false_t {};
	template<class T> struct is_volatile_t<volatile T> : true_t {};

	template<class T> struct is_lref_t     : false_t {}; /**/ template<class T> struct is_rref_t      : false_t {};
	template<class T> struct is_lref_t<T&> : true_t {};  /**/ template<class T> struct is_rref_t<T&&> : true_t {};

	template<class T> using is_void_t = is_same_t<void, typename remove_cv_t<T>::type>;

	template<class T> using is_floating_point_t = is_any_of_t<typename naked_t<T>::type, float, double, long double>;

	template<class T>           struct is_array_t       : false_t {};
	template<class T>           struct is_array_t<T[]>  : true_t {};
	template<class T, size_t N> struct is_array_t<T[N]> : true_t {};

	template<class T>
	struct is_function_t {
		constexpr static bool v = !is_const_t<const T>::v && !is_lref_t<T>::v && !is_rref_t<T>::v;
	};
	// }}}
	// conditional {{{
	namespace _internal {
		template<bool>
		struct conditional_aux {
			template<class T, class F> using type = F;
		};

		template<>
		struct conditional_aux<true> {
			template<class T, class F> using type = T;
		};
	}

	template<bool cond, class A, class B>
	using conditional = typename _internal::conditional_aux<cond>::template type<A, B>;
	// }}}
	// is_base_of {{{
	namespace _internal {
		template<class B, class D>
		class is_base_of_aux {
			template<class Base, class Derived>
			struct _aux_ {
				operator Base*() const;
				operator Derived*();
			};

		public:
			template<class T>
			static true_t _test_aux_(D*, T);
			static false_t _test_aux_(B*, int);

			constexpr static bool v = decltype(_test_aux_(_aux_<B, D>(), int()))::v;
		};

		// Void is always unrelated.
		template<>        struct is_base_of_aux<void, void> : false_t {};
		template<class B> struct is_base_of_aux<B, void> : false_t {};
		template<class D> struct is_base_of_aux<void, D> : false_t {};
	}

	template<class B, class D>
	struct is_base_of_t : _internal::is_base_of_aux<typename naked_t<B>::type,
	                                                typename naked_t<D>::type> {};
	// }}}
	// is_constant_evaluated {{{
	constexpr bool is_constant_evaluated() noexcept {
		return __builtin_is_constant_evaluated();
	}
	// }}}
	// forward / move {{{
	template<class T>
	constexpr T&& forward(typename remove_ref_t<T>::type& v) noexcept {
		return static_cast<T&&>(v);
	}

	template<class T>
	constexpr T&& forward(typename remove_ref_t<T>::type&& v) noexcept {
		static_assert(!is_lref_t<T>::v, "forward must not be used to convert an rvalue to an lvalue");
		return static_cast<T&&>(v);
	}

	template<class T>
	constexpr typename remove_ref_t<T>::type&& move(T&& v) noexcept {
		return static_cast<typename remove_ref_t<T>::type&&>(v);
	}
	// }}}
	// tuple {{{
	namespace _internal::tuple_aux {
		template<class R,
		         class T = typename remove_ref_t<R>::type,
				 bool = is_function_t<T>::v>
		struct decay_t;
		template<class R, class T>           struct decay_t<R, T, false>    { using type = typename remove_cv_t<T>::type; };
		template<class R, class F>           struct decay_t<R, F, true>     { using type = typename add_pointer_t<F>::type; };
		template<class R, class T>           struct decay_t<R, T[], false>  { using type = T*; };
		template<class R, class T, size_t N> struct decay_t<R, T[N], false> { using type = T*; };
	}

	template<class...> struct tuple {};
	template<class T, class... Ts>
	struct tuple<T, Ts...> : tuple<Ts...> {
		constexpr static size_t size = 1 + sizeof...(Ts);

		constexpr tuple() noexcept = default;
		constexpr tuple(const tuple&) noexcept = default;
		constexpr tuple(tuple&&) noexcept = default;

		template<class Arg, class... Args>
		constexpr tuple(Arg&& arg, Args&&... args) noexcept
			: tuple<Ts...>(forward<Args>(args)...),
			  x(forward<Arg>(arg)) {}

		template<size_t N> 
		constexpr decltype(auto) get() {
			if constexpr(N == 0) return data();
			else return tuple<Ts...>::template get<N - 1>();
		}

		constexpr T& data() { return x; }
		constexpr const T& data() const { return x; }
	private:
		T x;
	};

	template<class... Args>
	constexpr auto make_tuple(Args&&... args) {
		return tuple<typename _internal::tuple_aux::decay_t<Args>::type...>(forward<Args>(args)...);
	}
	// }}}
	// const_counter {{{
	#if defined(__clang__)
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-inline"
	#elif defined(__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wnon-template-friend"
	#endif
	
	template<class Tag, int N = 0>
	struct const_counter_t {
		struct define {
			friend consteval bool is_defined(const_counter_t) { return true; }
		};
		friend consteval bool is_defined(const_counter_t);
	
		template<class T = const_counter_t, bool = is_defined(T{})>
		consteval static bool set(int) { return true; }
		consteval static bool set(...) { return define(), false; }
	
		template<class T = const_counter_t, bool = is_defined(T{})>
		consteval static bool get(int) { return true; }
		consteval static bool get(...) { return false; }
	};
	#if defined(__clang__)
		#pragma clang diagnostic pop
	#elif defined(__GNUC__)
		#pragma GCC diagnostic pop
	#endif
	
	struct global_counter_tag{};
	
	template<class T = global_counter_tag, int N = 0, auto = []{}>
	consteval int const_counter() {
		if constexpr(const_counter_t<T, N>::set(N))
			return const_counter<T, N + 1>();
		else return N;
	}
	
	template<class T = global_counter_tag, int N = 0, auto = []{}>
	consteval int check_const_counter() {
		if constexpr(const_counter_t<T, N>::get(N))
			return check_const_counter<T, N + 1>();
		else return N;
	}
	// }}}
	// NATTED {{{
	// A technique to modify template parameters one at a time.
	// 
	// NATTED(X) declares template X_t, to be specialized, and the alias X. X_t should only be specialized and never used.
	// User defined X_t specializations can remove parameters, change or accept them by moving them to the end.
	// 
	// If no other specialization is selected, a default specialization accepts types as they are. Until the first parameter
	// is X_NAT, a token indicating the end of the process.
	// 
	// NATTED(do_nothing);
	// // do_nothing<int, bool> = type_list<int, bool>
	// 
	// NATTED(char2void);
	// template<class... Ts> struct char2void_t<char, Ts...> : char2void_t<Ts..., void> {};
	// // char2void<int, char, char, bool> = type_list<int, void, void, bool>
	// 
	#define NATTED(C)                                                                                  \
		struct C##_NAT {};                                                                             \
		template<class...> struct C##_t { using type = dragon::type_list<>; };                         \
		template<class T, class... Ts> struct C##_t<T, Ts...> : C##_t<Ts..., T> {};                    \
		template<class... T> struct C##_t<C##_NAT, T...> { using type = dragon::type_list<T...>; };    \
		template<class... T> using C = typename C##_t<T..., C##_NAT>::type

	// NATTED2 has an extra user supplied parameter, and an integer that is incremented when parameters are stored.
	// NATTED2(rm_type);
	// template<int I, class RM, class... Ts>
	// struct rm_type_t<I, RM, RM, Ts...> : rm_type_t<I, RM, Ts...> {};
	// // rm_type<void, int, void, void, bool> = type_list<int, bool>
	// 
	#define NATTED2(C)                                                                                                       \
		struct C##_NAT {};                                                                                                   \
		template<int I, class P, class...> struct C##_t { using type = dragon::type_list<>; };                               \
		template<int I, class P, class T, class... Ts> struct C##_t<I, P, T, Ts...> : C##_t<I + 1, P, Ts..., T> {};          \
		template<int I, class P, class... T> struct C##_t<I, P, C##_NAT, T...> { using type = dragon::type_list<T...>; };    \
		template<class... T> using C = typename C##_t<0, T..., C##_NAT>::type
	// }}}
	// nth {{{
	namespace _internal {
		NATTED2(nth_base);
		NATTED2(nth_value_base);
		template<int I, class T, class... Ts> struct          nth_base_t<I, constant<I>, T, Ts...> { using type = T; };
		template<int I, class T, class... Ts> struct    nth_value_base_t<I, constant<I>, T, Ts...> { using type = T; };
	}
	template<int I, class... T> using nth = _internal::nth_base<constant<I>, T...>;

	template<int I, auto... X>
	constexpr auto nth_value = _internal::nth_value_base<constant<I>, constant<X>...>::v;
	// }}}
	// value_list {{{
	template<auto... V>
	struct value_list {
		static constexpr size_t size = sizeof...(V);

		template<auto... U> using insert = value_list<U..., V...>;
		template<auto... U> using append  = value_list<V..., U...>;

		template<size_t index>
		constexpr static auto get() { return nth_value<index, V...>; }
	};
	
	template<class List, auto... V> using insert_values = typename List::template insert<V...>;
	template<class List, auto... V> using append_values = typename List::template append<V...>;
	// }}}
	// params {{{
	// Modify template parameters.
	template<class T> struct params;

	template<template<class...> class This,
			 class... T>
	struct params<This<T...>> {
		template<class... U> using insert = This<U..., T...>;
		template<class... U> using append = This<T..., U...>;

		template<size_t Index> using get = nth<Index, T...>;  // returns the entire list if index is out of bounds!

		template<class U>
		constexpr static bool has = (is_same_t<U, T>::v || ...);

		// Cast this list to another template.
		template<template<class...> class To>
		using cast = To<T...>;

		template<auto eq>
		constexpr static bool values_eq_to = ((T::v == eq) && ...);

		constexpr static tuple<T...> instantiate() { return {}; }
	};

	// Returns the entire list if index is out of bounds!
	template<size_t Index, class List> using get_param = typename params<List>::template get<Index>;

	template<class T, class List>
	constexpr bool has_param = params<List>::template has<T>;

	template<class T, class... U> using insert_params = typename params<T>::template insert<U...>;
	template<class T, class... U> using append_params = typename params<T>::template append<U...>;

	template<template<class...> class To, class From>
	using template_cast = typename params<From>::template cast<To>;

	template<auto eq, class T> constexpr bool values_eq_to = params<T>::template values_eq_to<eq>;

	template<class T> constexpr bool true_types  = values_eq_to<true, T>;
	template<class T> constexpr bool false_types = values_eq_to<false, T>;

	// Instantiates T's template parameters and returns them in a tuple.
	template<class T>
	constexpr static auto instantiate() { return params<T>::instantiate(); }
	// }}}
	// type_vec {{{
	// Statically store types:
	//
	// using store = type_vec<struct tag>;    // store::current_type<> = type_list<>
	// store::push<void>();                   // store::current_type<> = type_list<void>
	//
	// This is accomplished by defining a friend function every time a type is added, i.e. friend injection.
	//
	#if defined(__clang__)
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wundefined-inline"
	#elif defined(__GNUC__)
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wnon-template-friend"
	#endif
	
	template<class Tag>
	class type_vec {
		template<int N, class = NAT>  // The N'th type is stored in _aux_is_defined()'s return value.
		struct store {
			friend constexpr auto _aux_is_defined(store<N>);
			constexpr static int v = N;
		};

		template<class T>  // Default is type_list<>.
		class store<0, T> {
			friend constexpr auto _aux_is_defined(store<0>) { return type_list<>(); }
		};

		template<class Store, class T>
		struct _aux_define {
			friend constexpr auto _aux_is_defined(store<Store::v>) { return T(); }
			constexpr static int v = Store::v;
		};

		template<class T,
		         class V = type_vec,
				 class New = typename V::template store<const_counter<V>() + 1>>
		constexpr static int add_type(int R = _aux_define<New, T>::v) { return R; }

	public:
		template<class V = type_vec,
		         int N = check_const_counter<V>()>
		using current_store = typename V::template store<N>;

		template<class V = type_vec,
		         class Store = typename V::template current_store<>>
		using current_type = decltype(_aux_is_defined(Store()));

		template<class... T, class V = type_vec>
		constexpr static void push(int R = add_type<append_params<typename V::template current_type<>, T...>>()) {}
	};

	#if defined(__clang__)
		#pragma clang diagnostic pop
	#elif defined(__GNUC__)
		#pragma GCC diagnostic pop
	#endif
	// }}}
	// set / for_each / seq {{{
	template<class T, class... Ts> struct set      : type_list<T, Ts...> {};
	template<class T, class... Ts> struct for_each : type_list<T, Ts...> {};

	namespace _internal {
		constexpr NAT seq_nat{};
	}

	template<auto F, auto S = _internal::seq_nat, auto L = _internal::seq_nat>
	struct seq {
		constexpr static int first = F;
		constexpr static int step  = S;
		constexpr static int last  = L;
		constexpr static size_t size = 1 + ((F < L) ? (L - F) : (F - L)) / ((S < 0) ? -S : S);
	};

	template<int L> struct seq<L, _internal::seq_nat, _internal::seq_nat> : seq<0, 1, L> {};
	template<int F, int L> struct seq<F, L, _internal::seq_nat> : seq<F, 1, L> {};
	// }}}
	// type_variant {{{
	namespace _internal {
		NATTED(unpack_sets);
		template<class... T, class... Ts> struct unpack_sets_t<     set<T...>, Ts...> : unpack_sets_t<T..., Ts...> {};
		template<class... T, class... Ts> struct unpack_sets_t<for_each<T...>, Ts...> : unpack_sets_t<T..., Ts...> {};
	}

	namespace _internal::variant_aux {
		template<template<class> class> struct variant_pkg {};

		NATTED2(expand_variants);
		template<int I, template<class> class V, class T, class... Ts>     // Normal type.
		struct expand_variants_t<I, variant_pkg<V>, T, Ts...>
			: expand_variants_t<I, variant_pkg<V>, Ts..., T, typename V<T>::type> {};

		template<int I, template<class> class V, class... T, class... Ts>  // Join other set<>s.
		struct expand_variants_t<I, variant_pkg<V>, set<T...>, Ts...>
			: expand_variants_t<I, variant_pkg<V>, T..., Ts...> {};

		template<int I, template<class> class V, class... T>               // Final: unpack set<>s produced by the variant.
		struct expand_variants_t<I, variant_pkg<V>, expand_variants_NAT, T...> {
			using type = template_cast<set, unpack_sets<T...>>;
		};
	}

	template<template<class> class Variant, class... T>
	struct type_variant_t {
		using type = _internal::variant_aux::expand_variants<_internal::variant_aux::variant_pkg<Variant>, T...>;
	};
	// }}}
	// classes {{{
	// Generate types from a template.
	// classes<T, int>             = set<T<int>>
	// classes<T, set<int, void>>  = set<T<int>, T<void>>
	//
	namespace _internal::classes_aux {
		// flat_reverse {{{
		template<class...> struct flat_reverse { using type = type_list<>; };
		template<class    T, class... Ts> struct flat_reverse<             T, Ts...> { using type = append_params<typename flat_reverse<Ts...>::type, T>; };
		template<class... T, class... Ts> struct flat_reverse<     set<T...>, Ts...> { using type = append_params<typename flat_reverse<Ts...>::type, template_cast<     set, unpack_sets<T...>>>; };
		template<class... T, class... Ts> struct flat_reverse<for_each<T...>, Ts...> { using type = append_params<typename flat_reverse<Ts...>::type, template_cast<for_each, unpack_sets<T...>>>; };
		// }}}
		// cast_template {{{
		// cast_template<To, From,
		//               int, A<char>, From<void>> = type_list<int, A<char>, To<void>>
		namespace _detail {
			NATTED2(cast_template_base);
			template<int I, template<class...> class To, template<class...> class From,
					 class... T, class... Ts>
			struct cast_template_base_t<I, template_list<To, From>, From<T...>, Ts...>
			     : cast_template_base_t<I, template_list<To, From>, Ts..., To<T...>> {};
		}

		template<template<class...> class To,
		         template<class...> class From,
				 class... T>
		using cast_template = _detail::cast_template_base<template_list<To, From>, T...>;
		// }}}
		// expand_for_each {{{
		// expand_for_each<type_list<type_list<A>, type_list<B>, type_list<C>>,  // Save
		//                 X, Y>                                                 // for_each<X, Y>
		//
		// = type_list<type_list<X, A>, type_list<X, B>, type_list<X, C>>,
		//             type_list<Y, A>, type_list<Y, B>, type_list<Y, C>>>
		NATTED2(expand_for_each);
		template<int I, class... Save, class F, class... Ts>
		struct expand_for_each_t<I, type_list<Save...>, F, Ts...>
			: expand_for_each_t<I, type_list<Save...>, Ts..., insert_params<Save, F>...> {};

		template<int I, class... Save, class... Ts>
		struct expand_for_each_t<I, type_list<Save...>, expand_for_each_NAT, Ts...> { using type = type_list<Ts...>; };
		// }}}
		// expand_set {{{
		template<size_t N, class Save, class Set, class...> struct expand_set_t;
		template<size_t N, class T, class... Ts, class S, class... Ss, class... Result>
		struct expand_set_t<N, type_list<T, Ts...>, type_list<S, Ss...>, Result...>
			: expand_set_t<N - 1, type_list<Ts..., T>, type_list<Ss..., S>, Result..., insert_params<T, S>>
		{};

		template<class T, class... Ts, class S, class... Ss, class... Result>
		struct expand_set_t<0, type_list<T, Ts...>, type_list<S, Ss...>, Result...> {
			using type = type_list<Result...>;
		};

		template<class Save, class Set>
		using expand_set = typename expand_set_t<(Save::size > Set::size) ? Save::size : Set::size, Save, Set>::type;
		// }}}
		// expand_seq {{{
		template<size_t N, class Save, int First, int Step, class...> struct expand_seq_t;
		template<size_t N, class T, class... Ts, int I, int Step, class... Result>
		struct expand_seq_t<N, type_list<T, Ts...>, I, Step, Result...>
			: expand_seq_t<N - 1, type_list<Ts..., T>, I + Step, Step, Result..., insert_params<T, constant<I>>>
		{};

		template<class T, class... Ts, int I, int Step, class... Result>
		struct expand_seq_t<0, type_list<T, Ts...>, I, Step, Result...> {
			using type = type_list<Result...>;
		};
		// }}}
		// expand: Build a list of all parameter sets, with all set/for_each resolved.
		template<class Save, class... T> struct expand_t;
		template<class Save, class... F, class... Ts>  // for_each<>
		struct expand_t<Save, for_each<F...>, Ts...> {
			using type = typename expand_t<expand_for_each<Save, F...>, Ts...>::type;
		};

		template<class Save, class... T, class... Ts>  // set<>
		struct expand_t<Save, set<T...>, Ts...> {
			using type = typename expand_t<expand_set<Save, type_list<T...>>, Ts...>::type;
		};

		template<class Save, auto... Seq, class... Ts>  // seq<>
		struct expand_t<Save, seq<Seq...>, Ts...> {
			using S = seq<Seq...>;
			using type = typename expand_t<typename expand_seq_t<S::size, Save, S::first, S::step>::type, Ts...>::type;
		};

		template<class Save, class T, class... Ts>  // normal type
		struct expand_t<Save, T, Ts...> {
			using type = typename expand_t<expand_set<Save, type_list<T>>, Ts...>::type;
		};

		template<class Save> struct expand_t<Save> { using type = Save; };  // fin

		template<class... T>
		using expand = typename expand_t<type_list<type_list<>>, T...>::type;
	}

	template<template<class...> class TT, class... Param>
	class classes_t {
		// In reverse order, with no recursive set/for_each.
		using sets = template_cast<_internal::classes_aux::expand,
		                           typename _internal::classes_aux::flat_reverse<Param...>::type>;

		template<class... T>
		using cast = _internal::classes_aux::cast_template<TT, type_list, T...>;
	public:
		using type = template_cast<set, template_cast<cast, sets>>;
	};
	// }}}
	// helpers {{{
	template<class T> using add_const    = typename add_const_t   <T>::type;
	template<class T> using add_volatile = typename add_volatile_t<T>::type;
	template<class T> using add_pointer  = typename add_pointer_t <T>::type;
	template<class T> using add_lref     = typename add_lref_t    <T>::type;
	template<class T> using add_rref     = typename add_rref_t    <T>::type;
	template<class T> using remove_const    = typename remove_const_t   <T>::type;
	template<class T> using remove_volatile = typename remove_volatile_t<T>::type;
	template<class T> using remove_pointer  = typename remove_pointer_t <T>::type;
	template<class T> using remove_lref     = typename remove_lref_t    <T>::type;
	template<class T> using remove_rref     = typename remove_rref_t    <T>::type;
	template<class T> using remove_ref      = typename remove_ref_t   <T>::type;
	template<class T> using remove_cv       = typename remove_cv_t    <T>::type;
	template<class T> using remove_cvref    = typename remove_cvref_t <T>::type;

	template<class T> constexpr bool is_const          = is_const_t<T>::v;
	template<class T> constexpr bool is_volatile       = is_volatile_t<T>::v;
	template<class T> constexpr bool is_lref           = is_lref_t<T>::v;
	template<class T> constexpr bool is_rref           = is_rref_t<T>::v;
	template<class T> constexpr bool is_ref            = is_lref<T> || is_rref<T>;
	template<class T> constexpr bool is_floating_point = is_floating_point_t<T>::v;
	template<class T> constexpr bool is_array          = is_array_t<T>::v;
	template<class T> constexpr bool is_function       = is_function_t<T>::v;
	template<class T> constexpr bool is_void           = is_void_t<T>::v;

	template<class T> using naked = typename naked_t<T>::type;

	template<class A, class B> constexpr bool is_same = is_same_t<A, B>::v;
	template<class B, class D> constexpr bool is_base_of = is_base_of_t<B, D>::v;
	template<class... T> constexpr bool are_same = are_same_t<T...>::v;
	template<class... T> constexpr bool is_any_of = is_any_of_t<T...>::v;

	template<template<class...> class TT, class... Param>
	using classes = typename classes_t<TT, Param...>::type;

	template<template<class> class Variant, class... T>
	using type_variant = typename type_variant_t<Variant, T...>::type;

	namespace _internal {
		template<class T> struct add_ref { using type = set<T&, T&&>; }; // Special variant adds two types.
	}

	template<class... T> using and_const    = type_variant<add_const_t, T...>;
	template<class... T> using and_volatile = type_variant<add_volatile_t, T...>;
	template<class... T> using and_pointer  = type_variant<add_pointer_t, T...>;
	template<class... T> using and_lref     = type_variant<add_lref_t, T...>;
	template<class... T> using and_rref     = type_variant<add_rref_t, T...>;
	template<class... T> using and_ref      = type_variant<_internal::add_ref, T...>;
	template<class... T> using and_cv       = and_const<and_volatile<T...>>;
	template<class... T> using and_cvref    = and_ref<and_cv<T...>>;
	// }}}
} // dragon
// }}}

#include <stdio.h>
#include <stdlib.h>

namespace test {
	template<class... T> struct parameters : dragon::type_list<T...> {};

	namespace _internal {
		using namespace dragon;

		extern unsigned errors;
		extern unsigned completed;

		enum class category { R, C, CR };  // Runtime? Compile time?

		template<class T> struct type_wrapper { using type = T; };

		template<class PointerClass, class Object>
		constexpr bool related_pointer = is_same<naked<PointerClass>, naked<Object>> || is_base_of<PointerClass, Object>;

		struct not_static {};

		// member_ptr {{{
		// Take a member pointer and produce a reference to the variable it points to, or a callable
		// proxy if it is a function. This is because non-static member functions must be called.
		// Static member.
		template<auto MP, class T = decltype(MP)>
		struct member_ptr {
			constexpr static decltype(auto) dereference() { return *MP; }
			constexpr static decltype(auto) dereference(auto&&) { return *MP; }  // Allow and ignore object for static members.
		};

		// Non-static member.                                                   
		template<auto MP, class T, class Class>                                 
		class member_ptr<MP, T Class::*> {                                      //
			template<class To, class From,                                      // Dereferencing a member pointer to a private base is an error, but
					 class F = remove_ref<From>,                                // converting an object to its base is ok, even if it is inaccessible.
					 class C = conditional<is_const<F>, const To, To>,          //
					 class V = conditional<is_volatile<F>, volatile C, C>>      // class A { int a; }
			using copy_cv = V;                                                  // class B : private A {};
		public:                                                                 //
			constexpr static decltype(auto) dereference(auto&& object) {        // B b;
				using O = decltype(object);                                     // b.*(&A::a);         // error: A is an inaccessible base of B
				static_assert(related_pointer<Class, O>,                        // ((A&)b).*(&A::a);   // ok
				              "member pointer and object are not related.");    //
				return ((copy_cv<Class, O>&)object).*MP;                        
			}                                                                   
			constexpr static not_static dereference() { return {}; }
		};                                                                      

		// Static function.
		template<auto MP, class R, class... A>
		struct member_ptr<MP, R (*)(A...)> {
			constexpr static member_ptr dereference()       { return {}; };
			constexpr static member_ptr dereference(auto&&) { return {}; }  // Allow and ignore object for static members.

			template<class... Args>
			constexpr decltype(auto) operator()(Args&&... args) const { return (*MP)(forward<Args>(args)...); }
		};

		// Non-static functions.
		template<auto MP, class Class, bool callable = true>
		class non_static_function {
			Class& o;

			constexpr non_static_function(Class& object) noexcept : o(object) {}
			non_static_function() noexcept = delete;

			template<auto, class, bool> friend class non_static_function;
		public:
			constexpr static auto dereference(auto&& object) {  // (see non-static member)
				auto& o = object;
				using O = decltype(o);
				static_assert(related_pointer<Class, O>, "member pointer and object are not related.");
				return non_static_function<MP, Class, !is_const<remove_ref<O>> || is_const<Class>>((Class&)o);
			};
			constexpr static not_static dereference() { return {}; }

			template<class... Args>
			constexpr decltype(auto) operator()(Args&&... args) {
				static_assert(callable, "const object cannot call non-const function.");
				return (o.*MP)(forward<Args>(args)...);
			}
		};

		template<auto MP, class R, class C, class... A> struct member_ptr<MP, R (C::*)(A...)> : non_static_function<MP, C> {};
		template<auto MP, class R, class C, class... A> struct member_ptr<MP, R (C::*)(A...) const> : non_static_function<MP, const C> {};
		template<auto MP, class R, class C, class... A> struct member_ptr<MP, R (C::*)(A...) volatile> : non_static_function<MP, C> {};
		template<auto MP, class R, class C, class... A> struct member_ptr<MP, R (C::*)(A...) volatile const> : non_static_function<MP, const C> {};

		// Types.
		template<auto E, class T> struct member_ptr<E, type_wrapper<T>> : member_ptr<E, const type_wrapper<T>> {};
		template<auto E, class T> struct member_ptr<E, const type_wrapper<T>> {
			constexpr static type_wrapper<T> dereference() { return {}; }
		};
		// }}}
		// walk_members {{{
		template<class MPs, size_t... I> struct walk_members;
		template<class MPs, size_t I> struct walk_members<MPs, I> : member_ptr<MPs::template get<I>()> {};  // direct member

		template<class MPs, size_t I, size_t... Is>  // deeper
		struct walk_members<MPs, I, Is...> : walk_members<MPs, Is...> {
			constexpr static decltype(auto) dereference(auto&& o) {
				return walk_members<MPs, Is...>::dereference(
				                    walk_members<MPs, I>::dereference(forward<decltype(o)>(o)));
			}
		};
		// }}}
		// tests {{{
		struct registry { using tests = type_vec<registry>; };

		template<template<class> class Test, class Param, auto... Prv>
		class define_test {
			using MPs = value_list<Prv...>;
			class access {
				template<size_t... I>
				struct walk {
					static_assert(sizeof...(I) > 0 && (((I < MPs::size) && ...)), "prv: indices out of bounds");
					using type = walk_members<MPs, I...>;
				};
			public:
				using param = Param;

				template<size_t... I> constexpr static decltype(auto) prv() {
					static_assert(!is_same<not_static, decltype(walk<(..., I)>::type::dereference())>,
					              "Non-static member requires an object.");
					return walk<(..., I)>::type::dereference();
				}
				template<size_t... I> constexpr static decltype(auto) prv(auto&& o) { return walk<I...>::type::dereference(forward<decltype(o)>(o)); }
			};

			decltype(registry::tests::push<Test<access>>()) register_this_test();  // never used
		};
		// }}}
		// exec {{{
		template<class... T> struct unwrap_parameters                   { using type = type_list<T...>; };
		template<class... T> struct unwrap_parameters<parameters<T...>> { using type = type_list<T...>; };

		template<class Tests>
		class exec {
			template<class Test>
			class gen_all_tests {
				template<auto F>
				class exec_test_t {
					consteval static void constcall() { F(); }
					static void call() { F(); }
				public:
					exec_test_t() noexcept {
						if constexpr(Test::_test_internals_::cat != category::R) { constcall(); ++completed; }
						if constexpr(Test::_test_internals_::cat != category::C) {      call(); ++completed; }
					}
				};

				template<class... T> using function = member_ptr<&Test::template body<nth<0, T...>, T...>>;
				template<class... T> using exec_test = exec_test_t<function<T...>::dereference()>;
				template<class... T> using make_exec_tests = classes<exec_test, T...>;

				using P = typename unwrap_parameters<typename Test::_test_internals_::access::param>::type;
			public:
				using type = template_cast<make_exec_tests, P>;
			};

			template<int I = 0>
			void exec_all() {
				if constexpr(I < Tests::size) {
					using Test = get_param<I, Tests>;
					instantiate<typename gen_all_tests<Test>::type>();
					exec_all<I + 1>();
				}
			}

		public:
			exec() noexcept { exec_all(); }
		};
		// }}}
	} // _internal

	template<class T>
	constexpr _internal::type_wrapper<T> type = _internal::type_wrapper<T>{};

	struct printer_t {
		bool prt;
		constexpr printer_t(bool p) noexcept : prt(p) {}

		template<class... Args>
		const printer_t& print(Args&&... args) const { if (prt) fprintf(stderr, dragon::forward<Args>(args)...); return *this; }

		const printer_t& operator<<(int i) const         { return print("%d\n", i); }
		const printer_t& operator<<(const char* s) const { return print("%s\n", s); }
	};

	template<bool prt> 
	constexpr printer_t printer(prt);

	constexpr printer_t expect(bool cond, const char* file, size_t line, const char* msg) {
		if (cond) return printer<false>;
		if (dragon::is_constant_evaluated())
			return (3 / (0 + cond)); // error: EXPECT() failed
		else {  // runtime error
			fprintf(stderr, "\x1B[1m%s:%lu: \x1B[31merror:\x1B[0m expected %s\n", file, line, msg);
			if (TDD_MAX_ERRORS == (__atomic_fetch_add(&_internal::errors, 1, __ATOMIC_RELAXED) + 1))
				exit(_internal::errors);
		}
		return printer<true>;
	}
}

#define prv_ref decltype(auto)

#define TEST_INTERNAL_STR_(a) #a
#define TEST_INTERNAL_NTOS_(a) TEST_INTERNAL_STR_(a)
#define TEST_INTERNAL_ERROR_MSG_(EXPR) \
	__FILE__ ":" TEST_INTERNAL_NTOS_(__LINE__) ": error: expected '" #EXPR "'\n"

#define EXPECT(...) test::expect((__VA_ARGS__), __FILE__, __LINE__, TEST_INTERNAL_STR_((__VA_ARGS__)))


#define DECL_TEST_(CONSTEXPR, CAT, NAME, PARAM, ...)                                                                                                 \
	namespace test::_internal {                                                                                                                      \
		template<class Access> struct test_## NAME ##_ {                                                                                             \
			struct _test_internals_ { using access = Access; constexpr static category cat = CAT; };                                                 \
			template<size_t... I> using prv_type = typename decltype(Access::template prv<I...>())::type;                                            \
			template<size_t... I> constexpr static decltype(auto) prv()         { return Access::template prv<I...>(); }                             \
			template<size_t... I> constexpr static decltype(auto) prv(auto&& o) { return Access::template prv<I...>(forward<decltype(o)>(o)); }      \
			template<class, class...> CONSTEXPR static void body();                                                                                  \
		};                                                                                                                                           \
		template class define_test<test_## NAME ##_, PARAM __VA_OPT__(,) __VA_ARGS__>;                                                               \
	}                                                                                                                                                \
	template<class Access> template<class X, class... Xs>                                                                                            \
	CONSTEXPR void test::_internal::test_## NAME ##_<Access>::body()


#define   TESTX(NAME, ...) DECL_TEST_(         , test::_internal::category::R,  NAME __VA_OPT__(,) __VA_ARGS__)
#define  CTESTX(NAME, ...) DECL_TEST_(constexpr, test::_internal::category::C,  NAME __VA_OPT__(,) __VA_ARGS__)
#define CRTESTX(NAME, ...) DECL_TEST_(constexpr, test::_internal::category::CR, NAME __VA_OPT__(,) __VA_ARGS__)

#define   TEST(NAME, ...)   TESTX(NAME, void __VA_OPT__(, ) __VA_ARGS__)
#define  CTEST(NAME, ...)  CTESTX(NAME, void __VA_OPT__(, ) __VA_ARGS__)
#define CRTEST(NAME, ...) CRTESTX(NAME, void __VA_OPT__(, ) __VA_ARGS__)

#define RUN_ALL()                                                                      \
	namespace test::_internal {                                                        \
		static exec<registry::tests::current_type<>> run_all_define_exec_object_{};    \
	}
