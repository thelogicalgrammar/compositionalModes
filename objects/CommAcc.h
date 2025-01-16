# pragma once

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void printProgress(double percentage) {
    int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r\033[K%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

template <typename Hyp>
void estimateCommAcc(
		std::string stringRepr,
		size_t nObs,
		size_t cSize,
		double likelihoodWeight,
		std::mt19937& rng,
		const std::string& fname,
		size_t searchDepth = 2
	){

	Agent<Hyp> agent(stringRepr);

	Hyp::setParams(nObs, cSize, likelihoodWeight, rng, searchDepth);

	std::cout 
		<< "Value: " 
		<< agent.getHypothesis().string() 
	<< std::endl;

	// store 100 runs 
	std::vector<double> logliks;
	for (int i = 0; i < 500; i++) {

		std::cout << "Generating contexts " << i << std::endl;
		std::vector<t_context> cs = generateContexts(cSize, nObs, rng);

		// produce data for approximating communicative accuracy
		std::cout << "Producing data from enumeration" << std::endl;
		typename Hyp::data_t commData = agent.produceDataFromEnumeration(
				cs, rng, searchDepth);
		/* typename Hyp::data_t commData = agent.produceData( */
		/* 		cs, rng, searchDepth); */

		// the new agent computes its communicative accuracy
		std::cout << "Calculating communicative accuracy" << std::endl;
		double commAcc = agent.communicativeAccuracy(commData, rng);

		// The likelihood is the weighted sum of the communicative accuracy
		// and the simplicity of the language.
		// Note that commAcc is already the log of a probability
		double loglik = log(likelihoodWeight) * commAcc;
		
		// add to the logliks
		logliks.push_back(loglik);

		// print progress
		printProgress(static_cast<double>(i)/500);
	}
	
	// store the logliks in a file
    std::ofstream file;
    file.open(fname);
    for (const auto& loglik : logliks) {
        file << loglik << std::endl;
    }
    file.close();
}
