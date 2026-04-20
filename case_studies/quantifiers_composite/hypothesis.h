#pragma once

#include "../_shared/int_lexicon.h"

#ifndef NUM_QUANTS
#define NUM_QUANTS 3
#endif

static constexpr size_t num_quants = NUM_QUANTS;

// ============================================================
// Input/output types for the two grammars
// ============================================================

// Composition grammar: how [Q restrictor] combines with scope
using t_comp_input = std::tuple<t_Q, t_IV, t_IV, t_context>;

// Quantifier grammar: each quantifier takes restrictor, scope, context
using t_quant_input = std::tuple<t_IV, t_IV, t_context>;

// Datum types for the inner hypotheses (required by Fleet, not used for likelihood)
using t_comp_datum  = defaultdatum_t<t_comp_input, t_t>;
using t_quant_datum = defaultdatum_t<t_quant_input, t_t>;

// ============================================================
// Type compatibility for standard function application
// ============================================================

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template <typename T, typename U> struct accepts_arg : std::false_type {};
template <> struct accepts_arg<t_UC_M,  t_t_M>  : std::true_type {};
template <> struct accepts_arg<t_BC_M,  t_t_M>  : std::true_type {};
template <> struct accepts_arg<t_TC_M,  t_t_M>  : std::true_type {};
template <> struct accepts_arg<t_IV_M,  t_e_M>  : std::true_type {};
template <> struct accepts_arg<t_DP_M,  t_IV_M> : std::true_type {};
template <> struct accepts_arg<t_TV_M,  t_e_M>  : std::true_type {};
template <> struct accepts_arg<t_PM_M,  t_IV_M> : std::true_type {};
template <> struct accepts_arg<t_PMM_M, t_IV_M> : std::true_type {};
template <> struct accepts_arg<t_Q_M,   t_IV_M> : std::true_type {};
template <typename T, typename U>
inline constexpr bool accepts_arg_v = accepts_arg<T, U>::value;

// ============================================================
// DSL: operations shared by both grammars
// ============================================================

namespace SharedDSL {

	auto union_ =
		+[](t_IV m1, t_IV m2) -> t_IV {
			return [m1, m2](t_e e) -> t_t { return m1(e) || m2(e); };
		};

	auto intersection =
		+[](t_IV m1, t_IV m2) -> t_IV {
			return [m1, m2](t_e e) -> t_t { return m1(e) && m2(e); };
		};

	auto setminus =
		+[](t_IV m1, t_IV m2) -> t_IV {
			return [m1, m2](t_e e) -> t_t { return m1(e) && !m2(e); };
		};

	auto universe =
		+[](t_context c) -> t_IV {
			return [c](t_e e) -> t_t {
				for (const auto& x : c) { if (x == e) return true; }
				return false;
			};
		};

	auto not_ = +[](t_t b) -> t_t { return !b; };
	auto and_ = +[](t_t b1, t_t b2) -> t_t { return b1 && b2; };
	auto or_  = +[](t_t b1, t_t b2) -> t_t { return b1 || b2; };
}

// ============================================================
// DSL: composition-specific operations
// ============================================================

namespace CompDSL {
	auto q_ = +[](t_comp_input i) -> t_Q       { return std::get<0>(i); };
	auto l_ = +[](t_comp_input i) -> t_IV      { return std::get<1>(i); };
	auto r_ = +[](t_comp_input i) -> t_IV      { return std::get<2>(i); };
	auto c_ = +[](t_comp_input i) -> t_context  { return std::get<3>(i); };

	auto applyIVtoQ  = +[](t_Q q,  t_IV iv) -> t_DP { return q(iv); };
	auto applyIVtoDP = +[](t_DP dp, t_IV iv) -> t_t  { return dp(iv); };
}

// ============================================================
// DSL: quantifier-specific operations
// ============================================================

namespace QuantDSL {
	auto l_ = +[](t_quant_input i) -> t_IV      { return std::get<0>(i); };
	auto r_ = +[](t_quant_input i) -> t_IV      { return std::get<1>(i); };
	auto c_ = +[](t_quant_input i) -> t_context  { return std::get<2>(i); };

	auto cardinality =
		+[](t_IV m, t_context c) -> int {
			int count = 0;
			for (const auto& x : c) { if (m(x)) count++; }
			return count;
		};

	auto intEq = +[](int i1, int i2) -> t_t { return i1 == i2; };
	auto intGt = +[](int i1, int i2) -> t_t { return i1 > i2; };
	auto plus  = +[](int i1, int i2) -> int { return i1 + i2; };
	auto minus = +[](int i1, int i2) -> int { return i1 - i2; };
}

// ============================================================
// CompGrammar
// ============================================================

class CompGrammar : public Grammar<
		t_comp_input, t_t,
		t_comp_input, t_t, t_Q, t_IV, t_DP, t_context
	>, public Singleton<CompGrammar> {

	using Super = Grammar<
		t_comp_input, t_t,
		t_comp_input, t_t, t_Q, t_IV, t_DP, t_context>;
	using Super::Super;

public:
	CompGrammar() {
		using namespace CompDSL;
		using namespace SharedDSL;

		add("X",                      Builtins::X<CompGrammar>, 1);

		add("%s.Q",                   q_,            1);
		add("%s.L",                   l_,           10);
		add("%s.R",                   r_,           10);
		add("%s.c",                   c_,            1);

		add("( %s %s )",              applyIVtoQ,    1);
		add("( %s %s )",              applyIVtoDP,  10);

		add("( universe %s )",        universe,     10);
		add("( union %s %s )",        union_,        1);
		add("( intersection %s %s )", intersection,  1);
		add("( setminus %s %s )",     setminus,      1);

		add("( not %s )",             not_,          1);
		add("( and %s %s )",          and_,          1);
		add("( or %s %s )",           or_,           1);
	}
} comp_grammar;

// ============================================================
// QuantGrammar
// ============================================================

class QuantGrammar : public Grammar<
		t_quant_input, t_t,
		t_quant_input, t_t, t_IV, int, t_context
	>, public Singleton<QuantGrammar> {

	using Super = Grammar<
		t_quant_input, t_t,
		t_quant_input, t_t, t_IV, int, t_context>;
	using Super::Super;

public:
	QuantGrammar() {
		using namespace QuantDSL;
		using namespace SharedDSL;

		add("X",                      Builtins::X<QuantGrammar>, 1);

		add("%s.L",                   l_,           10);
		add("%s.R",                   r_,           10);
		add("%s.c",                   c_,            1);

		add("( universe %s )",        universe,     10);
		add("( union %s %s )",        union_,        1);
		add("( intersection %s %s )", intersection,  1);
		add("( setminus %s %s )",     setminus,      1);

		add("( cardinality %s %s )",  cardinality,  10);
		add_terminal("0",             0,            10);
		add_terminal("1",             1,            10);
		add("( + %s %s )",            plus,          1);
		add("( - %s %s )",            minus,         1);

		add("( intEq %s %s )",        intEq,        10);
		add("( intGt %s %s )",        intGt,        10);

		add("( not %s )",             not_,          1);
		add("( and %s %s )",          and_,          1);
		add("( or %s %s )",           or_,           1);
	}
} quant_grammar;

// Alias so Main.cpp TESTGRAMMAR case compiles
[[maybe_unused]] static QuantGrammar& grammar = quant_grammar;

// ============================================================
// Inner hypothesis types
// ============================================================

class InnerCompHyp : public DeterministicLOTHypothesis<
		InnerCompHyp, t_comp_input, t_t,
		CompGrammar, &comp_grammar, t_comp_datum> {
	using Super = DeterministicLOTHypothesis<
		InnerCompHyp, t_comp_input, t_t,
		CompGrammar, &comp_grammar, t_comp_datum>;
public:
	using Super::Super;
};

class InnerQuantHyp : public DeterministicLOTHypothesis<
		InnerQuantHyp, t_quant_input, t_t,
		QuantGrammar, &quant_grammar, t_quant_datum> {
	using Super = DeterministicLOTHypothesis<
		InnerQuantHyp, t_quant_input, t_t,
		QuantGrammar, &quant_grammar, t_quant_datum>;
public:
	using Super::Super;
};

// ============================================================
// QuantsHypothesis — composite hypothesis
//
// Holds one composition program (CompGrammar) and N quantifier
// programs (QuantGrammar). MCMC proposes to one component at a
// time, exploring the joint space of composition functions and
// quantifier meanings without wrapper types.
// ============================================================

class QuantsHypothesis : public MCMCable<QuantsHypothesis, t_datum> {

private:

	mutable InnerCompHyp comp;
	mutable std::vector<InnerQuantHyp> quants;

	// ---- Static parameters (shared, set once before MCMC) ----
	static inline size_t nObs = 0;
	static inline size_t cSize = 0;
	static inline double likelihoodWeight = 0.0;
	static inline thread_local std::mt19937 local_rng{std::random_device{}()};
	static inline size_t searchDepth = 2;
	static inline double pTarget = 0.5;
	static inline bool pragmatic = false;
	static inline bool exclude_empty_qs = false;

	// Communication data produced during likelihood evaluation (for logging)
	data_t commData;

	// Lexicon configuration
	bool add_es   = true;
	bool add_BFs  = false;
	bool add_IVs  = true;
	bool add_TVs  = true;
	bool add_DPs  = false;
	bool add_PMs  = true;
	bool add_PMMs = true;
	bool add_Qs   = false;

public:

	using Super = MCMCable<QuantsHypothesis, t_datum>;

	// ---- Static parameter setup ----

	static void setParams(const size_t nObs,
	                      const size_t cSize,
	                      const double likelihoodWeight,
	                      std::mt19937& rng,
	                      const size_t searchDepth,
	                      const double pTarget,
	                      const bool pragmatic,
	                      const bool exclude_empty) {
		QuantsHypothesis::nObs = nObs;
		QuantsHypothesis::cSize = cSize;
		QuantsHypothesis::likelihoodWeight = likelihoodWeight;
		QuantsHypothesis::local_rng = rng;
		QuantsHypothesis::searchDepth = searchDepth;
		QuantsHypothesis::pTarget = pTarget;
		QuantsHypothesis::pragmatic = pragmatic;
		QuantsHypothesis::exclude_empty_qs = exclude_empty;
	}

	// ---- Constructors ----

	QuantsHypothesis() : quants(num_quants) {
		comp_grammar.GRAMMAR_MAX_DEPTH = 50;
		quant_grammar.GRAMMAR_MAX_DEPTH = 50;
	}

	// Construct from serialized string: comp_ser|||q1_ser|||q2_ser|||...
	QuantsHypothesis(const std::string& parseable) : QuantsHypothesis() {
		const std::string delim = "|||";
		std::vector<std::string> parts;
		size_t start = 0, pos;
		while ((pos = parseable.find(delim, start)) != std::string::npos) {
			parts.push_back(parseable.substr(start, pos - start));
			start = pos + delim.size();
		}
		parts.push_back(parseable.substr(start));

		if (parts.size() != num_quants + 1) {
			throw std::runtime_error(
				"QuantsHypothesis: expected " + std::to_string(num_quants + 1)
				+ " parts separated by |||, got " + std::to_string(parts.size()));
		}
		comp = InnerCompHyp::deserialize(parts[0]);
		for (size_t i = 0; i < num_quants; i++) {
			quants[i] = InnerQuantHyp::deserialize(parts[i + 1]);
		}
	}

	// ---- Data access ----

	data_t getCommData() const { return commData; }

	// ---- Composition function ----
	// For Q+IV: delegates to the comp program.
	// For all other type combinations: standard function application.

	t_BTC_compose getCompositionF() const {
		return [this](t_meaning a, t_meaning b) -> t_meaning {
			return std::visit(
				[this](auto&& f, auto&& arg) -> t_meaning {
					using T = std::decay_t<decltype(f)>;
					using U = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, t_Q_M> &&
					              std::is_same_v<U, t_IV_M>) {
						return t_DP_M(
							[this, f, arg](t_context c) -> t_DP {
								return [this, f, arg, c](t_IV scope) -> t_t {
									return comp.call(
										std::make_tuple(f(c), arg(c), scope, c)
									);
								};
							}
						);
					} else if constexpr (accepts_arg_v<T, U>) {
						return [f, arg](t_context c) -> auto {
							return f(c)(arg(c));
						};
					} else {
						return t_meaning(Empty_M());
					}
				},
				a, b
			);
		};
	}

	// ---- Quantifier meanings ----
	// Each quantifier is a separate program: (restrictor, scope, context) → bool

	t_meaning q_n(size_t i) const {
		return t_meaning([this, i](t_context c) -> t_Q {
			return [this, i, c](t_IV restrictor) -> t_DP {
				return [this, i, c, restrictor](t_IV scope) -> t_t {
					return quants[i].call(
						std::make_tuple(restrictor, scope, c)
					);
				};
			};
		});
	}

	// ---- Lexicon ----

	LexicalSemantics getLexicon() const {
		LexicalSemantics lex{
			cSize, add_es, add_BFs, add_IVs, add_TVs,
			add_DPs, add_PMs, add_PMMs, add_Qs
		};
		// Int-specific predicates (even, prime, gt, equal) —
		// content-agnostic language.h no longer provides them.
		addIntLexicon(lex);
		for (size_t i = 0; i < num_quants; i++) {
			lex.add("Q" + std::to_string(i + 1), q_n(i));
		}
		return lex;
	}

	// ============================================================
	// MCMCable interface
	// ============================================================

	[[nodiscard]] static QuantsHypothesis sample() {
		QuantsHypothesis h;
		h.comp = InnerCompHyp::sample();
		for (size_t i = 0; i < num_quants; i++) {
			h.quants[i] = InnerQuantHyp::sample();
		}
		return h;
	}

	[[nodiscard]] std::optional<std::pair<QuantsHypothesis, double>>
	propose() const override {
		// Pick one component uniformly: 0 = comp, 1..N = quants
		static thread_local std::mt19937 propose_rng{std::random_device{}()};
		std::uniform_int_distribution<size_t> dist(0, num_quants);
		size_t idx = dist(propose_rng);

		QuantsHypothesis proposed = *this;
		double fb = 0.0;

		if (idx == 0) {
			auto p = comp.propose();
			if (!p) return {};
			proposed.comp = std::move(p->first);
			fb = p->second;
		} else {
			auto p = quants[idx - 1].propose();
			if (!p) return {};
			proposed.quants[idx - 1] = std::move(p->first);
			fb = p->second;
		}

		return std::make_pair(std::move(proposed), fb);
	}

	[[nodiscard]] QuantsHypothesis restart() const override {
		QuantsHypothesis h;
		h.comp = comp.restart();
		for (size_t i = 0; i < num_quants; i++) {
			h.quants[i] = quants[i].restart();
		}
		return h;
	}

	bool operator==(const QuantsHypothesis& other) const override {
		if (!(comp == other.comp)) return false;
		for (size_t i = 0; i < num_quants; i++) {
			if (!(quants[i] == other.quants[i])) return false;
		}
		return true;
	}

	// ============================================================
	// Bayesable interface
	// ============================================================

	size_t hash() const override {
		size_t h = comp.hash();
		for (const auto& q : quants) {
			h ^= q.hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
		}
		return h;
	}

	std::string string(std::string prefix = "") const override {
		auto strip_lambda = [](const std::string& s) -> std::string {
			const std::string lam = "\xCE\xBBx.";  // "λx." in UTF-8
			if (s.compare(0, lam.size(), lam) == 0)
				return s.substr(lam.size());
			return s;
		};
		// comp keeps its λx. prefix; quants have it stripped
		std::string result = comp.string();
		for (const auto& q : quants) {
			result += " | " + strip_lambda(q.string());
		}
		return prefix + result;
	}

	std::string serialize() const {
		std::string result = comp.serialize();
		for (const auto& q : quants) {
			result += "|||" + q.serialize();
		}
		return result;
	}

	double compute_prior() override {
		this->prior = comp.compute_prior();
		for (auto& q : quants) {
			this->prior += q.compute_prior();
		}
		return this->prior;
	}

	double compute_likelihood(const data_t& x,
	                          const double breakout = -infinity) override {

		if (exclude_empty_qs) {
			for (const auto& q : quants) {
				auto s = q.string();
				if (s.find("X.L") == std::string::npos &&
				    s.find("X.R") == std::string::npos) {
					this->likelihood = -std::numeric_limits<double>::infinity();
					return this->likelihood;
				}
			}
		}

		Agent<QuantsHypothesis> agent{*this};

		std::vector<t_context> cs = generateContexts(
			cSize, nObs, local_rng, pTarget);

		auto data = agent.produceDataFromEnumeration(
			cs, local_rng, searchDepth, pragmatic);

		commData = std::get<0>(data);
		auto utilities = std::get<1>(data);

		double commAcc = 0;
		for (size_t i = 0; i < utilities.size(); i++) {
			commAcc += utilities[i];
		}
		commAcc /= nObs;

		this->likelihood = likelihoodWeight * commAcc;

		std::cout << "Hypothesis: " << this->string() << std::endl;
		std::cout << "Communicative accuracy: " << commAcc << std::endl;
		std::cout << "Log likelihood: " << this->likelihood << std::endl;
		std::cout << std::endl;

		return this->likelihood;
	}
};

// Common aliases so Main.cpp only needs to change the #include
using ActiveHypothesis = QuantsHypothesis;
static constexpr size_t num_learned_items = num_quants;
