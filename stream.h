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


