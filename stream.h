# pragma once
#include <iomanip>
#include <chrono>
#include <unistd.h>

// Note: this needs to go at the end or
// there's errors with the json library
#include "json.hpp"
using json = nlohmann::json;


std::ostream& operator<<(
		std::ostream& os, const t_context& context) {
	// Print the context
	// which is a set of e
	// e.g., { (1, true), (2, false) }
	os << "{ ";
	for (auto& e : context){
		os 
			<< "(" 
			<< std::get<0>(e) 
			<< ", " 
			<< std::get<1>(e) 
			<< "), ";
	}
	os << " }";
	return os;
}

std::ostream& operator<<(
		std::ostream& os, const t_terminalsMap& terminals) {

	for (auto& [k, v] : terminals) {
		// print the key and the value
		os << k << " -> ";
		// Each value is a set of strings
		for (auto& v1 : v) {
			os << v1 << " ";
		}
		os << std::endl;
	}
	return os;
}

std::ostream& operator<<(
	std::ostream& os, const std::vector<double>& v) {
	// Print the vector
	// e.g., [ 1, 2, 3 ]
	os << "[ ";
	for (auto& e : v){
		os << e << ", ";
	}
	os << " ]";
	return os;
}


std::ostream& operator<<(std::ostream& os, const t_cfgMap& cfg) {
    // Determine the maximum width needed
    int max_width = 0;
    for (const auto& [k, v] : cfg) {
        for (const auto& [v1, v2] : v) {
            max_width = std::max(max_width, static_cast<int>(v1.length()));
        }
    }

    for (const auto& [k, v] : cfg) {
        // print the key and the value
        os << k << " -> \n";
        // Each value is a set of pairs of strings
        for (const auto& [v1, v2] : v) {
            os << "\t" << std::left << std::setw(max_width + 2) << v1 << v2 << "\n";
        }
        os << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<t_datum>& data) {
	// go through the data and print each datapoint
	for (auto& datum : data) {
		os << datum << std::endl;
	}
	return os;
}

/* std::ostream& operator<<( */
/* 		std::ostream& os, const */ 

std::string generateUniqueSuffix() {
    // Get current time in milliseconds
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    // Get process ID (Unix/Linux specific)
    auto pid = getpid();

    // Generate a random number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000000);
    int randomNum = dis(gen);

    // Combine time, PID, and random number
    std::stringstream ss;
    ss << millis << "_" << pid << "_" << randomNum;
    return ss.str();
}

template<typename HYP, typename Ag>
void saveGeneration(
		std::filesystem::path filepath,
		size_t i,
		std::vector<Ag> children,
		std::optional<std::vector<typename HYP::data_t>> childrenCommData,
		std::optional<std::vector<typename HYP::data_t>> childrenLearningData,
		std::vector<double> childrenCommAccs,
		std::optional<std::vector<size_t>> parentIndices
	){
	// I need to save the following information 
	// for each agent in each generation:
	// - index of generation 
	// - index of agent in its generation
	// - chosen hypothesis as a readable string
	// - chosen hypothesis as a string parseable by the grammar
	// - data produced by agent for next generation 
	// 		(which is also used to calculate communicative accuracy)
	// - communicative accuracy of agent's language
	// - index of agent's parent in previous generation
	
	std::cout << "Saving generation " << i << " to " << filepath << std::endl;

	std::ofstream file(filepath);

	json j;
	j["generation"] = i;
	j["children"] = json::array();
	int nAgents = children.size();
	for( size_t k = 0; k < nAgents; k++ ){

		json childk;
		childk["agentIndex"] = k;

		if( parentIndices.has_value() ){
			childk["parentIndex"] = parentIndices.value()[k];
		}

		// If childrenCommData is not empty, then we are saving
		if( childrenCommData.has_value() ){
			typename HYP::data_t childCommData = 
				childrenCommData.value()[k];
			// create a vector of strings from the data using <<
			std::vector<std::string> childCommDataStr;
			for( auto& d : childCommData ){
				std::stringstream ss;
				ss << d;
				childCommDataStr.push_back( ss.str() );
			}
			childk["commData"] = childCommDataStr;
		}
		
		if( childrenLearningData.has_value() ){
			typename HYP::data_t childLearningData = 
				childrenLearningData.value()[k];
			std::vector<std::string> childLearningDataStr;
			for( auto& d : childLearningData ){
				std::stringstream ss;
				ss << d;
				childLearningDataStr.push_back( ss.str() );
			}
			childk["learningData"] = childLearningDataStr;
		}

		double commAcc = childrenCommAccs[k];
		childk["commAcc"] = commAcc;

		Ag child = children[k];
		auto childHyp = child.getHypothesis();
		childk["hypothesis"] = childHyp.string();
		childk["hypothesisSerialized"] = childHyp.serialize();

		if (child.wasMutated()){
			auto originalHyp = child.getOriginalHypothesis();
			childk["originalHypothesis"] = originalHyp.string();
			childk["originalHypothesisSerialized"] = 
				originalHyp.serialize();
		}

		// If the agent's hypothesis was initialized directly
		// they will not have a TopN since they did not run inference
		try {
			TopN<HYP> childTopN = child.getTop();
			std::string topNStr = "";
			std::string topNSerialized = "";
			for(auto& h : childTopN.sorted()) {
				topNStr +=
					std::to_string(h.posterior) + " " +
					std::to_string(h.prior) + " " +
					std::to_string(h.likelihood) + " " +
					QQ(h.string());
			}
			topNSerialized += childTopN.serialize();
			childk["topNSerialized"] = topNSerialized;
			childk["topN"] = topNStr;
		} catch (const std::runtime_error& e) {}

		j["children"].push_back(childk);
	}
	if (file.is_open()){
		file << j.dump(4) << std::endl;
		file.close();
	} else {
		std::cout << "Unable to open file" << std::endl;
	}
}

