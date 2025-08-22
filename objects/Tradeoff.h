#pragma once
#include <typeinfo>

// This returns a TopN object with the best N hypotheses
template <typename LangHyp>
void runTradeoffAnalysis(
		const size_t nObs,
		const size_t cSize,
		const double likelihoodWeight,
		std::mt19937& rng,
		const size_t searchDepth,
		const double pTarget,
		const bool pragmatic,
		const std::filesystem::path& datafilepath,
		const std::filesystem::path& hypfilepath,
		const bool exclude_empty_qs
	){

	// Set the parameters for the language hypothesis
	// This is a static method of the LangHyp class
	LangHyp::setParams(
		nObs,
		cSize,
		likelihoodWeight,
		rng,
		searchDepth,
		pTarget,
		pragmatic,
		exclude_empty_qs
	);

	// TopN object to store the best hypotheses
	TopN<LangHyp> top(size_t{FleetArgs::steps});

	// Sample the initial hypothesis
	// This is following the usual Fleet procedure
	auto h0 = LangHyp::sample();

	// Initialize empty data
	// Data is not really used here, because the hypothesis search
	// is just guided by the communicative accuracy
	typename LangHyp::data_t emptyData;
	
	// the last argument is the max temperature
	ParallelTempering<LangHyp> samp(
		h0,
		&emptyData,
		FleetArgs::nchains,
		10.0
	);

	int i = 0;
	// This is where the magic happens,
	// the interesting part is in the likelihood function
	// defined in the LangHyp class
	for(auto& h : samp.run(
		Control(FleetArgs::steps)) | top | printer(FleetArgs::print)){
	
		// The commented code is for debugging by running everything
		// in a single thread
	/* for(auto& h : */ 
	/* 		samp.unthreaded_run(Control(FleetArgs::steps)) */ 
	/* 		| top */ 
	/* 		| printer(FleetArgs::print)){ */
		
		addLineToHypCSV(hypfilepath, h);
		addLineToDataFile(datafilepath, h);
		std::cout << i << " " << std::flush;
		i++;

		// Hypotheses are added to top with the pipe operator above
	}

	std::cout << "Top hypotheses" << std::endl;
	top.print();
	/* return top; */
}

