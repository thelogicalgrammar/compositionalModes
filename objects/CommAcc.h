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
	int nruns = 10;
	for (int i = 0; i < nruns; i++) {

		std::vector<t_context> cs = generateContexts(cSize, nObs, rng, 0.25);
		typename Hyp::data_t commData = agent.produceDataFromEnumeration(
				cs, rng, searchDepth);
		double commAcc = agent.communicativeAccuracy(commData, rng);
		double loglik = likelihoodWeight * commAcc;
		logliks.push_back(loglik);
		printProgress(static_cast<double>(i)/nruns);
	}
	
	// store the logliks in a file
    std::ofstream file;
    file.open(fname);
    for (const auto& loglik : logliks) {
        file << loglik << std::endl;
    }
    file.close();
}
