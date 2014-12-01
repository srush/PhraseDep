# Lingpeng Kong, lingpenk@cs.cmu.edu

import argparse
import codecs
import os
import re
import sys
from nltk import Tree

parser = argparse.ArgumentParser(description='')
parser.add_argument('inputf', type=str, metavar='', help='')

A = parser.parse_args()

def split_sentences(filename):
    f = codecs.open(filename, "r", "utf-8")
    l_str = f.read()
    mode = 0
    sen = ""
    corpus = []
    left = 0
    # outside a tree
    mode = 0
    for i in xrange(0, len(l_str)):
        if mode == 0:
            if l_str[i] in [' ', '\n', '\t']:
                continue
        if l_str[i] == u'(':
            mode = 1
            left = left + 1
        if l_str[i] == u')':
            left = left - 1
        sen = sen + l_str[i]
        if left ==0:
            # split point
            corpus.append(sen)
            mode = 0
            #print sen
            sen = ""
    f.close()
    return corpus

def flat_print(t):
    # print the tree in one single line
    return t._pprint_flat(nodesep='', parens='()', quotes=False)

if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)
    corpus = split_sentences(A.inputf)
    for s in corpus:
        #print "*" + s + "*"
        t = Tree.fromstring(s, remove_empty_top_bracketing=False)
        print flat_print(t)

