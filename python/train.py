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
        encoder = LexicalizedCFGEncoder(self.temp_x.words, self.temp_x.tags, self.grammar)
        parts = map(tuple, list(encoder.transform_structure(y)))
        partsb = map(tuple, list(encoder.transform_structure(yhat)))

        # print len([1 for p in parts if p in partsb]), float(len(parts))
        return len([1 for p in parts if p in partsb]) / float(len(parts))
        # a = tree.make_spans(y)
        # b = tree.make_spans(yhat)
        # return 1.0 - fscore(a, b)

    def max_loss(self, y):
        return 1.0

    def set_grammar(self, grammar):
        self.grammar = grammar

    def set_from_disk(self, path):
        self.path = path


    def initialize(self, X, Y):
        self.bins = [1, 2, 3, 5, 8, 10, 20, 40]
        self.last = None
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
            children = defaultdict(list)
            p = self.preprocessor.transform(
                [self._preprocess_word(word, tag)
                 for word, tag in zip(x.words, x.tags)] \
                    + [self._preprocess_word("*END*", "*END*")])

            self.cache["PP", x] = p
        return self.cache["PP", x]

    def dynamic_program(self, x):
        self.temp_x = x
        if x.index is not None:
            if self.last is not None and x.index == self.last[0]:
                return self.last[1], self.last[2]
            # print x.index
            if self.path and x.index is not None:
                if  ("DP", x.index) not in self.cache:
                    graph = pydecode.load("%s/graphs%s.graph"%(self.path,
                                                               x.index))
                    encoder = LexicalizedCFGEncoder(x.words, x.tags, self.grammar)
                    encoder.load("%s/encoder%s.pickle"%(self.path, x.index), graph)
                        # self.cache["DP", x.index] = graph
                else:
                    graph, encoder = self.cache["DP", x.index]
                    #if False and len(x.words) < 10:
                # self.cache["DP", x.index] = graph, encoder
                if x.index% 100 == 0: print x.index
                self.last = (x.index, graph, encoder)
                return graph, encoder
            else:
                # print "running cky", x.index, self.path
                graph, enc = dp.cky(x.words, x.tags, self.grammar, x.deps)
                #self.cache["DP", x.index] = graph, enc
                return graph, enc
        return self.cache["DP", x.index]

    def parts_features(self, x, parts):
        p = self._preprocess(x)
        o = parts
        mods = np.zeros((len(x.words) + 1, len(x.words) + 1), dtype=np.int8)
        mod_count = np.zeros((len(x.words) + 1, 2), dtype=np.int8)
        deps = x.deps
        for h in range(len(x.words)):
            for m in range(h, len(x.words)):
                if deps[h][m] == 1.0:
                    mod_count[h,1] += 1
                    mods[h, m] = mod_count[h,1]

            for m in range(h, 0, -1):
                if deps[h][m] == 1.0:
                    mod_count[h, 0] += 1
                    mods[h, m] = mod_count[h, 0]

        # Parts are of the following form.
        POS_i = 0
        POS_j = 1
        POS_k = 2
        HEAD = 3
        MOD = 4
        RULE = 5
        dist1 = np.abs(o[:, POS_i] - o[:, POS_j])
        dist2 = np.abs(o[:, POS_j] - o[:, POS_k])
        dist = o[:, HEAD] - o[:, MOD]
        adist = np.abs(dist)
        direction = (dist >= 0).astype(int)
        bdist = np.digitize(adist, self.bins)
        bdist1 = np.digitize(dist1, self.bins)
        bdist2 = np.digitize(dist2, self.bins)
        first_child = (mods[o[:, HEAD], o[:,MOD]] == 1).astype(int)
        last_child = (mods[o[:, HEAD], o[:, MOD]] == mod_count[o[:, HEAD], direction]).astype(int)
        X = self.grammar.rule_table[o[:, RULE], 0]
        Y = self.grammar.rule_table[o[:, RULE], 1]
        Z = self.grammar.rule_table[o[:, RULE], 2]
        G = len(self.grammar.rules)
        size = np.abs(o[:, POS_k] - o[:, POS_i])
        length = len(x.words)
        top = size == length
        is_unary = o[:, RULE] > G


        base = [(X, p["TAG"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
                (X, Y, p["TAG"][o[:, HEAD]]),
                (X, Z, p["TAG"][o[:, HEAD]]),
                (o[:, RULE], p["TAG"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
                (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),

                (X, p["TAG"][o[:, HEAD]]),
                (X, p["WORD"][o[:, HEAD]]),
                (o[:, RULE], p["WORD"][o[:, HEAD]]),
                (X, Y),
                (X, Z),
                (is_unary, p["TAG"][o[:, HEAD]]),
                (is_unary, X),
                (is_unary, o[:, RULE]),
                (o[:, RULE], p["TAG"][o[:, HEAD]]),
                (o[:, RULE], p["TAG"][o[:, MOD]]),
                (is_unary, size == 0),
                (X, top)]
        #EXTRA
                # (mod_count[o[:, HEAD], 0], o[:, RULE]),
                # (mod_count[o[:, HEAD], 1], o[:, RULE]),
                # (first_child, o[:, RULE]),
                # (last_child, o[:, RULE]),
                # (is_unary,),

                # (is_unary, X),

                # (X, Y, p["TAG"][o[:, HEAD]]),
                # (X, Z, p["TAG"][o[:, HEAD]]),
                # (X, p["WORD"][o[:, HEAD]]),
                # (o[:, RULE], p["TAG"][o[:, MOD]]),
                # (o[:, RULE], p["TAG"][o[:, HEAD]]),
                # (o[:, RULE], p["TAG"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
                # (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
                # (bdist, o[:, RULE]),
                # (direction, bdist1, bdist2),
                # (o[:, RULE],)]


        # base = [(X, p["TAG"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
        #         (X, p["TAG"][o[:, HEAD]]),
        #         (X, top),
        #         (X, Y),
        #         (X, Z),
        #         (mod_count[o[:, HEAD], 0], o[:, RULE]),
        #         (mod_count[o[:, HEAD], 1], o[:, RULE]),
        #         (first_child, o[:, RULE]),
        #         (last_child, o[:, RULE]),
        #         (is_unary,),
        #         (is_unary, p["TAG"][o[:, HEAD]]),
        #         (is_unary, X),
        #         (is_unary, size == 0),
        #         (X, Y, p["TAG"][o[:, HEAD]]),
        #         (X, Z, p["TAG"][o[:, HEAD]]),
        #         (X, p["WORD"][o[:, HEAD]]),
        #         (o[:, RULE], p["TAG"][o[:, MOD]]),
        #         (o[:, RULE], p["TAG"][o[:, HEAD]]),
        #         (o[:, RULE], p["TAG"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
        #         (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
        #         (bdist, o[:, RULE]),
        #         (direction, bdist1, bdist2),
        #         (o[:, RULE],)]

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
                (nonterms, nonterms, s("TAG")),
                (nonterms, nonterms, s("TAG")),
                (rules, s("TAG"), s("TAG")),
                (rules, s("WORD"), s("TAG")),


                (nonterms, s("TAG")),
                (nonterms, s("WORD")),
                (rules, s("WORD")),
                (nonterms, nonterms),
                (nonterms, nonterms),
                (2, s("TAG")),
                (2, nonterms),
                (2, rules),
                (rules, s("TAG")),
                (rules, s("TAG")),
                (2, 2),
                (nonterms, 2)]


        # base = [(nonterms, s("TAG"), s("TAG")),
        #         (nonterms, s("TAG")),
        #         (nonterms, 2),
        #         (nonterms, nonterms),
        #         (nonterms, nonterms),
        #         (200, rules),
        #         (200, rules),
        #         (2, rules),
        #         (2, rules),
        #         (2,),
        #         (2, s("TAG")),
        #         (2, nonterms),
        #         (2, 2,),
        #         (nonterms, nonterms, s("TAG")),
        #         (nonterms, nonterms, s("TAG")),
        #         (nonterms, s("WORD")),
        #         (rules, s("TAG")),
        #         (rules, s("TAG")),
        #         (rules, s("TAG"), s("WORD")),
        #         (rules, s("WORD"), s("TAG")),
        #         (200, rules),
        #         (2, 200, 200),
        #         (rules,)]
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
    print limit
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
