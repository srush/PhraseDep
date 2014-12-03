from collections import namedtuple, defaultdict
import argparse
import pandas as pd
import math
import numpy as np

class Item(namedtuple("Item", ("i", "j", "k", "h", "m", "r"))):
    pass

def find_spans(h, deps, left, right, children):
    left[h] = h
    right[h] = h
    children[h] = []
    for i in range(0, len(deps)):
        if deps[i] == h:
            children[h].append(i)
            find_spans(i, deps, left, right, children)
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
        tags = handle.readline().split()

        deps = map(int, handle.readline().split())
        left = [-1] * len(words)
        right = [-1] * len(words)
        children = [None] * len(words)
        for i in range(len(deps)):
            if deps[i] == -1:
                find_spans(i, deps, left, right, children)

        labels = handle.readline()
        for i in range(n_items):
            l = handle.readline()
            items.append((sent_num, Item(*map(int, l.split()))))

        sent_num += 1
        sents.append( {"items" : items,
                       "spans" : zip(left, right),
                       "deps" : deps,
                       "tags" : tags,
                       "children": children
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

def nCr(n,r):
    f = math.factorial
    return f(n) / f(r) / f(n-r)

def prune():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('--gold_items', type=str, metavar='', help='')
    args = parser.parse_args()
    s1 = read_file(open(args.gold_items))
    d = defaultdict(lambda: [set(), 0])
    pairs = defaultdict(lambda: [0, 0])
    for sent in s1:
        # print sent["deps"], sent["spans"]
        for h in range(len(sent["deps"])):
            left = [c for c in sent["children"][h] if c < h]
            left.reverse()
            right = [c for c in sent["children"][h] if c > h]
            l = 0
            r = 0
            if left and right:
                # print len(left), len(right)
                b_string = ""
                for _, item in sent["items"]:
                    m = item.m
                    h_tag = sent["tags"][h]
                    l_tag = "*END*" if l >= len(left) else (sent["tags"][left[l]] + str(l == 0))
                    r_tag = "*END*" if r >= len(right) else (sent["tags"][right[r]] + str(r == 0))

                    key = h_tag, l_tag, r_tag
                    if item.h == h and item.m != h:
                        if item.m > h:
                            b_string += "1"

                            pairs[key][1] += 1
                            assert(right[r] == m)
                            r += 1
                        else:
                            b_string += "0"
                            pairs[key][0] += 1
                            assert(left[l] == m)
                            l += 1
                        # print "\t", h, item.j, item.m, item.k -item.i, item.m > h

                d[len(left), len(right), sent["tags"][h]][0].add(b_string)
                d[len(left), len(right), sent["tags"][h]][1] += 1

            #d[left, right, sent["tags"][i]] = 1

    to_write = []
    for key in pairs:
        total = sum(pairs[key])
        if "*END*" not in key:
            to_write.append((key, max(pairs[key][0], pairs[key][1]) / float(total), total))
    to_write.sort(key=lambda a: a[2])

    for l in to_write:
        print l


    # all_prune = []
    # for a in d:
    #     total = nCr(a[0] + a[1], a[1])
    #     if d[a][1] > 10 and (a[0] > 2 and a[1] > 2):
    #         valid = np.zeros((a[0]+1, a[1]+1))
    #         for bstring in d[a][0]:
    #             l = 0
    #             r = 0
    #             for b in bstring:
    #                 if b == "0":
    #                     l += 1
    #                 else:
    #                     r += 1
    #                 valid[l, r] = 1

    #         all_prune.append((a, d[a][1], len(d[a][0]), valid))
    # all_prune.sort(key=lambda a: a[1])

    # for j in all_prune:
    #     print j[0], j[1], j[2]
    #     print j[3]


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
prune()
