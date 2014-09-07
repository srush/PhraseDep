"""
This file implements the dynamic programming algorithms for
lexicalized CFG parsing.
"""
from encoder import LexicalizedCFGEncoder, auto
from collections import defaultdict
import numpy as np
import pydecode

# def count_mods(dep_matrix):
#     n = len(dep_matrix)
#     counts = np.zeros((n, 2))
#     for h in range(n):
#         for m in range(n):
#             if dep_matrix[h][m]:
#                 dir_ = 0 if m < h else 1
#                 counts[h, dir_] += 1
#     return counts

# def make_mod(sentence, dep_matrix):
#     n = len(sentence)
#     mod_of = defaultdict(list)
#     for h in range(n):
#         for m in range(n):
#             if dep_matrix[h][m]:
#                 mod_of[h].append(m)
#     return mod_of

def span_pruning(n, dep_matrix):
    """
    Construct a chart giving spans to prune.

    Parameters
    ----------
    n : int
        The length of the sentence.

    dep_matrix : tuple
        dep_matrix[h][m] true is h->m link.

    Returns
    -------
    chart : dict
       chart[i, k] is a list of items of the form (h, m, j)
       indicating all valid split-points j and h -> m arcs.
    """

    chart = defaultdict(list)
    heads = defaultdict(set)

    for i in range(n):
        chart[i, i].append((i, i, i))
        heads[i, i].add(i)

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for j in range(i, k):
                for h1 in heads[i, j]:
                    for h2 in heads[j+1, k]:
                        if dep_matrix[h1][h2]:
                            chart[i, k].append((h1, h2, j))
                            heads[i, k].add(h1)
                        if dep_matrix[h2][h1]:
                            chart[i, k].append((h2, h1, j))
                            heads[i, k].add(h2)
    return chart

def grammar_pruning(preterms, grammar, span_pruner):
    """
    Construct a pruning chart for grammar rules.


    Returns
    -------
    chart : dict
       chart[i, k, Y, dir] is a list of items of the form (r, X, Z)
       indicating a rule with non terms possible at this location.

    """
    N = len(grammar.nonterms)
    n = len(preterms)


    # Indicator of cell in span.
    has_item = np.zeros((n, n, N), dtype=np.uint8)

    # Nonterminals in span (i, k).
    cell_nts = defaultdict(set)

    # Rules used for span/split (i,j,k)
    cell_rules = defaultdict(list)
    G = len(grammar.rules)

    for i in range(n):
        has_item[i, i, preterms[i]] = 1
        cell_nts[i, i].add(preterms[i])
        Y = preterms[i]
        for r, X in grammar.unary_rules_by_first[Y]:
            has_item[i, i, X] = 1
            cell_nts[i, i].add(X)
            for _, X_up in grammar.unary_rules_by_first[X]:
                if not has_item[i, i, X_up]:
                    has_item[i, i, X_up] = 1
                    cell_nts[i, i].add(X_up)

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for h, _, j in span_pruner[i, k]:
                req_dir_ = 0 if h <= j else 1
                for Y in cell_nts[i, j]:
                    for r, X, Z, dir_ in grammar.rules_by_first[Y]:
                        if dir_ != req_dir_:
                            continue

                        if not has_item[i, k, X]:
                            has_item[i, k, X] = 1
                            cell_nts[i, k].add(X)
                            for _, X_up in grammar.unary_rules_by_first[X]:
                                has_item[i, k, X_up] = 1
                                cell_nts[i, k].add(X_up)

                                for _, X_up2 in grammar.unary_rules_by_first[X_up]:
                                    has_item[i, k, X_up2] = 1
                                    cell_nts[i, k].add(X_up2)

                        assert r < G
                        if dir_ == 0:
                            cell_rules[i, k, Y, 0].append((r, X, Z))
                        else:
                            cell_rules[i, k, Z, 1].append((r, X, Y))


    return cell_rules

import time

# Two layers to allow unary rules.
START=0
MID=1
DONE=2

def cky(sentence, tags, grammar, dep_matrix):
    """
    """
    # Preprocessing.
    n = len(sentence)
    preterms = [grammar.nonterms[tag]
                for tag in tags]

    span_pruner = span_pruning(n, dep_matrix)

    cell_rules = \
        grammar_pruning(preterms, grammar, span_pruner)

    span_nts = defaultdict(set)
    encoder = LexicalizedCFGEncoder(sentence, tags, grammar)
    items = defaultdict(auto())

    G = len(grammar.rules)

    labels = encoder.encoder
    temp = np.arange(1000000)
    chart = pydecode.ChartBuilder(temp, unstrict=True)
    has_item = defaultdict(lambda: 0)
    #np.zeros(n*n*n*N).reshape((n,n,n,N))

    # Initialize the chart.
    for i in range(n):
        chart.init(items[i, i, i, preterms[i], START])
        chart.set(items[i, i, i, preterms[i], DONE],
                  [[items[i, i, i, preterms[i], START]]], [-1])

        has_item[i, i, i, preterms[i]] = 1
        span_nts[i, i, i].add(preterms[i])
        Y = preterms[i]
        unary = defaultdict(set)
        seen = set()
        for r, X in grammar.unary_rules_by_first[Y]:
            chart.set(items[i, i, i, X, MID],
                      [[items[i, i, i, Y, START]]],
                      labels=[labels[i, i, i, i, i, r]])
            # chart.set(items[i, i, i, X, DONE],
            #           [[items[i, i, i, X, MID]]])
            has_item[i, i, i, X] = 1
            span_nts[i, i, i].add(X)
            for r_up, X_up in grammar.unary_rules_by_first[X]:
                unary[X_up].add((r_up, X))

            seen.add(X)

        to_finish = set(unary.keys()) | seen
        for X_up in to_finish:
            if X_up in unary:
                edges, labels_ = zip(*[
                    ([items[i, i, i, X, MID]],
                     labels[i, i, i, i, i, r_up])
                    for r_up, X in unary[X_up]])
            else:
                edges, labels_ = (), ()
            if X_up in seen:
                labels_ += (-1,)
                edges += ([items[i, i, i, X_up, MID]],)
            chart.set(items[i, i, i, X_up, DONE], edges, labels_)
            has_item[i, i, i, X_up] = 1
            span_nts[i, i, i].add(X_up)
            # print i, grammar.nonterm_name(X_up)

    # Main loop.
    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            # if i == 3 and k == 4:
            #     print span_pruner[i, k]
            to_add = defaultdict(list)
            for h, m, j in span_pruner[i, k]:


                if h <= j:
                    for Y in span_nts[i, j, h]:
                        # if i == 3 and k == 4:
                        #     print grammar.nonterm_name(Y)
                        for r, X, Z in cell_rules[i, k, Y, 0]:
                            # if i == 3 and k == 4:
                            #     print j+1, k, m, grammar.nonterm_name(Z), has_item[j+1, k, m, Z]
                            if has_item[j+1, k, m, Z]:
                                to_add[X, h].append([r, Y, Z, j, h, m, 0])
                                assert r < G, "%s %s"%(r, G)
                if h > j:
                    for Z in span_nts[j+1, k, h]:
                        for r, X, Y in cell_rules[i, k, Z, 1]:
                            if has_item[i, j, m, Y]:

                                to_add[X, h].append([r, Y, Z, j, h, m, 1])
                                assert r < G, "%s %s"%(r, G)

            unary = defaultdict(list)
            unary2 = defaultdict(list)
            for X, h in to_add:
                labels_, edges  = zip(*[
                    (labels[i, j, k, h, m, r],
                     [items[i, j, h if dir_ == 0 else m, Y, DONE],
                      items[j+1, k, m if dir_ == 0 else h, Z, DONE]])
                    for r, Y, Z, j, h, m, dir_ in to_add[X, h]])


                chart.set(items[i, k, h, X, START], edges,
                          labels=labels_)
                assert not has_item[i, k, h, X]
                has_item[i, k, h, X] = 1
                span_nts[i, k, h].add(X)
                for r_unary, X_up in grammar.unary_rules_by_first[X]:
                    unary[X_up, h].append((r_unary, X))


            for X, h in unary:
                labels_, edges  = zip(*[
                    (labels[i, k, k, h, h, r],
                     [items[i, k, h, Y, START]])
                    for r, Y in unary[X, h]])
                has_item[i, k, h, X] = 1
                span_nts[i, k, h].add(X)
                chart.set(items[i, k, h, X, MID], edges,
                          labels=labels_)
                for r_unary, X_up in grammar.unary_rules_by_first[X]:
                    unary2[X_up, h].append((r_unary, X))

            # Unary rules.
            finish = set()
            finish.update(unary.keys())
            finish.update(unary2.keys())
            finish.update(to_add.keys())
            for X, h in finish:
                if (X, h) in unary2:
                    labels_, edges  = zip(*[
                        (labels[i, k, k, h, h, r],
                         [items[i, k, h, Y, MID]])
                        for r, Y in unary2[X, h]])
                else:
                    labels_, edges = (), ()
                if (X, h) in unary:
                    edges += ([items[i, k, h, X, MID]],)
                    labels_ += (-1,)

                if (X, h) in to_add:
                    edges += ([items[i, k, h, X, START]],)
                    labels_ += (-1,)
                has_item[i, k, h, X] = 1
                span_nts[i, k, h].add(X)
                chart.set(items[i, k, h, X, DONE], edges,
                          labels=labels_)



            # Unary rules.
            # TODO: add in rules here.
            # firsts = set()
            # seconds = set()

            # for X, h in to_add:
            #     firsts.add((X, h))
            #     for r_unary, X_up in grammar.unary_rules_by_first[X]:
            #         unary[X_up, h].append((r_unary, X))
            #         seconds.add((X_up, h))
            # firsts = firsts - seconds
            # l = list(seconds)
            # l.sort(key=lambda k: grammar.unary_order[k[0]])
            #l.reverse()


                #else:
                #    labels_, edges = (), ()
                # for r, Y in unary[X, h]:
                #     if (Y, h) not in seen: continue
                #     labels_ += (labels[i, k, k, h, h, r],)
                #     edges += ([items[i, k, h, Y]], )

                #if labels_:
                        # print grammar.root, n, has_item[0, n-1, 2, 3]
    # print [([0, n-1, h, grammar.root])
    #        for h in range(n)
    #        if has_item[0, n-1, h, grammar.root]]

    chart.set(items[n-1, 0, 0, 0, DONE],
              [[items[0, n-1, h, root, DONE]]
               for h in range(n)
               for root in grammar.roots
               if has_item[0, n-1, h, root]
           ])
    return chart.finish(True), encoder
