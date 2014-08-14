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
