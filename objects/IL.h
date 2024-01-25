# pragma once
#include <future>

// Define a structure to hold the results for each agent
template< typename ILHyp >
struct AgentResult {
    Agent<ILHyp> child;
    typename ILHyp::data_t childData;
    double commAcc;
};

// This function encapsulates the processing for a single agent
template< typename ILHyp >
AgentResult<ILHyp> processAgent(
		const Agent<ILHyp>& parent,
		const typename ILHyp::data_t& parentdata,
		size_t nObs,
		size_t cSize,
		double pRight,
		unsigned int seed
	) {

	// Create a new RNG instance with the given seed
    std::mt19937 local_rng(seed);

    AgentResult<ILHyp> result;

	// create a new agent
	Agent<ILHyp> child{};

	// the new agent learns from the parent
	// (the `top` attribute is updated)
	child.learn(parentdata);
	// the new agent picks a hypothesis from `top`
	// (the `chosenHyp` attribute is updated)
	child.pickHypothesis(local_rng);

	// produce data for next generation
	typename ILHyp::data_t childData;
	std::vector<t_context> cs = generateContexts(cSize, nObs, local_rng);
	// loop over contexts
	for (auto c : cs) {
		// the new agent produces data
		auto maybeS = child.produceSingleString(c, local_rng);
		// add data to the vector
		if (maybeS.has_value()) {
			std::string stringS = maybeS.value();
			typename ILHyp::datum_t d{c,stringS,pRight};
			childData.push_back(d);
		} else {
			throw std::runtime_error("No data produced");
		}
	}
	// the new agent computes its communicative accuracy
	double commAcc = child.communicativeAccuracy(childData, local_rng);

	result.child = child;
	result.childData = childData;
	result.commAcc = commAcc;

    return result;
}

template< typename ILHyp >
std::tuple<
	std::vector<std::vector<Agent<ILHyp>>>,
	twoDDouble,
	std::vector<std::vector<typename ILHyp::data_t>>
>
runIL(
		std::mt19937& rng,
		size_t nGenerations,
		size_t nAgents,
		size_t nObs,
		size_t cSize,
		double pRight = 0.9,
		std::string fnameAddition = "",
		// the type of initialization 
		// for agents in the first generation
		HypothesisInit initType = HypothesisInit::HYPOTHESIS
	){

	// Define name of directory to store data
	// and create directory if it doesn't exist
	std::filesystem::path dir = "./data";
	dir /= generateUniqueSuffix();
	dir /= fnameAddition;
	std::filesystem::create_directories(dir);

	// save parameters to json file
	std::filesystem::path jpath = dir;
	jpath /= "parameters.json";
	std::ofstream jfile(jpath);
	nlohmann::json j;
	j["nAgents"] = nAgents;
	j["nObs"] = nObs;
	j["cSize"] = cSize;
	j["pRight"] = pRight;
	j["initType"] = initType;
	j["nGenerations"] = nGenerations;
	jfile << j.dump() << std::endl;

	// communicative accuracy in each generation for each agent 
	// dims (nGenerations, nAgents)
	twoDDouble allCommAccs;
	// data produced in each generation by each agent 
	// dims (nGenerations, nAgents)
	std::vector<std::vector<typename ILHyp::data_t>> allData;
	// create generations
	std::vector<std::vector<Agent<ILHyp>>> generations;

	// create vector of agents for first generation
	// and data produced by each agent in first generation
	std::vector<Agent<ILHyp>> agents;
	std::vector<typename ILHyp::data_t> data;
	std::vector<double> commAccs;
	for( size_t i = 0; i < nAgents; i++ ){
		std::vector<t_context> cs = generateContexts(cSize, nObs, rng);
		typename ILHyp::data_t agentdata;
		double commAcc;
		Agent< ILHyp > agent;
		switch( initType ){
			case HypothesisInit::HYPOTHESIS:
				agent = Agent< ILHyp >();
				agentdata = agent.randomInitializationFromHypothesis(rng, cs);
				commAcc = agent.communicativeAccuracy(agentdata, rng);
				break;

			case HypothesisInit::RANDOMSTRING: {
				agent = Agent< ILHyp >();
				ILHyp hyp{};
				LexicalSemantics lex = hyp.getLexicon();
				agentdata = agent.randomInitializationFromString(rng, cs, lex);
				// Since we don't have an actual hypothesis
				commAcc = 1.0;
				break;
			}

			default:
				throw std::runtime_error("Invalid initType");
				break;
		}

		agents.push_back( agent );
		data.push_back( agentdata );
		commAccs.push_back( commAcc );
	}
	saveGeneration<ILHyp,Agent<ILHyp>>(
		dir,
		0,
		agents,
		data,
		commAccs,
		// vector of "parent indices" is nonsense for first generation
		// but has length nAgents
		std::vector<size_t>(nAgents, 0)
	);

	// add agents of first generation
	generations.push_back( agents );
	// add data produced by the first generation
	allData.push_back( data );
	// add communicative accuracy of the first generation
	allCommAccs.push_back( commAccs );

	std::cout << "Starting to run generations" << std::endl;
	std::cout << "Initial communicative accuracy: " << std::endl;
	for( auto& acc : commAccs ){
		std::cout << acc << " ";
	}
	std::cout << std::endl;
	std::cout << "length: " << commAccs.size() << std::endl;

	// run generations 
	for( size_t i = 1; i <= nGenerations; i++ ){

		std::cout << "Starting generation " << i << std::endl;
			
		// get agents from last generation
		std::vector<Agent<ILHyp>> parents = generations.back();
		std::vector< typename ILHyp::data_t> parentData = allData.back();
		commAccs = allCommAccs.back();

		// define a categorical distribution
		// with probabilities proportional to communicative accuracy
		t_discr_dist dist(commAccs.begin(), commAccs.end());

		// Run agents within a generation in parallel
		std::vector<std::future<AgentResult<ILHyp>>> futures;
		std::vector<size_t> parentIndices;
		for( size_t j = 0; j < nAgents; j++ ){

			// pick a parent based on communicative accuracy
			// by sampling from a categorical distribution
			int parentIndex = dist(rng);
			parentIndices.push_back(parentIndex);
			Agent parent = parents[ parentIndex ];
			typename ILHyp::data_t parentdata = parentData[ parentIndex ];

			unsigned int seed = rng();

			futures.push_back(std::async(
				std::launch::async,
				processAgent<ILHyp>,
				parent,
				parentdata,
				nObs,
				cSize,
				pRight,
				seed
			));
		}

		// Collect results from parallel execution
		std::vector<Agent<ILHyp>> children;
		std::vector<typename ILHyp::data_t> childrenData;
		std::vector<double> childrenCommAccs;
		int k = 0;
		for (auto& future : futures) {
			// This blocks until the result is ready
			AgentResult<ILHyp> result = future.get(); 
			children.push_back(result.child);
			childrenData.push_back(result.childData);
			childrenCommAccs.push_back(result.commAcc);
			k++;
		}

		// add generation
		generations.push_back( children );
		// add data produced by the generation
		allData.push_back( childrenData );
		// add communicative accuracy of the generation
		allCommAccs.push_back( childrenCommAccs );

		/////// Save generation

		std::filesystem::path filepath = 
			dir / ("generation_" + std::to_string(i) + ".json");

		saveGeneration<ILHyp,Agent<ILHyp>>(
			filepath,
			i,
			children,
			childrenData,
			childrenCommAccs,
			parentIndices
		);

		//// Print top hypotheses for each agent
		for(auto& child : children){
			std::cout << "Printing child n. " << i << std::endl;
			TopN<ILHyp> top = child.getTop();
			top.print();
			std::cout << std::endl;
		}

		// print FleetStatistics::depth_exceptions
		std::cout 
			<< "depth exceptions: " 
			<< FleetStatistics::depth_exceptions 
			<< std::endl;
		FleetStatistics::depth_exceptions = 0;
			std::cout << "Generation " << i << " done" << std::endl;
	}

	// return generations
	return std::make_tuple( generations, allCommAccs, allData );
}

