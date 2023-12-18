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
// (which takes two meanings and returns a meaning)
namespace COMP_DSL{
	
	// Intensional right-application
	// Of meaning b to meaning a
	// i.e., a(b)
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
						// If the meaning cannot be applied
						// to the argument, return an empty meaning
						// that indicates nonsensical composition
						return t_meaning(Empty_M());
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

	// check if a meaning is of a specific type
	template <typename T>
	std::function<bool(t_meaning)> is_T = [](t_meaning m) 
		-> bool {return std::holds_alternative<T>(m);};

	auto is_t_M   = is_T<t_t_M>;
	auto is_e_M   = is_T<t_e_M>;
	auto is_UC_M  = is_T<t_UC_M>;
	auto is_BC_M  = is_T<t_BC_M>;
	auto is_BC2_M = is_T<t_BC2_M>;
	auto is_IV_M  = is_T<t_IV_M>;
	auto is_DP_M  = is_T<t_DP_M>;
	auto is_TV_M  = is_T<t_TV_M>;
	auto is_Q_M   = is_T<t_Q_M>;

	auto if_else = 
		[](bool b, t_meaning m1, t_meaning m2) -> t_meaning {
			if (b) {
				return m1;
			} else {
				return m2;
			}
		};

	// First it checks if the input meaning 
	// is type <s,<e,t>> (i.e., a property).
	// Then, it returns a function of type <s,e>
	// which returns an entity in the extension of the property.
	// If the input meaning does not pick out any property in the context,
	// it throws a PresuppositionFailure exception.
	auto getEntity = 
		[](
			t_meaning meaning,
			std::function<t_e(t_context,t_IV_M)> f
		) -> t_meaning { 
			return std::visit(
				[f](auto&& m) -> t_meaning {
					using T = std::decay_t<decltype(m)>;
					// If the meaning encodes a property
					if constexpr (std::is_same_v<T,t_IV_M>) {
						// return a meaning that is a function
						// from a context to an entity
						// that satisfies the property
						// (which entity depends on the function)
						return [m,f](t_context c) -> t_e {return f(c,m);};
					} else {
						// if the meaning is not a property
						// return an empty meaning
						return t_meaning(Empty_M());
					}
				},
				meaning
			);
		};

	// Get a function from an integer i to
	// a function from a context and a property
	// to the i-th entity in the context that satisfies the property
	auto nThInContext(int i) -> std::function<t_e(t_context,t_IV)> {
			return [i](t_context c, t_IV m) -> t_e {
			// loop through the context
			// (which is a set of entities)
			// and return the i-th entity
			// that satisfies the property
			int j = 0;
			for (auto e : c) {
				if (m(e)) {
					if (j == i) {
						return e;
					} else {
						j++;
					}
				}
			}
			// if no entity satisfies the property
			// throw an exception PresuppositionFailure
			throw PresuppositionFailure();
		};
	};

}

// Define CompGrammar


// Define CompHypothesis
