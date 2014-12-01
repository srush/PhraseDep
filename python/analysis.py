yyfrom collections import namedtuple
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
                       "spans" : zip(left, right)
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


def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--gold_items', type=str, metavar='', help='')
    parser.add_argument('--predicted_items', type=str, metavar='', help='')
    parser.add_argument('--rules', type=str, metavar='', help='')

    args = parser.parse_args()

    s1 = read_file(open(args.gold_items))
    s2 = read_file(open(args.predicted_items))
    rules = read_rules(open(args.rules))

    d = {"sent" : [],
         "head_match" : [],
         "mod_match" : [],
         "exact_match" : [],
         "span_match" : [],
         "top_match" : [],
         "spine" : []
    }

    total_gold = 0
    total_predicted = 0
    for a, b in zip(s1, s2):
        gold = set(b["items"])
        total_gold += len(b["items"])
        total_predicted += len(a["items"])
        spans = []
        for sent, item in b["items"]:
            spans.append(((item.i, item.k), item.r))
        gold_spans = dict(spans)

        for sent, item in a["items"]:
            d["sent"].append(sent)
            d["head_match"].append(a["spans"][item.h] == b["spans"][item.h])
            d["mod_match"].append(a["spans"][item.m] == b["spans"][item.m])
            d["exact_match"].append((sent, item) in gold)
            d["span_match"].append((item.i, item.j) in gold_spans)
            d["top_match"].append(gold_spans.get((item.i, item.j), None) == rules[item.r])
            d["spine"].append()
    df = pd.DataFrame(d)
    hits = df["exact_match"].sum()
    recall = hits / float(total_gold)
    precision = hits / float(total_predicted)
    print recall, precision
main()
