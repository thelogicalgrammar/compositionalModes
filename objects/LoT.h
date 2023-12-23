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
template <> struct accepts_arg< t_UC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_BC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_TC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_IV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_DP_M, t_IV_M>:  std::true_type {};
template <> struct accepts_arg< t_TV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_Q_M,  t_IV_M>:  std::true_type {};
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
		+[](t_meaning a, t_meaning b) -> t_meaning {
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
	std::function<t_input(t_input)> flipArgs = 
		+[](t_input in) -> t_input {
			return std::tuple<t_meaning,t_meaning>(
				std::get<1>(in),
				std::get<0>(in)
			);
		};

	std::function<t_input(t_meaning,t_meaning)> buildTuple = 
		+[](t_meaning a, t_meaning b) -> t_input {
			return std::tuple<t_meaning,t_meaning>(a,b);
		};

	// check if a meaning is of a specific type
	template <typename T>
	std::function<bool(t_meaning)> is_T = 
		+[](t_meaning m) -> bool {
			return std::holds_alternative<T>(m);
		};

	auto is_t_M  = is_T<t_t_M>;
	auto is_e_M  = is_T<t_e_M>;
	auto is_UC_M = is_T<t_UC_M>;
	auto is_BC_M = is_T<t_BC_M>;
	auto is_TC_M = is_T<t_TC_M>;
	auto is_IV_M = is_T<t_IV_M>;
	auto is_DP_M = is_T<t_DP_M>;
	auto is_TV_M = is_T<t_TV_M>;
	auto is_Q_M  = is_T<t_Q_M>;

	std::function<t_meaning(bool,t_meaning,t_meaning)> ifElseMeaning = 
		+[](bool b, t_meaning m1, t_meaning m2) -> t_meaning {
			if (b) {
				return m1;
			} else {
				return m2;
			}
		};

	// First it checks if the input meaning 
	// is type <s,<e,t>> (i.e., a property).
	// Then, it returns a function of type <s,e>
	// which given a context returns an entity.
	// If the input meaning does not pick out any property in the context,
	// it throws a PresuppositionFailure exception.
	std::function<t_meaning(t_meaning, t_BTC_PtoE)> 
	getEntity = 
		+[](t_meaning meaning, t_BTC_PtoE f) -> t_meaning { 
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
	// (if there is no such entity, throw an exception)
	// (This function could be used as an f argument to getEntity)
	t_BTC_chooseF nThInContext =
		+[](int i) -> t_BTC_PtoE {
			return [i](t_context c, t_IV_M m) -> t_e {
				t_IV iv = m(c);
				// loop through the context
				// (which is a set of entities)
				// and return the i-th entity
				// that satisfies the property
				int j = 0;
				for (auto e : c) {
					if (iv(e)) {
						if (j == i) {
							return t_e(e);
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

	// Helper function to define binary operations on properties
	t_BTC_compose propIntersection = 
		+[](t_meaning leftM, t_meaning rightM) -> t_meaning {
			return std::visit(
				[](auto&& m1, auto&& m2) -> t_meaning {
					using T = std::decay_t<decltype(m1)>;
					using U = std::decay_t<decltype(m2)>;
					// If both meanings are properties
					if constexpr (
						std::is_same_v<T,t_IV_M> && 
						std::is_same_v<U,t_IV_M>
					) {
						// return a meaning that is a function
						// from a context to a property
						t_IV_M outM = [m1,m2](t_context c) -> t_IV {
							t_IV mc1 = m1(c);
							t_IV mc2 = m2(c);
							return t_IV(
								[mc1,mc2,c](t_e e) -> t_t {
									/* return t_t(f(mc1(e), mc2(e))); */
									return mc1(e) && mc2(e);
								}
							);
						};
						return t_meaning(outM);
					} else {
						return t_meaning(Empty_M());
					}
				},
				leftM, rightM
			);
		};

	// Helper function to define binary operations on properties
	t_BTC_compose propUnion = 
		+[](t_meaning leftM, t_meaning rightM) -> t_meaning {
			return std::visit(
				[](auto&& m1, auto&& m2) -> t_meaning {
					using T = std::decay_t<decltype(m1)>;
					using U = std::decay_t<decltype(m2)>;
					// If both meanings are properties
					if constexpr (
						std::is_same_v<T,t_IV_M> && 
						std::is_same_v<U,t_IV_M>
					) {
						// return a meaning that is a function
						// from a context to a property
						t_IV_M outM = [m1,m2](t_context c) -> t_IV {
							t_IV mc1 = m1(c);
							t_IV mc2 = m2(c);
							return t_IV(
								[mc1,mc2,c](t_e e) -> t_t {
									/* return t_t(f(mc1(e), mc2(e))); */
									return mc1(e) || mc2(e);
								}
							);
						};
						return t_meaning(outM);
					} else {
						return t_meaning(Empty_M());
					}
				},
				leftM, rightM
			);
		};

	t_BTC_transform propComplement = 
		+[](t_meaning m) -> t_meaning {
			t_meaning out = t_meaning(
				std::visit(
					[](auto&& m) -> t_meaning {
						using T = std::decay_t<decltype(m)>;
						if constexpr (std::is_same_v<T,t_IV_M>) {
							return t_meaning([m](t_context c) -> t_IV {
								t_IV mc = m(c);
								// an IV is a function from entities 
								// to truth values
								t_IV out = 
									[mc](t_e e) -> t_t {return !mc(e);};
								return out;
							});
						} else {
							return t_meaning(Empty_M());
						}
					},
					m
				)
			);
			return out;
		};

}

// Define CompGrammar
class CompGrammar : public Grammar< 
		t_input,
		t_meaning,
		
		t_input,
		t_meaning,
		t_t,
		int,
		std::function<t_input(t_input)>,
		std::function<t_input(t_meaning,t_meaning)>, 
		std::function<bool(t_meaning)>,
		std::function<t_meaning(bool,t_meaning,t_meaning)>,
		t_BTC_chooseF,
		t_BTC_PtoE,
		t_BTC_transform
	>, public Singleton<CompGrammar> {

	using Super = Grammar<
		t_input,
		t_meaning,
		
		t_input,
		t_meaning,
		t_t,
		int,
		std::function<t_input(t_input)>,
		std::function<t_input(t_meaning,t_meaning)>, 
		std::function<bool(t_meaning)>,
		std::function<t_meaning(bool,t_meaning,t_meaning)>,
		t_BTC_chooseF,
		t_BTC_PtoE,
		t_BTC_transform
	>;
	using Super::Super;

public:

	CompGrammar(){

		using namespace COMP_DSL;

		add("( %s %s )", 		rapply			, 0.2);
		add("( flipArgs %s )", 	flipArgs		, 0.2);
		add("( tuple %s %s )", 	buildTuple		, 0.2);
		add("( if %s %s %s )", 	ifElseMeaning	, 0.2);
		add("( getE %s %s )", 	getEntity		, 0.2);
		add("( nTh %s )", 		nThInContext	, 0.2);
		add("( pAND %s %s )", 	propIntersection, 0.2);
		add("( pOR %s %s  )",	propUnion		, 0.2);
		add("( pNOT %s )", 		propComplement	, 0.2);

		add_terminal("true",  true);
		add_terminal("false", false);

		// Implement a simple base-10 encoding for integers
		/* add("(%s + %s)", */    
		/* 	+[](int s, int n) -> int { return s + n; }); */
		/* add("tens(%s)", */    
		/* 	+[](int n) -> int {return n*10; }); */
		for(int i=1;i<=10;i++) {
			add_terminal( "#"+str(i), i, 0.1);
		}
		
		// Add boolean constants
		add_terminal("true",  true);
		add_terminal("false", false);
		add("( not %s )", 
			+[](bool b) -> bool {return !b;});
		add("( and %s %s )",
			+[](bool b1, bool b2) -> bool {return b1 && b2;});
		add("( or %s %s )",
			+[](bool b1, bool b2) -> bool {return b1 || b2;});

		// Functions to check for specific types
		add("( is_t %s )" , is_t_M,  0.1);
		add("( is_e %s )" , is_e_M,  0.1);
		add("( is_UC %s )", is_UC_M, 0.1);
		add("( is_BC %s )", is_BC_M, 0.1);
		add("( is_TC %s )", is_TC_M, 0.1);
		add("( is_IV %s )", is_IV_M, 0.1);
		add("( is_DP %s )", is_DP_M, 0.1);
		add("( is_TV %s )", is_TV_M, 0.1);
		add("( is_Q %s )" , is_Q_M,  0.1);

		// Get left and right argument meanings
		add("X",				
			Builtins::X<CompGrammar>, 5);
		add("%s.L",				
			+[](t_input i) -> t_meaning {return std::get<0>(i);});
		add("%s.R",				
			+[](t_input i) -> t_meaning {return std::get<1>(i);});

	}
} grammar;

// Define CompHypothesis

class MyHypothesis final : public DeterministicLOTHypothesis<
	MyHypothesis,
	t_input,
	t_meaning,
	CompGrammar,
	&grammar,
	defaultdatum_t<std::tuple<t_context,LexicalSemantics>,std::string>
	> {

public:
	using Super = DeterministicLOTHypothesis<
		MyHypothesis,
		t_input,
		t_meaning,
		CompGrammar,
		&grammar,
		defaultdatum_t<std::tuple<t_context,LexicalSemantics>,std::string>
		>;
	using Super::Super; 
	
	double compute_single_likelihood(const datum_t& x) override {

		// context is the first element of the input
		t_context context = std::get<0>(x.input);

		// LexicalSemantics is the second element of the input
		LexicalSemantics lexSem = std::get<1>(x.input);

		// output: a SExpr representation of the parseTree 
		const std::unique_ptr<BTC> parseTree = 
			BTC::fromSExpression(x.output,lexSem);

		// call takes an input and returns a meaning
		t_BTC_compose
		compositionF = [this](t_meaning m1, t_meaning m2) -> t_meaning {
			// put the meanings in a tuple
			t_input m = std::make_tuple(m1,m2);
			return this->call(m);
		};

		try{
			// outputMeaning will contain type t_t_M
			t_meaning outputMeaning = parseTree->compose(compositionF);
			try 
			{
				auto f = std::get<t_t_M>(outputMeaning); 
				t_t out = f(context);
				double logp =
					out
					? log(x.reliability + (1.0-x.reliability)/2.0) 
					: log((1.0-x.reliability)/2.0);
				return logp;
			}
			catch (std::bad_variant_access&) 
			{
				// if the variant is not of type t_t_M, return -inf
				return -std::numeric_limits<double>::infinity();
			}
		} catch (PresuppositionFailure& e) {
			// if the composition fails, return -inf
			return -std::numeric_limits<double>::infinity();
		}
	}
};

