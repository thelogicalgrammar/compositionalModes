# pragma once
# include <typeinfo>

// This returns a TopN object with the best N hypotheses
template <typename LangHyp>
TopN<LangHyp> runTradeoffAnalysis(
		size_t nObs,
		size_t cSize,
		double likelihoodWeight,
		std::mt19937& rng,
		size_t searchDepth,
		std::string& fname
	){
	
	LangHyp::setParams(nObs, cSize, likelihoodWeight, rng, searchDepth);

	// TopN object to store the best hypotheses
	TopN<LangHyp> top(size_t{100});

	auto h0 = LangHyp::sample();

	// initialize empty data
	typename LangHyp::data_t emptyData;
	// the last argument is the max temperature
	ParallelTempering<LangHyp> samp(
		h0,
		&emptyData,
		FleetArgs::nchains,
		10.0
	); 

	for(auto& h : samp.run(
		Control(FleetArgs::steps)) | top | printer(FleetArgs::print)
	){
		// Add hypothesis to top
		top << h;
		std::cout << h.string() << std::endl;
	}

	std::cout << "Top hypotheses" << std::endl;

	top.print();

	return top;
}