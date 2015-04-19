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

if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)
    corpus = codecs.open(A.inputf, 'r', 'utf-8')
    for s in corpus:
        t = Tree.fromstring(s, remove_empty_top_bracketing=False)
        print " ".join(t.leaves())