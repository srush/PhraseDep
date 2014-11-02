"""
This file implements an Encoder for lexicalized parsing.
Its job is to map between lexicalized trees and
the parts representation as annotated spans.
"""
from collections import defaultdict
import pydecode
from pydecode.encoder import StructuredEncoder
from nltk import ImmutableTree as Tree
import numpy as np
from tree import *
from itertools import izip

class auto:
    """
    A simple auto increment dictionary.
    """
    def __init__(self):
        self.id = 0

    def __call__(self):
        self.id += 1
        return self.id

class SparseEncoder(StructuredEncoder):
    """
    A sparse structured encoder. Gives a new id to each new key.
    """
    def __init__(self):
        self.encoder = defaultdict(auto())

    def transform_labels(self, labels):
        reverse = dict(zip(self.encoder.values(), self.encoder.keys()))
        return np.array([reverse[label] for label in labels])

class LexicalizedCFGEncoder(SparseEncoder):
    def save(self, file, graph):
        s = set(graph.labeling)
        out = [(k, v) for k, v in izip(self.encoder.keys(), self.encoder.values())
                if v in s]

        out = [(k, v) for k, v in izip(self.encoder.keys(), self.encoder.values())
                if v in s]

        a1 = np.array([a for (a, b) in out])
        b1 = np.array([b for (a, b) in out])

        with open(file, "wb") as o:
            np.save(file + ".keys", a1)
            np.save(file + ".vals", b1)

    def load(self, file, graph):
        with open(file, "rb") as i:
            keys = np.load(file + ".keys.npy")
            vals = np.load(file + ".vals.npy")
            self.encoder = dict([(tuple(keys[i]), vals[i]) for i in range(len(keys))])

    def __init__(self, sentence, tags, grammar):
        self.grammar = grammar
        self.sentence = sentence
        self.tags = tags
        super(LexicalizedCFGEncoder, self).__init__()

    def transform_structure(self, parse):
        r"""
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
            X, h = annotated_label(node)
            if len(node) == 2:

                Y, h_1 = annotated_label(node[0])
                Z, h_2 = annotated_label(node[1])
                other = h_1 if h == h_2 else h_2
                assert h == h_1 or h == h_2, "%s %s %s\n%s"%(h, h_1, h_2, parse)
                r = self.grammar.rule_index(X, Y, Z)
                if not terminal(node[0]):
                    j = i + len(node[0].leaves()) - 1
                else:
                    j = i + 1 - 1
                assert(i <= h <= k), "%s %s %s %s %s %s\n%s"%(h, h_1, h_2, i, j, k, parse)

                parts.append((i, j, k, h, other, r))

            if len(node) == 1 and not terminal(node[0]):
                Y, h_1 = annotated_label(node[0])
                r = self.grammar.rule_index(X, Y)
                parts.append((i, k, k, h, h, r))

            for n in node:
                if terminal(n):
                    cur += 1
                    continue
                stack.append((n, cur, cur + len(n.leaves())-1))
                cur += len(n.leaves())
        parts.reverse()
        return np.array(parts)

    def from_parts(self, parts):
        r"""
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
            X = self.sentence[i]
            parse[i, i] = Tree(annotate_label(self.tags[i], i),
                               (annotate_label(self.sentence[i], i),))

        for part in parts:
            i, j, k, h, _, r = part
            if j != k:
                X, _, __ = self.grammar.rule_nonterms(r)
                parse[i, k] = Tree(annotate_label(X, h),
                                   (parse[i,j], parse[j+1, k]))

            else:
                X, _ = self.grammar.rule_nonterms(r)
                parse[i, k] = Tree(annotate_label(X, h),
                                   (parse[i, k],))

        parse =  parse[0, len(self.sentence)-1]
        return parse

    def structure_path(self, graph, parse):
        """
        Helper method for debugging. Checks that a graph contains parse.

        Parameters
        -----------
        graph : hypergraph

        parse : nltk.Tree

        """
        parts = self.transform_structure(parse)
        #labels = [self.encoder[part]
        #print parts
        label_weights = np.zeros(len(self.encoder)+20, dtype=np.int8)

        for part in parts:
            #assert
            if not (tuple(part) in self.encoder):  print part
            # print part[-1], [self.grammar.nonterms[nt] for nt in self.grammar.rule_nonterms(part[-1])]
            label_weights[self.encoder[tuple(part)]] = 1


        weights = pydecode.transform(graph, label_weights)

        part_set = set([self.encoder[tuple(part)] for part in parts])
        for edge in graph.edges:
            if edge.label == -1:
                weights[edge.id] = 1
            else:
                if edge.label in part_set:
                    part_set.remove(edge.label)
        bad_parts = self.transform_labels([part for part in part_set])
        # print part_set, bad_parts, [self.grammar.rule_nonterms(p[-1]) for p in bad_parts]
        #assert not part_set, [self.transform_labels([part for part in part_set])]

        chart = pydecode.inside(graph, weights, weight_type=pydecode.Boolean)
        for edge in graph.edges:
            if edge.label != -1 and weights[edge.id] == 1 or edge.head.id == graph.root.id:
                # print len(edge.tail), edge.label
                for node in edge.tail:
                    # print node.id
                    if chart[node.id] != 1:
                        pass
                        # print self.transform_labels([edge.label])
                #         assert(False)
                # assert chart[edge.head.id] == 1

        # print chart
        if not chart[graph.root.id]:
            print "fail"
        else:
            print "good"
        return
