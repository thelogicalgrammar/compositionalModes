#pragma once

// Case-study configuration for the gradable adjectives case study.
// Must be included BEFORE types.h (which uses t_e_content to define t_e).
//
// Entities are multi-dimensional: each entity has NUM_DIMS float-valued
// dimensions (think "height, weight, temperature, ..."). Different
// adjectives can select different dimensions via the LoT grammar.
//
// std::array<float, N> is used rather than e.g., std::tuple<float,...,float>

#include <random>
#include <string>
#include <array>
#include <sstream>

#ifndef NUM_DIMS
#define NUM_DIMS 5   // override with: -DNUM_DIMS=N
#endif
static constexpr size_t num_dims = NUM_DIMS;

using t_e_content = std::array<float, num_dims>;

// Each dimension is sampled independently from a standard normal
// (mean 0, sd 1). Constants 0.0 and 1.0 in the DSL then correspond
// roughly to "typical" values — "one sd above the mean" is meaningful.
inline t_e_content sample_content(std::mt19937& rng) {
	std::normal_distribution<float> dist(0.0f, 1.0f);
	t_e_content c;
	for (size_t i = 0; i < num_dims; i++) c[i] = dist(rng);
	return c;
}

inline std::string content_to_string(const t_e_content& c) {
	std::ostringstream os;
	os << "[";
	for (size_t i = 0; i < num_dims; i++) {
		if (i > 0) os << ",";
		os << c[i];
	}
	os << "]";
	return os.str();
}
