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
// Overloaded operators for printing for the various types
#include "stream.h"
// The basic lexical semantics. NOTE: No composition function here!
#include "objects/language.h"
// The world that produces the context
#include "objects/world.h"
// The agents that produce, interpret, and learn
#include "objects/agent.h"
// Grammar and Hypothesis for the parts of language to infer
// INCLUDE ONLY THE ONE YOU NEED
/* #include "LoTs/LoTCompFunc.h" */
#include "LoTs/LoTQuantifiers.h"
// Implementation of the tradeoff analysis
#include "objects/Tradeoff.h"
#include "objects/CommAcc.h"

// Names for the possible simulations that we can run in main
enum class SimulationType {
	TESTGRAMMAR,
	TESTCOMMUNICATION,
	TRADEOFF
};

int main(int argc, char** argv) {

	// default include to process a bunch of global variables: 
	// mcts_steps, mcc_steps, etc
	Fleet fleet("Modes of composition");

	// Adding some command line options that I need below

	size_t nObs 			= 50;
	size_t cSize 			= 5;
	double likelihoodWeight = 1;
	double searchDepth 		= 2;
	std::string fname 		= "./data/tradeoff/";

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
		"--likelihoodweight",
		likelihoodWeight,
		"Weight of communicative accuracy in tradeoff"
	);
	fleet.add_option<double>(
		"--searchdepth",
		searchDepth,
		"Maximum depth of signals when enumerating utterances to estimate communicative accuracy"
	);
	fleet.add_option<std::string>(
		"--fname",
		fname,
		"Folder name for saving runs"
	);
	
	// Note that Fleet uses CLI11, so you can add your own options
	fleet.initialize(argc, argv);

	// Since we use TopN as a finite approximation
	FleetArgs::MCMCYieldOnlyChanges = true;

    std::random_device rd;
    std::mt19937 rng(rd());

	// Decide what simulation to run
	/* SimulationType simulationType = SimulationType::TESTCOMMUNICATION; */
	/* SimulationType simulationType = SimulationType::TESTGRAMMAR; */
	SimulationType simulationType = SimulationType::TRADEOFF;

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
		
		case SimulationType::TESTCOMMUNICATION: {

			// High accuracy quantifiers 
			/* ( ( X.Q X.R ) ( intersection X.R X.L ) ) */
			std::string highQuantString = "1:%s | %s | %s | %s;3:( %s %s );7:( %s %s );6:%s.Q;0:X;4:%s.R;0:X;4:( intersection %s %s );4:%s.R;0:X;4:%s.L;0:X;";
			/* ( intEq ( cardinality X.L X.c ) ( cardinality X.R X.c ) ) */
			std::string highQ1string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;";
			/* ( intGt ( cardinality X.R X.c ) 0 ) */
			std::string highQ2string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:0;";
			/* ( intGt ( cardinality X.R X.c ) ( cardinality X.L X.c ) ) */
			std::string highQ3string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X";

			// Sentence to reconstruct
			std::string highStringRepr = 
				highQuantString + highQ1string + highQ2string + highQ3string;

			estimateCommAcc<QuantsHypothesis>(
				highStringRepr,
				nObs,
				cSize,
				likelihoodWeight,
				rng,
				"./data/commAccHigh.txt"
			);
			
			// MediumHigh accuracy quantifiers 
			/* ( ( X.Q X.R ) X.L ) */
			std::string mediumHighQuantString = "1:%s | %s | %s | %s;3:( %s %s );7:( %s %s );6:%s.Q;0:X;4:%s.R;0:X;4:%s.L;0:X;";
			/* ( intEq ( cardinality X.L X.c ) ( cardinality X.R X.c ) ) */
			std::string mediumHighQ1string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;";
			/* ( intGt ( cardinality X.R X.c ) 0 ) */
			std::string mediumHighQ2string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:0;";
			/* ( intGt ( cardinality X.R X.c ) ( cardinality X.L X.c ) ) */
			std::string mediumHighQ3string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X";

			// Sentence to reconstruct
			std::string mediumHighStringRepr = 
				mediumHighQuantString + mediumHighQ1string + mediumHighQ2string + mediumHighQ3string;

			estimateCommAcc<QuantsHypothesis>(
				mediumHighStringRepr,
				nObs,
				cSize,
				likelihoodWeight,
				rng,
				"./data/commAccMediumHigh.txt"
			);

			// MediumLow accuracy quantifiers 
			/* ( ( X.Q X.R ) X.R ) */
			std::string mediumLowQuantString = "1:%s | %s | %s | %s;3:( %s %s );7:( %s %s );6:%s.Q;0:X;4:%s.R;0:X;4:%s.R;0:X;";
			/* ( intEq ( cardinality X.L X.c ) ( cardinality X.R X.c ) ) */
			std::string mediumLowQ1string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;";
			/* ( intGt ( cardinality X.R X.c ) 0 ) */
			std::string mediumLowQ2string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:0;";
			/* ( intGt ( cardinality X.R X.c ) ( cardinality X.L X.c ) ) */
			std::string mediumLowQ3string = "8:( intGt %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X";

			// Sentence to reconstruct
			std::string mediumLowStringRepr = 
				mediumLowQuantString + mediumLowQ1string + mediumLowQ2string + mediumLowQ3string;

			estimateCommAcc<QuantsHypothesis>(
				mediumLowStringRepr,
				nObs,
				cSize,
				likelihoodWeight,
				rng,
				"./data/commAccMediumLow.txt"
			);

			// Low accuracy quantifiers 
			/* ( intGt 0 1 ) */
			std::string LowQuantString = "1:%s | %s | %s | %s;3:( intGt %s %s );5:0;5:1;";
			/* ( intEq ( cardinality X.R X.c ) ( cardinality X.L X.c ) ) */
			std::string LowQ1string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;";
			std::string LowQ2string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;";
			std::string LowQ3string = "8:( intEq %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;";

			// Sentence to reconstruct
			std::string lowStringRepr = 
				LowQuantString + LowQ1string + LowQ2string + LowQ3string;

			estimateCommAcc<QuantsHypothesis>(
				lowStringRepr,
				nObs,
				cSize,
				likelihoodWeight,
				rng,
				"./data/commAccLow.txt"
			);

			break;
		}

		case SimulationType::TRADEOFF: {

			TopN<QuantsHypothesis> results = runTradeoffAnalysis<QuantsHypothesis>(
				// number of samples to estimate communicative accuracy
				nObs,
				// size of contexts
				cSize,
				// likelihoodWeight
				likelihoodWeight,
				// seed
				rng,
				// search depth
				searchDepth,
				// add this to the folder name 
				fname
			);

			// Determine folder name and create
			auto dir = std::filesystem::path(fname);

			try {
				// create directory if it doesn't exist
				std::filesystem::create_directories(dir);
			} catch (const std::filesystem::filesystem_error& e) {
				std::cerr 
					<< "Error while creating dir" 
					<< e.what() 
					<< std::endl;
			}

			// save parameters to json file
			std::filesystem::path jpath = dir / "parameters.json";
			std::ofstream jfile(jpath);
			nlohmann::json j;
			j["nobs"] = nObs;
			j["csize"] = cSize;
			j["likelihoodweight"] = likelihoodWeight;
			j["searchdepth"] = searchDepth;
			jfile << j.dump() << std::endl;

			// save results to file with likelihood weight and nObs
			std::filesystem::path filepath = 
				dir / (
					"likweight_" + std::to_string(likelihoodWeight) 
					+ "_nobs_" + std::to_string(nObs) 
					+ ".json"
				);

			saveResults<QuantsHypothesis>(filepath, results);

			break;
		}
	}
}
