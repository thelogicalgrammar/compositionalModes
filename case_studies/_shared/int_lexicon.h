#pragma once

// Int-specific lexicon items for case studies whose t_e_content is int.
//
// These predicates operate on the entity's content as an integer:
//   - even: <s,<e,t>>
//   - prime: <s,<e,t>>
//   - gt: <s,<e,<e,t>>>
//   - equal: <s,<e,<e,t>>>
//
// Case studies that want these items include this header and call
// addIntLexicon(lex) from their getLexicon(). They must not be in
// language.h because language.h is content-agnostic.

inline void addIntLexicon(LexicalSemantics& lex) {

	// -- IVs (<s,<e,t>>) --

	lex.add( "even",
		t_meaning([](const t_context& c) -> t_IV {
			return [](t_e x) -> t_t {
				int o = content(x);
				return o % 2 == 0;
			};
		})
	);

	lex.add( "prime",
		t_meaning([](const t_context& c) -> t_IV {
			return [](t_e x) -> t_t {
				int o = content(x);
				if (o <= 1) return false;
				if (o == 2) return true;
				if (o % 2 == 0) return false;
				for (int i = 3; i < o; i += 2) {
					if (o % i == 0) return false;
				}
				return true;
			};
		})
	);

	// -- TVs (<s,<e,<e,t>>>) --
	// NOTE: Rarely used in current case studies because there are no
	// individual-type expressions (<s,e>) in their lexicons.

	lex.add( "gt",
		t_meaning([](const t_context& c) -> t_TV {
			return [](t_e y) -> t_IV {
				int o1 = content(y);
				return [o1](t_e x) -> t_t {
					int o2 = content(x);
					return o2 > o1;
				};
			};
		})
	);

	lex.add( "equal",
		t_meaning([](const t_context& c) -> t_TV {
			return [](t_e y) -> t_IV {
				int o1 = content(y);
				return [o1](t_e x) -> t_t {
					int o2 = content(x);
					return o2 == o1;
				};
			};
		})
	);
}
