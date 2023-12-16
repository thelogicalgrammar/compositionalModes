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
		if constexpr (std::is_same_v<T, t_t_M>) {
            return "<s,t>";
        } else if constexpr (std::is_same_v<T, t_UC_M>) {
            return "<s,<t,t>>";
        } else if constexpr (std::is_same_v<T, t_BC_M>) {
            return "<s,<t,<t,t>>>";
        } else if constexpr (std::is_same_v<T, t_BC2_M>) {
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

	// Initialize the lexical meanings
	// (this leaves the composition function
	// still undefined)
	LexicalSemantics() {

		// e are numbers
		// there are no `t`s in the lexicon directly
		// Empty is an empty struct

		// t_UC
		
		t_UC_M l_not = 
			[](t_context c) -> t_UC {
				return [](t_t x) -> t_t { 
					return !x; 
				};
			};
		interpretation_f["l_not"] = l_not;

		// t_BC
		
		t_BC_M l_and =
			[](t_context c) -> t_BC{
				return [](t_t x) -> t_UC {
					// the function is curried
					return [x](t_t y) -> t_t { 
						return x && y; 
					};
				};
			};
		interpretation_f["l_and"] = l_and;

		t_BC_M l_or = 
			[](t_context c) -> t_BC{
				return [](t_t x) -> t_UC {
					return [x](t_t y) -> t_t {
						return x || y;
					};
				};
			};
		interpretation_f["l_or"] = l_or;
		
		// t_BC2
		t_BC2_M l_if_else = 
			[](t_context c) -> t_BC2 {
				return [](t_t x) -> t_BC {
					return [x](t_t y) -> t_UC {
						return [x,y](t_t z) -> t_t {
							return (x && y) || (!x && z);
						};
					};
				};
			};
		interpretation_f["l_if_else"] = l_if_else;

		// t_IV
		t_IV_M positive = 
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o > 0;
				};
			};
		interpretation_f["positive"] = positive;

		t_IV_M negative = 
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o < 0;
				};
			};
		interpretation_f["negative"] = negative;

		t_IV_M even = 
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					int o = std::get<0>(x);
					return o % 2 == 0;
				};
			};
		interpretation_f["even"] = even;

		t_IV_M prime = 
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
			};
		interpretation_f["prime"] = prime;

		// t_TV

		// greater than
		t_TV_M gt = 
			[](t_context c) -> t_TV{
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 > o1;
					};
				};
			};
		interpretation_f["gt"] = gt;

		// two properties are equal
		// with respect to their content
		// (ignores target status)
		t_TV_M equal = 
			[](t_context c) -> t_TV {
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 == o1;
					};
				};
			};
		interpretation_f["equal"] = equal;

		t_TV_M divides = 
			[](t_context c) -> t_TV {
				return [](t_e y) -> t_IV {
					int o1 = std::get<0>(y);
					return [o1](t_e x) -> t_t {
						int o2 = std::get<0>(x);
						return o2 % o1 == 0;
					};
				};
			};
		interpretation_f["divides"] = divides;
		
		// add (common) names for the numbers 0 to 9
		for (int i = 0; i < 10; i++) {
			 t_meaning intmeaning = 
				[i](t_context c) -> t_IV {
					return [i](t_e x) -> t_t {
						return std::get<0>(x) == i;
					};
				};

			interpretation_f[std::to_string(i)] = intmeaning;
		}
		
		// t_DP
		
		t_DP_M something = 
			[](t_context c) -> t_DP {
				return [c](t_IV x) -> t_t {
					// loop over the domain
					// if any of the elements are true, return true
					for (auto obj : c) {
						if (x(obj)) return true;
					}
					return false;
				};
			};
		interpretation_f["something"] = t_meaning(something);

		t_DP_M everything = 
			[](t_context c) -> t_DP {
				return [c](t_IV x) -> t_t {
					// loop over the domain
					for (auto obj : c) {
						if (!x(obj)) return false;
					}
					return true;
				};
			};
		interpretation_f["everything"] = t_meaning(everything);

		// t_IV
		
		t_IV_M target =
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					// check that the object is a target
					return std::get<1>(x);
				};
			};
		interpretation_f["target"] = t_meaning(target);

		t_IV_M distractor =
			[](t_context c) -> t_IV {
				return [](t_e x) -> t_t {
					// check that the object is a target
					return !std::get<1>(x);
				};
			};
		interpretation_f["distractor"] = t_meaning(distractor);
		
		// t_Q
		
		t_Q_M every = 
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
			};
		interpretation_f["every"] = t_meaning(every);

		t_Q_M some = 
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
			};
		interpretation_f["some"] = t_meaning(some);

		// returns true if there is exactly one x and it is y
		// If there is no or more than one x also returns false
		// TODO: Implement presupposition failure
		// in the type system
		// It cannot be done with Empty
		t_Q_M the = 
			[](t_context c) -> t_Q {
				return [c](t_IV x) -> t_DP {
					return [x,c](t_IV y) -> t_t {
						bool found = false;
						for (auto e : c) {
							if (x(e)) {
								// if we've already found one,
								if (found) return false;
								// if this one isn't y, return false
								if (!y(e)) return false;
								// otherwise, we've found one
								found = true;
							}
						}
						if (!found) return false;
						// if we found one, return true
						return true;
					};
				};
			};
		interpretation_f["the"] = t_meaning(the);
	
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
			LexicalSemantics& lex
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
    t_meaning compose(t_BTC_compose composition_fn) {
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

	// Static method to create a BTC tree from an S-expression
    static std::unique_ptr<BTC> fromSExpression(
			const std::string& sExpr,
			LexicalSemantics& lex
			) {

        std::istringstream ss(sExpr);
        return parseAndBuild(ss, lex);
    }

	// Method to convert the BTC to an S-expression
    std::string toSExpression() const {
        if (std::holds_alternative<t_meaning>(data)) {
            return description;
        } else {
            const auto& children = std::get<Children>(data);
            std::string leftExpr = children.left ? 
				children.left->toSExpression() : 
				"";
            std::string rightExpr = children.right ?
				children.right->toSExpression() : 
				"";
            return "(" + leftExpr + " " + rightExpr + ")";
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

};

