"""
This file implements training.
"""

from collections import defaultdict, namedtuple
import pydecode.model
import pydecode.nlp
import nltk
import itertools
import numpy as np
import dp
import tree 
from encoder import LexicalizedCFGEncoder

ParseInput = namedtuple("ParseInput", ["words", "tags", "deps", "index"])

def fscore(a, b):
    hit = len(a & b)
    recall =  hit / float(len(a))
    precision = hit / float(len(b))
    return 2 * ((precision * recall) / float(precision + recall))


class ReconstructionModel(pydecode.model.DynamicProgrammingModel):

    def loss(self, y, yhat):
        a = tree.make_spans(y)
        b = tree.make_spans(yhat)             
        return 1.0 - fscore(a, b)

    def max_loss(self, y):
        return 1.0

    def set_grammar(self, grammar):
        self.grammar = grammar

    def set_from_disk(self, path):
        self.path = path

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
        if x.index is not None and ("DP", x.index) not in self.cache:
            if self.path and x.index:
                graph = pydecode.load("%s/graphs%s.graph"%(self.path, 
                                                           x.index))
                encoder = LexicalizedCFGEncoder(x.words, x.tags, self.grammar)
                encoder.load("%s/encoder%s.pickle"%(self.path, x.index))
                self.cache["DP", x.index] = graph, encoder
                return graph, encoder
            else:
                graph, enc = dp.cky(x.words, x.tags, self.grammar, x.deps)
                #self.cache["DP", x.index] = graph, enc
                return graph, enc
        return self.cache["DP", x.index]

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

        X = self.grammar.rule_table[o[:, RULE], 0]
        Y = self.grammar.rule_table[o[:, RULE], 1]
        Z = self.grammar.rule_table[o[:, RULE], 2]
        base = [(X, p["TAG"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
                (X, p["TAG"][o[:, HEAD]]),
                (X, Y, p["TAG"][o[:, HEAD]]),
                (X, Z, p["TAG"][o[:, HEAD]]),
                (X, p["WORD"][o[:, HEAD]]),
                (o[:, RULE], p["TAG"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
                (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
                (np.abs(o[:, POS_i] - o[:, POS_j]), 
                 np.abs(o[:, POS_j] - o[:, POS_k]),),
                (o[:, RULE],)]

        # dist = o[:, HEAD] - o[:, MOD]
        # bdist = np.digitize(dist, self.bins)
        # direction = dist >= 0
        # base = [(p["WORD"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
        #         (p["WORD"][o[:, HEAD]], p["TAG"][ o[:, MOD]]),
        #         (p["TAG"][o[:, HEAD]],  p["WORD"][o[:, MOD]]),
        #         (p["TAG"][o[:, HEAD]],  p["TAG"][ o[:, MOD]])]
        # t = base
        # t += [(direction, bdist) + b for b in base]

        # for d in [(-1, -1), (-1, 1), (1, -1), (1, 1)]:
        #     t += [(p["TAG"][o[:, HEAD] + d[0]],
        #            p["TAG"][o[:, HEAD]],
        #            p["TAG"][o[:, MOD] + d[1]],
        #            p["TAG"][o[:, MOD]])]
        # t = [(o[:, RULE],)]
        return base

    def templates(self):
        def s(t):
            return self.preprocessor.size(t)
        nonterms = len(self.grammar.nonterms)
        rules = len(self.grammar.rule_table)
        base = [(nonterms, s("TAG"), s("TAG")),
                (nonterms, s("TAG")),
                (nonterms, nonterms, s("TAG")),
                (nonterms, nonterms, s("TAG")),
                (nonterms, s("WORD")),
                (rules, s("TAG"), s("WORD")),
                (rules, s("WORD"), s("TAG")),
                (200, 200),
                (rules,)]
        # def s(t):
        #     return self.preprocessor.size(t)
        # base = [(s("WORD"), s("WORD")),
        #         (s("WORD"), s("TAG")),
        #         (s("TAG"),  s("WORD")),
        #         (s("TAG"),  s("TAG"))]
        # t = base
        # t += [(2, len(self.bins) + 1) + b for b in base]
        # t += [(s("TAG"), s("TAG"),
        #        s("TAG"), s("TAG"))] * 4
        return base


def read_data_set(dep_file, ps_file, limit):
    """
    Read in the X, Y data from files.

    Parameters
    -----------

    dep_file : string
       Dependencies in CoNLL format.

    ps_file : string
       Binrarized phrase structures in bracket format with head annotations.
    """
    deps = pydecode.nlp.read_csv_records(dep_file, limit=limit)
    pss = []
    for i, l in enumerate(open(ps_file)):
        pss.append(nltk.ImmutableTree.fromstring(l))
        if i > limit: 
            break


    X = []
    Y = []
    f = pydecode.nlp.CONLL
    for i, (record, ps) in enumerate(itertools.izip(deps, pss)):
        heads = record[:, f["HEAD"]]
        n = len(heads)
        dep_matrix = np.zeros((n, n))
        for m, h in enumerate(heads, 1):
            # Drop the root.
            if int(h) == 0: 
                continue
            dep_matrix[int(h)-1, int(m)-1] = 1

        X.append(ParseInput(tuple(record[:, f["WORD"]]),
                            tuple(record[:, f["TAG"]]),
                            tuple(map(tuple,dep_matrix)),
                            index=i))
        Y.append(ps)
        assert len(ps.leaves()) == n, "%d %d"%(n, len(ps.leaves()))

    return X, Y
