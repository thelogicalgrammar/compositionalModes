# pragma once

// define a function to generate
// random contexts of a given size
// The context is a set of objects,
// where each object is a tuple (int,bool)
// NOTE: Ints must be unique!
t_context generateContext(
		size_t size,
		std::mt19937& rng,
		float p_target = 0.5
	){

	t_context context;
	// pay attention to uniqueness of ints
	std::set<int> ints;
	t_intdist dist(-10, 10);
	while (context.size() < size) {
		// Define the integer component of the element
		int i = dist(rng);
		bool target = std::bernoulli_distribution(p_target)(rng);
		if (ints.find(i) == ints.end()) {
			ints.insert(i);
			context.insert(std::make_tuple(
				i,
				target
			));
		}
	}

	return context;
}

std::vector<t_context> generateContexts(
		size_t size,
		// number of contexts to generate
		size_t num,
		std::mt19937& rng,
		float p_target = 0.5
	){

	std::vector<t_context> contexts;
	for (size_t i = 0; i < num; ++i) {
		contexts.push_back(generateContext(size, rng, p_target));
	}
	return contexts;
}
