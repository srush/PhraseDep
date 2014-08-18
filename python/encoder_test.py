from nltk import ImmutableTree as Tree
from grammar import RuleSet, BinarizedRule, UnaryRule
from encoder import *

def test_grammar():
    grammar = RuleSet()
    grammar.add(BinarizedRule("S", "NP", "VP", 0, 0, (0,0)))
    grammar.add_unary(UnaryRule("NP", "N"))
    grammar.add_unary(UnaryRule("VP", "V"))
    grammar.finish()
    tree = Tree.fromstring("(S^2 (NP^1 (N^1 dog^1)) (VP^2 (V^2 flies^2)))")

    encoder = LexicalizedCFGEncoder(["dog", "flies"], 
                                    ["N", "V"], 
                                    grammar)

    parts = encoder.transform_structure(tree)
    print parts
    tree2 = encoder.from_parts(parts)
    print tree
    print tree2
    assert(tree == tree2)


