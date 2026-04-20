# pragma once

// define a function to generate
// random contexts of a given size
// The context is a set of objects,
// where each object is a tuple (content, bool).
// Content is sampled by the case study's sample_content().
// NOTE: contents must be unique within a context!
t_context generateContext(
		size_t size,
		std::mt19937& rng,
		float pTarget
	){

	t_context context;
	// pay attention to uniqueness of contents
	std::set<t_e_content> contents;
	while (context.size() < size) {
		t_e_content c = sample_content(rng);
		bool target = std::bernoulli_distribution(pTarget)(rng);
		if (contents.find(c) == contents.end()) {
			contents.insert(c);
			context.insert(std::make_tuple(c, target));
		}
	}

	return context;
}

std::vector<t_context> generateContexts(
		size_t size,
		// number of contexts to generate
		size_t num,
		std::mt19937& rng,
		float pTarget
	){

	std::vector<t_context> contexts;
	for (size_t i = 0; i < num; ++i) {
		contexts.push_back(generateContext(size, rng, pTarget));
	}
	return contexts;
}
