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


