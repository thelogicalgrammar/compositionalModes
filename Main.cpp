#include <random>
#include <string>
#include <memory>
#include <variant>
#include <functional>
#include <iostream>
#include <ostream>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <set>
#include <map>
#include <vector>
#include <tuple>
#include <math.h>
#include <bitset>
#include <exception>
#include <cassert>

// Fleet stuff
#include "Functional.h"
#include "Grammar.h"
#include "Singleton.h"
#include "DeterministicLOTHypothesis.h"
#include "TopN.h"
#include "ParallelTempering.h"
#include "Fleet.h"

// Model stuff 

// The various abbreviations for types
#include "types.h"
// Overloaded operators for printing
#include "stream.h"
// The basic lexical semantics. NOTE: No composition function here!
#include "objects/Language.h"
// The world that produces the context
#include "objects/World.h"
// Grammar and Hypothesis for the parts of language to infer
/* #include "LoTs/LoTCompFunc.h" */
#include "LoTs/LoTQuantifiers.h"
// The agents that produce, interpret, and learn
#include "objects/Agent.h"

// Implementation of iterated learning
#include "objects/IL.h"

enum class SimulationType {
	TESTGRAMMAR,
	TESTLEARNING,
	IL
};

int main(int argc, char** argv) {

	// default include to process a bunch of global variables: 
	// mcts_steps, mcc_steps, etc
	Fleet fleet("Modes of composition");

	// Adding some command line options
	// that I need below
	size_t nGenerations = 5;
	size_t nAgents 		= 10;
	size_t nObs 		= 100;
	size_t cSize 		= 5;
	double pRight 		= 0.9999;
	std::string fnameAddition = "";

	fleet.add_option<size_t>(
		"--ngenerations",
		nGenerations,
		"Number of generations"
	);
	fleet.add_option<size_t>(
		"--nagents",
		nAgents,
		"Number of agents"
	);
	fleet.add_option<size_t>(
		"--nobs",
		nObs,
		"Number of observations"
	);
	fleet.add_option<size_t>(
		"--csize",
		cSize,
		"Context size"
	);
	fleet.add_option<double>(
		"--pright",
		pRight,
		"1-probability of noise"
	);
	fleet.add_option<std::string>(
		"--fnameaddition",
		fnameAddition,
		"Addition to the filename"
	);

	// Note that Fleet uses CLI11, so you can add your own options
	fleet.initialize(argc, argv);

    std::random_device rd;
    std::mt19937 rng(rd());

	SimulationType simulationType = SimulationType::IL;
	/* SimulationType simulationType = SimulationType::TESTLEARNING; */

	switch (simulationType) {

		case SimulationType::TESTGRAMMAR: {
			// Test the grammar
			/////// Sample some sentences from the prior
			/// to check that the grammar works as expected
			for (int i = 0; i < 10; i++) {
				std::cout << "Run " << i << std::endl;
				auto sample = grammar.__generate();
				std::cout << sample.string() << std::endl;
				std::cout << sample.parseable() << std::endl;
				std::cout << std::endl;
			}
			break;
		}
		case SimulationType::TESTLEARNING: {

			////// Learn from a specific sentence

			// Here I used the Fleet Grammar's to_parseable 
			// to generate a string representation
			// that the Hypothesis can use to parse 
			// the string into a hypothesis
			// This is a bit of a hack, but it works for now. 
			// It might stop working if the grammar changes!

			/* ( ( X.Q X.R ) ( intersection X.R X.L ) ) */
			std::string quantString = "1:%s | %s | %s | %s;3:( %s %s );7:( %s %s );6:%s.Q;0:X;4:%s.R;0:X;4:( intersection %s %s );4:%s.R;0:X;4:%s.L;0:X;";
			/* ( intEq ( cardinality X.L X.c ) ( cardinality X.R X.c ) ) */
			std::string q1string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;";
			/* ( intGt ( cardinality X.R X.c ) 0 ) */
			std::string q2string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:0;";
			/* ( intGt ( cardinality X.R X.c ) ( cardinality X.L X.c ) ) */
			std::string q3string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X";

			// Sentence to reconstruct
			std::string stringRepr = quantString + q1string + q2string + q3string;
			// Contexts
			std::vector<t_context> cs = generateContexts(
				size_t{5},size_t{100},rng
			);
			// Generate data from the sentence
			Agent<QuantsHypothesis> teacher(&grammar, stringRepr);
			auto data = teacher.produceData(cs, rng, 0.9999);
			// Print data
			for (auto& d : data) {
				std::cout << d << std::endl;
			}
			// Let the agent learn the data
			Agent<QuantsHypothesis> learner{};
			learner.learn(data);
			auto top = learner.getTop();
			top.print();
			// Test the communicative accuracy of the sentence
			learner.pickHypothesis(rng);
			double commAcc = learner.communicativeAccuracy(data, rng);
			std::cout << "Communicative accuracy: " << commAcc << std::endl;

			break;
		}

		case SimulationType::IL: {

			////// Run iterated learning
			
			auto results = runIL<QuantsHypothesis>(
				rng,
				// number of generations
				nGenerations,
				// number of agents
				nAgents,
				// number of datapoints
				nObs,
				// size of contexts
				cSize,
				// 1-noise in learner's signal observation
				pRight,
				// add this to the folder name
				fnameAddition,
				HypothesisInit::HYPOTHESIS
			);

			break;
		}
	}
}
