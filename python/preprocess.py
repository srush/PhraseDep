import argparse
from encoder import LexicalizedCFGEncoder
import train, tree
from grammar import read_rule_set

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


    args = parser.parse_args()

    X, Y = train.read_data_set(args.training_dep,
                               args.training_ps,
                               args.limit)

    grammar = read_rule_set(open(args.binarized_rules))
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
    for x, y in zip(X, Y):
        # if len(x.tags) > 50:  continue
        encoder = LexicalizedCFGEncoder(x.words, x.tags, grammar)
        bin_y = tree.binarize(orules, y)
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


if __name__ == "__main__":
    main()
