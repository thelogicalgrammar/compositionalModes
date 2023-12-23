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


std::optional<std::tuple<MyHypothesis,MyHypothesis::data_t>>
initialProduceData(
		Agent speaker,
		std::mt19937 rng
	){

	// Sample a random hypothesis
	// containing a composition function
	MyHypothesis trueH = MyHypothesis::sample();
	t_BTC_compose trueComposeF = 
		[&trueH](t_meaning m1, t_meaning m2) -> t_meaning {
			// put the meanings in a tuple
			t_input m = std::make_tuple(m1,m2);
			return trueH.call(m);
		};

	t_cfgMap cfgMap = speaker.generateCFGMap(
		trueComposeF
	);

	std::cout << "CFG map:" << std::endl;
	std::cout << cfgMap << std::endl;

	std::cout << "Hypothesis: " << std::endl;
	std::cout << trueH << std::endl;
	std::cout << std::endl;
	
	MyHypothesis::data_t mydata;
	for(size_t i=0;i<10;i++){

		t_context c = generateContext(5, rng);

		std::optional<t_BTC_dist> produced = speaker.produce(
			c,
			trueComposeF,
			rng
		);

		// if the speaker did not produce anything, 
		// break out of the loop
		if(produced.has_value()){
			// Get the array of utterances and their probabilities
			auto utts = std::move(std::get<0>(produced.value()));
			auto utts_probs = std::get<1>(produced.value());
			int chosen_index = utts_probs(rng);
			// Get the chosen utterance as an S-expression
			std::string utt = utts[chosen_index].get()->toSExpression();
			// get the lexical semantics of the chosen utterance
			LexicalSemantics lexSem = LexicalSemantics();
			std::tuple<t_context, LexicalSemantics> 
				inputTuple(c,lexSem);

			// print the utterance
			std::cout << "Utterance: " << utt << std::endl;
			// print the interpreted utterance
			std::unique_ptr<BTC> tree =
				BTC::fromSExpression(utt, lexSem);
			tree->printTree(lexSem);

			MyHypothesis::datum_t datum = MyHypothesis::datum_t(
				inputTuple,
				utt,
				0.99
			);
			
			mydata.push_back(datum);

		} else {
			return std::nullopt;
		}

	}
	return std::make_tuple(trueH, mydata);
}


int main(int argc, char** argv) {
	
	// default include to process a bunch of global variables: 
	// mcts_steps, mcc_steps, etc
	Fleet fleet("Modes of composition");
	fleet.initialize(argc, argv);

    std::random_device rd;
    std::mt19937 rng(rd());

	///// LEARN A COMPOSITION FUNCTION

	/* t_BTC_compose trueComposeF = COMP_DSL::rapply; */


	Agent speaker = Agent();

	MyHypothesis::data_t mydata;
	MyHypothesis trueH;

	while (true) {
		auto maybedata = initialProduceData(speaker, rng);
		if(maybedata.has_value()){
			trueH = std::get<0>(maybedata.value());
			mydata = std::get<1>(maybedata.value());
			break;
		}
	}

	// top stores the top hypotheses we have found
	TopN<MyHypothesis> top;
	
	auto h0 = MyHypothesis::sample();
	ParallelTempering samp(h0, &mydata, FleetArgs::nchains, 10.0); 
	for(auto& h : samp.run(Control()) | top | printer(FleetArgs::print)) { 
		top << h;
	}
	
	std::cout << "Best hypothesis: " << std::endl;
	// Show the best we've found
	top.print();

}
