
// Define the types of the extensions
// the individuals are tuples of (int, bool)
// where the int is the actual value
// and the bool is whether or not it is a target
// NOTE: I assume all ADJs are actually t_IVs
using t_e     	= std::tuple<int, bool>;
using t_t   	= bool;
using t_context = std::set<t_e>;

using t_UC  = ft< t_t,  t_t  >;
using t_BC  = ft< t_UC, t_t  >;
using t_TC  = ft< t_BC, t_t	 >; // e.g., "if x then y else z"
using t_IV  = ft< t_t,  t_e  >; // IV, CN, ADJ
using t_DP  = ft< t_t,  t_IV >; // DP (e.g., "something (is)")
using t_TV  = ft< t_IV, t_e  >; // TV, P
using t_Q   = ft< t_DP, t_IV >;
/* using t_ADJ = ft< t_IV, t_IV >; */

// TO ADD
//
// Extensional meanings
// --------------------
// <<e,t>,<e,t>>
// <e,e>
// Add event types v (needs more complex context models)
//
// Intensional meanings
// --------------------
// Proposition to truth value: <s,<<s,t>,t>>
// 		E.g., "must", "can"
// Property to property: <s,<<s,<e,t>>,<s,<e,t>>>>
// 		E.g., "former" in "former president", "fake"
// Propositional attitude verbs??
//
// Kratzerian conditionals: <s,<<s,t>,<<s,t>,t>>
// 		"if" and "unless"
// TODO: define types recursively
// 		e.g., can use https://github.com/codeinred/recursive-variant


// The Empty type represents non-composibility
// (i.e. meaninglessness)
struct Empty {};

// The PresuppositionFailure type represents a failure to
// satisfy a presupposition.
// It can be raised by a meaning function to indicate that
// the meaning function cannot be applied to the given input.
// Can also be raised during composition.
struct PresuppositionFailure : public std::exception {
    const char* what() const noexcept override {
        return "Presupposition failure occurred";
    }
};


// Define the types of the intensions
using t_e_M   = ft< t_e,   t_context >;
using t_t_M   = ft< t_t,   t_context >;
using t_UC_M  = ft< t_UC,  t_context >;
using t_BC_M  = ft< t_BC,  t_context >;
using t_TC_M  = ft< t_TC,  t_context >;
using t_IV_M  = ft< t_IV,  t_context >;
using t_DP_M  = ft< t_DP,  t_context >;
using t_TV_M  = ft< t_TV,  t_context >;
using t_Q_M   = ft< t_Q,   t_context >;
using Empty_M = ft< Empty, t_context >;


// Variant type for all possible meaning types
// possible inputs: t, e, ft<t,e>
// possible outputs: t, ft<t,t>, ft<t,e>, ft<t,ft<t,e>>
// NOTE: add t_ADJ if needed
using t_extension = std::variant<
	t_e,
	t_t,
	t_UC,
	t_BC,
	t_TC,
	t_IV,
	t_DP, 
	t_TV,
	t_Q,
	Empty
>;

// Define the types of the meaning functions
using t_meaning = std::variant<
	t_e_M, 
	t_t_M,
	t_UC_M,
	t_BC_M,
	t_TC_M,
	t_IV_M,
	t_DP_M, 
	t_TV_M,
	t_Q_M,
	Empty_M
>;

// Define the types of the composition function
// i.e., a meaning transformation

using t_BTC_compose = 
	std::function< t_meaning(t_meaning, t_meaning) >;

using t_BTC_transform = 
	std::function< t_meaning(t_meaning) >;

using t_BTC_PtoE =
	std::function<t_e(t_context,t_IV_M)>;

using t_BTC_chooseF =
	std::function<t_BTC_PtoE(int)>;

using t_TandTtoT = 
	std::function<t_t(t_t,t_t)>;

// Some useful distributions
using t_intdist = std::uniform_int_distribution<int>;
using t_discr_dist = std::discrete_distribution<>;
using t_bernoulli_dist = std::bernoulli_distribution;

// Map from a type description to a tuple of 
// (left type, right type)
// saying what types can be composed to produce the type
using t_cfgMap = 
	std::map<
		std::string,
		std::set<std::tuple<std::string, std::string>>
	>;

// map from a type description to a set of terminals
// of that type
using t_terminalsMap = 
	std::map<
		std::string,
		std::set<std::string>
	>;

using t_contextVector = std::vector<t_context>;

// Type of single datapoint for the Hypotheses.
// The input is a context and the output is a sentence
using t_datum = defaultdatum_t<t_context,std::string>;

