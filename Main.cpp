#include <random>
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <math.h>
#include <bitset>

// Fleet stuff
#include "Functional.h"
#include "Grammar.h"
#include "Singleton.h"
#include "DeterministicLOTHypothesis.h"
#include "Fleet.h"

// Model stuff
//
// The various abbreviations for types
#include "types.h"
// Overloaded operators for printing
#include "stream.h"
// The language (w/o composition function)
#include "objects/Language.h"
// The world that produces the context
#include "objects/World.h"
// DSL, CompGrammar, CompHypothesis
#include "objects/LoT.h"
// The agents that produce, interpret, and learn
#include "objects/Agent.h"
// The population of agents implementing the IL 
#include "objects/Population.h"


int main(int argc, char** argv) {
	
	// default include to process a bunch of global variables: 
	// mcts_steps, mcc_steps, etc
	Fleet fleet("Modes of composition");
	fleet.initialize(argc, argv);

    std::random_device rd;
    std::mt19937 rng(rd());

	// Set up the context manually
	/* e x0 = std::make_tuple(0, true); */
	/* e x1 = std::make_tuple(-1, true); */
	/* e x2 = std::make_tuple(-6, false); */
	/* e x3 = std::make_tuple(1, false); */
	/* e x4 = std::make_tuple(4, true); */
	/* t_context c = {x0, x1, x2, x3, x4}; */

	t_context c = generateContext(5, rng);

	// print the context
	std::cout << "context: " << c << std::endl;
	
	// the dependency of the meanings on the context
	// is encoded directly in the lexicon
	LexicalSemantics lex = LexicalSemantics();

	/////// Test the lexicon

/* 	auto tree = BTC::fromSExpression( */
/* 		/1* "( ( some target ) positive )", *1/ */
/* 		"( ( l_and ( ( some target ) positive ) ) ( ( some target ) negative ) )", */
/* 		lex */
/* 	); */

/* 	tree -> printTree(lex); */

/* 	t_meaning output = tree -> compose( */
/* 		COMP_DSL::rapply */
/* 	); */

/* 	// Get t_t_M from t_meaning */
/* 	t_t_M output_t = std::get<t_t_M>(output); */

/* 	std::cout << "output: " << output_t(c) << std::endl; */

	Agent speaker = Agent();
	
	// I need to explicitly specify the type of the function
	// because in COMP_DSL is it defined as lambda
	// and the definition of t_BTC_compose is std::function
	// which is not quite the same thing!
	t_BTC_compose rapply = COMP_DSL::rapply;
	t_cfgMap cfg = speaker.generateCFGMap(rapply);

	for (auto& [k, v] : cfg) {
		// print the key and the value
		std::cout << k << " -> ";
		// Each value is a set of pairs of strings
		for (auto& [v1, v2] : v) {
			std::cout << "(" << v1 << ", " << v2 << ") ";
		}
		std::cout << std::endl;
	}
	
	// Test the terminal map
	t_terminalsMap terminals = 
		speaker.generateTerminalsMap();
	std::cout << "Terminals map: " << std::endl;
	std::cout << terminals << std::endl;

    auto btcTrees = speaker.generateRandomBTCsWithEvaluation(
		c,
		rapply, 
		rng
	);

	/* for (auto& tree : btcTrees) { */
	/* 	tree -> printTree(lex); */
	/* 	/1* auto s = tree -> toSExpression(); *1/ */
	/* 	/1* std::cout << s << std::endl; *1/ */
	/* 	std::cout << std::endl; */
	/* } */

	std::cout << "Number of trees: " << 
		btcTrees.size() << std::endl;

	//// PRODUCE AN UTTERANCE
	
	t_BTC_dist produced = speaker.produce(c, rapply, rng);
	
	auto utts = std::move(std::get<0>(produced));
	auto utts_probs = std::get<1>(produced);

	int chosen_index = utts_probs(rng);

	auto& utt = utts[chosen_index];
	utt->printTree(lex);
	std::cout << "size: " << utt->size() << std::endl;

	std::vector<double> probs = speaker.interpret(utt, c, rapply);
	std::cout << "probs: "  << std::endl;
	for (auto& p : probs) {
		std::cout << p << " ";
	}
}
