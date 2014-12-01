from nltk import Tree
from copy import deepcopy
import sys
import NewTree
import argparse
import os
import re
import sys
import codecs

parser = argparse.ArgumentParser(description='')
parser.add_argument('--phrase_tree_file', type=str, metavar='', help='')
parser.add_argument('--tagging_file', type=str, metavar='', help='')

A = parser.parse_args()

def read_corpus(filename):
    f = codecs.open(filename, "r","utf-8")
    corpus = []
    sentence = []
    for line in f:
        if line[:-1] == "":
            corpus.append(sentence)
            sentence = []
            continue
        else:
            line = line[:-1]
            cline = line.split(u"\t")
            sentence.append(cline)
    f.close()
    return corpus

def tag_phrase_tree(treebank_file, corpus):
    f = codecs.open(treebank_file, "r", "utf-8")
    s_ind = -1
    for sentence in f:
        s_ind += 1
        if s_ind % 10 == 0:
            sys.stderr.write(unicode(s_ind) + u"\n")
        tree = Tree.fromstring(sentence, remove_empty_top_bracketing=False)
        preterminals = [t for t in tree.subtrees(lambda t: t.height() == 2)]
        for i in xrange(len(preterminals)):
            preterminals[i].set_label(corpus[s_ind][i][1])
        sys.stdout.write(NewTree.flat_print(tree) + u"\n")


if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    corpus = read_corpus(A.tagging_file)
    tag_phrase_tree(A.phrase_tree_file, corpus)