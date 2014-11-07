from nltk import Tree
from copy import deepcopy
import sys
import NewTree
import argparse
import os
import re
import sys

parser = argparse.ArgumentParser(description='')
parser.add_argument('inputf', type=str, metavar='', help='')


A = parser.parse_args()

def gen_root(treebank_file):
    # if you use unicode here, there is a bug...
    f = open(treebank_file, "r")
    root_set = set([])
    for sentence in f:
        t = Tree.fromstring(sentence, remove_empty_top_bracketing=True)
        root = t.label()
        root_set.add(root)
    f.close()
    for r in root_set:
        print r

def add_top_to_tree(treebank_file):
    f = open(treebank_file, "r")
    root_set = set([])
    for sentence in f:
        t = Tree.fromstring(sentence, remove_empty_top_bracketing=False)
        top_node = Tree("TOP", [])
        top_node.append(t)
        print NewTree.flat_print(top_node)
    f.close()

if __name__ == '__main__':
    # sentence = """(S (VP (NP (NNP Ms.) (NNP Haag)) (VP (VBZ plays) (NP (NNP Elianti))) (. .)))"""
    # t = Tree.fromstring(sentence, remove_empty_top_bracketing=False)
    # top_node = Tree("TOP", [])
    # top_node.append(t)
    # NewTree.collapse_unary(top_node)
    # print top_node.pprint()

    add_top_to_tree(A.inputf)

    #gen_root(A.inputf)
