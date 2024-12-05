# pragma once

// Here I define the DSL for the *meanings* of the language.
// The meaning of a sentence is a function from a context
// to a truth value.

// I assume that the context is a set of
// <int, bool> tuples, where the int is the value
// and the bool is whether or not it is a target

// This function goes from a meaning into a string
// representation of the type of that meaning.
std::string meaningTypeToString(const t_meaning& meaning) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
		if 		  constexpr (std::is_same_v<T, t_e_M>) {
			return "<s,e>";
		} else if constexpr (std::is_same_v<T, t_t_M>) {
            return "<s,t>";
        } else if constexpr (std::is_same_v<T, t_UC_M>) {
            return "<s,<t,t>>";
        } else if constexpr (std::is_same_v<T, t_BC_M>) {
            return "<s,<t,<t,t>>>";
        } else if constexpr (std::is_same_v<T, t_TC_M>) {
            return "<s,<t,<t,<t,t>>>>";
        } else if constexpr (std::is_same_v<T, t_IV_M>) {
            return "<s,<e,t>>";
        } else if constexpr (std::is_same_v<T, t_DP_M>) {
            return "<s,<<e,t>,t>>";
        } else if constexpr (std::is_same_v<T, t_TV_M>) {
            return "<s,<e,<e,t>>>";
        } else if constexpr (std::is_same_v<T, t_Q_M>) {
            return "<s,<<e,t>,<<e,t>,t>>>";
        } else if constexpr (std::is_same_v<T, Empty_M>) {
            return "empty";
        } else {
            return "unknown";
        }
    }, meaning);
}

// Define a class to hold the lexical meanings
class LexicalSemantics {

private:

	std::map<std::string, t_meaning> interpretation_f;

public:

	void add(std::string name, t_meaning m) {
		interpretation_f[name] = m;
	}

	// Add Boolean constants "true" and "false"
	void addBCs() {

		// true
		add(
			"true",
			[](t_context c) -> t_t {
				return true;
			}
		);

		// false
		add(
			"false",
			[](t_context c) -> t_t {
				return false;
			}
		);

	}

	// Boolean functions
	void addBFs() {

		// t_UC
		
		add( 
			"l_not", 
			[](t_context c) -> t_UC {
				return [](t_t x) -> t_t { 
					return !x; 
				};
			}
		);

		// t_BC
		
		add(
			"l_and",
			[](t_context c) -> t_BC {
				return [](t_t x) -> t_UC {
					// the function is curried
					return [x](t_t y) -> t_t { 
						return x && y; 
					};
				};
			}
		);

		add(
			"l_or",
			[](t_context c) -> t_BC {
				return [](t_t x) -> t_UC {
					return [x](t_t y) -> t_t {
						return x || y;
					};
				};
			}
		);
		
		// t_TC
		
		add(
			"l_if_else",
			[](t_context c) -> t_TC {
				return [](t_t x) -> t_BC {
					return [x](t_t y) -> t_UC {
						return [x,y](t_t z) -> t_t {
							return (x && y) || (!x && z);
						};
					};
				};
			}
		);

	}

	void addIVs() {

		add( "positive",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o > 0;
				};
			}
	   	);

		add( "negative",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o < 0;
				};
			}
		);

		add( "even",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o % 2 == 0;
				};
			}
		);

		add( "prime",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					if (o <= 1) return false;
					if (o == 2) return true;
					if (o % 2 == 0) return false;
					for (int i = 3; i < o; i += 2) {
						if (o % i == 0) return false;
					}
					return true;
				};
			}
		);

		add( "target",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					// check that the object is a target
					return std::get<1>(x);
				};
			}
		);

		add( "distractor",
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					// check that the object is a target
					return !std::get<1>(x);
				};
			}
		);
	}

	void addTVs() {

		// greater than
		add( "gt",
			[](t_context c) -> t_TV{
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 > o1;
					};
				};
			}
		);

		// two properties are equal
		// with respect to their content
		// (ignores target status)
		add( "equal",
			[](t_context c) -> t_TV {
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 == o1;
					};
				};
			}
		);

		add( "divides",
			[](t_context c) -> t_TV {
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 % o1 == 0;
					};
				};
			}
		);
		
		// add (common) names for the numbers 0 to 5
		for (int i = 0; i < 6; i++) {
			add(
				std::to_string(i),
				[i](t_context c) -> t_IV {
					return [i](t_e x) -> t_t {
						return std::get<0>(x) == i;
					};
				}
			);
		}
	}

	void addDPs() {
		
		add( "something",
			[](t_context c) -> t_DP {
				return [c](t_IV x) -> t_t {
					// loop over the domain
					// if any of the elements are true, return true
					for (auto obj : c) {
						if (x(obj)) return true;
					}
					return false;
				};
			}
		);

		add( "everything",
			[](t_context c) -> t_DP {
				return [c](t_IV x) -> t_t {
					// loop over the domain
					for (auto obj : c) {
						if (!x(obj)) return false;
					}
					return true;
				};
			}
		);

	}
	
	void addQs() {
		
		add( "every",
			[](t_context c) -> t_Q {
				return [c](t_IV x) -> t_DP {
					return [x,c](t_IV y) -> t_t {
						// returns true if all xs are ys
						for (auto e : c) {
							if (x(e) && !y(e)) return false; 
						}
						return true;
					};
				};
			}
		);

		add( "some",
			[](t_context c) -> t_Q {
				return [c](t_IV x) -> t_DP {
					return [x,c](t_IV y) -> t_t {
						// returns true if any xs are ys
						for (auto e : c) {
							if (x(e) && y(e)) return true; 
						}
						return false;
					};
				};
			}
		);

		// returns true if there is exactly one x and it is y.
		// If there is no or more than one x,
		// throws PresuppositionFailure.
		// otherwise returns false
		add( "the",
			[](t_context c) -> t_Q {
				return [c](t_IV x) -> t_DP {
					return [x,c](t_IV y) -> t_t {
						int count_x = 0;
						bool found = false;
						for (auto e : c) {
							if (x(e) && y(e)) found = true;
							if (x(e)) count_x++;
							if (count_x > 1) 
								throw PresuppositionFailure();
						}
						if (count_x == 0) 
							throw PresuppositionFailure();
						return found;
					};
				};
			}
		);

	}

	LexicalSemantics(
			/* bool add_BCs, */
			bool add_BFs,
			bool add_IVs,
			bool add_TVs,
			bool add_DPs,
			bool add_Qs
		) {
		if (add_BFs) addBFs();
		if (add_IVs) addIVs();
		if (add_TVs) addTVs();
		if (add_DPs) addDPs();
		if (add_Qs) addQs();
	}

	LexicalSemantics() {
		addBFs();
		addIVs();
		addTVs();
		addDPs();
		addQs();
	}

	// Define .at access for the interpretation map
	t_meaning at(std::string name) const {
		auto it = interpretation_f.find(name);
        if (it != interpretation_f.end()) {
			// Key found, return the corresponding value
            return it->second;  
        }
        // Handle the case where the key is not found
        // throw std::runtime_error("Key not found");
		throw std::runtime_error("Key not found: " + name);	
	}

	std::vector<std::string> getNames() const {
		std::vector<std::string> names;
		for (
			auto it = interpretation_f.begin(); 
			it != interpretation_f.end();
			++it
		){names.push_back(it->first);}
		return names;
	}

	///// For looping

    auto begin() -> decltype(interpretation_f.begin()) {
        return interpretation_f.begin();
    }

    auto end() -> decltype(interpretation_f.end()) {
        return interpretation_f.end();
    }

};


// Define a binary tree structure for the language
// This is the "syntax" of the language
// This is a recursive data structure
// It is *not* specific to a particular composition function!
// It is also *not* specific to a particular lexicon,
// since the lexicon is dependent on the context
// and we might want to evaluate the same sentence
// in different contexts.

// NOTE: Since the composition function is not necessarily 
// simple application, the type of the node 
// cannot be figured out from its children!
class BTC {
private:

    struct Children {

        std::unique_ptr<BTC> left;
        std::unique_ptr<BTC> right;

		// Constructor for the Children struct
		// Takes ownership of the left and right children
        Children(
				std::unique_ptr<BTC> l,
				std::unique_ptr<BTC> r
		) : left(std::move(l)), right(std::move(r)) {}

    };

	// The data stored in the BTC node
    std::variant<t_meaning, Children> data;

	// Helper method for parsing and building the BTC tree
	// from an S-expression and a lexicon
    static std::unique_ptr<BTC> parseAndBuild(
			std::istringstream& ss,
			const LexicalSemantics& lex
		) {

        std::string token;
        ss >> token;

        if (token == "(") {
            auto left = parseAndBuild(ss, lex);
            auto right = parseAndBuild(ss, lex);
			// Consume the closing ')'
            ss >> token; 
            if (token != ")") {
                throw std::runtime_error(
					"Malformed S-expression: Expected ')'"
				);
            }
            return std::make_unique<BTC>(
				std::move(left),
				std::move(right)
			);
        } else {
			auto value = lex.at(token);
			// Use appropriate description
            return std::make_unique<BTC>(
				value,
				token
			);  
        }
    }

public:
	
	// String to hold the node's description
    std::string description;

	// Constructor for terminal node with description
    BTC(t_meaning terminal, std::string desc) 
        : data(terminal), description(std::move(desc)) {}

    // Constructor for non-terminal node
    BTC(std::unique_ptr<BTC> left, std::unique_ptr<BTC> right)
        : data(Children(std::move(left), std::move(right))) {}

	// Compose method
	// which evaluates the BTC
	// into a t_meaning object
    t_meaning compose(t_BTC_compose composition_fn) const {
        if (std::holds_alternative<t_meaning>(data)) {
            // If it's a terminal, just return it as is
            return std::get<t_meaning>(data);
        } else {
			
            // If it's a BTC with children,
			// recursively apply composition_fn to its children
            auto& children = std::get<Children>(data);
            t_meaning left_result = 
				children.left ?
				// if children.left is not null,
				// recursively compose it
				children.left->compose(composition_fn) :
				// otherwise, return Empty{}
				// as a t_meaning
				t_meaning(Empty_M{});
            t_meaning right_result = 
				children.right ?
				children.right->compose(composition_fn) :
				t_meaning(Empty_M{});

            t_meaning out = composition_fn(
				left_result,
				right_result
			);

			// If either of the children returned Empty{},
			// return Empty{}
			if (std::holds_alternative<Empty_M>(left_result) 
				|| std::holds_alternative<Empty_M>(right_result)
					) {
                return t_meaning(Empty_M{});
            }

			// If out contains Empty{},
			// print the description of the node
			// to help with debugging
			/* if (std::holds_alternative<Empty>(out)) { */
			/* 	std::cout << "Empty{} on: "; */
			/* 	// run print on the node */
			/* 	print(); */
			/* 	std::cout << std::endl; */
			/* } */

			return out;
        }
    }

	// Generate random S-expression
	static std::string randomSExpression(
			std::vector<std::string>& names,
			std::mt19937& rng,
			int depth = 0, 
			int maxDepth = 3
		) {
		std::uniform_int_distribution<> dist(0, names.size() - 1);
		// Decide between leaf or internal node
		std::uniform_int_distribution<> decide(0, 1); 

		if (depth >= maxDepth || decide(rng) == 0) {
			// Leaf node case
			int index = dist(rng);
			return names[index];
		} else {
			// Recursive case: internal node
			std::string leftExpr = randomSExpression(
				names,
				rng,
				depth + 1,
				maxDepth
			);
			std::string rightExpr = randomSExpression(
				names,
				rng,
				depth + 1,
				maxDepth
			);
			return "( " + leftExpr + " " + rightExpr + " )";
		}
	}

	// Static method to create a BTC tree from an S-expression
    static std::unique_ptr<BTC> fromSExpression(
			const std::string& sExpr,
			const LexicalSemantics& lex
		) {

        std::istringstream ss(sExpr);
        return parseAndBuild(ss, lex);
    }

	// Method to convert the BTC to an S-expression
    std::string toSExpression() const {
		// If the node is a terminal node,
		// return the description
        if (std::holds_alternative<t_meaning>(data)) {
            return description;
        } else {
			// If the node is not a terminal node,
			// return the S-expression of the left and right children
            const auto& children = std::get<Children>(data);
            std::string leftExpr = children.left ? 
				children.left->toSExpression() : 
				"";
            std::string rightExpr = children.right ?
				children.right->toSExpression() : 
				"";
            return "( " + leftExpr + " " + rightExpr + " )";
        }
    }


	// function to find the size of the tree
	int size() const {
		if (std::holds_alternative<t_meaning>(data)) {
			return 1;
		} else {
			const auto& children = std::get<Children>(data);
			int leftDepth = children.left ?
				children.left->size() : 
				0;
			int rightDepth = children.right ?
				children.right->size() : 
				0;
			return leftDepth + rightDepth;
		}
	}

	// function to find the depth of the tree
	int depth() const {
		if (std::holds_alternative<t_meaning>(data)) {
			return 0;
		} else {
			const auto& children = std::get<Children>(data);
			int leftDepth = children.left ?
				children.left->depth() : 
				0;
			int rightDepth = children.right ?
				children.right->depth() : 
				0;
			return std::max(leftDepth, rightDepth) + 1;
		}
	}

	// Print in tree format - sometimes clearer than S-expression
	// for large trees
    void printTree(LexicalSemantics& lex, int depth = 0) const {
        std::visit(
			[&](const auto& node) {
				std::cout << std::string(depth * 2, ' ');
				// if the node is a Children,
				// print the left and right subtrees
				if constexpr (
							std::is_same_v<
								std::decay_t<decltype(node)>,
								Children
							>
						) {
					std::cout << "(\n";
					if (node.left) 
						node.left->printTree(lex, depth + 1);
					if (node.right) 
						node.right->printTree(lex, depth + 1);
					std::cout << std::string(depth * 2, ' ') << ")\n";
				} else {
					// if the node is a t_meaning,
					// print the description for terminal node
					// along with its type
					t_meaning meaning = lex.at(description);
					std::string type = meaningTypeToString(meaning);
					std::cout << description << " : " << type << "\n";
				}
			},
			// data is the content of the node
			// either a t_meaning or a Children
			data
		);
    }

	bool contains(const std::string& name) const {
		if (std::holds_alternative<t_meaning>(data)) {
			return description == name;
		} else {
			const auto& children = std::get<Children>(data);
			bool leftContains = children.left ?
				children.left->contains(name) :
				false;
			bool rightContains = children.right ?
				children.right->contains(name) :
				false;
			return leftContains || rightContains;
		}
	}

	// check if the tree contains any of the names
	bool contains(const std::vector<std::string>& names) const {
		for (const auto& name : names) {
			if (contains(name)) {
				return true;
			}
		}
		return false;
	}

	std::unique_ptr<BTC> copy() const {
		if (std::holds_alternative<t_meaning>(data)) {
			return std::make_unique<BTC>(
				std::get<t_meaning>(data),
				description
			);
		} else {
			const auto& children = std::get<Children>(data);
			return std::make_unique<BTC>(
				children.left->copy(),
				children.right->copy()
			);
		}
	}

};


std::vector<std::unique_ptr<BTC>> copyBTCVec(
		const std::vector<std::unique_ptr<BTC>>& vec
	) {
	std::vector<std::unique_ptr<BTC>> out;
	for (auto& btc : vec) {
		std::unique_ptr<BTC> btcCopy = btc->copy();
		out.push_back(std::move(btcCopy));
	}
	return out;
}
