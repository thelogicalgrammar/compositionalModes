# pragma once

// define a function to generate
// random contexts of a given size
// The context is a set of objects,
// where each object is a tuple (int,bool)
// NOTE: Ints must be unique!
t_context generateContext(
		size_t size,
		std::mt19937& rng
	){

	t_context context;
	// pay attention to uniqueness of ints
	std::set<int> ints;
	t_intdist dist(-10, 10);
	while (context.size() < size) {
		int i = dist(rng);
		if (ints.find(i) == ints.end()) {
			ints.insert(i);
			context.insert(std::make_tuple(
				i,
				dist(rng) > 0
			));
		}
	}

	return context;
}

std::vector<t_context> generateContexts(
		size_t size,
		// number of contexts to generate
		size_t num,
		std::mt19937& rng
	){

	std::vector<t_context> contexts;
	for (size_t i = 0; i < num; ++i) {
		contexts.push_back(generateContext(size, rng));
	}
	return contexts;
}
