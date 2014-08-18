from encoder import LexicalizedCFGEncoder

def pruning(n, dep_matrix):
    has_item = np.zeros(n*n*n).reshape((n,n,n))
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

def count_mods(dep_matrix):
    n = len(dep_matrix)
    counts = np.zeros((n,2))
    for h in range(n):
        for m in range(n):
            if dep_matrix[h][m]:
                dir = 0 if m < h else 1
                counts[h, dir] += 1
    return counts

def make_mod(sentence, dep_matrix):
    n = len(sentence)
    mod_of = defaultdict(list)
    for h in range(n):
        for m in range(n):
            if dep_matrix[h][m]:
                mod_of[h].append(m)
    return mod_of

def nt_pruning(sentence, grammar, span_pruner, dep_matrix):
    mods = count_mods(dep_matrix)

    N = len(grammar.nonterms)
    n = len(sentence)
    has_item = np.zeros((n, n, N), dtype=np.uint8)
    in_cell = defaultdict(list)
    cell_rules = defaultdict(list)

    # All the rules that could use this.
    cell_rule_set = defaultdict(set)

    for i in range(n):
        has_item[i,i, sentence[i]] = 1
        in_cell[i, i].append(sentence[i])
        for r, (X, Y) in enumerate(grammar.unary_table):
            if Y == sentence[i]:
                has_item[i, i, X] = 1
                in_cell[i, i].append(X)

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for h, m, j in span_pruner[i, k]:

                req_dir = 0 if h <= j else 1
                for Y in in_cell[i, j]:
                    for r, X, Z, dir in grammar.rules_by_first[Y]:
                        if dir != req_dir: continue

                        if not has_item[i, k, X]:
                            has_item[i, k, X] = 1
                            in_cell[i, k].append(X)
                        if dir == 0:
                            cell_rules[i, k, Y, 0].append((r, X, Z))
                        else:
                            cell_rules[i, k, Z, 1].append((r, X, Y))


    return has_item, in_cell, cell_rules


import time
def cky(sentence, rules, dep_matrix):
    """
    """
    # Preprocessing.
    n = len(sentence)

    start = time.time()
    pruning_chart, span_pruner = pruning(n, dep_matrix)
    #print "A", time.time() - start
    start = time.time()
    nt_pruning_chart, in_cell, cell_rules = nt_pruning(sentence, rules, span_pruner, dep_matrix)
    seen_item = defaultdict(set)

    encoder = LexicalizedCFGEncoder(sentence, rules)
    items = defaultdict(auto())

    G = len(rules.rules)
    N = len(rules.nonterms)

    labels = encoder.encoder
    temp = np.arange(1000000)
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


    #print "B", time.time() - start
    start = time.time()
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
                    label, edges  = zip(*[
                            (labels[i, j, k, h, m, r],
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
    #print "C", time.time() - start
    return chart.finish(True), encoder


# def pruning_graph(n, mod_of):
#     labels = defaultdict(auto())
#     items = np.arange(n * n * n).reshape((n, n, n))
#     chart = pydecode.ChartBuilder(items, unstrict=True)
#     has_item = defaultdict(lambda: 0)

#     for i in range(n):
#         chart.init(items[i, i, i])
#         has_item[i, i, i] = 1

#     for d in range(1, n):
#         for i in range(n):
#             k = i + d
#             if k >= n: continue
#             for h in range(i, k+1):
#                 edges = [[items[i, j, h], items[j+1, k, m]]
#                          for m in mod_of[h]
#                          for j in range(i, k)
#                          if has_item[i, j, h] and has_item[j+1, k, m]] + \
#                          [[items[i, j, m], items[j+1, k, h]]
#                           for m in mod_of[h]
#                           for j in range(i, k)
#                           if has_item[i, j, m] and has_item[j+1, k, h]]

#                 if len(edges) > 0:
#                     chart.set(items[i, k, h], edges)
#                     has_item[i, k, h] = 1
#     chart.set(items[n-1, 0, 0],
#               [[items[0, n-1, h]]
#                for h in range(n)
#                if has_item[0, n-1, h]])
#     return chart.finish(True)

# def nt_pruning_graph(sentence, grammar, span_pruner):
#     N = len(grammar.nonterms)
#     n = len(sentence)
#     labels = defaultdict(auto())
#     items = np.arange(n * n * N).reshape((n, n, N))
#     chart = pydecode.ChartBuilder(items, unstrict=True)

#     has_item = np.zeros((n, n, N), dtype=np.uint8)
#     in_cell = defaultdict(list)

#     for i in range(n):
#         has_item[i,i, sentence[i]] = 1
#         in_cell[i, i].append(sentence[i])
#         chart.init(items[i, i, sentence[i]])
#         for r, (X, Y) in enumerate(grammar.unary_table):
#             if Y == sentence[i]:
#                 has_item[i, i, X] = 1
#                 in_cell[i, i].append(X)
#                 chart.set(items[i, i, X],
#                           [[items[i, i, sentence[i]]]])

#     for d in range(1, n):
#         for i in range(n):
#             k = i + d
#             if k >= n:
#                 continue
#             if not span_pruner[i, k]: continue
#             stash = defaultdict(list)
#             for h, m, j in span_pruner[i, k]:
#                 req_dir = 0 if h <= j else 1
#                 if not span_pruner[i, j] or not span_pruner[j+1, k]: continue
#                 for Y in in_cell[i, j]:
#                     for r, X, Z, dir in grammar.rules_by_first[Y]:
#                         if dir != req_dir: continue
#                         if has_item[j+1, k, Z]:
#                             stash[X].append((Y, Z, j, r))
#             for X in stash:
#                 chart.set(items[i, k, X],
#                           [[items[i, j, Y], items[j+1, k, Z]]
#                            for Y, Z, j, r in stash[X]])

#                 has_item[i, k, X] = 1
#                 in_cell[i, k].append(X)

#     chart.set(items[n-1, 0, 0],
#               [[items[0, n-1, X]]
#                for X in range(N)
#                if has_item[0, n-1, X]])

#     return chart.finish(True)
