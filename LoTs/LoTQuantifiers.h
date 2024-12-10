# pragma once

// Here I extent the typing system of H&K slightly
// to include integers as a basic type
// to match the definitions in Van de Pol et al 2023.
using t_int = int;

// A bare wrapper function around a meaning type
// so Fleet can distinguish between the operations
// involved in the composition function and those involved
// in the quantifiers definitions (since the specific arguments
// that the two can take are different)
template <typename T>
struct WrapperC {
	T i;
	WrapperC( T x ) : i(x) {}
	// default initializer
	WrapperC() : i() {}
	// == operator
	bool operator==(const WrapperC& other) const {
		return i == other.i;
	}
	// >= operator
	bool operator>=(const WrapperC& other) const {
		return i >= other.i;
	}
	// || operator
	WrapperC operator||(const WrapperC& other) const {
		return WrapperC(i || other.i);
	}
	// && operator
	WrapperC operator&&(const WrapperC& other) const {
		return WrapperC(i && other.i);
	}
	// !
	WrapperC operator!() const {
		return WrapperC(!i);
	}
	// +
	WrapperC operator+(const WrapperC& other) const {
		return WrapperC(i + other.i);
	}
	// -
	WrapperC operator-(const WrapperC& other) const {
		return WrapperC(i - other.i);
	}
	// >
	bool operator>(const WrapperC& other) const {
		return i > other.i;
	}
};
template <typename T, typename V>
struct WrapperF {
	T i;
	WrapperF( T x ) : i(x) {}
	// default initializer
	WrapperF() : i() {}
	auto operator()(V x) const -> decltype(i(x)) {
		return i(x);
	}
};
using t_e_w   = WrapperC<t_e>;
using t_t_w   = WrapperC<t_t>;
using t_int_w = WrapperC<t_int>;
using t_UC_w  = WrapperF<t_UC,t_t>;
using t_BC_w  = WrapperF<t_BC,t_UC>;
using t_TC_w  = WrapperF<t_TC,t_BC>;
using t_IV_w  = WrapperF<t_IV,t_e>;
using t_DP_w  = WrapperF<t_DP,t_IV>;
using t_TV_w  = WrapperF<t_TV,t_e>;
using t_Q_w   = WrapperF<t_Q,t_IV>;

// Input and output types for each sentence produced by the grammar.
// The agent learns a distribution over sentences in this grammar, which
// encodes a part of their language, namely, the three quantifiers and
// how they compose with IVs. 
// The sentence does two things:
// 1. It encodes how to compose Qs with IVs (first three inputs, first output)
// 2. It encodes three quantifiers (last two inputs, last three outputs)
using t_grammar_input = std::tuple<
	// Arguments to compfunction for node [Q IV]
	t_Q,
	t_IV,
	t_IV,
	// same context is used for both inferred components
	t_context,
	// arguments to quantifier meaning
	// (these should not appear in the two components above
	// because once the agent uses them for communication they are 
	// not event defined in the context of composing Q and IV.
	// Therefore, everything is defined with wrappers)
	t_IV_w,
	t_IV_w
>;
// Type of output of the Grammar sentences
using t_grammar_output = std::tuple<
	// the meaning of the node [[Q IV] IV]
	// as a function of the input meanings
	t_t,
	// the output of the three quantifiers in the language
	// given the inputs above
	t_t_w, 
	t_t_w, 
	t_t_w
>;

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
template <typename T, typename U> struct accepts_arg : std::false_type {};
template <> struct accepts_arg< t_UC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_BC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_TC_M, t_t_M> :  std::true_type {};
template <> struct accepts_arg< t_IV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_DP_M, t_IV_M>:  std::true_type {};
template <> struct accepts_arg< t_TV_M, t_e_M> :  std::true_type {};
template <> struct accepts_arg< t_Q_M,  t_IV_M>:  std::true_type {};
template <typename T, typename U>
inline constexpr bool accepts_arg_v = accepts_arg<T, U>::value;

namespace Quants_DSL{

	// function to glue together the components
	// of each sentence produced by the grammar
	t_grammar_output makeGrammarOutput(
		t_t   tQ,
		t_t_w t1,
		t_t_w t2,
		t_t_w t3
	) {
		return std::make_tuple(tQ,t1,t2,t3);
	}

	template < typename tIV >
	auto union_ = 
		+[](tIV m1, tIV m2) -> tIV {
			return tIV([m1,m2](t_e e) -> t_t {
				return m1(e) || m2(e);
			});
		};

	template < typename tIV >
	auto intersection = 
		+[](tIV m1, tIV m2) -> tIV {
			return tIV([m1,m2](t_e e) -> t_t {
				return m1(e) && m2(e);
			});
		};

	template < typename tIV >
	auto setminus =
		+[](tIV m1, tIV m2) -> tIV {
			return tIV([m1,m2](t_e e) -> t_t {
				return m1(e) && !m2(e);
			});
		};

	// Function from an integer and a property
	// to a property that is true of the
	// nth element that satisfies the property
	template < typename tIV, typename INT >
	auto nTh =
		+[](INT i, t_context c, tIV m) -> tIV {
			std::vector<bool> vec;
			t_e ithTrueElement;
			INT count{0};
			for (const auto& x : c) {
				bool result = m(x);
				vec.push_back(result);
				if (result) {
					if (count == i) {  
						ithTrueElement = x;
					}
					count = count+INT{1};
				}
			}
			if (i >= count) {
				throw PresuppositionFailure();
			}
			return tIV([ithTrueElement](t_e e) {
				return e == ithTrueElement;
			});
		};

	template < typename tIV, typename INT >
	auto cardinality =
		+[](tIV m, t_context c) -> INT {
			int count = 0;
			for (const auto& x : c) {
				if (m(x)) {
					count++;
				}
			}
			return count;
		};

	template < typename INT, typename BOOL >
	auto intEq =
		+[](INT i1, INT i2) -> BOOL {
			return i1 == i2;
		};

	template < typename INT, typename BOOL >
	auto intGt = 
		+[](INT i1, INT i2) -> BOOL {
			return i1 > i2;
		};
	
	template < typename BOOL >
	auto not_ = 
		+[](BOOL b) -> BOOL {
			return !b;
		};

	template < typename BOOL >
	auto and_ = 
		+[](BOOL b1, BOOL b2) -> BOOL {
			return b1 && b2;
		};

	template < typename BOOL >
	auto or_ = 
		+[](BOOL b1, BOOL b2) -> BOOL {
			return b1 || b2;
		};

	template < typename INT >
	auto plus = 
		+[](INT i1, INT i2) -> INT {
			return INT(i1 + i2);
		};

	template < typename INT >
	auto minus = 
		+[](INT i1, INT i2) -> INT {
			return INT(i1 - i2);
		};

	// Apply one meaning to another
	auto applyIVtoQ =
		+[](t_Q m1, t_IV m2) -> t_DP {
			// returns a t_DP
			return m1(m2);
		};

	auto applyIVtoQ_w =
		+[](t_Q_w m1, t_IV_w m2) -> t_DP_w {
			return m1(m2);
		};

	auto applyIVtoDP =
		+[](t_DP m1, t_IV m2) -> t_t {
			return m1(m2);
		};

	auto applyIVtoDP_w =
		+[](t_DP_w m1, t_IV_w m2) -> t_t_w {
			return m1(m2);
		};

	// The input components
	
	auto q_ =
		+[](t_grammar_input i) -> t_Q  {
			return std::get<0>(i);
		};

	auto l_Q =
		+[](t_grammar_input i) -> t_IV  {
			return std::get<1>(i);
		};

	auto r_Q =
		+[](t_grammar_input i) -> t_IV  {
			return std::get<2>(i);
		};

	auto c_ =
		+[](t_grammar_input i) -> t_context  {
			return std::get<3>(i);
		};

	auto l_ =
		+[](t_grammar_input i) -> t_IV_w {
			return std::get<4>(i);
		};

	auto r_ =
		+[](t_grammar_input i) -> t_IV_w {
			return std::get<5>(i);
		};

	/////// NOT IN VAN DE POL ///////

	/* 	auto complement = */ 
	/* 		+[](t_IV m) -> t_IV { */
	/* 			return t_IV([m](t_e e) -> t_t {return !m(e);}); */
	/* 		}; */

	/* 	auto include = */
	/* 		+[](t_IV m1, t_IV m2) -> t_t { */
	/* 			bool out = true; */
	/* 			for (auto e : c) { */
	/* 				if (m1(e) && !m2(e)) { */
	/* 					out = false; */
	/* 					break; */
	/* 				} */
	/* 			} */
	/* 			return out; */
	/* 		}; */

}

class QuantsGrammar : public Grammar< 
		t_grammar_input,
		t_grammar_output,
		
		t_grammar_input,
		t_grammar_output,
		t_context,

		t_t,
		t_IV,
		t_int,
		t_Q,
		t_DP,

		t_t_w,
		t_IV_w,
		t_int_w,
		t_Q_w,
		t_DP_w
	>, public Singleton<QuantsGrammar> {

	using Super = Grammar<
		t_grammar_input,
		t_grammar_output,
		
		t_grammar_input,
		t_grammar_output,
		t_context,

		t_t,
		t_IV,
		t_int,
		t_Q,
		t_DP,

		t_t_w,
		t_IV_w,
		t_int_w,
		t_Q_w,
		t_DP_w
	>;
	using Super::Super;

public:

	QuantsGrammar() {

		using namespace Quants_DSL;

		add("%s | %s | %s | %s"		, makeGrammarOutput);

		// NOTE: The weights need to be calibrated so that 
		// sampled sentences have a reasonable length.
		// In practice this means upweighting non-type-recursive rules.

		// The concepts to define the bit of the composition function
		// that interprets nodes [Q IV]

		// -> DP
		add("( %s %s )"				, applyIVtoQ				, 0.1);

		// -> IV
		add("( union %s %s )"		, union_<t_IV>				, 0.1);
		add("( intersection %s %s )", intersection<t_IV>		, 0.1);
		add("( setminus %s %s )"	, setminus<t_IV>			, 0.1);
		add("( nTh %s %s %s )"		, nTh<t_IV,t_int>			, 0.1);

		// -> bool
		add("( %s %s )"				, applyIVtoDP				, 1);
		add("( intEq %s %s )" 		, intEq<t_int,t_t>			, 0.5);
		add("( intGt %s %s )"		, intGt<t_int,t_t>			, 0.5);
		/* add_terminal("true"		, true						, 0.1); */
		/* add_terminal("false"		, false						, 0.1); */
		add("( not %s )"			, not_<t_t>					, 0.1);
		add("( and %s %s )"			, and_<t_t>					, 0.1);
		add("( or %s %s )"			, or_<t_t>					, 0.1);

		// -> int
		add("( cardinality %s %s )"	, cardinality<t_IV,t_int>	, 0.5);
		add_terminal("0"			, 0							, 0.5);
		add_terminal("1"			, 1							, 0.5);
		add("( + %s %s )"			, plus<t_int>				, 0.1);
		add("( - %s %s )"			, minus<t_int>				, 0.1);

		// The wrappers to define the quantifiers
		// so that the first two arguments of the input do not
		// appear in their definition
		
		// -> DP
		// Don't need this for defining quantifiers
		/* add("( %s %s )"			, applyIVtoQ_w				, 0.1); */

		// -> IV
		add("( union %s %s )"		, union_<t_IV_w>				, 0.1);
		add("( intersection %s %s )", intersection<t_IV_w>			, 0.1);
		add("( setminus %s %s )"	, setminus<t_IV_w>				, 0.1);
		add("( nTh %s %s %s )"		, nTh<t_IV_w,t_int_w>			, 0.1);
		
		// -> bool
		/* add("( %s %s )"			, applyIVtoDP_w					, 1); */
		add("( intEq %s %s )" 		, intEq<t_int_w,t_t_w>			, 0.5);
		add("( intGt %s %s )"		, intGt<t_int_w,t_t_w>			, 0.5);
		// Don't need this for defining quantifiers
		/* add_terminal("true"		, t_t_w{true}				, 0.1); */
		/* add_terminal("false"		, t_t_w{false}				, 0.1); */
		add("( not %s )"			, not_<t_t_w>					, 0.1);
		add("( and %s %s )"			, and_<t_t_w>					, 0.1);
		add("( or %s %s )"			, or_<t_t_w>					, 0.1);

		// -> int
		add("( cardinality %s %s )"	, cardinality<t_IV_w,t_int_w>	, 0.5);
		add_terminal("0"			, t_int_w(0)					, 0.1);
		add_terminal("1"			, t_int_w(1)					, 0.1);
		add("( + %s %s )"			, plus<t_int_w>					, 0.1);
		add("( - %s %s )"			, minus<t_int_w>				, 0.1);

		// The inputs
		add("X"						, Builtins::X<QuantsGrammar>, 5);
		add("%s.Q"					, q_						, 1);
		add("%s.c"					, c_						, 1);
		add("%s.L"					, l_Q						, 1);
		add("%s.R"					, r_Q						, 1);
		add("%s.L"					, l_						, 1);
		add("%s.R"					, r_						, 1);

		/* for(int i=1;i<=10;i++) { */
		/* 	add_terminal( */
		/* 		"#"+str(i), */
		/* 		i, */
		/* 		0.05 */
		/* 	); */
		/* 	add_terminal( */
		/* 		"#"+str(i), */
		/* 		t_int_w(i), */
		/* 		0.05 */
		/* 	); */
		/* } */

	}
} grammar;

// The idea here is to run a tradeoff analysis of communicative
// accuracy and simplicity of the language. 
// This is done by using Fleet to find languages that are simple
// with a likelihood function that is just the weighted communicative accuracy.
// This simulation runs the analysis for a single likelihood weight.
// Keep meaningful t_datum and t_data even if they are not used,
// because they can be used in e.g., communicativeAccuracy to manage the
// produced observations.

class QuantsHypothesis : public DeterministicLOTHypothesis<
		QuantsHypothesis,
		t_grammar_input,
		t_grammar_output,
		QuantsGrammar,
		&grammar,
		t_datum > {

private:
	// Initialize before setting them below
	
	// Number of observations
	static inline size_t nObs = 0;
	// Context size
	static inline size_t cSize = 0;
	// Weight of communicative accuracy in tradeoff
	static inline double likelihoodWeight = 0.0;
	// Initialize random number generator anew 
	static inline std::mt19937 local_rng = std::mt19937(std::random_device{}());
	// Maximum depth of signals when enumerating utterances
	// to estimate communicative accuracy
	static inline size_t searchDepth = 2;
	// For storing
	static inline data_t commData;

public:
	using Super = DeterministicLOTHypothesis<
		QuantsHypothesis,
		t_grammar_input,
		t_grammar_output,
		QuantsGrammar,
		&grammar,
		t_datum >;

	using Super::Super; 

	static void setParams(size_t nObs, 
						  size_t cSize,
						  double likelihoodWeight,
						  std::mt19937& local_rng,
						  size_t searchDepth) {

		QuantsHypothesis::nObs = nObs;
		QuantsHypothesis::cSize = cSize;
		QuantsHypothesis::likelihoodWeight = likelihoodWeight;
		QuantsHypothesis::local_rng = local_rng;
		QuantsHypothesis::searchDepth = searchDepth;
	}

	QuantsHypothesis() : Super () {
		// Maximum depth of the hypotheses
		grammar.GRAMMAR_MAX_DEPTH = 50;
	}

	data_t getCommData() {
		return commData;
	}

	double compute_likelihood(const data_t& x,
							  const double breakout=-infinity) override {
		// NOTE: Here I disregard the data,
		// since the likelihood only depends on communicative accuracy
		// which I calculate inside this function.
		
		// print parameters
		std::cout << "Setting parameters:" << std::endl;
		std::cout << "nObs: " << nObs << ", " << this->nObs << std::endl;
		std::cout << "cSize: " << cSize << ", " << this->cSize << std::endl; 
		std::cout << "lw: " << likelihoodWeight << ", " << this->likelihoodWeight << std::endl;
		std::cout << "searchDepth: " << searchDepth << ", " << this->searchDepth << std::endl;
		std::cout << std::endl;

		// Agent to calculate communicative accuracy with.
		Agent<QuantsHypothesis> agent{};

		// set the hypothesis of the agent to the current hypothesis
		agent.setHypothesis(*this);

		std::vector<t_context> cs = generateContexts(cSize, nObs, local_rng);

		// produce data for approximating communicative accuracy
		commData = agent.produceDataFromEnumeration(cs, local_rng, searchDepth);

		// the new agent computes its communicative accuracy
		double commAcc = agent.communicativeAccuracy(commData, local_rng);

		// The likelihood is the weighted sum of the communicative accuracy
		// and the simplicity of the language.
		// Note that commAcc is already the log of a probability
		double loglik = likelihoodWeight * commAcc;

		// print value
		std::cout << "Log likelihood: " << loglik << std::endl;
		std::cout << "Value: " << this->value << std::endl;
		std::cout << std::endl;

		return loglik;
	}

	// Extracts the component of a sentence from the LOT 
	// that encodes the composition function.
	t_BTC_compose getCompositionF() {
		// Return a composition function that:
		// 1. If the types are t_Q_M and t_IV_M, then calls the relevant
		//   hypothesis to compose the meanings.
		// 2. Otherwise, applies the first meaning to the second.
		return [this](t_meaning a, t_meaning b) -> t_meaning {
			return std::visit(
				[this](auto&& f, auto&&arg) -> t_meaning {
					using T = std::decay_t<decltype(f)>;
					using U = std::decay_t<decltype(arg)>;
					// if T is type t_Q_M and U is type t_IV_M
					// then use the inferred hypothesis
					// to compose the meanings
					if constexpr (std::is_same_v<T,t_Q_M> && 
								  std::is_same_v<U,t_IV_M>) {
						t_meaning dpM = t_DP_M(
							[this,f,arg](t_context c) -> t_DP {
								t_DP dp = [this,f,arg,c](t_IV iv) -> t_t{
									try {
										// call the relevant hypothesis
										auto x = call(std::make_tuple(
											f(c),
											// left argument to Q
											arg(c),
											// right argument to Q
											iv,
											c,
											// Unused.
											// If they are initialized empty 
											// they throw a bad_function_call,
											// because they're called in hyp,
											// so use standins instead.
											// TODO: Find better way!
											/* t_IV_w{}, */
											t_IV_w(
												[](t_e e) -> t_t {
													return true;
												}
											),
											/* t_IV_w{} */
											t_IV_w(
												[](t_e e) -> t_t {
													return true;
												}
											)
										));
										// get the relevant part of
										// the output
										t_t tv = std::get<0>(x);
										return tv;
									// catch bad_function_call
									} catch (std::bad_function_call& e) {
										std::cout << "here 1" << std::endl;
										std::terminate();
									}
								};
								return dp;
							}
						);
						return dpM;
					} else if constexpr (accepts_arg_v<T,U>) {
						// Otherwise apply f to arg
						return [f,arg](t_context c) -> auto {
							return f(c)(arg(c));
						};
					} else {
						// They don't match
						return t_meaning(Empty_M());
					}
				},
				a, b
			);
		};
	}

	// Returns a t_meaning containing type t_Q_M.
	// Effectively, this is taking the component of a grammar's sentence
	// that deals with defining a quantifier
	// (which of the quantifiers is specified by i)
	template< int i >
	auto q_n () {
		return t_meaning([this](t_context c) -> t_Q {
			return [c,this](t_IV x) -> t_DP {
				return [x,c,this](t_IV y) -> t_t {
					// First two not used, so default initialize
					try {
						auto tup = std::make_tuple(
							// unused
							/* t_Q{}, */ 
							[](t_IV m1) -> t_DP {
								return [](t_IV m2) -> t_t {
									return true;
								};
							},
							/* t_IV{}, */
							[](t_e e) -> t_t {
								return true;
							},
							/* t_IV{}, */
							[](t_e e) -> t_t {
								return true;
							},
							// used
							c,
							t_IV_w{x},
							t_IV_w{y}
						);
						
						// next line is causing segmentation fault
						// when the hypothesis is called
						/* std::cout << "here 0" << std::endl; */
						/* std::cout << std::get<0>(tup) << std::endl; */
						/* std::cout << std::get<1>(tup) << std::endl; */
						/* std::cout << this << std::endl; */
						/* std::cout << std::endl; */

						// the relevant bit of the hypothesis
						// takes a context and two IVs and returns
						// a t_t
			 			auto o = this->call(tup);
						// get the unwrapped t_Q_M value
						return std::get<i>(o).i;
					} catch (std::bad_function_call& e) {
						std::cout << "here 2" << std::endl;
						std::terminate();
					}
				};
			};
		});
	}

	LexicalSemantics getLexicon() {
		// the booleans are specifying which groups of words to include 
		// in the lexicon, by their type.
		LexicalSemantics lexSem{
			true,
			true,
			true,
			true,
			false
		};
		lexSem.add("Q1", q_n<1>());
		lexSem.add("Q2", q_n<2>());
		lexSem.add("Q3", q_n<3>());
		return lexSem;
	}
	
};

