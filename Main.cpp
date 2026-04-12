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
/* #include "LoTs/LoTQuantifiers.h" */
#include "LoTs/LoTQuantifiersComposite.h"
// Implementation of the tradeoff analysis
#include "objects/Tradeoff.h"
#include "objects/CommAcc.h"

// FOR DEBUGGING
/* #define BACKWARD_HAS_BFD 1 */
/* #include "backward.hpp" */
/* backward::SignalHandling sh; */
// END FOR DEBUGGING

// Names for the possible simulations that we can run in main
enum class SimulationType {
	TESTGRAMMAR,
	MEASURECOMMUNICATION,
	TRADEOFF,
	DEBUG,
	ENTAILMENTS
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
	bool pragmatic 			= false;
	double pTarget 			= 0.5;
	std::string fname 		= "./data/tradeoff/";
	bool exclude_empty_qs 	= false;
	std::string quantifier_string = "";
	std::string filename_quant = "";
	std::string simType = "";

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
	fleet.add_option<bool>(
		"--pragmatic",
		pragmatic,
		"Whether to use pragmatic reasoning in the communicative accuracy computation"
	);
	fleet.add_option<double>(
		"--ptarget",
		pTarget,
		"Probability of each object being a target"
	);
	fleet.add_option<std::string>(
		"--fname",
		fname,
		"Folder name for saving runs"
	);
	fleet.add_option<bool>(
		"--exclude-empty-qs",
		exclude_empty_qs,
		"Whether to exclude systems containing empty quantifiers as possible hypotheses"
	);
	fleet.add_option<std::string>(
		"--quantifier-string",
		quantifier_string,
		"String representation of the quantifiers if mode is MEASURECOMMUNICATION"
	);
	fleet.add_option<std::string>(
		"--filename_quant",
		filename_quant,
		"Filename for saving quantifier runs if MEASURECOMMUNICATION"
	);
	fleet.add_option<std::string>(
		"--simtype",
		simType,
		"Simulation type: TESTGRAMMAR, MEASURECOMMUNICATION, TRADEOFF, DEBUG, ENTAILMENTS"
	);

	// Note that Fleet uses CLI11, so you can add your own options
	fleet.initialize(argc, argv);

	// Decide what simulation to run based on command line argument
	static const std::map<std::string, SimulationType> simTypeMap = {
		{"TESTGRAMMAR", SimulationType::TESTGRAMMAR},
		{"MEASURECOMMUNICATION", SimulationType::MEASURECOMMUNICATION},
		{"TRADEOFF", SimulationType::TRADEOFF},
		{"DEBUG", SimulationType::DEBUG},
		{"ENTAILMENTS", SimulationType::ENTAILMENTS}
	};
	if (simType.empty()) {
		throw std::runtime_error("--simtype is required. Valid options: TESTGRAMMAR, MEASURECOMMUNICATION, TRADEOFF, DEBUG, ENTAILMENTS");
	}
	auto it = simTypeMap.find(simType);
	if (it == simTypeMap.end()) {
		throw std::runtime_error("Invalid simulation type: " + simType + 
			". Valid options: TESTGRAMMAR, MEASURECOMMUNICATION, TRADEOFF, DEBUG, ENTAILMENTS");
	}
	SimulationType simulationType = it->second;

	// Since we use TopN as a finite approximation
	FleetArgs::MCMCYieldOnlyChanges = true;

    std::random_device rd;
    std::mt19937 rng(rd());

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
		
		case SimulationType::MEASURECOMMUNICATION: {

			if (quantifier_string == "") {
				throw std::runtime_error("Quantifier string is required for simulation type MEASURECOMMUNICATION");
			}

			if (filename_quant == "") filename_quant = "quantifier_runs.txt";

			estimateCommAcc<QuantsHypothesis>(
				quantifier_string,
				nObs,
				cSize,
				likelihoodWeight,
				pTarget,
				rng,
				"./data/individual_systems/" + filename_quant + ".txt",
				searchDepth,
				pragmatic
			);

			break;
		}

		case SimulationType::DEBUG: {

			/* t_BTC_vec sentences; */
			/* LexicalSemantics lex = LexicalSemantics(); */
			/* std::unique_ptr<BTC> sentence = BTC::fromSExpression( */
			/* 	/1* "( ( X.Q X.R ) ( intersection X.R X.L ) )", *1/ */
			/* 	"( everything distractor )", */
			/* 	lex */
			/* ); */
			/* sentences.push_back(std::move(sentence)); */


			Agent<QuantsHypothesis> agent = Agent<QuantsHypothesis>(
				"1:%s | %s | %s | %s;3:( %s %s );7:( %s %s );6:%s.Q;0:X;4:%s.L;0:X;4:%s.R;0:X;8:( intEq %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;8:( intGt %s %s );10:( cardinality %s %s );9:%s.L;0:X;2:%s.c;0:X;10:1;8:( intEq %s %s );10:( cardinality %s %s );9:%s.R;0:X;2:%s.c;0:X;10:( cardinality %s %s );9:( setminus %s %s );9:%s.L;0:X;9:%s.R;0:X;2:%s.c;0:X"
			);

			auto data = agent.produceDataFromEnumeration(
				generateContexts(cSize, nObs, rng, pTarget),
				rng
			);

			break;
		}

		case SimulationType::TRADEOFF: {
			
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
			std::filesystem::path jpath = dir / (
				"data_likweight_" + std::to_string(likelihoodWeight) 
				+ "_nobs_" + std::to_string(nObs) 
				+ "_parameters.json"
			);
			std::ofstream jfile(jpath);
			nlohmann::json j;
			j["nobs"] 				= nObs;
			j["csize"] 				= cSize;
			j["likelihoodweight"] 	= likelihoodWeight;
			j["searchdepth"] 		= searchDepth;
			j["pragmatic"] 			= pragmatic;
			j["exclude_empty_qs"] 	= exclude_empty_qs;
			j["steps"] 				= FleetArgs::steps;
			j["num_quants"] 		= NUM_QUANTS;
			jfile << j.dump() << std::endl;

			std::filesystem::path datafilepath = dir / "data.txt";
			std::filesystem::path hypfilepath = dir / "hyp.csv";

			initializeHypCSV(hypfilepath);

			/* TopN<QuantsHypothesis> results = */ 
			runTradeoffAnalysis<QuantsHypothesis>(
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
					pTarget,
					pragmatic,
					datafilepath,
					hypfilepath,
					exclude_empty_qs
				);

			// save results to file with various params
			/* std::filesystem::path filepath = */ 
			/* 	dir / ( */
			/* 		"likweight_" + std::to_string(likelihoodWeight) */ 
			/* 		+ "_nobs_" + std::to_string(nObs) */ 
			/* 		+ ".json" */
			/* 	); */

			/* saveResults<QuantsHypothesis>(filepath, results); */

			break;
		}

		case SimulationType::ENTAILMENTS: {

			// NOT IMPLEMENTED YET

			// QuantsHypothesis::setParams(
			// 	nObs,
			// 	cSize,
			// 	likelihoodWeight,
			// 	rng,
			// 	searchDepth,
			// 	pTarget,
			// 	pragmatic,
			// 	exclude_empty_qs
			// );

			// Agent<QuantsHypothesis> agent{quantifier_string};

			// // get everything from the trueHyp
			// LexicalSemantics lex 		= agent.getHypothesis().getLexicon();
			// t_terminalsMap terminalsMap = agent.generateTerminalsMap(lex);
			// t_BTC_compose compositionFn = agent.getHypothesis().getCompositionF();
	
			// // Find *all* sentences (true and false) given the grammar
			// // up to a certain depth
			// t_BTC_vec allSentences = enumerateSentences(
			// 		compositionFn,
			// 		lex,
			// 		terminalsMap,
			// 		searchDepth
			// 	);


			break;
		}
	}
}
