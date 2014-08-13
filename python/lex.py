from collections import defaultdict, namedtuple
from pydecode.encoder import StructuredEncoder
import nltk
from nltk import Tree
import numpy as np
import pydecode
import cPickle as pickle

def read_original_rules(file):
    d = {}
    for l in open(file):
       t = l.split()
       X = t[0]
       rhs = t[2:-1]
       head = -1
       for i, nt in enumerate(rhs):
           if nt[-1] == "*":
               head = i
       rhs = tuple([i.strip("*") for i in t[2:-1]])
       rule_num = int(t[-1])
       d[X, rhs] = (rule_num, head)
    return d

def un_z(tree):
    if isinstance(tree, str): return [tree]
    if label(tree)[:2] == "Z_":
        return [flat
                for node in tree
                for flat in un_z(node)]
    else:
        return [unbinarize(tree)]

def un_z_node(tree):
    if isinstance(tree, str):
        return tree
    else:
        return [flat
                for node in tree
                for flat in un_z(node)]

def label(tree):
    if isinstance(tree, str): return tree
    return tree.label()

def unbinarize(tree):
    if isinstance(tree, str): return tree

    if label(tree[0])[:2] == "Z_":
        return Tree(label(tree), un_z_node(tree[0]) + [unbinarize(tree[1])])


    if len(tree) > 1 and label(tree[1])[:2] == "Z_":
        return Tree(label(tree), [unbinarize(tree[0])] + un_z_node(tree[1]))

    return Tree(tree.label(), [unbinarize(node)
                               for node in tree])


def binarize(original_rules, tree, head=None):
   if isinstance(tree, str): return tree
   children = []
   binarized_children = []
   for node in tree:
      binarized_children.append(binarize(original_rules, node))

   if len(tree) <= 2:
       return Tree(tree.label(), binarized_children)

   if len(tree) >=3:
       rule_num, new_head = original_rules[tree.label().split("^")[0],
                                           tuple([label(node).split("^")[0]
                                                  for node in tree])]
       absolute_head = int(tree.label().split("^")[1])
       if head is None:
           head = new_head
       cur = binarized_children[head]
       for i, node in enumerate(tree[head+1:]):
          node_label = "Z_(%d,r,%d)^%d"%(rule_num, len(tree) -2 -(head + 1 + i), absolute_head)
          if head + i + 2 == len(tree):
             if not tree[:head]:
                node_label = tree.label()
             else:
                node_label = "Z_(%d,l,%d)^%d"%(rule_num, len(tree[:head])- 1, absolute_head)

          cur = nltk.Tree(node_label, [cur, binarized_children[head + i + 1]])


       for i, node in enumerate(reversed(tree[:head])):
          node_label = "Z_(%d,l,%d)^%d"%(rule_num, head - i - 2, absolute_head)
          if head - i - 2 == -1:
             node_label = tree.label()
          cur = nltk.Tree(node_label, [binarized_children[head - i - 1], cur])

       return cur

class auto:
    def __init__(self):
        self.id = 0

    def __call__(self):
        self.id += 1
        return self.id

class SparseEncoder(StructuredEncoder):
    """
    A sparse structured encoder.
    """
    def __init__(self):
        self.encoder = defaultdict(auto())

    def transform_labels(self, labels):
        reverse = dict(zip(self.encoder.values(), self.encoder.keys()))
        return np.array([reverse[label] for label in labels])


class LexicalizedCFGEncoder(SparseEncoder):
    def save(self, file):
        out = zip(self.encoder.keys(), self.encoder.values())
        pickle.dump(out, open(file, "wb"))

    def load(self, file):
        out = pickle.load(open(file, "rb"))
        self.encoder = dict(out)



    def __init__(self, sentence, grammar):
        self.grammar = grammar
        self.sentence = sentence
        super(LexicalizedCFGEncoder, self).__init__()

    def transform_structure(self, parse):
        """
        Decompose a parse structure to parts.

        Parameters
        ----------
        parse : nltk.Tree
           A lexicalized parse tree with annotated nonterminals of the
           form X^i where X is a non-terminal symbol in the grammar and
           :math:`i \in \{0 \ldots n-1\}` is the index of the head word.

        Returns
        -------
        parts : int ndarray
           Each row is a part of the form (i, j, k, h, m, r) where
           i < j < k is the span and the split point, h is the head
           index, m is the modifier index, and r is the index of
           the rule used.
        """
        stack = [(parse, 0, len(parse.leaves())-1)]
        parts = []
        while stack:
            (node, i, k) = stack.pop()
            cur = i
            X, h = node.label().split("^")
            h = int(h)
            if len(node) == 2:
                Y, h_1 = label(node[0]).split("^")
                Z, h_2 = label(node[1]).split("^")
                h_1, h_2 = int(h_1), int(h_2)
                other = h_1 if h == h_2 else h_2

                r = self.grammar.rule_indices[X, Y, Z]
                j = i + len(node[0].leaves()) - 1
                parts.append((i, j, k, h, other, r))

            if len(node) == 1 and not isinstance(node[0], str):
                Y, h_1 = label(node[0]).split("^")
                r = self.grammar.unary_indices[X, Y]
                parts.append((i, k, k, h, h, r))

            for n in node:
                if isinstance(n, str):
                    cur += 1
                    continue
                stack.append((n, cur, cur + len(n.leaves())-1))
                cur += len(n.leaves())
        parts.reverse()
        return np.array(parts)

    def from_parts(self, parts):
        """
        Compose a set of parts into a parse structure.

        Parameters
        ----------
        parts : int ndarray
           Each row is a part of the form (i, j, k, h, m, r) where
           i < j < k is the span and the split point, h is the head
           index, m is the modifier index, and r is the index of
           the rule used.

        Returns
        -------
        parse : nltk.Tree
           A lexicalized parse tree with annotated nonterminals of the
           form X^i where X is a non-terminal symbol in the grammar and
           :math:`i \in \{0 \ldots n-1\}` is the index of the head word.
        """

        parse = {}
        for i in range(len(self.sentence)):
            parse[i, i] = str(self.sentence[i]) + "^" + str(i)

        for part in parts:
            i, j, k, h, _, r = part

            if i != k:
                X, _, __ = self.grammar.rule_rev_indices[r]
                parse[i, k] = nltk.Tree(X+"^"+str(h),
                                        [parse[i,j], parse[j+1, k]])
            else:
                X, _ = self.grammar.unary_rev_indices[r]
                parse[i, i] = nltk.Tree(X+"^"+str(h),
                                        [parse[i, i]])

        return parse[0, len(self.sentence)-1]

def pruning(n, dep_matrix):
    has_item = np.zeros(n*n*n).reshape((n,n,n)) # defaultdict(lambda: 0)
    head_item = defaultdict(list)
    heads = defaultdict(set)
    for i in range(n):
        has_item[i, i, i] = 1
        head_item[i, i].append((i, i, i))
        heads[i, i].add(i)

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n: continue
            for j in range(i, k):
                for h1 in heads[i, j]:
                    for h2 in heads[j+1, k]:
                        if dep_matrix[h1][h2]:
                            has_item[i, k, h1] = 1
                            head_item[i, k].append((h1, h2, j))
                            heads[i,k].add(h1)
                        if dep_matrix[h2][h1]:
                            has_item[i, k, h2] = 1
                            head_item[i, k].append((h2, h1, j))
                            heads[i,k].add(h2)
    return has_item, head_item


                # # for h in range(i, k+1):
                # #     for m in mod_of[h]:
                #         if (has_item[i, j, h] and has_item[j+1, k, m]) or \
                #                 (has_item[i, j, m] and has_item[j+1, k, h]):

def nt_pruning(sentence, grammar, span_pruner):
    N = len(grammar.nonterms)
    n = len(sentence)
    has_item = np.zeros((n, n, N), dtype=np.uint8)
    in_cell = defaultdict(list)
    cell_rules = defaultdict(list)
    cell_rule_set = defaultdict(set)
    for i in range(n):
        has_item[i,i, sentence[i]] = 1
        in_cell[i, i].append(sentence[i])
        for r, (X, Y) in enumerate(grammar.unary_table):
            if Y == sentence[i]:
                has_item[i, i, X] = 1
                in_cell[i, i].append(X)
        cell_rule_set[i, i, 0].update(*[
                grammar.rule_vec_first[X]
                for X in in_cell[i,i]])
        cell_rule_set[i, i, 1].update(*[
                grammar.rule_vec_second[X]
                for X in in_cell[i,i]])

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for h, m, j in span_pruner[i, k]:
                req_dir = 0 if h <= j else 1

                for r in (cell_rule_set[i, j, 0] & cell_rule_set[j+1, k, 1]):
                    # print r
                    # print grammar.rule_table[r]
                    X, Y, Z, dir = grammar.rule_table[r]
                    if req_dir != dir: continue
                    if not has_item[i, k, X]:
                        has_item[i, k, X] = 1
                        in_cell[i, k].append(X)
                    if dir == 0:
                        cell_rules[i, k, Y, 0].append((r, X, Z))
                    else:
                        cell_rules[i, k, Z, 1].append((r, X, Y))

            cell_rule_set[i, k, 0].update(*[
                grammar.rule_vec_first[X]
                for X in in_cell[i,k]])
            cell_rule_set[i, k, 1].update(*[
                grammar.rule_vec_second[X]
                for X in in_cell[i,k]])

    return has_item, in_cell, cell_rules

def make_mod(sentence, dep_matrix):
    n = len(sentence)
    mod_of = defaultdict(list)
    for h in range(n):
        for m in range(n):
            if dep_matrix[h][m]:
                mod_of[h].append(m)
    return mod_of

import time
def cky(sentence, rules, dep_matrix):
    """
    """
    # Preprocessing.
    n = len(sentence)

    # start = time.time()
    pruning_chart, span_pruner = pruning(n, dep_matrix)
    # print "A", time.time() - start
    # start = time.time()
    nt_pruning_chart, in_cell, cell_rules = nt_pruning(sentence, rules, span_pruner)
    seen_item = defaultdict(set)

    encoder = LexicalizedCFGEncoder(sentence, rules)
    items = defaultdict(auto())

    G = len(rules.rules)
    N = len(rules.nonterms)

    labels = encoder.encoder
    temp = np.arange(100000)
    chart = pydecode.ChartBuilder(temp, unstrict=True)
    has_item = defaultdict(lambda: 0) #np.zeros(n*n*n*N).reshape((n,n,n,N))

    # Initialize the chart.
    for i in range(n):
        chart.init(items[i, i, i, sentence[i]])
        has_item[i, i, i, sentence[i]] = 1
        seen_item[i, i, i].add(sentence[i])
        for r, (X, Y) in enumerate(rules.unary_table):
            if Y == sentence[i]:
                chart.set(items[i, i, i, X],
                          [[items[i, i, i, Y]]],
                          labels=[labels[i, i, i, i, i, G + r]])
                has_item[i, i, i, X] = 1
                seen_item[i, i, i].add(X)


    # print "B", time.time() - start
    # start = time.time()
    # Main loop.
    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for h, m, j in span_pruner[i, k]:
                stash = defaultdict(list)

                if h <= j:
                    for Y in seen_item[i, j, h]:
                        for r, X, Z in cell_rules[i, k, Y, 0]:
                            if has_item[j+1, k, m, Z]:
                                stash[X].append([r, Y, Z, j, h, m, 0])

                if h > j:
                    for Z in seen_item[j+1, k, h]:
                        for r, X, Y in cell_rules[i, k, Z, 1]:
                            if has_item[i, j, m, Y]:
                                stash[X].append([r, Y, Z, j, h, m, 1])

                for X in stash:
                    label, edges  = zip(*[(labels[i, j, k, h, m, r],
                                           [items[i, j, h if dir == 0 else m, Y],
                                            items[j+1, k, m if dir == 0 else h, Z]])
                                          for r, Y, Z, j, h, m, dir in stash[X]])

                    chart.set(items[i, k, h, X], edges, labels=label)
                    has_item[i, k, h, X] = 1
                    seen_item[i, k, h].add(X)
    chart.set(items[n-1, 0, 0, 0],
              [[items[0, n-1, h, nonterm]]
               for h in range(n)
               for nonterm in range(N)
               if has_item[0, n-1, h, nonterm]])
    # print "C", time.time() - start
    return chart.finish(True), encoder


def pruning_graph(n, mod_of):
    labels = defaultdict(auto())
    items = np.arange(n * n * n).reshape((n, n, n))
    chart = pydecode.ChartBuilder(items, unstrict=True)
    has_item = defaultdict(lambda: 0)

    for i in range(n):
        chart.init(items[i, i, i])
        has_item[i, i, i] = 1

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n: continue
            for h in range(i, k+1):
                edges = [[items[i, j, h], items[j+1, k, m]]
                         for m in mod_of[h]
                         for j in range(i, k)
                         if has_item[i, j, h] and has_item[j+1, k, m]] + \
                         [[items[i, j, m], items[j+1, k, h]]
                          for m in mod_of[h]
                          for j in range(i, k)
                          if has_item[i, j, m] and has_item[j+1, k, h]]

                if len(edges) > 0:
                    chart.set(items[i, k, h], edges)
                    has_item[i, k, h] = 1
    chart.set(items[n-1, 0, 0],
              [[items[0, n-1, h]]
               for h in range(n)
               if has_item[0, n-1, h]])
    return chart.finish(True)

def nt_pruning_graph(sentence, grammar, span_pruner):
    N = len(grammar.nonterms)
    n = len(sentence)
    labels = defaultdict(auto())
    items = np.arange(n * n * N).reshape((n, n, N))
    chart = pydecode.ChartBuilder(items, unstrict=True)

    has_item = np.zeros((n, n, N), dtype=np.uint8)
    in_cell = defaultdict(list)

    for i in range(n):
        has_item[i,i, sentence[i]] = 1
        in_cell[i, i].append(sentence[i])
        chart.init(items[i, i, sentence[i]])
        for r, (X, Y) in enumerate(grammar.unary_table):
            if Y == sentence[i]:
                has_item[i, i, X] = 1
                in_cell[i, i].append(X)
                chart.set(items[i, i, X],
                          [[items[i, i, sentence[i]]]])

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            if not span_pruner[i, k]: continue
            stash = defaultdict(list)
            for h, m, j in span_pruner[i, k]:
                req_dir = 0 if h <= j else 1
                if not span_pruner[i, j] or not span_pruner[j+1, k]: continue
                for Y in in_cell[i, j]:
                    for r, X, Z, dir in grammar.rules_by_first[Y]:
                        if dir != req_dir: continue
                        if has_item[j+1, k, Z]:
                            stash[X].append((Y, Z, j, r))
            for X in stash:
                chart.set(items[i, k, X],
                          [[items[i, j, Y], items[j+1, k, Z]]
                           for Y, Z, j, r in stash[X]])

                has_item[i, k, X] = 1
                in_cell[i, k].append(X)

    chart.set(items[n-1, 0, 0],
              [[items[0, n-1, X]]
               for X in range(N)
               if has_item[0, n-1, X]])

    return chart.finish(True)
