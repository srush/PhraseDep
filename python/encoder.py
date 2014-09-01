"""
This file implements an Encoder for lexicalized parsing. 
Basically its job is to map between lexicalized trees and 
the parts representation as annotated spans.
"""

from collections import defaultdict
import pydecode
from pydecode.encoder import StructuredEncoder
from nltk import ImmutableTree as Tree
import numpy as np
import cPickle as pickle
from tree import *

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

    def __init__(self, sentence, tags, grammar):
        self.grammar = grammar
        self.sentence = sentence
        self.tags = tags
        super(LexicalizedCFGEncoder, self).__init__()


    def structure_path(self, graph, parse):
        parts = self.transform_structure(parse)
        #labels = [self.encoder[part] 
        # print parts
        label_weights = np.zeros(len(self.encoder)+20, dtype=np.int8)

        for part in parts:
            # assert (tuple(part) in self.encoder), part
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
        # assert not part_set, [self.transform_labels([part for part in part_set])]

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

                r = self.grammar.rule_index(X, Y, Z)
                if not terminal(node[0]):
                    j = i + len(node[0].leaves()) - 1
                else:
                    j = i + 1 - 1

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
            if i != k:
                X, _, __ = self.grammar.rule_nonterms(r)
                parse[i, k] = Tree(annotate_label(X, h),
                                   (parse[i,j], parse[j+1, k]))

            else:
                X, _ = self.grammar.rule_nonterms(r)
                parse[i, i] = Tree(annotate_label(X, h),
                                   (parse[i, i],))

        parse =  parse[0, len(self.sentence)-1]
        return parse

