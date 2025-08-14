import pandas as pd
import numpy as np
from glob import glob
import json
import pickle
from os.path import exists, join


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

    split_df = pd.DataFrame(
        df['hypothesis'].str.split('|').to_list(),
        columns=['c', 'q1', 'q2', 'q3']
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
    if isinstance(expression, list):
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
    if include_commdata:
        return df, [x['commData'] for x in data]
    return df


def count_substantial(hyp):
    """
    Count the number of substantial quantifiers in a hypothesis.
    Where substantial means that they somehow depend on the 
    """
    return sum(1 for q in hyp.split('|')[1:] if ('X.Q' in q) or ('X.c' in q))