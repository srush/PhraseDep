import numpy as np
import pydecode
from pydecode.encoder import StructuredEncoder
import nltk
nonterminals = []
rules = []

class LexicalizedCFGEncoder(StructuredEncoder):
    def __init__(self, n, N, grammar):
        """

        Parameters
        ----------
        n : int
            The sentence length.
        N : int
            The number of non-terminals.
        grammar :
        """
        self.n = n
        self.N = N
        self.grammar = grammar
        G = len(grammar)
        shape = (n, n, n, n, n, G)

        super(LexicalizedCFGEncoder, self).__init__(shape)

    def transform_structure(self, parse):
        pass


    def from_parts(self, parts):
        parse = np.zeros((self.n, self.n, self.n))
        parse.fill(-1)
        for part in parts:
            rule = part[5]
            X, Y, Z, dir = self.grammar[rule]
            head = part[3 + dir]
            parse[part[0], part[2], head] = X
        return parse

def make_index_set(n, N):
    return np.arange(n*n*n*N).reshape((n, n, n, N))

def make_items(items, n, N):
    return np.array(np.unravel_index(items, (n, n, n, N))).T

def xbar_cky(n, N, grammar, dep_matrix):
    """
    """
    encoder = LexicalizedCFGEncoder(n, N, grammar)
    items = make_index_set(n, N)
    labels = encoder.encoder
    chart = pydecode.ChartBuilder(items, unstrict=True)
    has_item = np.zeros(n*n*n*N).reshape((n,n,n, N))

    for i in range(n):
        for nonterminal in range(N):
            chart.init(items[i, i, i, nonterminal])
            has_item[i, i, i, nonterminal] = 1

    for d in range(1, n):
        for i in range(n):
            k = i + d
            if k >= n:
                continue
            for X in range(N):
                rules = [(r,rule) for r, rule in enumerate(grammar) if rule[0] == X]
                for h in range(i, k+1):
                    edges = [[items[i, j, h, Y], items[j+1, k, h2, Z]]
                             for j in range(h, k)
                             for h2 in range(j+1, k+1)
                             if dep_matrix[h, h2]
                             for r, (_, Y, Z, dir) in rules
                             if dir == 0
                             if has_item[i, j, h, Y]
                             if has_item[j+1, k, h2, Z]]
                    label = [labels[i, j, k, h, h2, r]
                             for j in range(h, k)
                             for h2 in range(j+1, k+1)
                             if dep_matrix[h, h2]
                             for r, (_, Y, Z, dir) in rules
                             if dir == 0
                             if has_item[i, j, h, Y]
                             if has_item[j + 1, k, h2, Z]]

                    edges += [[items[i, j, h1, Y], items[j+1, k, h, Z]]
                              for j in range(i, h)
                              for h1 in range(i, j+1)
                              if dep_matrix[h, h1]
                              for r, (_, Y, Z, dir) in rules
                              if dir == 1
                              if has_item[i, j, h1, Y]
                              if has_item[j+1, k, h, Z]]
                    label += [labels[i, j, k, h, h1, r]
                              for j in range(i, h)
                              for h1 in range(i, j+1)
                              for r, (_, Y, Z, dir) in rules
                              if dir == 1
                              if dep_matrix[h, h1]
                              if has_item[i, j, h1, Y]
                              if has_item[j+1, k, h, Z]]

                    if len(edges) > 0:
                        chart.set(items[i, k, h, X], edges, labels=label)
                        has_item[i, k, h, X] = 1

    chart.set(items[n-1, 0, 0, 0],
              [[items[0, n-1, h, 0]]
               for h in range(n)
               if has_item[0, n-1, h, 0]],
              labels=[0] * n)
    return chart.finish(True), encoder



class ReconstructionModel(pydecode.model.HammingLossModel,
                          pydecode.model.DynamicProgrammingModel):

    def set_grammar(N, grammar):
        self.N = N
        self.grammar = grammar

    def initialize(self, X, Y):
        self.bins = [1, 2, 3, 5, 8, 20, 40]

        # Preprocessor
        self.preprocessor = pydecode.model.Preprocessor()
        for x in X:
            for word, tag in zip(x.words, x.tags):
                self.preprocessor.add(self._preprocess_word(word, tag))
            self.preprocessor.add(self._preprocess_word("*END*", "*END*"))
        super(ReconstructionModel, self).initialize(X, Y)

    def _preprocess_word(self, word, tag):
        return {"WORD": word, "TAG": tag}

    def _preprocess(self, x):
        if ("PP", x) in self.cache:
            return self.cache["PP", x]
        else:
            p = self.preprocessor.transform(
                [self._preprocess_word(word, tag)
                 for word, tag in zip(x.words, x.tags)] \
                    + [self._preprocess_word("*END*", "*END*")])
            self.cache["PP", x] = p
        return p

    def dynamic_program(self, x):
        n = len(x.words)
        return xbar_cky(n, self.N, self.grammar, x.dep_matrix)

    def parts_features(self, x, parts):
        p = self._preprocess(x)
        o = parts

        # Parts are of the following form.
        POS_i = 0
        POS_j = 1
        POS_k = 2
        HEAD = 3
        MOD = 4
        RULE = 5

        X = self.grammar[o[:, RULE]][0]
        Y = self.grammar[o[:, RULE]][1]
        Z = self.grammar[o[:, RULE]][2]


        dist = o[:, HEAD] - o[:, MOD]
        bdist = np.digitize(dist, self.bins)
        direction = dist >= 0
        base = [(p["WORD"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
                (p["WORD"][o[:, HEAD]], p["TAG"][ o[:, MOD]]),
                (p["TAG"][o[:, HEAD]],  p["WORD"][o[:, MOD]]),
                (p["TAG"][o[:, HEAD]],  p["TAG"][ o[:, MOD]])]
        t = base
        t += [(direction, bdist) + b for b in base]

        for d in [(-1, -1), (-1, 1), (1, -1), (1, 1)]:
            t += [(p["TAG"][o[:, HEAD] + d[0]],
                   p["TAG"][o[:, HEAD]],
                   p["TAG"][o[:, MOD] + d[1]],
                   p["TAG"][o[:, MOD]])]
        return t

    def templates(self):
        def s(t):
            return self.preprocessor.size(t)
        base = [(s("WORD"), s("WORD")),
                (s("WORD"), s("TAG")),
                (s("TAG"),  s("WORD")),
                (s("TAG"),  s("TAG"))]
        t = base
        t += [(2, len(self.bins) + 1) + b for b in base]
        t += [(s("TAG"), s("TAG"),
               s("TAG"), s("TAG"))] * 4
        return t


# In[18]:

# mat = np.zeros((4,4))
# mat[1, 0] = 1
# mat[3, 2] = 1
# mat[1, 3] = 1


# # In[19]:

# graph = xbar_cky(4, mat)


# # In[11]:

# pydecode.draw(graph, make_labels(graph.labeling, 4),
#               make_items(graph.node_labeling, 4), out="/tmp/graph.png")


# # In[22]:

# import pydecode.test
# all = pydecode.test.all_paths(graph)
# for path in all:
#     print make_items([edge.head.label for edge in path], 4)


# # Out[22]:

# #     [[0 1 1]
# #      [2 3 3]
# #      [0 3 1]
# #      [3 0 0]]
# #     [[2 3 3]
# #      [1 3 1]
# #      [0 3 1]
# #      [3 0 0]]
# #

# # In[7]:

# make_items([4], 4)


# # Out[7]:

# #     array([[0, 1, 0]])
