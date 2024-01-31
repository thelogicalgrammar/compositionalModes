# pragma once
#include <future>

std::vector<double> softmax(const std::vector<double>& V, double alpha=3.0)
{
    std::vector<double> out;
    double sum(0.0);
    for(double v:V) {
        sum=sum+exp(v*alpha);
    }
    for(double v:V) {
        out.push_back(exp(v*alpha)/sum);
    }
    return out;
}

// Define a structure to hold the results for each agent
template< typename ILHyp >
struct AgentResult {
    Agent<ILHyp> child;
    typename ILHyp::data_t childCommData;
    typename ILHyp::data_t childLearningData;
    double commAcc;
};

// This function encapsulates the processing for a single agent
template< typename ILHyp >
AgentResult<ILHyp> processAgent(
		const Agent<ILHyp>& parent,
		size_t nObs,
		size_t cSize,
		double pRight,
		double pMutation,
		unsigned int seed
	) {

	// Create a new RNG instance with the given seed
    std::mt19937 local_rng(seed);

	// create a new agent
	Agent<ILHyp> child{};

	// Parent produces data for learning
	typename ILHyp::data_t learningData = 
		parent.produceData(cSize, nObs, local_rng, pRight);

	// a function to filter the data produced by each agent
	// before it's observed by the next generation.
	// This can make the simulation faster
	typename ILHyp::data_t filteredLearningData = 
		ILHyp::dataFilter(learningData);

	std::cout << "filtered learning data:" << std::endl;
	std::cout << filteredLearningData << std::endl;
	std::cout << std::endl;

	// the new agent learns from the parent
	// (the `top` attribute is updated)
	child.learn(filteredLearningData);
	
	// the new agent picks a hypothesis from `top`
	// (the `chosenHyp` attribute is updated)
	child.pickHypothesis(local_rng);
	
	// Mutate the agent's hypothesis with a certain probability
	// NOTE: This should happen before data is produced
	// and communicative accuracy calculated
	std::uniform_real_distribution<double> t_dist(0.0,1.0);
	double t = t_dist(local_rng);
	if (t < pMutation) {
		child.mutate();
	}

	// produce data for computing accuracy
	auto commData = child.produceData(cSize, nObs, local_rng, pRight);
	
	// the new agent computes its communicative accuracy
	double commAcc = child.communicativeAccuracy(commData, local_rng);

    AgentResult<ILHyp> result;
	result.child = child;
	result.childCommData = commData;
	result.childLearningData = learningData;
	result.commAcc = commAcc;

    return result;
}

template< typename ILHyp >
std::tuple<
	std::vector<Agent<ILHyp>>, 
	std::vector<typename ILHyp::data_t>,
	std::vector<double>
>
runGeneration0(
		size_t nAgents,
		size_t nObs,
		size_t cSize,
		double pRight,
		std::mt19937& rng,
		HypothesisInit initType
	) {

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
				// communication data
				agentdata = agent.randomInitializationFromHypothesis(
					rng, cs, pRight);
				commAcc = agent.communicativeAccuracy(
					agentdata, rng);
				break;

			case HypothesisInit::RANDOMSTRING: {
				// The hypothesis is not used in this case
				agent = Agent< ILHyp >();
				ILHyp hyp{};
				LexicalSemantics lex = hyp.getLexicon();
				agentdata = agent.randomInitializationFromString(
					rng, cs, lex, pRight);
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
	
	return std::make_tuple(agents, data, commAccs);
}

template< typename ILHyp >
std::tuple<
	std::vector<std::vector<Agent<ILHyp>>>,
	std::vector<std::vector<typename ILHyp::data_t>>,
	std::vector<std::vector<typename ILHyp::data_t>>,
	twoDDouble
>
runIL(
		std::mt19937& rng,
		size_t nGenerations,
		size_t nAgents,
		// Both to estimate communicative accuracy and to
		// learn from the data produced by the agents
		size_t nObs,
		size_t cSize,
		double pRight,
		double pMutation,
		double commSelectionStrength,
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
	j["commSelectionStrength"] = commSelectionStrength;
	jfile << j.dump() << std::endl;

	// communicative accuracy in each generation for each agent 
	// dims (nGenerations, nAgents)
	twoDDouble allCommAccs;
	// data produced in each generation by each agent 
	// for determining communicative accuracy
	// dims (nGenerations, nAgents)
	std::vector<std::vector<typename ILHyp::data_t>> allCommunicationData;
	// Data seen by the agent, produced by the parent
	std::vector<std::vector<typename ILHyp::data_t>> allLearningData;
	// create generations
	std::vector<std::vector<Agent<ILHyp>>> generations;

	auto[agents,commData,commAccs] = runGeneration0<ILHyp>(
		nAgents,
		nObs,
		cSize,
		pRight,
		rng,
		initType
	);

	saveGeneration<ILHyp,Agent<ILHyp>>(
		dir / ("generation_0.json"),
		0,
		agents,
		commData,
		// Empty vector since there is no parent to learn from
		std::nullopt,
		commAccs,
		// vector of "parent indices" is nonsense for first generation
		std::nullopt
	);

	// add agents of first generation
	generations.push_back( agents );
	// add data produced by the first generation
	allCommunicationData.push_back( commData );
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
			
		// get agents, data, and commacc from last generation
		std::vector<Agent<ILHyp>> parents = generations.back();
		std::vector< typename ILHyp::data_t> parentsCommData = 
			allCommunicationData.back();
		commAccs = allCommAccs.back();

		// Go from communicative accuracy to probabilities
		std::vector<double> accDist = softmax(commAccs, commSelectionStrength);
		std::cout << "Communicative accuracy distribution: " << std::endl;
		for( auto& acc : accDist ){
			std::cout << acc << " ";
		}
		std::cout << std::endl;

		// define a categorical distribution
		// with probabilities proportional to communicative accuracy
		t_discr_dist dist(accDist.begin(), accDist.end());

		// Run agents within a generation in parallel
		std::vector<std::future<AgentResult<ILHyp>>> futures;
		std::vector<size_t> parentIndices;
		for( size_t j = 0; j < nAgents; j++ ){

			// pick a parent based on communicative accuracy
			// by sampling from a categorical distribution
			int parentIndex = dist(rng);
			parentIndices.push_back(parentIndex);
			Agent parent = parents[ parentIndex ];
			unsigned int seed = rng();

			futures.push_back(std::async(
				std::launch::async,
				processAgent<ILHyp>,
				parent,
				nObs,
				cSize,
				pRight,
				pMutation,
				seed
			));
		}

		// Collect results from parallel execution
		std::vector<Agent<ILHyp>> children;
		std::vector<typename ILHyp::data_t> childrenCommData;
		std::vector<typename ILHyp::data_t> childrenLearningData;
		std::vector<double> childrenCommAccs;
		int k = 0;
		for (auto& future : futures) {
			// This blocks until the result is ready
			AgentResult<ILHyp> result = future.get(); 
			children.push_back(result.child);
			childrenCommData.push_back(result.childCommData);
			childrenLearningData.push_back(result.childLearningData);
			childrenCommAccs.push_back(result.commAcc);
			k++;
		}

		// add generation
		generations.push_back( children );
		// add data produced by the generation
		allCommunicationData.push_back( childrenCommData );
		allLearningData.push_back( childrenLearningData );
		// add communicative accuracy of the generation
		allCommAccs.push_back( childrenCommAccs );

		/////// Save generation

		std::filesystem::path filepath = 
			dir / ("generation_" + std::to_string(i) + ".json");

		saveGeneration<ILHyp,Agent<ILHyp>>(
			filepath,
			i,
			children,
			childrenCommData,
			childrenLearningData,
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
	return std::make_tuple(
		generations,
		allCommunicationData,
		allLearningData,
		allCommAccs
	);
}

