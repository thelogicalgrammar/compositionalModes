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
		bool exclude_empty = false
	){
	// Make sure that this is the same as compute_likelihood

	if (exclude_empty) {
		// split the hypothesis into the composition function and the quantifiers
		auto parts = split(stringRepr, '|');
		// get the quantifiers
		std::vector<std::string> quantifiers(parts.begin() + 1, parts.end());
		// check that each quantifier contains X.L or X.R
		// if not, return -infinity
		for (auto& q : quantifiers) {
			if (q.find("X.L") == std::string::npos && q.find("X.R") == std::string::npos) {
				std::cout << "Excluding hypothesis: " << stringRepr << std::endl;
				return;
			}
		}
	}

	Agent<Hyp> agent(stringRepr);

	Hyp::setParams(
		nObs,
		cSize,
		likelihoodWeight,
		rng,
		searchDepth,
		pTarget,
		pragmatic,
		exclude_empty
	);

	// store runs 
	std::vector<double> logliks;
	std::vector<std::string> utilities_counts_strs;
	// in case we want to run multiple times
	int nruns = 1;
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
		// calculate the average utility
		for (size_t j = 0; j < utilities.size(); j++) {commAcc += utilities[j];}
		commAcc /= nObs;

		// produce a dict with utilities and counts
		auto utilities_counts = countUniqueElements<double>(utilities);
		// accumulate the utilities into a string
		std::string utilities_counts_str = "";
		for (size_t j = 0; j < utilities_counts.size(); j++) {
			utilities_counts_str += 
				std::to_string(utilities_counts[j].first) 
				+ " : " 
				+ std::to_string(utilities_counts[j].second) 
				+ "\n";
		}
		utilities_counts_strs.push_back(utilities_counts_str);

		double loglik = likelihoodWeight * commAcc;
		logliks.push_back(loglik);
		printProgress(static_cast<double>(i+1)/nruns);
	}

	// store the logliks in a file
    std::ofstream file;
    file.open(fname);
    for (size_t i = 0; i < logliks.size(); i++) {
        file << stringRepr << "||" << logliks[i] << "||" << utilities_counts_strs[i] << std::endl;
    }
    file.close();
}
