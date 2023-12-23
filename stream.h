# pragma once
#include <iomanip>

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


/* std::ostream& operator<<( */
/* 		std::ostream& os, const */ 
