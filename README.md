# Evolving a semantic system in an LoT

## Rough model description

This is a model of cultural evolution of semantics in an LoT from a combination of pressures for accuracy and learnability. The evolution of both the composition function and lexical meanings can be studied with this model. Crucially, the co-inference of lexical meanings and composition function allows for a higher compression of the language by putting some of the concepts shared across different lexical entries into the composition function.

## The world (aka context)

In the current implementation the world consists of a set of individuals. Each individual is a tuple `(int,bool)`. The int is the 'content' of the individual, the bool specifies whether the individual is a 'target'.

## Communication

In each communicative event, the speaker sees a full world while the listener sees the int components of each entity. The speaker sends a signal and based on the signal the listener gives a probability to each object being a target. E.g. a possible sentence could mean the English 'Every even object is a target'. This is not the usual discriminative task, but rather closer to the descriptive task.

## The language

### Syntax 

The syntax consists just of binary-branching trees. The syntax is semantically transparent: only those sentences that are meaningful according to the composition function are well-formed (see semantics below).

### Semantics

The semantic system has two components.

The lexical interpretation function: assigns a meaning to each word in the language. The meanings are intensional, meaning that their types is function from a 'world' to some extension.

The composition function: constructs the meaning of a whole sentence by recursively taking the meaning of two nodes of the tree and returning a meaning.

### Type system

The model's meanings essentially follow an intensional version of the Heim & Krazter's model. The extensions includes `e`, `t`, and some types recursively defined from these. The meanings are functions `<s,\phi>` for some extensional type `\phi`.

The main difficulty is getting around C++'s rigid typing system. In particular, the composition function needs to take meanings of different types and returns meanings of different types. To do so, I used `std::variant` types to encode a general 'meaning' type. This meaning type can take on any of a manually specified list of semantic types. I included a list of the commonly discussed ones. Unfortunately, this makes everything more complicated. Sometimes you eat the bear and sometimes the bear eats you.

The meaning variant type also contains an `Empty` type that indicates an object that is meaningless. This simplifies the implementation of various things, such as the composition function and the presupposition failure mechanism.

## Learning / evolution

The evolution happens for a part of the language that is learned in each generation. Which part of the language is learned depends on the Hypothesis class specified by the file in `./LoTs`. In order to change to a different Hypothesis, you can just change the hypothesis template parameter of the IL function in Main. The Hypothesis needs some stuff in addition to the usual Fleet stuff, see below.

## The tricky bits

- Whether the composition function returns an Empty meaning must depend *only* on the *types* of the input meanings, but not on e.g., context or other semantic features. This is because, in order to make the search over meaningful sentences more efficient, the simulation constructs a CFG based on checking whether the composition function returns empty for various combinations of input types, and then produce sentences from the CFG.
- There is also a way of dealing with presupposition failure which doesn't interfere with the construction of the CFG. Essentially, a presupposition function exception is raised. This is captured e.g. when searching a sentence for production.
- The BTCs are unique pointers (for various reasons), and I did not yet implemented copy semantics. This means that they are messy to pass around directly. Easier to pass them around as SExpressions and interpret them as needed. This is not as efficient but saves a lot of headaches.

## Codebase roadmap

`objects/Agent` implements the IL's agents, including:
- Functions to compute the complexity of a sentence and the informativity of a meaning in context.
- Utility to generate a random *meaningful* sentence of a given type.
- Utility to randomly initialize the agent with a hypothesis that can produce sentences that are true or false in the given contexts.
- Production
- Interpretation
- Learning and picking an hypothesis given the posterior

`objects/IL` implements a function to run the Iterated Learning simulation.

`objects/language` implements the following:
- `LexicalSemantics`: Implements the basic structure of the language's lexical semantics, including a looping interface, `.at` access to meanings, and a collection of default meanings that can be added.
- `BTC`: Binary Tree Class, implements the general structure of sentences, effectively the "syntax".

`objects/World` contains a simple utility function to generate contexts.

`LoTs` folder:
- Each file in `./LoTs` implements one part of the language that the agents might infer, e.g., the composition function alone, or the compfunc+meanings of a certain type, etc.
- Each file in `./LoT` implements:
	1. A Fleet Grammar
	2. A Fleet Hypothesis. This needs to have (on top of usual Fleet stuff) two additional methods:
		- `getCompositionF`: Returns an object of type `t\_BTC\_compose`, which takes two meanings and returns a meaning.
		- `getLexicon`: Returns a LexicalSemantics. This is the only entry point for the agent's LexicalSemantics; The agent doesn't keep one a separate one of its own. However, note that the Hypothesis can trivially return a default initialized LexicalSemantics.

