from copy import deepcopy
import sys
import argparse
import os
import re
import sys
import codecs
import random

parser = argparse.ArgumentParser(description='')
parser.add_argument('--ptb', type=str, metavar='', help='')

A = parser.parse_args()

if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    # l = ['a','b','c','d','e','f','g']
    # rset = random.sample(range(len(l)),2)
    # c = [l[i] for i in rset]
    # r = [l[i] for i in xrange(0,len(l)) if i not in rset]
    # print c
    # print r
    # dep_corpus = read_corpus(A.dep)

    ptb_corpus = []
    f = codecs.open(A.ptb, "r", "utf-8")
    for line in f:
        ptb_corpus.append(line)
    f.close()

    num5 = int(0.05 * len(ptb_corpus))
    num20 = int(0.20 * len(ptb_corpus))
    num50 = int(0.50 * len(ptb_corpus))
    num80 = int(0.80 * len(ptb_corpus))
    num100 = int(0.20 * len(ptb_corpus))

    # sentence
    random.shuffle(ptb_corpus)
    
    set5 = ptb_corpus[:num5]
    f5 = codecs.open(A.ptb + ".split0-5", "w")
    for sen in set5:
        f5.write(sen.strip()+"\n")
    f5.close()

    set20 = ptb_corpus[num5:num20]
    f20 = codecs.open(A.ptb + ".split5-20", "w")
    for sen in set20:
        f20.write(sen.strip()+"\n")
    f20.close()

    set50 = ptb_corpus[num5:num50]
    f50 = codecs.open(A.ptb + ".split5-50", "w")
    for sen in set50:
        f50.write(sen.strip()+"\n")
    f50.close()

    set80 = ptb_corpus[num5:num80]
    f80 = codecs.open(A.ptb + ".split5-80", "w")
    for sen in set80:
        f80.write(sen.strip()+"\n")
    f80.close()

    set100 = ptb_corpus[num5:]
    f100 = codecs.open(A.ptb + ".split5-100", "w")
    for sen in set100:
        f100.write(sen.strip()+"\n")
    f100.close()

