# pragma once

// Here I extent the typing system slightly
// to include integers as a basic type
// to match the definitions in Van de Pol et al 2023.
using t_int = int;

// A bare wrapper function around a meaning type
// so Fleet can distinguish between the operations
// involved in the composition function and those involved
// in the quantifiers definitions (since the arguments
// that need to appear in the two are different)
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

using t_grammar_input = std::tuple<
	// Arguments to compfunction for node [Q IV]
	t_Q,
	t_IV,
	t_IV,
	// context is used for both inferred components
	t_context,
	// arguments to quantifier meaning
	// (these should not appear in the two components above
	// and therefore everything is defined with wrappers)
	t_IV_w,
	t_IV_w
>;
// Type of output of the Grammar sentences
using t_grammar_output = std::tuple<
	// the meaning of the node [[Q IV] IV]
	// as a function of the input meanings
	t_t,
	// three quantifier's outputs 
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

	t_grammar_output makeGrammarOutput(
		t_t tQ,
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

// Define CompHypothesis

class QuantsHypothesis final : public DeterministicLOTHypothesis<
		QuantsHypothesis,
		t_grammar_input,
		t_grammar_output,
		QuantsGrammar,
		&grammar,
		t_datum
	> {

public:
	using Super = DeterministicLOTHypothesis<
		QuantsHypothesis,
		t_grammar_input,
		t_grammar_output,
		QuantsGrammar,
		&grammar,
		t_datum
		>;
	using Super::Super; 

	QuantsHypothesis() : Super () {
		// Maximum depth of the hypotheses
		grammar.GRAMMAR_MAX_DEPTH = 50;
	}


	t_BTC_compose getCompositionF() {
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
										auto x = call(std::make_tuple(
											f(c),
											// left argument to Q
											arg(c),
											// right argument to Q
											iv,
											c,
											// unused.
											// If they are empty initialized
											// they throw a bad_function_call,
											// because it's called inside hyp
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
										t_t tv = std::get<0>(x);
										return tv;
									// catch bad_function_call
									} catch (std::bad_function_call& e) {
										std::cout << "here 1" << std::endl;
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

	// Returns a t_meaning of type t_Q_M
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
						// the relevant bit of the hypothesis
						// takes a context and two IVs and returns
						// a t_t
						auto o = this->call(tup);
						// get the unwrapped t_Q_M value
						return std::get<i>(o).i;
					} catch (std::bad_function_call& e) {
						std::cout << "here 2" << std::endl;
					}
				};
			};
		});
	}

	LexicalSemantics getLexicon() {
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

	double compute_single_likelihood(const datum_t& x) override {
		
		// context is the first element of the input
		t_context context = x.input;

		t_BTC_compose compF = getCompositionF();
		LexicalSemantics lexSem = getLexicon();

		// output: a SExpr representation of the parseTree 
		const std::unique_ptr<BTC> parseTree = 
			BTC::fromSExpression(x.output,lexSem);

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

	static data_t dataFilter(data_t data) {
		auto substrings = std::vector<std::string>{"Q1","Q2","Q3"};
		data_t filteredData;
		for (auto& d : data) {
			for (const auto& substring : substrings) {
				if (d.output.find(substring) != std::string::npos) {
					filteredData.push_back(d);
					// break out of the inner loop
					break;
				}
			}
		}

		return filteredData;
	}
};


