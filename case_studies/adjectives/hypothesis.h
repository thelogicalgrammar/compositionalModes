#pragma once

#ifndef NUM_ADJS
#define NUM_ADJS 3   // override with: -DNUM_ADJS=4 (etc.)
#endif

static constexpr size_t num_adjs = NUM_ADJS;

// ============================================================
// Gradable Adjectives case study (with co-learned POS)
//
// Entities have NUM_DIMS float-valued dimensions (see config.h).
// Each adjective selects and combines dimensions into a single
// degree; a shared POS morpheme maps that degree to truth.
//
// Each adjective LoT program: (entity, context) → float
//   Produces a DEGREE along the adjective's chosen scale.
//   DSL: dim(d, x), per-dim context aggregates, arithmetic.
//
// Shared composition function: (measure_fn, entity, context) → bool
//   Defines the STANDARD (how degrees map to truth values).
//   Has access to the adjective function itself, so it can
//   compute adjective-relative context statistics (e.g.,
//   "above the mean of this adjective across the context").
//   DSL: applyAdj, adjective aggregates, comparisons, etc.
//
// The resulting t_IV_M is used uniformly in all syntactic
// positions (restrictor, scope, with PMs, etc.) via FA.
// ============================================================

// ============================================================
// Types
// ============================================================

// Dimension index (keeping it distinct from other ints keeps the
// grammar from mixing dimension indices with arithmetic operands).
using t_dim = size_t;

// Measure function: entity to float (the adjective's degree output)
using t_measure = std::function<float(t_e)>;

// Adjective grammar: (entity, context) to float
using t_adj_input = std::tuple<t_e, t_context>;
using t_adj_datum = defaultdatum_t<t_adj_input, float>;

// Composition grammar (POS): (measure_fn, entity, context) to bool
using t_adj_comp_input = std::tuple<t_measure, t_e, t_context>;
using t_adj_comp_datum = defaultdatum_t<t_adj_comp_input, t_t>;

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
// DSL: adjective measure operations
// ============================================================

namespace AdjMeasureDSL {

	// Input accessors
	auto x_ = +[](t_adj_input i) -> t_e       { return std::get<0>(i); };
	auto c_ = +[](t_adj_input i) -> t_context { return std::get<1>(i); };

	// Pick one dimension of an entity's content vector.
	// Grammar-produced d is always in [0, num_dims), but we clamp
	// for safety.
	auto dim = +[](t_dim d, t_e e) -> float {
		return content(e)[d < num_dims ? d : 0];
	};

	// Context aggregates over a specified dimension
	auto maxDim = +[](t_dim d, t_context c) -> float {
		size_t di = d < num_dims ? d : 0;
		float m = std::numeric_limits<float>::lowest();
		for (const auto& e : c) m = std::max(m, content(e)[di]);
		return m;
	};
	auto minDim = +[](t_dim d, t_context c) -> float {
		size_t di = d < num_dims ? d : 0;
		float m = std::numeric_limits<float>::max();
		for (const auto& e : c) m = std::min(m, content(e)[di]);
		return m;
	};
	auto meanDim = +[](t_dim d, t_context c) -> float {
		size_t di = d < num_dims ? d : 0;
		float sum = 0.0f;
		for (const auto& e : c) sum += content(e)[di];
		return sum / (float)c.size();
	};

	// Arithmetic on floats
	auto plus  = +[](float a, float b) -> float { return a + b; };
	auto minus = +[](float a, float b) -> float { return a - b; };
	auto times = +[](float a, float b) -> float { return a * b; };
	auto neg   = +[](float a) -> float          { return -a; };
	auto absF  = +[](float a) -> float          { return a < 0 ? -a : a; };
	auto maxF  = +[](float a, float b) -> float { return a > b ? a : b; };
	auto minF  = +[](float a, float b) -> float { return a < b ? a : b; };
}

// ============================================================
// DSL: composition / POS operations
// ============================================================

namespace AdjCompDSL {

	// Input accessors
	auto adj_ = +[](t_adj_comp_input i) -> t_measure { return std::get<0>(i); };
	auto x_   = +[](t_adj_comp_input i) -> t_e       { return std::get<1>(i); };
	auto c_   = +[](t_adj_comp_input i) -> t_context { return std::get<2>(i); };

	// Apply the adjective measure to an entity
	auto applyAdj = +[](t_measure adj, t_e x) -> float { return adj(x); };

	// Adjective-relative context statistics
	auto maxAdj = +[](t_measure adj, t_context c) -> float {
		float m = std::numeric_limits<float>::lowest();
		for (const auto& e : c) m = std::max(m, adj(e));
		return m;
	};
	auto minAdj = +[](t_measure adj, t_context c) -> float {
		float m = std::numeric_limits<float>::max();
		for (const auto& e : c) m = std::min(m, adj(e));
		return m;
	};
	auto meanAdj = +[](t_measure adj, t_context c) -> float {
		float sum = 0.0f;
		for (const auto& e : c) sum += adj(e);
		return sum / (float)c.size();
	};

	// Float comparisons. Exact equality is OK here because the floats
	// being compared come from the same adj applied to the same
	// context — no rounding paths diverge.
	auto fltGt  = +[](float a, float b) -> t_t { return a >  b; };
	auto fltGte = +[](float a, float b) -> t_t { return a >= b; };
	auto fltEq  = +[](float a, float b) -> t_t { return a == b; };

	// Arithmetic on floats
	auto plus  = +[](float a, float b) -> float { return a + b; };
	auto minus = +[](float a, float b) -> float { return a - b; };
	auto times = +[](float a, float b) -> float { return a * b; };
	auto neg   = +[](float a) -> float          { return -a; };
	auto absF  = +[](float a) -> float          { return a < 0 ? -a : a; };
	auto maxF  = +[](float a, float b) -> float { return a > b ? a : b; };
	auto minF  = +[](float a, float b) -> float { return a < b ? a : b; };

	// Boolean
	auto not_ = +[](t_t b) -> t_t { return !b; };
	auto and_ = +[](t_t b1, t_t b2) -> t_t { return b1 && b2; };
	auto or_  = +[](t_t b1, t_t b2) -> t_t { return b1 || b2; };
}

// ============================================================
// AdjMeasureGrammar — what dimension the adjective measures
// ============================================================

class AdjMeasureGrammar : public Grammar<
		t_adj_input, float,
		t_adj_input, float, t_e, t_dim, t_context
	>, public Singleton<AdjMeasureGrammar> {

	using Super = Grammar<
		t_adj_input, float,
		t_adj_input, float, t_e, t_dim, t_context>;
	using Super::Super;

public:
	AdjMeasureGrammar() {
		using namespace AdjMeasureDSL;

		add("X",                      Builtins::X<AdjMeasureGrammar>, 1);

		// -> t_e
		add("%s.x",                   x_,            10);
		// -> t_context
		add("%s.c",                   c_,             1);

		// -> float (pick dimension d of entity)
		add("( dim %s %s )",          dim,            10);

		// -> float (per-dimension context aggregates)
		add("( maxDim %s %s )",       maxDim,          5);
		add("( minDim %s %s )",       minDim,          5);
		add("( meanDim %s %s )",      meanDim,         5);

		// -> float (arithmetic)
		add("( + %s %s )",            plus,            1);
		add("( - %s %s )",            minus,           1);
		add("( * %s %s )",            times,           1);
		add("( neg %s )",             neg,             1);
		add("( abs %s )",             absF,            1);
		add("( maxF %s %s )",         maxF,            1);
		add("( minF %s %s )",         minF,            1);

		// -> float (constants)
		add_terminal("0.0",           0.0f,           10);
		add_terminal("-1.0",          -1.0f,           5);
		// Fine-grained constants 0.1 to 1.0 with exponentially
		// decreasing weights, matching the POS grammar.
		// Enables non-monotonic convex categories centered at
		// arbitrary points (e.g., abs(dim - 0.3) for a category
		// centered at the 62nd percentile).
		{
			const char* names[] = {"0.1","0.2","0.3","0.4","0.5",
			                       "0.6","0.7","0.8","0.9","1.0"};
			for (int i = 0; i < 10; i++) {
				float v = (i + 1) * 0.1f;
				double w = 10.0 * std::exp(-2.3 * v);
				add_terminal(names[i], v, w);
			}
		}

		// -> t_dim (dimension index terminals)
		for (size_t i = 0; i < num_dims; i++) {
			add_terminal("d" + std::to_string(i),
			             t_dim(i),
			             10);
		}
	}
} adj_measure_grammar;

// ============================================================
// AdjCompGrammar — the shared POS / standard of comparison
// ============================================================

class AdjCompGrammar : public Grammar<
		t_adj_comp_input, t_t,
		t_adj_comp_input, t_t, t_measure, t_e, float, t_context
	>, public Singleton<AdjCompGrammar> {

	using Super = Grammar<
		t_adj_comp_input, t_t,
		t_adj_comp_input, t_t, t_measure, t_e, float, t_context>;
	using Super::Super;

public:
	AdjCompGrammar() {
		using namespace AdjCompDSL;

		add("X",                      Builtins::X<AdjCompGrammar>, 1);

		// -> t_measure (adjective function)
		add("%s.adj",                 adj_,           1);
		// -> t_e (entity)
		add("%s.x",                   x_,            10);
		// -> t_context
		add("%s.c",                   c_,             1);

		// -> float (apply adjective to entity)
		add("( apply %s %s )",        applyAdj,      10);

		// -> float (adjective-relative context statistics)
		add("( maxAdj %s %s )",       maxAdj,         5);
		add("( minAdj %s %s )",       minAdj,         5);
		add("( meanAdj %s %s )",      meanAdj,        5);

		// -> float (arithmetic)
		add("( + %s %s )",            plus,            1);
		add("( - %s %s )",            minus,           1);
		add("( * %s %s )",            times,           1);
		add("( neg %s )",             neg,             1);
		add("( abs %s )",             absF,            1);
		add("( maxF %s %s )",         maxF,            1);
		add("( minF %s %s )",         minF,            1);

		// -> float (constants)
		add_terminal("0.0",           0.0f,           10);
		add_terminal("-1.0",          -1.0f,           5);
		// Positive thresholds 0.1 to 1.0 with exponentially
		// decreasing weights: w = 10 * exp(-2.3 * v).
		// Encodes a prior preference for thresholds near the
		// population mean (0) over extreme thresholds (1 sd+).
		{
			const char* names[] = {"0.1","0.2","0.3","0.4","0.5",
			                       "0.6","0.7","0.8","0.9","1.0"};
			for (int i = 0; i < 10; i++) {
				float v = (i + 1) * 0.1f;
				double w = 10.0 * std::exp(-2.3 * v);
				add_terminal(names[i], v, w);
			}
		}

		// -> t_t (float comparisons)
		add("( fltGt %s %s )",        fltGt,          10);
		add("( fltGte %s %s )",       fltGte,         10);
		add("( fltEq %s %s )",        fltEq,          10);

		// -> t_t (boolean)
		add("( not %s )",             not_,            1);
		add("( and %s %s )",          and_,            1);
		add("( or %s %s )",           or_,             1);
	}
} adj_comp_grammar;

// Alias for Main.cpp TESTGRAMMAR case
[[maybe_unused]] static AdjCompGrammar& grammar = adj_comp_grammar;

// ============================================================
// Inner hypothesis types
// ============================================================

class InnerAdjMeasureHyp : public DeterministicLOTHypothesis<
		InnerAdjMeasureHyp, t_adj_input, float,
		AdjMeasureGrammar, &adj_measure_grammar, t_adj_datum> {
	using Super = DeterministicLOTHypothesis<
		InnerAdjMeasureHyp, t_adj_input, float,
		AdjMeasureGrammar, &adj_measure_grammar, t_adj_datum>;
public:
	using Super::Super;
};

class InnerAdjCompHyp : public DeterministicLOTHypothesis<
		InnerAdjCompHyp, t_adj_comp_input, t_t,
		AdjCompGrammar, &adj_comp_grammar, t_adj_comp_datum> {
	using Super = DeterministicLOTHypothesis<
		InnerAdjCompHyp, t_adj_comp_input, t_t,
		AdjCompGrammar, &adj_comp_grammar, t_adj_comp_datum>;
public:
	using Super::Super;
};

// ============================================================
// AdjsHypothesis — composite hypothesis
//
// Holds one composition program (POS / standard of comparison)
// and N adjective measure programs. MCMC proposes to one
// component at a time.
//
// The composition function wraps each adjective's degree output
// into a t_IV_M predicate, then FA handles the rest.
// ============================================================

class AdjsHypothesis : public MCMCable<AdjsHypothesis, t_datum> {

private:

	mutable InnerAdjCompHyp comp;
	mutable std::vector<InnerAdjMeasureHyp> adjs;

	// ---- Static parameters (shared, set once before MCMC) ----
	static inline size_t nObs = 0;
	static inline size_t cSize = 0;
	static inline double likelihoodWeight = 0.0;
	static inline double lengthWeight = 0.0;  // penalty on avg produced-sentence length
	static inline thread_local std::mt19937 local_rng{std::random_device{}()};
	static inline size_t searchDepth = 2;
	static inline double pTarget = 0.5;
	static inline bool pragmatic = false;
	static inline bool exclude_empty_adjs = false;

	// Communication data produced during likelihood evaluation (for logging)
	data_t commData;

	// Lexicon configuration
	bool add_es          = true;
	bool add_BFs         = false;
	bool add_IVs         = false;
	bool add_TVs         = false;
	bool add_DPs         = false;
	bool add_PMs         = false;
	bool add_PMMs        = true;
	bool add_Qs          = true;
	bool add_distractor  = false;

public:

	using Super = MCMCable<AdjsHypothesis, t_datum>;

	// ---- Static parameter setup ----

	static void setParams(const size_t nObs,
	                      const size_t cSize,
	                      const double likelihoodWeight,
	                      const double lengthWeight,
	                      std::mt19937& rng,
	                      const size_t searchDepth,
	                      const double pTarget,
	                      const bool pragmatic,
	                      const bool exclude_empty) {
		AdjsHypothesis::nObs = nObs;
		AdjsHypothesis::cSize = cSize;
		AdjsHypothesis::likelihoodWeight = likelihoodWeight;
		AdjsHypothesis::lengthWeight = lengthWeight;
		AdjsHypothesis::local_rng = rng;
		AdjsHypothesis::searchDepth = searchDepth;
		AdjsHypothesis::pTarget = pTarget;
		AdjsHypothesis::pragmatic = pragmatic;
		AdjsHypothesis::exclude_empty_adjs = exclude_empty;
	}

	// ---- Constructors ----

	AdjsHypothesis() : adjs(num_adjs) {
		adj_measure_grammar.GRAMMAR_MAX_DEPTH = 50;
		adj_comp_grammar.GRAMMAR_MAX_DEPTH = 50;
	}

	// Construct from serialized string: comp_ser|||a1_ser|||a2_ser|||...
	AdjsHypothesis(const std::string& parseable) : AdjsHypothesis() {
		const std::string delim = "|||";
		std::vector<std::string> parts;
		size_t start = 0, pos;
		while ((pos = parseable.find(delim, start)) != std::string::npos) {
			parts.push_back(parseable.substr(start, pos - start));
			start = pos + delim.size();
		}
		parts.push_back(parseable.substr(start));

		if (parts.size() != num_adjs + 1) {
			throw std::runtime_error(
				"AdjsHypothesis: expected " + std::to_string(num_adjs + 1)
				+ " parts separated by |||, got " + std::to_string(parts.size()));
		}
		comp = InnerAdjCompHyp::deserialize(parts[0]);
		for (size_t i = 0; i < num_adjs; i++) {
			adjs[i] = InnerAdjMeasureHyp::deserialize(parts[i + 1]);
		}
	}

	// ---- Data access ----

	data_t getCommData() const { return commData; }

	// ---- Composition function ----
	// Pure FA for all type combinations.
	// The adjective-specific composition (POS) is already baked
	// into the t_IV_M returned by adj_n().

	t_BTC_compose getCompositionF() const {
		return [](t_meaning a, t_meaning b) -> t_meaning {
			return std::visit(
				[](auto&& f, auto&& arg) -> t_meaning {
					using T = std::decay_t<decltype(f)>;
					using U = std::decay_t<decltype(arg)>;
					if constexpr (accepts_arg_v<T, U>) {
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

	// ---- Adjective meanings ----
	// Each adjective's measure program produces a degree (float).
	// The shared composition program (POS) maps (degree, context) → bool.
	// The result is a standard t_IV_M predicate.

	t_meaning adj_n(size_t i) const {
		return t_meaning([this, i](t_context c) -> t_IV {
			// Build the measure function for this adjective in this context
			t_measure measure = [this, i, c](t_e x) -> float {
				return adjs[i].call(std::make_tuple(x, c));
			};
			// Return predicate: apply POS to (measure, entity, context)
			return [this, measure, c](t_e x) -> t_t {
				return comp.call(std::make_tuple(measure, x, c));
			};
		});
	}

	// ---- Lexicon ----

	LexicalSemantics getLexicon() const {
		LexicalSemantics lex{
			cSize, add_es, add_BFs, add_IVs, add_TVs,
			add_DPs, add_PMs, add_PMMs, add_Qs, add_distractor
		};
		lex.add("thing", t_meaning([](const t_context&) -> t_IV {
			return [](t_e) -> t_t { return true; };
		}));
		for (size_t i = 0; i < num_adjs; i++) {
			lex.add("A" + std::to_string(i + 1), adj_n(i));
		}
		return lex;
	}

	// ============================================================
	// MCMCable interface
	// ============================================================

	[[nodiscard]] static AdjsHypothesis sample() {
		AdjsHypothesis h;
		h.comp = InnerAdjCompHyp::sample();
		for (size_t i = 0; i < num_adjs; i++) {
			h.adjs[i] = InnerAdjMeasureHyp::sample();
		}
		return h;
	}

	[[nodiscard]] std::optional<std::pair<AdjsHypothesis, double>>
	propose() const override {
		// Pick one component: 0 = comp, 1..N = adjs
		static thread_local std::mt19937 propose_rng{std::random_device{}()};
		std::uniform_int_distribution<size_t> dist(0, num_adjs);
		size_t idx = dist(propose_rng);

		AdjsHypothesis proposed = *this;
		double fb = 0.0;

		if (idx == 0) {
			auto p = comp.propose();
			if (!p) return {};
			proposed.comp = std::move(p->first);
			fb = p->second;
		} else {
			auto p = adjs[idx - 1].propose();
			if (!p) return {};
			proposed.adjs[idx - 1] = std::move(p->first);
			fb = p->second;
		}

		return std::make_pair(std::move(proposed), fb);
	}

	[[nodiscard]] AdjsHypothesis restart() const override {
		AdjsHypothesis h;
		h.comp = comp.restart();
		for (size_t i = 0; i < num_adjs; i++) {
			h.adjs[i] = adjs[i].restart();
		}
		return h;
	}

	bool operator==(const AdjsHypothesis& other) const override {
		if (!(comp == other.comp)) return false;
		for (size_t i = 0; i < num_adjs; i++) {
			if (!(adjs[i] == other.adjs[i])) return false;
		}
		return true;
	}

	// ============================================================
	// Bayesable interface
	// ============================================================

	size_t hash() const override {
		size_t h = comp.hash();
		for (const auto& a : adjs) {
			h ^= a.hash() + 0x9e3779b9 + (h << 6) + (h >> 2);
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
		// Format: POS | A1 | A2 | ...
		std::string result = comp.string();
		for (const auto& a : adjs) {
			result += " | " + strip_lambda(a.string());
		}
		return prefix + result;
	}

	std::string serialize() const {
		std::string result = comp.serialize();
		for (const auto& a : adjs) {
			result += "|||" + a.serialize();
		}
		return result;
	}

	double compute_prior() override {
		this->prior = comp.compute_prior();
		for (auto& a : adjs) {
			this->prior += a.compute_prior();
		}
		return this->prior;
	}

	double compute_likelihood(const data_t& x,
	                          const double breakout = -infinity) override {

		if (exclude_empty_adjs) {
			// Check composition uses the adjective
			auto cs = comp.string();
			if (cs.find("X.adj") == std::string::npos) {
				this->likelihood = -std::numeric_limits<double>::infinity();
				return this->likelihood;
			}
			// Check each adjective uses the entity
			for (const auto& a : adjs) {
				auto s = a.string();
				if (s.find("X.x") == std::string::npos) {
					this->likelihood = -std::numeric_limits<double>::infinity();
					return this->likelihood;
				}
			}
		}

		Agent<AdjsHypothesis> agent{*this};

		std::vector<t_context> cs = generateContexts(
			cSize, nObs, local_rng, pTarget);

		auto data = agent.produceDataFromEnumeration(
			cs, local_rng, searchDepth, pragmatic);

		commData = std::get<0>(data);
		auto utilities = std::get<1>(data);
		auto lengths = std::get<2>(data);

		double commAcc = 0;
		double avgLength = 0;
		for (size_t i = 0; i < utilities.size(); i++) {
			commAcc += utilities[i];
			avgLength += lengths[i];
		}
		commAcc /= nObs;
		avgLength /= nObs;

		this->likelihood = likelihoodWeight * commAcc - lengthWeight * avgLength;

		std::cout << "Hypothesis: " << this->string() << std::endl;
		std::cout << "Communicative accuracy: " << commAcc << std::endl;
		std::cout << "Average length: " << avgLength << std::endl;
		std::cout << "Log likelihood: " << this->likelihood << std::endl;
		std::cout << std::endl;

		return this->likelihood;
	}
};

// Common aliases so Main.cpp only needs to change the #include
using ActiveHypothesis = AdjsHypothesis;
static constexpr size_t num_learned_items = num_adjs;
