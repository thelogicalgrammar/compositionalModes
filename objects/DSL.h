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
// (There's probably some more elegant way to do this)
template <typename T, typename U> struct is_compatible : std::false_type {};
template <> struct is_compatible<t_UC_M,  t_t_M> :  std::true_type {};
template <> struct is_compatible<t_BC_M,  t_t_M> :  std::true_type {};
template <> struct is_compatible<t_BC2_M, t_t_M> :  std::true_type {};
template <> struct is_compatible<t_IV_M,  t_e_M> :  std::true_type {};
template <> struct is_compatible<t_DP_M,  t_IV_M>:  std::true_type {};
template <> struct is_compatible<t_TV_M,  t_e_M> :  std::true_type {};
template <> struct is_compatible<t_Q_M,   t_IV_M>:  std::true_type {};
template <typename T, typename U>
inline constexpr bool is_compatible_v = is_compatible<T, U>::value;

// apply needs to return a t_meaning
// which is a function from a context to a specific extension type
// But since we don't know what the extension type is in advance
// we need to treat each case separately

// Here I define the DSL for the language
// that encodes the *composition* function
namespace COMP_DSL{
	
	// Intensional right-application
	// Of one meaning to another
	auto rapply = [](t_meaning a, t_meaning b) -> t_meaning {
		// meanings are a function from a context
		// to one of the possible extensions
		return std::visit(
			// The types of f and arg here are specific meanings 
			// in the variant t_meaning
			// e.g., t_t_M, t_UC_M, etc.
			[](auto&& f, auto&&arg) -> t_meaning {
				using T = std::decay_t<decltype(f)>;
				using U = std::decay_t<decltype(arg)>;
				// Create a new meaning, 
				// i.e., a function from a context to an extension.
				if constexpr (is_compatible_v<T,U>) {
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
	auto flip = [](t_meaning a, t_meaning b, t_meaning_trans fn) 
		-> t_meaning { return fn(b, a); };
	
}

