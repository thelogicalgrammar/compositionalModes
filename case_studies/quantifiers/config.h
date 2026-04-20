#pragma once

// Case-study configuration: defines the entity content type and how
// content is sampled when generating contexts. Must be included
// BEFORE types.h (which uses t_e_content to define t_e).

#include <random>
#include <string>

using t_e_content = int;

inline t_e_content sample_content(std::mt19937& rng) {
	return std::uniform_int_distribution<int>(-10, 10)(rng);
}

// How to stringify content for logs / data files. Default: std::to_string for int.
inline std::string content_to_string(const t_e_content& c) {
	return std::to_string(c);
}
