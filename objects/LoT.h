# pragma once

// Here I define the functions for the LoT fragment
// that encodes rules for composing *meanings*.
// This is an intensional composition, so it 
// includes things like intensional application.
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// This is done to record for which combinations of meanings
// the second can be applied to the first
// NOTE: it cannot be done easily because we cannot "look"
// into the type of the extension easily
// (There's probably some more elegant way to do this but it 
// would require a restructuring of the way meanings are represented)
template <typename T, typename U> struct accepts_arg : std::false_type {};
template <> struct accepts_arg< t_UC_M,  t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_BC_M,  t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_BC2_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_IV_M,  t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_DP_M,  t_IV_M>:  std::true_type {};
template <> struct accepts_arg< t_TV_M,  t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_Q_M,   t_IV_M>:  std::true_type {};
template <typename T, typename U>
// this is a helper variable template
inline constexpr bool accepts_arg_v = accepts_arg<T, U>::value;

// apply needs to return a t_meaning
// which is a function from a context to a specific extension type
// But since we don't know what the extension type is in advance
// we need to treat each case separately

// Here I define the DSL for the language
// that encodes the *composition* function
namespace COMP_DSL{
	
	// Intensional right-application
	// Of one meaning to another
	t_BTC_compose rapply = 
		[](t_meaning a, t_meaning b) -> t_meaning {
			// meanings are a function from a context
			// to one of the possible extensions
			return std::visit(
				// The types of f and arg here 
				// are specific meanings 
				// in the variant t_meaning
				// e.g., t_t_M, t_UC_M, etc.
				[](auto&& f, auto&&arg) -> t_meaning {
					using T = std::decay_t<decltype(f)>;
					using U = std::decay_t<decltype(arg)>;
					// Create a new meaning, 
					// i.e., a function from a context 
					// to an extension.
					if constexpr (accepts_arg_v<T,U>) {
						return [f,arg](t_context c) 
							-> auto {return f(c)(arg(c));};
					} else {
						return [](t_context c) 
							-> Empty {return Empty{};};
					}
				},
				a, b
			);
		};

	// Switch the order of the arguments
	// of a function that composes two meanings
	std::function<t_meaning(t_meaning,t_meaning,t_BTC_compose)>
	flip = [](t_meaning a, t_meaning b, t_BTC_compose fn) -> 
		t_meaning {return fn(b, a);};

	// check if a meaning is a specific type
	template <typename T>
	std::function<bool(t_meaning)> is_T = [](t_meaning m) 
		-> bool {return std::holds_alternative<T>(m);};

	// if a meaning is of a specific type,
	// return it, otherwise return another
	template <typename T>
	std::function<t_meaning(t_meaning,t_meaning)>
	if_T_else = 
		[](t_meaning a, t_meaning b) -> t_meaning {
			if (is_T<T>(a)) {
				return a;
			} else {
				return b;
			}
		};
	t_BTC_compose if_t_else   = if_T_else<t_t_M>;
	t_BTC_compose if_e_else   = if_T_else<t_e_M>;
	t_BTC_compose if_UC_else  = if_T_else<t_UC_M>;
	t_BTC_compose if_BC_else  = if_T_else<t_BC_M>;
	t_BTC_compose if_BC2_else = if_T_else<t_BC2_M>;
	t_BTC_compose if_IV_else  = if_T_else<t_IV_M>;
	t_BTC_compose if_DP_else  = if_T_else<t_DP_M>;
	t_BTC_compose if_TV_else  = if_T_else<t_TV_M>;
	t_BTC_compose if_Q_else   = if_T_else<t_Q_M>;

	// implement a choice function
	// of an entity in context that satisfies a property
	// I.e., it chooses an entity (type `e`)
	// that satisfies a property.
	/* std::function<t_meaning(t_meaning)> */
	/* choice = [](t_meaning m) -> t_meaning { */
	/* 	return std::visit( */
	/* 		[](auto&& m) -> t_meaning { */
	/* 			using T = std::decay_t<decltype(m)>; */
	/* 			// If the meaning encodes a property, */
	/* 			// choose an entity that satisfies it */
	/* 			// from the context */
	/* 			if constexpr (std::is_same_v<T,t_IV_M>) { */
	/* 				return m; */
	/* 			} else { */
	/* 				return Empty{}; */
	/* 			} */
	/* 		}, */
	/* 		m */
	/* 	); */
	/* }; */

}

// Define CompGrammar


// Define CompHypothesis
