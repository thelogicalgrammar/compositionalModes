# pragma once
# include <typeinfo>

// This returns a TopN object with the best N hypotheses
template <typename LangHyp>
void runTradeoffAnalysis(
		size_t nObs,
		size_t cSize,
		double likelihoodWeight,
		std::mt19937& rng,
		size_t searchDepth,
		std::filesystem::path& datafilepath,
		std::filesystem::path& hypfilepath
	){

	LangHyp::setParams(nObs, cSize, likelihoodWeight, rng, searchDepth);

	// TopN object to store the best hypotheses
	TopN<LangHyp> top(size_t{FleetArgs::steps});

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

	int i = 0;
	for(auto& h : samp.run(
		Control(FleetArgs::steps)) | top | printer(FleetArgs::print)){
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

