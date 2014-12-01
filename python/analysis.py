from collections import namedtuple
import argparse
import pandas as pd
class Item(namedtuple("Item", ("i", "j", "k", "h", "m", "r"))):
    pass

def find_spans(h, deps, left, right):
    left[h] = h
    right[h] = h
    for i in range(0, len(deps)):
        if deps[i] == h:
            find_spans(i, deps, left, right)
            if i < h:
                left[h] = min(left[h], (left)[i]);
            else:
                right[h] = max(right[h], right[i]);


def read_file(handle):
    sent_num = 0
    sents = []
    while True:

        items = []
        l = handle.readline()
        if not l: break
        n, n_items = map(int, l.split())
        words = handle.readline().split()
        tags = handle.readline()
        deps = map(int, handle.readline().split())
        left = [-1] * len(words)
        right = [-1] * len(words)
        for i in range(len(deps)):
            if deps[i] == -1:
                find_spans(i, deps, left, right)

        labels = handle.readline()
        for i in range(n_items):
            l = handle.readline()
            items.append((sent_num, Item(*map(int, l.split()))))

        sent_num += 1
        sents.append( {"items" : items,
                       "spans" : zip(left, right),
                       "deps" : deps
                   })
    return sents

def read_rules(handle):
    rules = {}
    for l in handle:
        t = l.split()
        num = int(t[0])
        unary = int(t[1])
        A = t[2]
        rules[num] = A
    return rules

def fscore(s1, s2):
    hits = len(s1 & s2)
    total = len(s1)
    guess = len(s2)
    recall = hits / float(total)
    precision = hits / float(guess)
    print recall, precision


def score(gold_file, predicted_file, rules):
    s1 = read_file(open(gold_file))
    s2 = read_file(open(predicted_file))
    rules = read_rules(open(rules))

    d = {"sent" : [],
         "head_match" : [],
         "mod_match" : [],
         "exact_match" : [],
         "span_match" : [],
         "top_match" : [],
         "spine" : [],
         "top_nt" : [],
         "dep_correct" : []
    }

    total_gold = 0
    total_predicted = 0
    for a, b in zip(s2, s1):
        gold = set(b["items"])
        total_gold += len(b["items"])
        total_predicted += len(a["items"])
        spans = []
        for sent, item in b["items"]:
            spans.append((item.i, item.k))
        gold_spans = set(spans)

        for sent, item in b["items"]:
            spans.append((item.i, item.k, rules[item.r]))
        gold_spans_top = set(spans)

        for sent, item in a["items"]:
            d["sent"].append(sent)
            d["head_match"].append(a["spans"][item.h] == b["spans"][item.h])
            d["mod_match"].append(a["spans"][item.m] == b["spans"][item.m])
            d["exact_match"].append((sent, item) in gold)
            d["span_match"].append((item.i, item.k) in gold_spans)
            d["top_match"].append((item.i, item.k, rules[item.r]) in gold_spans_top)
            d["spine"].append((item.i, item.k) == a["spans"][item.h])
            d["dep_correct"].append(item.h == b["deps"][item.m])
            d["top_nt"].append(rules[item.r])
    return pd.DataFrame(d)

def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--gold_items', type=str, metavar='', help='')
    parser.add_argument('--predicted_items', type=str, metavar='', help='')
    parser.add_argument('--rules', type=str, metavar='', help='')

    args = parser.parse_args()

    hits = df["exact_match"].sum()
    recall = hits / float(total_gold)
    precision = hits / float(total_predicted)

    #print recall, precision
# main()
