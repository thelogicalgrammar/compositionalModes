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
		double pTarget,
		std::mt19937& rng,
		const std::string& fname,
		size_t searchDepth = 2,
		bool pragmatic = false,
		bool exclude_empty_qs = false
	){
	// Make sure that this is the same as compute_likelihood

	Agent<Hyp> agent(stringRepr);

	Hyp::setParams(
		nObs,
		cSize,
		likelihoodWeight,
		rng,
		searchDepth,
		pTarget,
		pragmatic,
		exclude_empty_qs
	);

	// store runs 
	std::vector<double> logliks;
	int nruns = 20;
	for (int i = 0; i < nruns; i++) {

		std::vector<t_context> cs = generateContexts(cSize, nObs, rng, pTarget);
		auto commData = agent.produceDataFromEnumeration(
			cs, rng, searchDepth, pragmatic);
		// data is a vector of datum_t
		auto data = std::get<0>(commData);
		// utilities is a vector of doubles 
		auto utilities = std::get<1>(commData);
		// print the data
		std::cout << std::endl;
		std::cout 
			<< "Value: " 
			<< agent.getHypothesis().string() 
			<< std::endl;
		for (size_t h = 0; h < data.size(); h++) {
			std::cout << data[h] << " Utility: " << utilities[h] << std::endl;
		}
		std::cout << std::endl;
		double commAcc = 0;
		// sum the utilities
		for (size_t j = 0; j < utilities.size(); j++) {commAcc += utilities[j];}
		commAcc /= nObs;
		double loglik = likelihoodWeight * commAcc;
		logliks.push_back(loglik);
		printProgress(static_cast<double>(i+1)/nruns);
	}

	// store the logliks in a file
    std::ofstream file;
    file.open(fname);
    for (const auto& loglik : logliks) {
        file << loglik << std::endl;
    }
    file.close();
}
