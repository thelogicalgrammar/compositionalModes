# pragma once

using t_BTC_vec = std::vector<std::unique_ptr<BTC>>;
// unnormalized distribution values
using t_BTC_dist = std::tuple<
		t_BTC_vec,
		std::vector<double>
	>;
enum productionMode { SAMPLE, ARGMAX };

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
        /* for (const auto& tuple : context) { */
		// loop over the size of the context
		for (int j = 0; j < N; ++j) {
            newContext.insert(std::make_tuple(
				intValues[j],
				binary[j]
			));
        }

        variations.push_back(newContext);
    }

    return variations;
}

template <typename T>
std::vector<std::pair<T, int>> countUniqueElements(const std::vector<T>& input) {

    std::unordered_map<T, int> freqMap;

    // Count occurrences of each element
    for (const auto& elem : input) {
        freqMap[elem]++;
    }

    // Transfer data to a vector
    std::vector<std::pair<T, int>> uniqueElements(freqMap.begin(), freqMap.end());

    // Optional: Sort by element value (if ordering is needed)
    std::sort(uniqueElements.begin(), uniqueElements.end());

    return uniqueElements;
}


// We assume Hyp defines the following on top of the usual stuff:
// - getLexicalMeanings : returns a map containing learned meanings
// - getCompositionF	: returns a t_BTC_compose function
// If a hypothesis does not infer a composition or meanings,
// just return empty values
template <typename Hyp>
class Agent {
private:

	// Initialize various constants
	int initialMaxDepth;
	// number of sampled random utterances
	// to find ones to consider in the first place
	int nSamples;
	double leafProb;
	double alpha;
	double sizeScaling;
	
	// the chosen hypothesis
	Hyp chosenHyp;
	bool hasChosenHyp = false;

	// the original hypothesis in case we mutate
	std::optional<Hyp> originalHyp = std::nullopt;
	bool mutated = false;

	double computeComplexity(BTC& sentence) const{
		// The complexity of the tree is just
		// the number of terminal nodes
		return double(sentence.size());
	}

	double computeInformativity(
			t_context observedC,
			t_t_M meaning
		) const{
		// Since the listener can see the world
		// except for the target feature,
		// the possible contexts are the ones that differ
		// from the observed one wrt what's a target.
		// Therefore, I loop through the possible contexts
		t_contextVector possibleContexts = 
			generateContextVariations(observedC);
		int nTrue = 0;
		for (t_context c : possibleContexts) {
			try {
				nTrue = nTrue + meaning(c);
			} catch (PresuppositionFailure& e) {
				// if the meaning fails to compute
				// it means that the context is not
				// compatible with the meaning
				// so we just ignore it
				// and it doesn't increase nTrue
				continue;
			}
		}
		// compute surprisal of true state given the meaning
		// same informativity measure as RSA!
		return -std::log((double)nTrue);
	}

	// Generates a random tree
	// given a description of the type of the root
	// and a CFG map
	std::optional<std::unique_ptr<BTC>> generateRandomTree(
			// a string describing the type of the node
			// (a key in cdfMap)
			std::string typeName,
			int maxDepth,
			// CFG map from each type 
			// to the types that can be composed with it
			// (this is inferred from the composition function)
			// where types are represented as strings
			const t_cfgMap& cfgMap,
			// The lexical semantics at this point
			LexicalSemantics lex,
			// map from each type to the terminals of that type
			const t_terminalsMap& terminalsMap,
			// rng is a random number generator
			std::mt19937& rng
		) const {

		if (maxDepth < 0) {
			// without this some grammars get stuck in a loop
			// alternating between creating new leafs and 
			// returning nullptr
			return std::nullopt;
		}

		// is typeName not in the terminals map?
		bool noterminal = terminalsMap.find(typeName) == terminalsMap.end();

		// is typeName not in the cfg map?
		bool nocfg = cfgMap.find(typeName) == cfgMap.end();

		// If there is no terminal of the given type
		// AND we cannot get it by composition,
		// there is nowhere to go.
		// Return an empty optional
		// and this is dealt with in the caller.
		if (noterminal & nocfg){
			return std::nullopt;
		}

		// decide whether to create a leaf node
		bool createLeaf;
		if (noterminal) {
			// if there is no terminal of the given type,
			// always create a branching
			createLeaf = false;
		} else if (maxDepth == 0 || nocfg) {
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
			// leafIndex is the index of the meaning name in terminalsMap
			// print typeName
            int leafIndex = t_intdist(
				0,
				terminalsMap.at(typeName).size() - 1
			)(rng);

            auto it = std::next(
				terminalsMap.at(typeName).begin(),
				leafIndex
			);
			
			// Create a leaf node
			// with the meaning and typeName
            return std::make_unique<BTC>(
				lex.at(*it),
				*it
			);

        } else {

			// choose random types tuple for the children
			// from the set of tuples of types whose composition
			// results in the current type
			auto it = cfgMap.find(typeName);
			if (it == cfgMap.end()) {
				throw std::runtime_error(
					"Unknown type: " + typeName
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
				lex,
				terminalsMap,
				rng
			);
			auto rightChild = generateRandomTree(
				rightChildType,
				maxDepth - 1,
				cfgMap,
				lex,
				terminalsMap,
				rng
			);
			if (leftChild.has_value() && rightChild.has_value()) {
				return std::make_unique<BTC>(
					std::move(leftChild.value()),
					std::move(rightChild.value())
				);
			} else {
				return std::nullopt;
			}
		}
	}

public:

	using t_sentences_utilities = std::tuple<
			typename Hyp::data_t, 
			std::vector<double>
		>;

	Agent() {
		// initialize the maximum depth of utterances
		initialMaxDepth = 4;
		// initialize the probability of creating a leaf node.
		// for 0.6, the expected depth is ~2.4
		leafProb = 0.6;
		alpha = 5;
		// Scale the size of the utterances when computing complexity
		/* sizeScaling = 0.2; */
		sizeScaling = 0.0;
		// initialize the number of sampled random utterances
		// to pick one to refer to the state
		// *Onliy used in the case of sampling*
		nSamples = 5000;
	}

	Agent(Hyp hyp) : Agent() {
		this->setHypothesis(hyp);
	}

	Agent( std::string parseable ) : Agent(Hyp(parseable)) {}

	// The agent sees a world of objects
	// They have to produce a signal that 
	// helps the listener identify the targets.
	// Return encodes (an approximation of)
	// the utility of producing
	// each sentence in the given context
	std::optional<t_BTC_dist> produce(
			t_context c, 
			t_BTC_compose compositionFn,
			LexicalSemantics& lex,
			t_terminalsMap& terminalsMap,
			std::mt19937& rng,
			t_BTC_vec& sentences
		) const {

		if (sentences.size() == 0) {
			// If the composition function cannot produce 
			// sentences that are true of the context
			// (e.g., randomly initialized composition function)
			return std::nullopt;
		} else {

			// Calculates the utility of each sentence
			std::vector<double> utilities;
			for (auto& s : sentences) {

				t_t_M meaning = std::get<t_t_M>(s->compose(compositionFn));

				// compute informativity
				double info = this->computeInformativity(c, meaning);
				
				/* std::cout << "Sentence: " << s->toSExpression() << " "; */
				/* std::cout << "Info: " << info << std::endl; */

				// compute complexity (multiply by sizeScaling
				// to make more comparable with info);
				double complexity = 
					this->computeComplexity(*s) * this->sizeScaling;

				double utility = this->alpha*(info - complexity);

				/* s->printTree(lex); */
				/* std::cout << "info: " << info << std::endl; */
				/* std::cout << "comp: " << complexity << std::endl; */
				/* std::cout << utility << std::endl; */
				/* std::cout << std::endl; */

				// Since this ends up as parameter to a 
				// discrete distribution that automatically normalizes, 
				// we don't need to normalize here
				utilities.push_back(utility);
			}

			return std::make_tuple(std::move(sentences), utilities);

		}
	}

	std::optional<t_BTC_dist> produce(
			Hyp trueHyp,
			t_context c, 
			std::mt19937& rng
		) const {

		// get everything from the trueHyp
		LexicalSemantics lex 		= trueHyp.getLexicon();
		t_terminalsMap terminalsMap = this->generateTerminalsMap(lex);
		t_BTC_compose compositionFn = trueHyp.getCompositionF();
		return produce(c, compositionFn, lex, terminalsMap, rng);
	}

	std::optional<t_BTC_dist> produce(
			t_context c, 
			std::mt19937& rng
		) const {

		// Use the chosen hypothesis by default
		return produce(
			this->getHypothesis(),
			c,
			rng
		);
	}

	// Returns the produced sentences and their utility
	t_sentences_utilities produceDataFromEnumeration(
			std::vector<t_context> cs, 
			std::mt19937& rng,
			size_t searchDepth = 2,
			productionMode mode = productionMode::ARGMAX
		) const {

		auto trueHyp = this->getHypothesis();

		// get everything from the trueHyp
		LexicalSemantics lex 		= trueHyp.getLexicon();
		t_terminalsMap terminalsMap = this->generateTerminalsMap(lex);
		t_BTC_compose compositionFn = trueHyp.getCompositionF();

		// Find *all* sentences (true and false) given the grammar
		// up to a certain depth
		t_BTC_vec allSentences = enumerateSentences(
				compositionFn,
				lex,
				terminalsMap,
				searchDepth
			);

		// store the produced sentences
		typename Hyp::data_t producedSentences;
		// utility of each produced sentence
		std::vector<double> utilities;
		// loop over contexts
		for (auto& context : cs) {

			// select the true sentences in context
			t_BTC_vec trueSentences = selectTrueSentences(
					context,
					compositionFn,
					copyBTCVec(allSentences)
				);
			
			// Define a (maybe) distribution over sentences
			// (it's empty if there are no true sentences)
			// NOTE: this moves 'sentences' so it is no longer usable here
			std::optional<t_BTC_dist> sentencesDist = produce(
					context,
					compositionFn,
					lex,
					terminalsMap,
					rng,
					trueSentences
				);

			// Select a single string from the distribution
			// either the ARGMAX or SAMPLE based on informativity
			if (sentencesDist.has_value()) {

				// get the sentences and the distribution
				const t_BTC_vec& sentences = 
					std::get<0>(*sentencesDist);
				const std::vector<double>& weights = 
					std::get<1>(*sentencesDist);

				int index;
				if (mode == productionMode::ARGMAX) {
					auto max_index = std::max_element(
						weights.begin(),
						weights.end()
					);
					// get the index of the most probable sentence
					index = std::distance(weights.begin(), max_index);
				} else if (mode == productionMode::SAMPLE) {
					// define distribution from the weights
					t_discr_dist dist = 
						t_discr_dist(weights.begin(),weights.end());
					// sample from the distribution
					index = dist(rng);
				} else {
					throw std::runtime_error("Unknown production mode");
				}

				std::string producedString = sentences[index]->toSExpression();
				producedSentences.push_back(typename Hyp::datum_t{
					context, 
					producedString,
					1.0
				});
				utilities.push_back(weights[index]);
			} else {
				throw std::runtime_error("No data produced");
			}
		}

		auto vec = countUniqueElements<double>(utilities);
		std::cout << std::endl;
		std::cout << "Utilities counts" << std::endl;
		for (const auto& [loglik, count] : vec) {
			std::cout << loglik << " : " << count << std::endl;
		}
	
		return std::make_tuple(producedSentences, utilities);
	}

	// Goes from a sentence to the probability
	// that each element in the context is a target
	std::vector<double> interpret(
			// The agent sees a full sentence
			const std::unique_ptr<BTC>& s,
			// we need the context but don't look at target value
			t_context observedC,
			t_BTC_compose compositionFn
		) const {
			
		// get sentence meaning
		t_t_M meaning = std::get<t_t_M>(s->compose(compositionFn));
		
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
					/* std::cout << " TRUE "; */
				} else {
					/* std::cout << " FALSE"; */
				}
			} catch (PresuppositionFailure& e) {
				// If the sentence presupposes something
				// that is not true of the context,
				// then the context can be ignored.
				/* std::cout << " PFAIL"; */
			}
		}
		// normalize the probabilities
		// (i.e. divide by the number of 
		// possible contexts given sentence)
		for (size_t i = 0; i < probs.size(); i++) {
			probs[i] /= numTrue;
		}
		return probs;
	}

	std::vector<double> interpret(
			std::string s,
			t_context observedC
		) const {
		// by default, use the chosen hypothesis
		LexicalSemantics lex = this->getHypothesis().getLexicon();
		// get the sentence
		std::unique_ptr<BTC> sentence = BTC::fromSExpression(s, lex);
		return interpret(
			sentence,
			observedC,
			this->getHypothesis().getCompositionF()
		);
	}
	
	void setHypothesis(Hyp h){
		if (hasChosenHyp) {
			std::cout << "WARNING: Overwriting chosen hypothesis!" << std::endl;
		}
		chosenHyp = h;
		hasChosenHyp = true;
	}

	Hyp getHypothesis() const {
		assert(hasChosenHyp&&"No hypothesis has been chosen!");
		return chosenHyp;
	}

	// This function takes a lexical semantics
	// and returns a map from each type in t_meaning
	// to the lexical entries that have that type
	t_terminalsMap generateTerminalsMap(LexicalSemantics& lex) const {
		t_terminalsMap tmap;
		// type of meaning a string
		for (auto&& [word, meaning] : lex) {
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
	t_cfgMap generateCFGMap(t_BTC_compose& compositionFn) const {

		// Map from each type to the types 
		// that can be composed with it
		t_cfgMap cfgMap;

		// List of all possible types in t_meaning
		// and give an example of each
		// TODO: Make this more elegant
		// NOTE: the fact that the utterance search
		// is restricted by the CFG defined in this function
		// means that whether the composition function 
		// returns Empty{} or not
		// (i.e., whether the nodes can be composed)
		// must depend *only* on the type
		std::vector<t_meaning> types = {
			t_e_M{},
			t_t_M{}, 
			t_UC_M{},
			t_BC_M{},
			t_TC_M{},
			t_IV_M{},
			t_DP_M{},
			t_TV_M{},
			t_Q_M{},
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
	// and returns a vector of BTCs that are true in that context,
	// by searching the space of utterances.
	// This function is part of the agent because it encodes the way
	// the agent searches the space of utterances.
	// Excludes BTCs that are:
	// - false in context
	// - not valid (i.e. have a type that can't be composed)
	// - have a presupposition failure
	t_BTC_vec generateRandomBTCsWithEvaluation(
			t_context context,
			t_BTC_compose compositionFn,
			LexicalSemantics& lex,
			t_terminalsMap& terminalsMap,
			std::mt19937& rng
		) const {

        std::vector<std::unique_ptr<BTC>> validBTCs;
        std::set<std::string> evaluatedTrees;
        std::set<std::string> invalidTrees;

		t_cfgMap cfgMap = this->generateCFGMap(compositionFn);

		// loop for nSamples
		for (int i = 0; i < this->nSamples; i++) {

			// Generate a random tree encoding a proposition
			// (function from a context to a bool)
            std::optional<std::unique_ptr<BTC>> maybeBtc = generateRandomTree(
				"<s,t>",
				this->initialMaxDepth,
				cfgMap,
				lex,
				terminalsMap,
				rng
			);

			if (!maybeBtc.has_value()) {
				// If the tree is invalid,
				// e.g., because of presupposition failure,
				// skip this iteration
				continue;
			} else {

				std::unique_ptr<BTC> btc = std::move(maybeBtc.value());

				// Convert it to an S-expression to check
				// if it has already been evaluated
				std::string treeRep = btc->toSExpression();

				// If the tree has already been evaluated
				// or if it has been marked as invalid skip it
				if (
					evaluatedTrees.find(treeRep) != evaluatedTrees.end() 
					||
					invalidTrees.find(treeRep) != invalidTrees.end()) {
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
							return t_extension(result_M(context));
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
					evaluatedTrees.insert(treeRep);
					validBTCs.push_back(std::move(btc));
					// Print the tree
					/* std::cout << "\n" << treeRep << std::endl; */
				// if it holds a bool but it's false
				} else if (std::holds_alternative<t_t>(extension)) {
					// print "false" to help with debugging
					/* std::cout << "false" << std::endl; */
					// Print the tree
					/* std::cout << "\n" << treeRep << std::endl; */
					invalidTrees.insert(treeRep);
				} else {
					/* std::cout << "\n" << treeRep << std::endl; */
					invalidTrees.insert(treeRep);
				}
			}
		}

		return validBTCs;
	}

	t_BTC_vec enumerateBTCsWithEvaluation(
		const std::string& typeName,
        int maxDepth,
        const t_cfgMap& cfgMap,
        const LexicalSemantics& lex,
        const t_terminalsMap& terminalsMap
	) const {

		std::vector<std::unique_ptr<BTC>> trees;

		// Base case: If maxDepth < 0, no trees can be created
		if (maxDepth < 0) {
			return trees;
		}

		// If typeName exists in the terminalsMap, create leaf nodes
		auto terminals = terminalsMap.find(typeName);
		if (terminals != terminalsMap.end()) {
			for (const auto& terminal : terminals->second) {
				trees.push_back(std::make_unique<BTC>(
					lex.at(terminal), terminal
				));
			}
		}

		// If typeName exists in the cfgMap, create non-leaf nodes
		auto cfgIt = cfgMap.find(typeName);
		if (cfgIt != cfgMap.end()) {
			for (const auto& childTypes : cfgIt->second) {

				const std::string& leftType = std::get<0>(childTypes);
				const std::string& rightType = std::get<1>(childTypes);

				// Recursively enumerate left and right children
				auto leftTrees = enumerateBTCsWithEvaluation(
					leftType,
					maxDepth - 1,
					cfgMap,
					lex,
					terminalsMap
				);
				auto rightTrees = enumerateBTCsWithEvaluation(
					rightType,
					maxDepth - 1,
					cfgMap,
					lex,
					terminalsMap
				);

				// Combine left and right children into non-leaf nodes
				for (const auto& leftTree : leftTrees) {
					for (const auto& rightTree : rightTrees) {
						std::string sExpr = "( " 
							+ leftTree->toSExpression() 
							+ " " 
							+ rightTree->toSExpression() 
							+ " )";
						trees.push_back(BTC::fromSExpression(sExpr, lex));
					}
				}
			}
		}

		return trees;
	}

	t_BTC_vec enumerateSentences(
				t_BTC_compose compositionFn,
				LexicalSemantics& lex,
				t_terminalsMap& terminalsMap,
				size_t searchDepth = 2
			) const {

		t_BTC_vec sentences;

		t_cfgMap cfgMap = this->generateCFGMap(compositionFn);

		auto possibleUtts = enumerateBTCsWithEvaluation(
			"<s,t>",
			searchDepth,
			cfgMap,
			lex,
			terminalsMap
		);

		// filter out the sentences that do not contain
		// 'target' or 'distractor'
		for (auto& utt : possibleUtts) {
			// if the sentence does not contain 'target'
			// or 'distractor', then we can ignore it
			// since it does not give us any information
			// about what's a target and what's a distractor.
			// Remove these sentences from the set of possible
			// utterances.
			if (!utt->contains("target") && !utt->contains("distractor")) { 
				continue; 
			} else {
				// append sentence to sentences
				sentences.push_back(std::move(utt));
			}
		}

		return sentences;
	}

	t_BTC_vec selectTrueSentences(
				const t_context& c,
				const t_BTC_compose& compositionFn,
				t_BTC_vec possibleUtts
			) const {

		t_BTC_vec sentences = {};

		// find the sentences that are true of the context
		for (auto& utt : possibleUtts) {

			// Evaluate the tree into a meaning of type <s,t>
			t_meaning result = utt->compose(compositionFn);

			// Apply the result to the context to get a bool
			t_extension extension = std::visit(
				[&c](auto&& result_M) {
					// All meanings are functions from 
					// contexts to something in t_extension
					// NOTE: Need to explicitly cast to t_extension
					// rather than directly return
					try {
						return t_extension(result_M(c));
					} catch (PresuppositionFailure& e) {
						// If there is a presupposition failure
						// return Empty{}
						return t_extension(Empty{});
					}
				},
				result
			);

			if (
				std::holds_alternative<t_t>(extension) && 
				std::get<t_t>(extension)
			) {
				sentences.push_back(std::move(utt));
			} 
		}
		
		return sentences;
	}
};
