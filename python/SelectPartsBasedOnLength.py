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

def select_parts(filename):
    f = codecs.open(filename, "r", "utf-8")

    f.close()
    return corpus

def flat_print(t):
    # print the tree in one single line
    return t._pprint_flat(nodesep='', parens='()', quotes=False)

if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)
    select_parts(A.inputf)

