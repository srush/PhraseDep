from copy import deepcopy
import sys
import argparse
import os
import re
import sys
import codecs

parser = argparse.ArgumentParser(description='')
parser.add_argument('--redshift_file', type=str, metavar='', help='')
parser.add_argument('--mode', type=str, metavar='', help='')

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

def print_sentence(sentence, outputf):
    for line in sentence:
        s = ""
        for field in line:
            s += field + "\t"
        s = s.strip()
        outputf.write(s+"\n")
    outputf.write("\n")
    return

if __name__ == '__main__':
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

    corpus = read_corpus(A.redshift_file)
    mode = A.mode

    for sen in corpus:
        length = len(sen)
        if mode == 0:
            for line in sen:
                if int(line[6]) == (length + 1):
                    line[6] = '0'
        else:
            for line in sen:
                if int(line[6]) == 0:
                    line[6] = str(length+1)
    for sen in corpus:
        print_sentence(sen, sys.stdout)