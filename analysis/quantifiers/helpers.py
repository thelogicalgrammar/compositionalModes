import pandas as pd
import numpy as np
from glob import glob
import json
import pickle
from os.path import exists, join
import re
from copy import deepcopy
import re
from functools import lru_cache
from collections import defaultdict


def parse_sexpr(s):
    """Parse a string s into a nested list representation of s-expressions."""
    def tokenize(s):
        """Break the input string into tokens."""
        s = s.replace('(', ' ( ').replace(')', ' ) ')  # Add spaces around parentheses
        return s.split()

    def read_from_tokens(tokens):
        """Recursively read an expression from the token list."""
        if not tokens:
            raise SyntaxError("Unexpected EOF while reading.")
        
        token = tokens.pop(0)

        if token == '(':
            # Start a new list
            result = []
            while tokens[0] != ')':
                result.append(read_from_tokens(tokens))
            tokens.pop(0)  # Pop the ')'
            return result
        elif token == ')':
            raise SyntaxError("Unexpected )")
        else:
            # Atom (either symbol, number, or string)
            return atom(token)

    def atom(token):
        """Convert a token into an appropriate Python type (e.g., number or string)."""
        try:
            return int(token)  # Try converting to integer
        except ValueError:
            try:
                return float(token)  # Try converting to float
            except ValueError:
                return token  # Treat as symbol (string)

    # Tokenize the input string and parse it into a nested list
    tokens = tokenize(s)
    return read_from_tokens(tokens)


def get_data(basepath, include_commdata=False):

    data = dict()
    
    datapath = join(basepath, "hyp.csv")
    fulldatapath = join(basepath, "data.txt")
    # paramspath = basepath + "data_likweight_40.000000_nobs_250parameters.json"
    paramspath = list(glob(basepath+"*.json"))[0]

    print('reading params')
    with open(paramspath) as openf:
        params = json.load(openf)
    data['params'] = params

    if include_commdata:

        picklepath = join(basepath, 'commdata.pickle')
        if exists(picklepath):
            print(f'getting commdata for {basepath} from pickle')
            with open(picklepath, 'rb') as openf:
                commdata = pickle.load(openf)
        else:
        
            print('reading data raw')
            with open(fulldatapath) as openfile:
                commdata_raw = openfile.readlines()
        
            print('processing data')
            commdata = []
            for datum in commdata_raw:
                hyp, x = datum.split('||')
                subdata = []
                for i, y in enumerate(x.split('|')[:-1]):
                    index_split = y.find('};(') + 1
                    tup = eval(y[:index_split])
                    out = [tup, y[index_split+1:]]
                    subdata.append(out)
                commdata.append(subdata)
            with open(picklepath, 'wb') as openf:
                pickle.dump(data, openf)
        
        data['commdata'] = commdata

    print('reading hypotheses data')
    df = pd.read_csv(datapath)

    nquants = len(df['hypothesis'].iloc[0].split('|')) - 1

    split_df = pd.DataFrame(
        df['hypothesis'].str.split('|').to_list(),
        columns=['c']+[f'q{n}' for n in range(nquants)]
    )
    print('processing hypothesis data')
    df = pd.concat([df,split_df], axis=1)
    df['likelihood'] = df['likelihood'] / params['likelihoodweight']
    df['posterior'] = df['posterior'] / params['likelihoodweight']

    l = list(map(expand_quants, df['hypothesis'].values))
    cons = []
    for x in l:
        dependences = [
            find_dependence(parse_sexpr(parse))
            for parse in x
        ]
        total_deps = [any(x) for x in zip(*dependences)]
        cons.append(is_conservative(total_deps))
    
    df['cons'] = cons
    df['likweight'] = params['likelihoodweight']
    df['pragmatic'] = params['pragmatic']
    
    data['hyps'] = df
    return data


def unparse_sexpr(expr):
    """
    Convert a nested-list representation of an S-expression back into a string.
    """
    if not isinstance(expr, list):
        return str(expr)
    else:
        return f"({' '.join(unparse_sexpr(e) for e in expr)})"


def replace_q_in_c(expr, q):
    # assuming we replaced X.L with X.Lq in quant and similarly for X.R
    if isinstance(expr,list):
        return [replace_q_in_c(x, q) for x in expr]
    elif expr == 'X.Q':
        return lambda x: lambda y: q.replace('X.Lq', x).replace('X.Rq', y)
    else:
        return expr


def apply_composition(expr):
    """
    Recursively walk 'expr' (a nested list or atom).

    We are looking for subexpressions of the form:
        [ [f, arg1], arg2 ]
    where 'f' is a 2-argument callable (e.g. your X.Q replacement).

    Then we "apply" them:
        partial = f(arg1)     # 1-argument function
        result  = partial(arg2)
    """
    if not isinstance(expr, list):
        # It's just an atom or a callable with no arguments
        return expr
    
    # 1) Recursively transform children first:
    transformed = [apply_composition(x) for x in expr]
    
    # 2) Now see if it has exactly 2 items => [ SOMETHING, arg2 ]
    if len(transformed) == 2:
        subexpr = transformed[0]  # the first half
        arg2 = transformed[1]
        
        # subexpr might be [f, arg1], if we want to interpret it as ( (f arg1) arg2 )
        if (isinstance(subexpr, list) and len(subexpr) == 2):
            f_candidate = subexpr[0]
            arg1 = subexpr[1]

            # Check if the first item is callable => it's presumably our 2-arg function
            if callable(f_candidate):
                # We expect arg1 and arg2 to be strings if you're doing .replace(...) inside f.
                # If they might be lists, you'd need a different approach or a structural substitution.
                if not isinstance(arg1, str):
                    arg1 = unparse_sexpr(arg1)
                if not isinstance(arg2, str):
                    arg2 = unparse_sexpr(arg2)

                # 3) Apply "f_candidate" to arg1 => returns a 1-arg function
                partially_applied = f_candidate(arg1)

                # 4) Then apply that to arg2 => final result
                result = partially_applied(arg2)
                
                # 'result' is presumably a string 
                # (the final substitution q.replace('X.L', ...).replace('X.R', ...)).
                return result
        elif len(transformed) == 3:
            return unparse_sexpr(transformed)

    # If it didn't match the pattern or wasn't callable, just return the structure:
    return transformed


def expand_quants(expr):
    c, *qs = expr.split('|')
    parsed_qs = []
    for q in qs:
        x = replace_q_in_c(parse_sexpr(c[3:]), q.replace('X.L', 'X.Lq').replace('X.R', 'X.Rq'))
        y = apply_composition(x)
        parsed_qs.append(unparse_sexpr(y))
    return parsed_qs


def find_dependence(expression):
    # NOTE: order of conditions matters
    if isinstance(expression, list) or isinstance(expression, tuple):
        leftarg, *rightargs = expression
    else:
        leftarg = expression
    # list[bool] refers to [U-(L or R), L-R, R-L, L&R]
    # Set of domain subsets on which a composition function
    # allows the quantifiers to depend
    if leftarg == 'X.Q':
        return find_dependence(rightargs)
    elif leftarg == 'X.L':
        return [False, True, False, True]
    elif leftarg == 'X.R':
        return [False, False, True, True]
    elif leftarg == 'X.c':
        return [True, True, True, True]
    elif leftarg in ['union', 'intEq', 'intGt', 'and', 'or', '+', '-']:
        # depends at most on the union of the sets
        # on which the arguments depend
        return list(map(
            lambda x, y: x or y, 
            *[find_dependence(x) for x in rightargs]
        ))
    elif leftarg == 'not':
        return find_dependence(rightargs)
    elif leftarg == 'universe':
        return find_dependence(rightargs)
    elif leftarg == 'intersection':
        return list(map(
            lambda x, y: x and y, 
            *[find_dependence(x) for x in rightargs]
        ))
    elif leftarg == 'setminus':
        # U-(L or R), L-R, R-L, L&R
        l, r = rightargs
        return list(map(
            lambda x, y: x and not y, 
            find_dependence(l),
            find_dependence(r)
        ))
    elif leftarg == 'nTh':
        return find_dependence(rightargs[2])
    elif leftarg == 'cardinality':
        return find_dependence(rightargs[0])
    elif leftarg in [0, 1]:
        return [False, False, False, False]
    elif rightargs == []:
        return find_dependence(leftarg)
    elif isinstance(leftarg, list):
        return list(map(
            lambda x, y: x or y, 
            find_dependence(leftarg),
            find_dependence(rightargs)
        ))
    else:
        assert False, f"Could not find {leftarg}"


def is_conservative(dependencies):
    #                    [U-(L|R), L-R, R-L,   L&R]
    return (
        (dependencies == [False, True,  False, True]) or
        (dependencies == [False, False, True,  True]) or
        (dependencies == [False, True,  False, False]) or
        (dependencies == [False, False, True,  False]) or
        (dependencies == [False, False, False, True])
    )


def pareto_frontier(points):
    """
    Find the Pareto frontier for a set of 2D points.
    
    A point is on the Pareto frontier if no other point dominates it
    (i.e., no other point has both higher x and higher y values).

    Parameters:
        points (array-like): List of 2D points (x, y).

    Returns:
        frontier (list): List of points on the Pareto frontier.
        indices (list): List of indices of points on the Pareto frontier.
    """
    # Convert points to a numpy array for easier manipulation
    points = np.array(points)
    
    if len(points) == 0:
        return [], []
    
    if len(points) == 1:
        return [points[0]], [0]
    
    # Find all Pareto optimal points
    frontier = []
    frontier_indices = []
    
    for i, point in enumerate(points):
        is_pareto_optimal = True
        
        # Check if this point is dominated by any other point
        for j, other_point in enumerate(points):
            if i != j:
                # Point is dominated if other_point has both higher x and higher y
                if (other_point[0] >= point[0] and other_point[1] >= point[1] and 
                    (other_point[0] > point[0] or other_point[1] > point[1])):
                    is_pareto_optimal = False
                    break
        
        if is_pareto_optimal:
            frontier.append(point)
            frontier_indices.append(i)
    
    # Sort the frontier points by x coordinate for consistent ordering
    if frontier:
        sorted_indices = np.argsort([p[0] for p in frontier])
        frontier = [frontier[i] for i in sorted_indices]
        frontier_indices = [frontier_indices[i] for i in sorted_indices]
    
    return frontier, frontier_indices


def get_data_from_glob(path, include_commdata=False):
    data = []
    for basep in glob(path):
        data.append(get_data(basep, include_commdata=include_commdata))

    df = pd.concat(
        [x['hyps'] for x in data]
    ).reset_index()

    df['expanded'] = df['hypothesis'].str[3:].apply(expand_quantifiers)
    ids_series, id_lookup = quantifier_partition_indices(df['expanded'], join=None)

    # ids_series is a Series of lists of IDs, e.g. [0,1], [1,0], [2], [0,0]
    # Normalize to an order-insensitive tuple key
    sig_col = ids_series.apply(lambda ids: tuple(sorted(ids)))

    # Add as a column alongside s
    df[['sig']] = pd.DataFrame({"sig": sig_col})

    # Now groupby sig
    df['meanlikelihood'] = (
        df.groupby(['sig', 'pragmatic'])
        ['likelihood']
        .transform('mean')
    )

    if include_commdata:
        return df, [x['commData'] for x in data]
    else:
        return df


def count_substantial(hyp):
    """
    Count the number of substantial quantifiers in a hypothesis.
    Where substantial means that they somehow depend on the 
    """
    return sum(1 for q in hyp.split('|')[1:] if ('X.Q' in q) or ('X.c' in q))

# ---------- S-expression parsing / unparsing ----------

def _tokenize(s: str):
    s = s.replace("λx.", "")  # ignore optional lambda, per your note
    s = re.sub(r"\s+", " ", s.strip())
    s = re.sub(r"([\(\)])", r" \1 ", s)
    return [t for t in s.split(" ") if t]

def _parse(tokens):
    """Parse tokens into a nested list (atoms are strings)."""
    stack = [[]]
    for tok in tokens:
        if tok == "(":
            new = []
            stack[-1].append(new)
            stack.append(new)
        elif tok == ")":
            if len(stack) == 1:
                raise ValueError("Unbalanced parentheses")
            stack.pop()
        else:
            stack[-1].append(tok)
    if len(stack) != 1:
        raise ValueError("Unbalanced parentheses at end")
    return stack[0][0] if len(stack[0]) == 1 else stack[0]

def _unparse(ast):
    if isinstance(ast, str):
        return ast
    return "(" + " ".join(_unparse(x) for x in ast) + ")"

# ---------- Core transforms ----------

def _subst(ast, A, B):
    """Deep substitute X.L ↦ A, X.R ↦ B within ast (preserve tuple structure)."""
    if isinstance(ast, str):
        if ast == "X.L": return deepcopy(A)
        if ast == "X.R": return deepcopy(B)
        return ast
    # ast is list/tuple → rebuild as tuple
    return tuple(_subst(x, A, B) for x in ast)

def _is_q_app(node):
    """
    Match ((X.Q A) B) encoded as (( "X.Q", A ), B).
    Return (A, B) if it matches, else None.
    """
    if not (isinstance(node, (list, tuple)) and len(node) == 2):
        return None
    left, right = node
    if isinstance(left, (list, tuple)) and len(left) == 2 and left[0] == "X.Q":
        return (left[1], right)
    return None

def expand_quantifiers(full_string):
    """
    Input: a pipe-separated string: 'COMPOSITION | Q1 | Q2 | ...'
    Output: list of strings [E1, E2, ...] where Ei is the entire composition
            with every ((X.Q A) B) expanded to Qi[A/X.L, B/X.R].
    """
    parts = [p.strip() for p in full_string.split("|")]
    if len(parts) < 2:
        raise ValueError("Expected 'COMPOSITION | Q1 | ...'")

    comp_ast = _parse(_tokenize(parts[0]))
    quant_asts = [_parse(_tokenize(q)) for q in parts[1:]]

    results = []
    for q_ast in quant_asts:
        rewritten = rewrite_for_quant(comp_ast, q_ast)
        results.append(_unparse(rewritten))

    return '|'.join(results)


# ========= Semantic partitioning for systems (N=5) =========
# Each system string is: "COMPOSITION | Q1 | Q2 | ...".
# Grammar supported in quantifier bodies:
#   Sets: X.L, X.R, (universe X.c), (union S T), (intersection S T), (setminus S T)
#   Ints: 0, 1, (+ a b), (- a b), (cardinality S X.c)
#   Bools: (intEq a b), (intGt a b), (not B), (and B C), (or B C)
#
# Composition: arbitrary Part-I expression that may contain any number of ((X.Q A) B) subterms.
# We rewrite the composition for each Qi by replacing every ((X.Q A) B) with Qi[X.L:=A, X.R:=B].


# ----------------- S-expression parsing -----------------

def _parse_sexpr(s: str):
    return _to_tuple(_parse(_tokenize(s)))

def _to_tuple(node):
    if isinstance(node, list):
        return tuple(_to_tuple(x) for x in node)
    return node  # atom (str)

# ----------------- Composition rewriting -----------------

def rewrite_for_quant(comp_ast, quant_ast):
    """
    Replace every ((X.Q A) B) in comp_ast with quant_ast where X.L:=A and X.R:=B.
    Returns a Boolean AST (tuple form).
    """
    hit = _is_q_app(comp_ast)
    if hit:
        A, B = hit
        return _subst(quant_ast, A, B)   # already returns tuple
    if isinstance(comp_ast, str):
        return comp_ast
    # Rebuild as tuple
    return tuple(rewrite_for_quant(x, quant_ast) for x in comp_ast)

# ----------------- Compiler to bitmask evaluator -----------------

def _compile_set(ast, U_MASK):
    # returns function f(L_mask, R_mask) -> int bitmask (subset of U)
    if isinstance(ast, str):
        if ast == "X.L": return lambda L, R: L
        if ast == "X.R": return lambda L, R: R
        raise TypeError(f"Unexpected atom in set position: {ast}")

    head, *args = ast
    if head == "universe":
        if len(args) != 1 or args[0] != "X.c":
            raise TypeError("Expected (universe X.c)")
        return lambda L, R: U_MASK

    if head == "union":
        if len(args) != 2: raise TypeError("(union S T)")
        f, g = _compile_set(args[0], U_MASK), _compile_set(args[1], U_MASK)
        return lambda L, R: f(L, R) | g(L, R)

    if head == "intersection":
        if len(args) != 2: raise TypeError("(intersection S T)")
        f, g = _compile_set(args[0], U_MASK), _compile_set(args[1], U_MASK)
        return lambda L, R: f(L, R) & g(L, R)

    if head == "setminus":
        if len(args) != 2: raise TypeError("(setminus S T)")
        f, g = _compile_set(args[0], U_MASK), _compile_set(args[1], U_MASK)
        return lambda L, R: f(L, R) & (~g(L, R) & U_MASK)

    raise TypeError(f"Unknown set constructor: {head}")

def _compile_int(ast, U_MASK):
    # returns function f(L,R) -> int
    if isinstance(ast, str):
        if ast == "0": return lambda L, R: 0
        if ast == "1": return lambda L, R: 1
        raise TypeError(f"Unexpected atom in int position: {ast}")

    head, *args = ast
    if head == "cardinality":
        if len(args) != 2 or args[1] != "X.c":
            raise TypeError("Expected (cardinality <set> X.c)")
        f = _compile_set(args[0], U_MASK)
        return lambda L, R: f(L, R).bit_count()

    if head == "+":
        if len(args) != 2: raise TypeError("(+ a b)")
        f, g = _compile_int(args[0], U_MASK), _compile_int(args[1], U_MASK)
        return lambda L, R: f(L, R) + g(L, R)

    if head == "-":
        if len(args) != 2: raise TypeError("(- a b)")
        f, g = _compile_int(args[0], U_MASK), _compile_int(args[1], U_MASK)
        return lambda L, R: f(L, R) - g(L, R)

    raise TypeError(f"Unknown int constructor: {head}")

def _compile_bool(ast, U_MASK):
    # returns function f(L,R) -> bool
    if isinstance(ast, str):
        if ast == "true":  return lambda L, R: True
        if ast == "false": return lambda L, R: False
        # If a set/int symbol shows up here, it's a type error
        raise TypeError(f"Unexpected atom in bool position: {ast}")

    head, *args = ast
    if head == "intEq":
        if len(args) != 2: raise TypeError("(intEq a b)")
        f, g = _compile_int(args[0], U_MASK), _compile_int(args[1], U_MASK)
        return lambda L, R: f(L, R) == g(L, R)

    if head == "intGt":
        if len(args) != 2: raise TypeError("(intGt a b)")
        f, g = _compile_int(args[0], U_MASK), _compile_int(args[1], U_MASK)
        return lambda L, R: f(L, R) > g(L, R)

    if head == "not":
        if len(args) != 1: raise TypeError("(not B)")
        f = _compile_bool(args[0], U_MASK)
        return lambda L, R: not f(L, R)

    if head == "and":
        if len(args) != 2: raise TypeError("(and B C)")
        f, g = _compile_bool(args[0], U_MASK), _compile_bool(args[1], U_MASK)
        return lambda L, R: f(L, R) and g(L, R)

    if head == "or":
        if len(args) != 2: raise TypeError("(or B C)")
        f, g = _compile_bool(args[0], U_MASK), _compile_bool(args[1], U_MASK)
        return lambda L, R: f(L, R) or g(L, R)

    # Application without operator shouldn't occur in Part II outputs
    raise TypeError(f"Unknown bool constructor: {head}")

# Caches to avoid recompiling / recomputing across many systems
# make cache keys depend on U_MASK (or N)
_compile_cache = {}
_tt_cache = {}

def _ast_key(ast, U_MASK):
    return (ast, U_MASK)

def compile_bool(ast, U_MASK):
    key = _ast_key(ast, U_MASK)
    fn = _compile_cache.get(key)
    if fn is None:
        fn = _compile_bool(ast, U_MASK)
        _compile_cache[key] = fn
    return fn

def truth_table(ast_bool, N, U_MASK):
    """
    Compute truth table for f(L,R) with |U|=N.
    Bit order: index = (L_mask << N) | R_mask  (L major). Total bits: 2^(2N).
    """
    key = _ast_key(ast_bool, U_MASK)
    tt = _tt_cache.get(key)
    if tt is not None:
        return tt

    f = compile_bool(ast_bool, U_MASK)
    table = 0
    bit = 1
    nrows = 1 << N
    for L in range(nrows):
        for R in range(nrows):
            if f(L, R):
                table |= bit
            bit <<= 1
    _tt_cache[key] = table
    return table

# ----------------- System signature & partitioning -----------------

def system_signature(system_string, N, U_MASK):
    """
    Given 'COMP | Q1 | Q2 | ...', return a canonical signature:
    a sorted tuple of 1024-bit integers, one per quantifier (order-agnostic).
    """
    parts = [p.strip() for p in system_string.split("|")]
    if len(parts) < 2:
        raise ValueError("Expected 'COMPOSITION | Q1 | ...'")

    comp_ast = _parse_sexpr(parts[0])
    quant_asts = [_parse_sexpr(q) for q in parts[1:]]

    # Expand each quantifier through the composition
    denotations = []
    for q_ast in quant_asts:
        expanded = rewrite_for_quant(comp_ast, q_ast)
        denotations.append(truth_table(expanded, N, U_MASK))

    # Order doesn't matter → sort; 
    # ignores duplicates since they don't affect
    # communicative accuracy
    unique = sorted(set(denotations))
    return tuple(unique)

def partition_systems(systems, N, U_MASK):
    """
    Group systems into semantic-equivalence classes for |U|=5.
    Returns: dict {signature: [indices]} and a parallel list of signatures.
    """
    classes = defaultdict(list)
    signatures = []
    for i, s in enumerate(systems):
        sig = system_signature(s, N, U_MASK)
        signatures.append(sig)
        classes[sig].append(i)
    return classes, signatures

def quantifier_partition_indices(series: pd.Series, N: int = 5, join: str = '|'):
    """
    series: pandas Series of strings, each like 'Q1 | Q2 | ...' (expanded quantifiers).
    join: if a string, join ids per-row using this separator; if None, return lists of ints.
    Returns: (ids_series, id_lookup)
      - ids_series: Series aligned to `series` with per-quantifier IDs.
      - id_lookup: dict {truth_table_int: cell_id}
    """

    U_MASK = (1 << N) - 1  # 0b11111 for |U|=5

    # 1) Split rows into quantifier strings
    split_rows = series.astype(str).str.split("|")
    # 2) Normalize whitespace
    split_rows = split_rows.apply(lambda lst: [q.strip() for q in lst if q.strip() != ""])

    # 3) Gather all unique quantifier strings (to parse/compile once)
    all_qs = pd.unique([q for lst in split_rows for q in lst])

    # 4) Compute truth table per unique quantifier
    q_to_tt = {}
    for q in all_qs:
        ast = _parse_sexpr(q)
        tt = truth_table(ast, N, U_MASK)
        q_to_tt[q] = tt

    # 5) Assign partition cell IDs by first appearance of each unique truth table
    tt_to_id = {}
    next_id = 0
    for q in all_qs:
        tt = q_to_tt[q]
        if tt not in tt_to_id:
            tt_to_id[tt] = next_id
            next_id += 1

    # 6) Build ids per row in the original quantifier order
    def row_ids(lst):
        ids = [tt_to_id[q_to_tt[q]] for q in lst]
        return (join.join(map(str, ids)) if isinstance(join, str) else ids)

    ids_series = split_rows.apply(row_ids)
    return ids_series, tt_to_id