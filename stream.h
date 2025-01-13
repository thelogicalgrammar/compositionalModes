# pragma once
#include <iomanip>
#include <chrono>
#include <unistd.h>

// Note: this needs to go at the end or
// there's errors with the json library
#include "json.hpp"
using json = nlohmann::json;


std::ostream& operator<<(std::ostream& os, const t_context& context) {
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

std::ostream& operator<<(std::ostream& os, const t_terminalsMap& terminals) {

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

std::ostream& operator<<(std::ostream& os, const std::vector<double>& v) {
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


// This is used implicitly by the json library
// to serialize the defaultdatum_t
template <typename TDatum>
void to_json(nlohmann::json& j, const TDatum& d) {
    j = nlohmann::json{
        {"input", d.input},          
        {"output", d.output}
    };
}

void initializeHypCSV(const std::filesystem::path& filename) {
	// Initialize a CSV file
	std::ofstream file(filename);
	file 
		<< "posterior,prior,likelihood,hypothesis,serialized" 
		<< std::endl;
	file.close();
}

template <typename Hyp>
void addLineToHypCSV(const std::filesystem::path& filename, const Hyp& h) {
	// Add a line to a CSV file
	std::ofstream file(filename, std::ios_base::app);

	// Add the hypothesis to the file
	file 
		<< h.posterior 
		<< "," << h.prior 
		<< "," << h.likelihood 
		<< "," << h.string() 
		<< "," 
		<< h.serialize() 
		<< std::endl;
	file.close();
}

template <typename Hyp>
void addLineToDataFile(const std::filesystem::path& filename, const Hyp& h) {

	// Add a line to a CSV file
	std::ofstream file(filename, std::ios_base::app);

	auto data = h.getCommData();
	for (auto& d : data) {
		// input is a list of (int, bool) pairs
		std::string input = "{ ";
		for (auto& [i, b] : d.input) {
			input 
				+= "(" 
				+ std::to_string(i) 
				+ ", " 
				+ std::to_string(b) 
				+ "), ";
		}
		input += " }";

		file 
			<< input
			<< "," 
			<< d.output 
			<< "|";
	}
	// Conclude with a newline
	// for the next hypothesis
	file << std::endl;
	file.close();
}

template <typename Hyp>
void saveResults(const std::filesystem::path& filename, 
				 const TopN<Hyp>& results) {
	// Save the results to a file
	std::ofstream file(filename);

	json j;

	std::string topNSerialized = results.serialize();
	j["topNSerialized"] = topNSerialized;

	j["topN"] = json::array();
	for (auto& h : results.sorted()) {
		json jH;
		jH["posterior"] = h.posterior;
		jH["prior"] = h.prior;
		jH["likelihood"] = h.likelihood;
		jH["hypothesisSerialized"] = h.serialize();
		jH["hypothesis"] = h.string();
		jH["data"] = json::array();
		auto data = h.getCommData();
		for (auto& d : data) {
			jH["data"].push_back(d);
		}
		j["topN"].push_back(jH);
	}

	// the 4 is the indentation
	file << j.dump(4) << std::endl;
	file.close();

}
