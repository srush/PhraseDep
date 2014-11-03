import argparse
from encoder import LexicalizedCFGEncoder
import train, tree
from grammar import read_rule_set
from collections import defaultdict

def make_bounds(dep_matrix):
    head = {}

    n = len(dep_matrix)
    used = set()
    for h in range(n):
        for m in range(n):
            if dep_matrix[h][m]:
                used.add(m)
                head[m] = h

    for m in range(n):
        if m not in used:
            head[m] = -1

    bounds = {}
    for i in range(n):
        for k in range(i, n+1):
            for j in range(i, k):
                if not (i <= head[j] < k):
                    bounds[i, k] = j
    return bounds


def main():
    parser = argparse.ArgumentParser(description='Preprocess parsing data.')
    parser.add_argument('--training_ps', type=str,
                        help='Lexicalized phrase structure file.')
    parser.add_argument('--training_dep', type=str, help='')
    parser.add_argument('--limit', type=int, help='')
    parser.add_argument('--binarized_rules', type=str, help='Binarized rule file')
    parser.add_argument('--original_rules', type=str, help='Original rule file')
    parser.add_argument('--parse_output', type=str, help='Binarized rule file')
    parser.add_argument('--rule_output', type=str, help='Binarized rule file')
    parser.add_argument('--pruning', type=str, help='Pruning  file')


    args = parser.parse_args()

    X, Y = train.read_data_set(args.training_dep,
                               args.training_ps,
                               args.limit)

    # X is,
    # ParseInput = namedtuple("ParseInput", ["words", "tags", "deps", "index"])
    # index is the index of the sentence, and deps is a matrix encode the dependencies
    # Y is,
    # ImmutableTree representation of the phrase structure tree, not necessarily binarized.

    grammar = read_rule_set(open(args.binarized_rules))

    # grammar contains a set of bin rules
    # BinarizedRule: namedtuple("BinarizedRule",["X", "Y", "Z", "original", "direction", "mins"])
    # where mins l, r is the min l and r number if use this rule
    # Say X -> Y_1* Y_2 Y_3
    # size is 3 here and head index is 0
    # 3 - 0 - 1 = 2 is the rightMin
    # left min is just 0 here (head position)

    orules = tree.read_original_rules(open(args.original_rules))

    out = open(args.rule_output, "w")
    rule_num = 0
    for rule in grammar.rules:
        print >>out, rule_num, 0, rule.X, rule.Y, rule.Z, rule.direction
        rule_num += 1

    for rule in grammar.unary_rules:
        print >>out, rule_num, 1, rule.X, rule.Y
        rule_num += 1


    out = open(args.parse_output, "w")
    counts = defaultdict(lambda:defaultdict(lambda:0))
    for x, y in zip(X, Y):
        # if len(x.tags) > 50:  continue
        encoder = LexicalizedCFGEncoder(x.words, x.tags, grammar)

        # y is lex (^index) ps tree
        # bin_y is bin lex (^index) ps tree
        # they are all instances of ImmutableTree

        bounds = make_bounds(x.deps)
        bin_y, _, _ = tree.binarize(orules, bounds, y)

        parts = encoder.transform_structure(bin_y)

        n = len(x.tags)
        print >>out, len(x.tags), len(parts)
        print >>out, " ".join(x.words)
        print >>out, " ".join(x.tags)
        for m in range(n):
            found = False
            for h in range(n):
                if x.deps[h][m]:
                    print >>out, h,
                    found = True
            if not found:
                print >>out, -1,
        print >>out, ""

        for p in parts:
            print >>out, " ".join(map(str, p))
            counts[p[5]][x.tags[p[3]]] += 1

    out = open(args.pruning, "w")
    for k in range(rule_num):
        print >>out, k, len(counts[k]), " ".join(("%s"%(c) for c, v in counts[k].iteritems()))
if __name__ == "__main__":
    main()
