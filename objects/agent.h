# pragma once

using t_BTC_vec = std::vector<std::unique_ptr<BTC>>;
using t_BTC_dist = std::tuple<
	t_BTC_vec,
	std::discrete_distribution<>
>;

// Context variations are contexts
// that differ from the observed context
// only in what is a target and what a distractor.
t_contextVector generateContextVariations(const t_context& context) {

	// a vector of contexts
    t_contextVector variations;
    int N = context.size();
    int totalCombinations = 1 << N;

    // Store the int values from the original context
    std::vector<int> intValues;
    for (const auto& tuple : context) {
        intValues.push_back(std::get<0>(tuple));
    }

    for (int i = 0; i < totalCombinations; ++i) {
        std::bitset<32> binary(i);
        t_context newContext;
        int j = 0;

        for (const auto& tuple : context) {
            newContext.insert(std::make_tuple(
				intValues[j],
				binary[j]
			));
            ++j;
        }

        variations.push_back(newContext);
    }

    return variations;
}

class Agent {

private:

	// Initialize various constants
	LexicalSemantics lex;
	t_terminalsMap terminalsMap;
	int initialMaxDepth;
	// number of sampled random utterances
	// to find ones to consider in the first place
	int nSamples;
	double leafProb;
	double alpha;
	double sizeScaling;

	double computeComplexity(BTC& sentence){
		// The complexity of the tree is just
		// the number of terminal nodes
		return double(sentence.size());
	}

	double computeInformativity(
			t_context observedC,
			t_t_M meaning
		){
		// Since the listener can see the world
		// except for the target feature,
		// the possible contexts are the ones that differ
		// from the observed one wrt what's a target.
		// Therefore, I loop through the possible contexts
		t_contextVector possibleContexts = 
			generateContextVariations(observedC);
		int nTrue = 0;
		for (t_context c : possibleContexts) {
			// catch 'PresuppositionFailure' exception

			try {
				nTrue = nTrue + meaning(c);
			} catch (PresuppositionFailure& e) {
				// if the meaning fails to compute
				// it means that the context is not
				// compatible with the meaning
				// so we just ignore it
				continue;
			}
		}
		// compute informativity against 
		// set of all possible alternatives
		return -std::log(
			(double)nTrue/
			(double)possibleContexts.size()
		);
	}

	// Generates a random tree
	// given a description of the type of the root
	// and a CFG map
	std::unique_ptr<BTC> generateRandomTree(
			// a string describing the type of the node
			// (a key in cdfMap)
			std::string description,
			// CFG map from each type 
			// to the types that can be composed with it
			// (this is inferred from the composition function)
			// where types are represented as strings
			int maxDepth,
			const t_cfgMap& cfgMap,
			// rng is a random number generator
			std::mt19937& rng
		) {
	
		// check if description is not in the terminals map
		bool noterminal = terminalsMap.find(description) == terminalsMap.end();

		// check if description is not in the cfg map
		bool nocfg = cfgMap.find(description) == cfgMap.end();

		// assert that description is in 
		// at least one of the maps
		assert(!noterminal || !nocfg);

		// decide whether to create a leaf node
		bool createLeaf;
		if (noterminal) {
			// if there is no terminal of the given type,
			// always create a branching
			createLeaf = false;
		} else if (maxDepth <= 0 || nocfg) {
			// if we are at the maximum depth
			// (we can also exceed it if there were no terminals to choose from)
			// or if there are no possible compositions,
			// always create a leaf node
			createLeaf = true;
		} else {
			// even if we are not forced to create a leaf node,
			// we might still want to do so
			createLeaf = t_discr_dist({1-this->leafProb, this->leafProb})(rng);
		}

		// create a leaf node 
        if (createLeaf) {

			// Create a leaf node
			// leafIndex is the index of the meaning name in terminalMap
			// print description
            int leafIndex = t_intdist(
				0,
				terminalsMap.at(description).size() - 1
			)(rng);

            auto it = std::next(
				terminalsMap.at(description).begin(),
				leafIndex
			);
			
			// Create a leaf node
			// with the meaning and description
            return std::make_unique<BTC>(
				this->lex.at(*it),
				*it
			);

        } else {

			// choose random types tuple for the children
			// from the set of tuples of types whose composition
			// results in the current type
			auto it = cfgMap.find(description);
			if (it == cfgMap.end()) {
				throw std::runtime_error(
					"Unknown type: " + description
				);
			}
			int leftIndex = t_intdist(
				0,
				it->second.size() - 1
			)(rng);
			// get the set of tuples of types
			auto it2 = std::next(
				it->second.begin(),
				leftIndex
			);

			// get the left child type
			std::string leftChildType = std::get<0>(*it2);
			// get the right child type
			std::string rightChildType = std::get<1>(*it2);

			// Create a non-leaf node
			auto leftChild = generateRandomTree(
				leftChildType,
				maxDepth - 1,
				cfgMap,
				rng
			);
			auto rightChild = generateRandomTree(
				rightChildType,
				maxDepth - 1,
				cfgMap,
				rng
			);
			return std::make_unique<BTC>(
				std::move(leftChild),
				std::move(rightChild)
			);
		}
    }

public:

	// Constructor
	Agent() {

		// initialize the lexical semantics
		// which includes the meaning of each terminal
		lex = LexicalSemantics();
		// initialize the terminals map
		// which is a map from each type to the terminals of that type
		terminalsMap = generateTerminalsMap();
		// initialize the maximum depth of utterances
		initialMaxDepth = 4;
		// initialize the number of samples
		nSamples = 100000;
		// initialize the probability of creating a leaf node
		leafProb = 0.7;
		alpha = 10;
		sizeScaling = 0.3;

	}

	// The agent sees a world of objects
	// They have to produce a signal that 
	// helps the listener identify the targets.
	// Return a distribution over BTCs
	// Encoding (an approximation of)
	// the probability of producing
	// each sentence in the given context
	t_BTC_dist produce(
			t_context c, 
			t_BTC_compose compositionFn,
			std::mt19937& rng
		){
		// Considers a bunch of random utterances
		// that are all true of the context
		t_BTC_vec sentences = 
			generateRandomBTCsWithEvaluation(
				c, compositionFn, rng
			);

		// Calculates the utility of each sentence
		std::vector<double> utilities;
		for (auto& s : sentences) {
			t_t_M meaning = std::get<t_t_M>(
				s->compose(compositionFn)
			);
			// compute informativity
			double info =
				this->computeInformativity(
					c, meaning
				);
			// compute complexity (divide by 4 
			// to make more comparable with info);
			double complexity = 
				this->computeComplexity(*s)*this->sizeScaling;

			double utility = std::exp(this->alpha*(
				info - complexity
			));

			/* s->printTree(this->lex); */
			/* std::cout << "info: " << info << std::endl; */
			/* std::cout << "comp: " << complexity << std::endl; */
			/* std::cout << utility << std::endl; */
			/* std::cout << std::endl; */

			// Since this ends up as parameter to a 
			// discrete distribution, we don't need to
			// normalize.
			utilities.push_back(utility);
		}

		// Selects an informative and simple utterance
		t_discr_dist dist(utilities.begin(),utilities.end());

		return std::make_tuple(
			std::move(sentences),
			dist
		);
	}

	// Goes from a sentence to the probability
	// that each element in the context is a target
	std::vector<double> interpret(
			// The agent sees a full sentence
			std::unique_ptr<BTC>& s,
			t_context observedC,
			t_BTC_compose compositionFn
		){
		
		// get sentence meaning
		t_t_M meaning = std::get<t_t_M>(
			s->compose(compositionFn)
		);
		
		// loop over all possible contexts and compute the probability 
		// of each element in the context being a target given the sentence.
		// `probs` is a vector of 0s with the length of the context
		// NOTE: The agent can only see the first component 
		// of each context element, which is an integer.
		t_contextVector possibleContexts = 
			generateContextVariations(observedC);
		int numTrue = 0;
		std::vector<double> probs(observedC.size(),0);
		// Now ints are unique in the context!
		for (auto& c : possibleContexts) {
			// print context
			std::cout << c;
			bool truthvalue;

			try {
				truthvalue = meaning(c);
				// if the sentence is true of the context
				if (truthvalue) {
					numTrue++;
					// if so, update the probability of each element
					// in the context being a target
					int i = 0;
					for (auto e : c) {
						probs[i] += std::get<1>(e);
						i++;
					}
					std::cout << " TRUE ";
				} else {
					std::cout << " FALSE";
				}
			} catch (PresuppositionFailure& e) {
				// If the sentence presupposes something
				// that is not true of the context,
				// then the context can be ignored.
				std::cout << " PFAIL";
			}

			// print the prob vector
			std::cout << " " << probs << std::endl;
		}
		// normalize the probabilities
		// (i.e. divide by the number of 
		// possible contexts given sentence)
		for (int i = 0; i < probs.size(); i++) {
			probs[i] /= numTrue;
		}
		return probs;
	}

	void updateLexicalSemantics(LexicalSemantics newlex){
		// update the lexical semantics
		this->lex = newlex;
		// update the terminals map
		this->terminalsMap = generateTerminalsMap();
	}
	
	// This function takes a lexical semantics
	// and returns a map from each type in t_meaning
	// to the lexical entries that have that type
	t_terminalsMap generateTerminalsMap() {
		t_terminalsMap tmap;
		// type of meaning a string
		for (auto&& [word, meaning] : this->lex) {
			auto stringType = meaningTypeToString(meaning);
			tmap[stringType].insert(word);
		}
		return tmap;
	}

	// This function takes a composition function
	// and all the types in t_meaning
	// and returns a map from each type 
	// to the combination of types that can be composed
	// (i.e. the types that can be the left and right
	// children of the composition function)
	// Nodes don't compose only if they return Empty{}.
	t_cfgMap generateCFGMap(t_BTC_compose& compositionFn) {

		// Map from each type to the types 
		// that can be composed with it
		t_cfgMap cfgMap;

		// List of all possible types in t_meaning
		// and give an example of each
		// TODO: Make this more elegant
		// NOTE: This introduces the assumption that
		// whether the composition function 
		// returns Empty{} or not
		// (i.e., whether the nodes can be composed)
		// depends *only* on the type
		
		std::vector<t_meaning> types = {

			// TODO: add "e" if I end up adding instances of it
			// directly in the lexicon
			
			// I have t_t cause that's always at the top
			// so I should know how to get to it
			t_t_M{}, 
			// t_UC
			this->lex.at("l_not"),
			// t_BC
			this->lex.at("l_and"),
			// t_BC2
			this->lex.at("l_if_else"),
			// t_IV
			this->lex.at("target"),
			// t_DP
			this->lex.at("everything"),
			// t_TV
			this->lex.at("equal"),
			// t_Q
			this->lex.at("every"),
			// Empty type (indicating that the nodes can't be composed)
			Empty_M{}
		};

		for (auto&& type1 : types) {
			for (auto&& type2 : types) {
				// calculate an output
				t_meaning result = compositionFn(type1, type2);
				// type of output as string
				std::string resultStr = meaningTypeToString(result);
				// type of output is not Empty{}
				if (!std::holds_alternative<Empty_M>(result)) {
					std::string type1Str = meaningTypeToString(type1);
					std::string type2Str = meaningTypeToString(type2);
					cfgMap[resultStr].insert(
						std::make_tuple(type1Str, type2Str)
					);
				}
			}
		}

		return cfgMap;
	}

	// This function takes a context and a composition function
	// and returns a vector of BTCs
	// that are true in that context
	// Excludes BTCs that are:
	// - not true in the context
	// - not valid (i.e. have a type that can't be composed)
	// - have a presupposition failure
	t_BTC_vec generateRandomBTCsWithEvaluation(
			t_context context,
			t_BTC_compose compositionFn,
			std::mt19937& rng
		) {

        std::vector<std::unique_ptr<BTC>> validBTCs;
        std::set<std::string> evaluatedTrees;
        std::set<std::string> invalidTrees;

		t_cfgMap cfgMap = this->generateCFGMap(compositionFn);

		/* Old code that finds nSamples valid BTCs */
        /* while (validBTCs.size() < static_cast<size_t>(nSamples)) { */

		// loop for nSamples
		for (int i = 0; i < this->nSamples; i++) {

			// Generate a random tree
			// encoding a proposition
			// (function from a context to a bool)
            auto btc = generateRandomTree(
				"<s,t>",
				this->initialMaxDepth,
				cfgMap,
				rng
			);
			// Convert it to an S-expression to check
			// if it has already been evaluated
            std::string treeRepresentation = btc->toSExpression();

			// If the tree has already been evaluated
			// or if it has been marked as invalid skip it
            if (
				evaluatedTrees.find(treeRepresentation) != evaluatedTrees.end() 
				||
                invalidTrees.find(treeRepresentation) != invalidTrees.end()) {
                continue; 
            }

			// Evaluate the tree
            t_meaning result = btc->compose(compositionFn);

			// Apply the result to the context to get a bool
			t_extension extension = std::visit(
				[&context](auto&& result_M) {
					// All meanings are functions from 
					// contexts to something in t_extension
					// NOTE: Need to explicitly 
					// cast to t_extension
					// rather than directly return
					try {
						t_extension result = result_M(context);
						return result;
					} catch (PresuppositionFailure& e) {
						// If there is a presupposition failure
						// return Empty{}
						return t_extension(Empty{});
					}
				},
				result
			);
			// if the result (a t_extension) has a bool value
			// (rather than being Empty{})
			// and that bool value is true
            if (
				std::holds_alternative<t_t>(extension) && 
				std::get<t_t>(extension)
			) {
                evaluatedTrees.insert(treeRepresentation);
                validBTCs.push_back(std::move(btc));
				// Print the tree
				/* std::cout << "\n" << treeRepresentation << std::endl; */
			// if it holds a bool but it's false
			} else if (std::holds_alternative<t_t>(extension)) {
				// print "false" to help with debugging
				/* std::cout << "false" << std::endl; */
				// Print the tree
				/* std::cout << "\n" << treeRepresentation << std::endl; */
				invalidTrees.insert(treeRepresentation);
            } else {
				/* std::cout << "\n" << treeRepresentation << std::endl; */
                invalidTrees.insert(treeRepresentation);
            }
        }

        return validBTCs;
    }

};
