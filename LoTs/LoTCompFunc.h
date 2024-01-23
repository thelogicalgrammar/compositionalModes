# pragma once

using t_input = 
	std::tuple<t_meaning,t_meaning>;

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
// might require a restructuring of the way meanings are represented)
template <typename T, typename U> struct accepts_arg : std::false_type {};
template <> struct accepts_arg< t_UC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_BC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_TC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_IV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_DP_M, t_IV_M>:  std::true_type {};
template <> struct accepts_arg< t_TV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_Q_M,  t_IV_M>:  std::true_type {};
// this is a helper variable template
template <typename T, typename U>
inline constexpr bool accepts_arg_v = accepts_arg<T, U>::value;


// OLD COMPOSE STUFF
/* #include <concepts> */
/* template<typename F, typename Arg> */
/* concept CallableWith = requires(F f, Arg arg) { */
/*     { f(arg) }; */
/* }; */
// This is done to record for which combinations of meanings
// can be composed. 
// For each type T with output type t, 
// find all the types U with input type t
/* template <typename T, typename U> struct can_compose : std::false_type {}; */
/* template <> struct can_compose< t_UC_M, t_UC_M> :  std::true_type {}; */
/* template <> struct can_compose< t_BC_M, t_UC_M> :  std::true_type {}; */
/* template <> struct can_compose< t_TC_M, t_UC_M> :  std::true_type {}; */
/* template <> struct can_compose< t_UC_M, t_IV_M> :  std::true_type {}; */
/* template <> struct can_compose< t_UC_M, t_DP_M> :  std::true_type {}; */
/* template <> struct can_compose< t_BC_M, t_IV_M> :  std::true_type {}; */
/* template <> struct can_compose< t_BC_M, t_DP_M> :  std::true_type {}; */
/* template <> struct can_compose< t_TC_M, t_IV_M> :  std::true_type {}; */
/* template <> struct can_compose< t_TC_M, t_DP_M> :  std::true_type {}; */
/* template <> struct can_compose< t_DP_M, t_TV_M> :  std::true_type {}; */
/* template <> struct can_compose< t_Q_M, t_TV_M > :  std::true_type {}; */
/* template <typename T, typename U> */
/* inline constexpr bool can_compose_v = can_compose<T, U>::value; */

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
				[](auto&& f, auto&&arg) -> t_meaning {
					// The types of f and arg here 
					// are specific meanings 
					// in the variant t_meaning
					// e.g., t_t_M, t_UC_M, etc.
					using T = std::decay_t<decltype(f)>;
					using U = std::decay_t<decltype(arg)>;
					// Create a new meaning, 
					// i.e., a function from a context 
					// to an extension.
					if constexpr (accepts_arg_v<T,U>) {
						return [f,arg](t_context c) -> auto {
							return f(c)(arg(c));
						};
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

	// If the first bit doesn't compose, go to the second
	std::function<t_meaning(t_meaning,t_meaning)> Else =
		+[](t_meaning m1, t_meaning m2) -> t_meaning {
			if (std::holds_alternative<Empty_M>(m1)) {
				return m2;
			} else {
				return m1;
			}
		};

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
	std::function<t_meaning(t_meaning, t_BTC_PtoE)> getEntity = 
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
		+[](t_meaning meaning) -> t_meaning {
			t_meaning out = t_meaning(
				std::visit(
					[](auto&& m) -> t_meaning {
						using T = std::decay_t<decltype(m)>;
						if constexpr (std::is_same_v<T,t_IV_M>) {
							return t_meaning([m](t_context c) -> t_IV {
								t_IV mc = m(c);
								// an IV is a function from entities 
								// to truth values
								t_IV returnvalue = 
									[mc](t_e e) -> t_t {return !mc(e);};
								return returnvalue;
							});
						} else {
							return t_meaning(Empty_M());
						}
					},
					meaning
				)
			);
			return out;
		};

	// Existential closure
	// Check if an entity satisfies a property
	std::function<t_meaning(t_meaning)> exClosure = 
		+[](t_meaning meaning) -> t_meaning {
			t_meaning out = t_meaning(
				std::visit(
					[](auto&& m) -> t_meaning {
						using T = std::decay_t<decltype(m)>;
						if constexpr (std::is_same_v<T,t_IV_M>) {
							return [m](t_context c) -> t_t {
								t_IV mc = m(c);
								// an IV is a function from entities
								// to truth values
								t_t returnvalue = false;
								for (auto e : c) {
									if (mc(e)) {
										returnvalue = true;
										break;
									}
								}
								return returnvalue;
							};
						} else {
							return t_meaning(Empty_M());
						}
					},
					meaning
				)
			);
			return out;
		};

	// Go from an entity to a property that is true of that entity
	std::function<t_meaning(t_meaning)> abstractE = 
		+[](t_meaning meaning) -> t_meaning {
			t_meaning out = t_meaning(
				std::visit(
					[](auto&& m) -> t_meaning {
						using T = std::decay_t<decltype(m)>;
						if constexpr (std::is_same_v<T,t_e_M>) {
							return [m](t_context c) -> t_IV {
								t_e me = m(c);
								// an IV is a function from entities
								// to truth values
								t_IV returnvalue = 
									[me](t_e e) -> t_t {return e == me;};
								return returnvalue;
							};
						} else {
							return t_meaning(Empty_M());
						}
					},
					meaning
				)
			);
			return out;
		};

	t_BTC_compose propDifference =
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
									return mc1(e) && !mc2(e);
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

	t_BTC_compose propInclusion =
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
						t_t_M outM = [m1,m2](t_context c) -> t_t {
							t_IV mc1 = m1(c);
							t_IV mc2 = m2(c);
							bool out = true;
							for (auto e : c) {
								if (mc1(e) && !mc2(e)) {
									out = false;
									break;
								}
							}
							return out;
						};
						return t_meaning(outM);
					} else {
						return t_meaning(Empty_M());
					}
				},
				leftM, rightM
			);
		};
	
	// functional composition
	/* t_BTC_compose fcompose = 
		+[]( t_meaning leftM, t_meaning rightM) -> t_meaning {
		return std::visit(
			[](auto&& f, auto&& g) -> t_meaning {
				using T = std::decay_t<decltype(f)>;
				using U = std::decay_t<decltype(g)>;
				// If both meanings are functions
				if constexpr (can_compose_v<T,U>) {
					// return a meaning that is a function
					// from a context to the composed function
					return [f,g](t_context c) -> auto {
						// Return the composed function
						// (e should be the input type to f(c))
						return [f,g,c](auto e) -> auto {
							auto fc = f(c);
							auto gc = g(c);
							// If the composed function is callable
							if constexpr ( 
									CallableWith<decltype(fc),decltype(e)>
								) {
								auto fce = fc(e);
								if constexpr ( 
										CallableWith<decltype(gc),decltype(fce)>
									) {
									return gc(fce);
								} else {
									return t_meaning(Empty_M());
								}
							} else {
								return t_meaning(Empty_M());
							}
						};
					};
				} else {
					return t_meaning(Empty_M());
				}
			},
			leftM, rightM
		);
	}; */
	
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

		// (t_meaning, t_meaning) -> t_meaning
		add("( Else %s %s )", 	Else			, 2.0);
		add("( %s %s )", 		rapply			, 0.1);
		add("( exClosure %s )", exClosure		, 0.1);
		add("( pAND %s %s )", 	propIntersection, 0.1);
		add("( pOR %s %s  )",	propUnion		, 0.1);
		add("( dDIFF %s %s )", 	propDifference	, 0.1);
		/* add("( compose %s %s )",compose			, 0.1); */
		// pINC tends to be used too much
		/* add("( pINC %s %s )", 	propInclusion	, 0.1); */

		// t_input -> t_input
		add("( flipArgs %s )", 	flipArgs		, 0.1);

		// (t_meaning,t_meaning) -> t_input
		add("( tuple %s %s )", 	buildTuple		, 0.1);

		// (t_meaning, ( (t_context, t_IV_M) -> t_e ) ) -> t_meaning
		add("( getE %s %s )", 	getEntity		, 0.1);

		// int -> ( (t_context, t_IV_M) -> t_e )
		add("( nTh %s )", 		nThInContext	, 0.1);

		// t_meaning -> t_meaning
		add("( pNOT %s )", 		propComplement	, 0.1);
		add("( abstractE %s )", abstractE		, 0.1);

		// (t_meaning, t_meaning, t_meaning) -> t_meaning
		add("( if %s %s %s )", 	ifElseMeaning	, 0.1);

		// t_input
		add("X",				
			Builtins::X<CompGrammar>, 4);
		
		// t_input -> t_meaning
		add("%s.L",				
			+[](t_input i) -> t_meaning {return std::get<0>(i);});
		add("%s.R",				
			+[](t_input i) -> t_meaning {return std::get<1>(i);});

		// ints
		// Implement a simple base-10 encoding for integers
		/* add("(%s + %s)", */    
		/* 	+[](int s, int n) -> int { return s + n; }); */
		/* add("tens(%s)", */    
		/* 	+[](int n) -> int {return n*10; }); */
		for(int i=1;i<=10;i++) {
			add_terminal( "#"+str(i), i, 0.05);
		}

		// (int, int) -> bool
		add("( intEq %s %s )", 
			+[](int i1, int i2) -> bool {return i1 == i2;});
		add("( intLt %s %s )",
			+[](int i1, int i2) -> bool {return i1 < i2;});

		// int -> t_meaning
		// return a t_meaning containing a <et,t> (i.e., a DP)
		// that returns true if the number of entities in the context
		// that satisfy the input meaning is equal to the input integer
		add("( card %s )",
			+[](int i) -> t_meaning {
				return [i](t_context c) -> t_DP {
					return [c,i](t_IV x) -> t_t {
						int count = 0;
						for (auto e : c) {
							if (x(e)) {
								count++;
							}
						}
						return count == i;
					};
				};
			}
		);
		
		// bool
		add_terminal("true",  true);
		add_terminal("false", false);

		// bool -> bool
		add("( not %s )", 
			+[](bool b) -> bool {return !b;});
		
		// (bool, bool) -> bool
		add("( and %s %s )",
			+[](bool b1, bool b2) -> bool {return b1 && b2;});
		add("( or %s %s )",
			+[](bool b1, bool b2) -> bool {return b1 || b2;});

		// t_meaning -> bool
		add("( is_t %s )" , is_t_M,  0.1);
		add("( is_e %s )" , is_e_M,  0.1);
		add("( is_UC %s )", is_UC_M, 0.1);
		add("( is_BC %s )", is_BC_M, 0.1);
		add("( is_TC %s )", is_TC_M, 0.1);
		add("( is_IV %s )", is_IV_M, 0.1);
		add("( is_DP %s )", is_DP_M, 0.1);
		add("( is_TV %s )", is_TV_M, 0.1);
		add("( is_Q %s )" , is_Q_M,  0.1);

		// Seems to be extreeeeemely slow
		// and sometimes kills the process
		/* add("recurse(%s)", */ 
		/* 	Builtins::Recurse<CompGrammar>, 1); */

	}
} grammar;

// Define CompHypothesis

class MyHypothesis final : public DeterministicLOTHypothesis<
	MyHypothesis,
	t_input,
	t_meaning,
	CompGrammar,
	&grammar,
	defaultdatum_t<t_context,std::string>
	> {

private:

	LexicalSemantics lexSem = LexicalSemantics();

public:
	using Super = DeterministicLOTHypothesis<
		MyHypothesis,
		t_input,
		t_meaning,
		CompGrammar,
		&grammar,
		defaultdatum_t<t_context,std::string>
		>;
	using Super::Super; 

	// NOTE //
	// lexSem, compositionF, and getLexicon 
	// need to be defined so it can be used by the agent
	// e.g., when producing a signal

	t_BTC_compose getCompositionF() {
		return 
			[this](t_meaning m1, t_meaning m2) -> t_meaning {
				// put the meanings in a tuple
				t_input m = std::make_tuple(m1,m2);
				return this->call(m);
			};
	}

	LexicalSemantics getLexicon() {
		// For this grammar, 
		// the lexicon is the same for all hypotheses
		return lexSem;
	}

	double compute_single_likelihood(const datum_t& x) override {

		// context is the first element of the input
		t_context context = x.input;

		// output: a SExpr representation of the parseTree 
		const std::unique_ptr<BTC> parseTree = 
			BTC::fromSExpression(x.output,lexSem);

		// call takes an input and returns a meaning
		t_BTC_compose compF = getCompositionF();

		try{
			// outputMeaning will contain type t_t_M
			t_meaning outputMeaning = parseTree->compose(compF);
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

