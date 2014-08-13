from StringIO import StringIO
import lex
import format
from format import BinarizedRule, UnaryRule
import nltk
from nltk import Tree
import numpy as np

def test_grammar():
    grammar = format.RuleSet()
    rule = BinarizedRule
    grammar.add(rule("S", "NP", "VP", 0, 0))
    grammar.add_unary(UnaryRule("NP", "N"))
    grammar.add_unary(UnaryRule("VP", "V"))
    grammar.finish()
    tree = nltk.Tree.fromstring("(S^1 (NP^0 N^0) (VP^1 V^1))")

    encoder = lex.LexicalizedCFGEncoder(["N", "V"], grammar)

    parts = encoder.transform_structure(tree)
    print parts
    tree2 = encoder.from_parts(parts)
    assert(tree == tree2)


def test_read_rules():
    rules = StringIO("""VP	-->	VBZ*	UCP		653
S	-->	NP*	ADVP		1390
Z_(2229,l,0)	-->	PRN	Z_(2229,l,1)*		2229
Z_(2229,l,1)	-->	VP*	.		2229
S	-->	S	Z_(2229,l,0)*		2229
Z_(673,l,0)	-->	NNP	NNS*		673
""")
    rules = format.read_rule_set(rules)
    assert rules.rules[0] == BinarizedRule("VP", "VBZ", "UCP", 653, 0)
    assert rules.rules[1] == BinarizedRule("S", "NP", "ADVP", 1390, 0)
    assert rules.rules[2] == BinarizedRule("Z_(2229,l,0)", "PRN", "Z_(2229,l,1)", 2229, 1), rules.rules[2]
    assert (rules.rule_table == np.array([[0, 1, 2, 0],
                                         [3, 4, 5, 0],
                                         [6, 7, 8, 1],
                                         [8, 0, 9, 0],
                                         [3, 3, 6, 1],
                                         [10, 11, 12, 1]])).all(), rules.rule_table


def test_binarize():
    rules = lex.read_original_rules("corpora/orules")
    tree = nltk.Tree('NP', ('DT', 'NNP', 'NNP', 'CC', 'NNP', 'NNP'))
    correct = Tree('NP', ['DT', Tree('Z_(4,l,0)', ['NNP', Tree('Z_(4,l,1)', ['NNP', Tree('Z_(4,l,2)', ['CC', Tree('Z_(4,l,3)', ['NNP', 'NNP'])])])])])

    bin_tree =  lex.binarize(rules, tree, 4)
    check = lex.unbinarize(rules, bin_tree)
    assert(check == tree)
    assert(bin_tree == correct)

    correct = Tree('NP', [Tree('Z_(4,r,0)', [Tree('Z_(4,r,1)', [Tree('Z_(4,r,2)', [Tree('Z_(4,r,3)', ['DT', 'NNP']), 'NNP']), 'CC']), 'NNP']), 'NNP'])
    bin_tree =  lex.binarize(rules, tree, 0)
    check = lex.unbinarize(rules, bin_tree)
    assert(check == tree)
    assert(bin_tree == correct)


    correct = Tree('NP', ['DT', Tree('Z_(4,l,0)', ['NNP', Tree('Z_(4,l,1)', ['NNP', Tree('Z_(4,l,2)', [Tree('Z_(4,r,0)', ['CC', 'NNP']), 'NNP'])])])])
    bin_tree =  lex.binarize(rules, tree, 3)
    assert(bin_tree == correct)
    check = lex.unbinarize(rules, bin_tree)
    assert(check == tree)
    #bin_tree.draw()
