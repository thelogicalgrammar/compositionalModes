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
	
	// top stores the top hypotheses we have found
	TopN<Hyp> top{size_t{100}};
	bool hasTop = false;

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
		bool noterminal = 
			terminalsMap.find(typeName) == terminalsMap.end();

		// is typeName not in the cfg map?
		bool nocfg = 
			cfgMap.find(typeName) == cfgMap.end();

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

	Agent() {
		// initialize the maximum depth of utterances
		initialMaxDepth = 4;
		// initialize the number of samples
		nSamples = 10000;
		// initialize the probability of creating a leaf node.
		// for 0.6, the expected depth is ~2.4
		leafProb = 0.6;
		alpha = 3;
		// Scale the size of the utterances when computing complexity
		sizeScaling = 0.2;
	}

	Agent( typename Hyp::Grammar_t* grammar, std::string parseable ) : Agent(){
		std::cout << "Agent: " << parseable << std::endl;
		this->setHypothesis(Hyp(parseable));
	}

	void mutate() {
		// Mutate the agent's hypothesis.
		
		// Propose a variation of the agent's hypothesis
		std::optional<std::pair<Hyp,double>> mutatedData = 
			this->getHypothesis().propose();
		// Get the mutated hypothesis and set it as the agent's hypothesis
		if (mutatedData.has_value()) {
			auto[mutatedHyp,logq] = mutatedData.value();
			// record original
			originalHyp = this->getHypothesis();
			mutated = true;
			// set new
			this->setHypothesis(mutatedHyp);
			std::cout << std::endl;
			std::cout << "Mutated the agent!" << std::endl;
			std::cout 
				<< "original: " 
				<< originalHyp.value().string() 
				<< std::endl;
			std::cout 
				<< "mutated:  " 
				<< mutatedHyp 
				<< std::endl;
			std::cout << std::endl;
		} else {
			// warn that mutation failed
			std::cout << "Mutation failed!" << std::endl;
		}
	}

	TopN<Hyp> getTop() const {
		if (!hasTop) {
			throw std::runtime_error("No top defined!");
		}
		return this->top;
	}

	Hyp::data_t randomInitializationFromString(
			std::mt19937 rng,
			std::vector<t_context> observations,
			LexicalSemantics lex,
			double pRight
		){
		typename Hyp::data_t mydata;
		// generate random combinations of utterances and contexts
		// without a true underlying hypothesis
		auto lexNames = lex.getNames();
		for(size_t i=0;i<observations.size();i++){
			t_context c = observations[i];
			// TODO: Sample a random utterance
			std::string utt = BTC::randomSExpression(lexNames, rng);
			typename Hyp::datum_t datum{c,utt,pRight};
			mydata.push_back(datum);
		}
		return mydata;
	}
	
	Hyp::data_t randomInitializationFromHypothesis(
			std::mt19937 rng,
			std::vector<t_context> observations,
			double pRight
		){

		bool allDefined;
		Hyp trueH;
		typename Hyp::data_t mydata;
		// The problem of random initialization in this case
		// is to find a composition function
		// that can compose into sentences of type t 
		// for the observed contexts
		do {
			// reset mydata
			mydata.clear();
			allDefined = true;
			// Sample a random hypothesis
			// containing a composition function
			trueH = Hyp::sample();
			t_BTC_compose trueComposeF = trueH.getCompositionF();
			t_cfgMap cfgMap = this->generateCFGMap(trueComposeF);
			for(size_t i=0;i<observations.size();i++){
				t_context c = observations[i];
				std::optional<t_BTC_dist> produced = 
					this->produce(trueH, c, rng);
				// if the speaker did not produce anything, 
				// break out of the loop
				if(!produced.has_value()){
					allDefined = false;
					break; // Exit the for loop early
				}
				
				// Get the array of utterances and their probabilities
				auto utts = std::move(std::get<0>(produced.value()));
				/* std::cout << "Utterances: " << std::endl; */
				/* // note that u is unique_ptr */
				/* for(auto& u : utts){ */
				/* 	std::cout << u.get()->toSExpression() << std::endl; */
				/* } */
				auto utts_probs = std::get<1>(produced.value());
				// Choose an utterance
				int chosen_index = utts_probs(rng);
				// Get the chosen utterance as an S-expression
				std::string utt = utts[chosen_index].get()->toSExpression();
				typename Hyp::datum_t datum{c,utt,pRight};
				mydata.push_back(datum);
			}
		} while(!allDefined);
		this->setHypothesis(trueH);
		return mydata;
	}

	double communicativeAccuracy(
			Hyp::data_t data,
			std::mt19937 rng
		) const {

		assert(hasChosenHyp&&"No hypothesis has been set yet");

		// print length of data
		std::cout << "Length of data: " << data.size() << std::endl;

		// Communicative accuracy is the total surprisal
		// of the listener over the unobserved data
		// after receiving the signal
		double cumCA = 0;
		// the data_t is a vector of datum_t
		// each datum is a tuple of (context, utterance string)
		for (auto datum : data) {

			// Get the context
			t_context c = datum.input;
			// Get the second element of each tuple in context
			// which says whether the element is a target
			std::vector<bool> targets;
			for (auto elem : c) {
				targets.push_back(std::get<1>(elem));
			}
			// Get the utterance as a string
			std::string utt = datum.output;
			// interpret the utterance, which gives the 
			// P(i is a target|utterance) for each i in context
			std::vector<double> probs = this->interpret(utt, c);
			// compute total surprisal of targetness of elements
			// in the context with the P(target|utterance)
			// NOTE: This is not weighted by the P(target|utt):
			// we are interested in total surprisal for the whole context!
			double CA = 0;
			for (size_t i = 0; i < targets.size(); i++) {
				if (targets[i] == 0) {
					// if the element is not a target
					// then we care about the probability of it being 0
					CA += std::log(1 - probs[i]);
				} else {
					// if the element is a target then 
					// the probability of it being 1
					CA += std::log(probs[i]);
				}
			}
			cumCA += CA;

			std::cout << datum << probs << "	: " << CA << std::endl;

		}
		std::cout << std::endl;
		return cumCA;
	}

	// The agent sees a world of objects
	// They have to produce a signal that 
	// helps the listener identify the targets.
	// Return a distribution over BTCs
	// Encoding (an approximation of)
	// the probability of producing
	// each sentence in the given context
	std::optional<t_BTC_dist> produce(
			t_context c, 
			t_BTC_compose compositionFn,
			LexicalSemantics& lex,
			t_terminalsMap& terminalsMap,
			std::mt19937& rng
		) const {
		// Considers a bunch of random utterances
		// that are all true of the context
		t_BTC_vec sentences = 
			generateRandomBTCsWithEvaluation(
				c, compositionFn, lex, terminalsMap, rng
			);

		if (sentences.size() == 0) {
			// If the composition function cannot produce 
			// sentences that are true of the context
			// (e.g., randomly initialized composition function)
			return std::nullopt;

		} else {

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
				// compute complexity (multiply by sizeScaling
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

	std::optional<std::string> produceSingleString(
			t_context c, 
			std::mt19937& rng
		) const {

		// Use the chosen hypothesis by default
		std::optional<t_BTC_dist> maybedist = produce(c, rng);
		if (maybedist.has_value()) {
			// get the sentences and the distribution
			const t_BTC_vec& sentences = std::get<0>(*maybedist);
			t_discr_dist dist = std::get<1>(*maybedist);
			// sample from the distribution
			int index = dist(rng);
			return std::make_optional(sentences[index]->toSExpression());
		} else {
			return std::nullopt;
		}
	}

	typename Hyp::data_t produceData(
			std::vector<t_context> cs, 
			std::mt19937& rng,
			double pRight
		) const {

		typename Hyp::data_t data;

		// loop over contexts
		for (auto& context : cs) {
			auto maybestring = produceSingleString(context, rng);
			if (maybestring.has_value()) {
				data.push_back(typename Hyp::datum_t{
					context, *maybestring, pRight
				});
			} else {
				throw std::runtime_error("No data produced");
			}
		}
		return data;
	}

	typename Hyp::data_t produceData(
			size_t cSize,
			size_t nObs,
			std::mt19937& rng,
			double pRight
		) const {

		std::vector<t_context> cs = generateContexts(
			cSize,
			nObs,
			rng
		);
		return produceData(cs, rng, pRight);
	}

	// Goes from a sentence to the probability
	// that each element in the context is a target
	std::vector<double> interpret(
			// The agent sees a full sentence
			std::unique_ptr<BTC>& s,
			// we need the context but don't look at target value
			t_context observedC,
			t_BTC_compose compositionFn
		) const {
			
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
			std::unique_ptr<BTC>& s,
			t_context observedC
		) const {
		return interpret(
			s,
			observedC,
			this->getHypothesis().getCompositionF()
		);
	}

	std::vector<double> interpret(
			std::string s,
			t_context observedC
		) const {
		// by default, use the chosen hypothesis
		LexicalSemantics lex = this->getHypothesis().getLexicon();
		// get the sentence
		std::unique_ptr<BTC> sentence = BTC::fromSExpression(s, lex);
		return interpret(sentence, observedC);
	}
	
	void learn(Hyp::data_t data){
		assert(!hasTop&&"Top already defined!");
		
		auto h0 = Hyp::sample();
		ParallelTempering samp(
			h0,
			&data,
			FleetArgs::nchains,
			10.0
		); 

		std::cout << "Running parallel tempering..." << std::endl;

		for(auto& h : samp.run(
			Control(100000)) |
			top | 
			printer(FleetArgs::print)
		){
			// Add hypothesis to top
			top << h;
		}
		hasTop = true;

	}

	void pickHypothesis(std::mt19937& rng){
		// sample one hypothesis from the posterior in top

		TopN<Hyp> localtop = getTop();
		std::cout << "About to pick!" << std::flush;
		assert(localtop.size() > 0);

		// get the posterior
		auto posterior = localtop.values();

		// get the unnormalized probabilities
		std::vector<double> probs;
		for (auto& h : posterior) {
			probs.push_back(h.posterior);
		}

		// create a discrete distribution
		t_discr_dist dist(
			probs.begin(),
			probs.end()
		);

		// sample from the distribution
		int index = dist(rng);

		// set the chosen hypothesis
		auto it = posterior.begin();
		std::advance(it, index);
		this->setHypothesis(*it);
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

	bool wasMutated() const {
		return mutated;
	}

	Hyp getOriginalHypothesis() const {
		assert(mutated&&"Hypothesis was not mutated!");
		return originalHyp.value();
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
	// and returns a vector of BTCs
	// that are true in that context.
	// This belongs to agent because it encodes the way
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
				// If the tree is invalid, skip this iteration
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

};
