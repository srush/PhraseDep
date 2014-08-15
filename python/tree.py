from collections import defaultdict, namedtuple
from pydecode.encoder import StructuredEncoder
import nltk
from nltk import ImmutableTree as Tree
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
        return tuple([flat
                      for node in tree
                      for flat in un_z(node)])
    else:
        return (unbinarize(tree),)

def un_z_node(tree):
    if isinstance(tree, str):
        return tree
    else:
        return tuple([flat
                for node in tree
                for flat in un_z(node)])

def remove_head(tree):
    if isinstance(tree, str): return tree.split("^")[0]
    return tree.label().split("^")[0]
    
def label(tree):
    if isinstance(tree, str): return tree
    return tree.label()

def clean_label(tree):
    if isinstance(tree, str): return tree.split("^")[0]
    return tree.label().split("^")[0]

def unbinarize(tree):
    if isinstance(tree, str): return tree

    if label(tree[0])[:2] == "Z_":
        return Tree(label(tree), un_z_node(tree[0]) + (unbinarize(tree[1]),))


    if len(tree) > 1 and label(tree[1])[:2] == "Z_":
        return Tree(label(tree), (unbinarize(tree[0]),) + un_z_node(tree[1]))

    return Tree(tree.label(), tuple([unbinarize(node)
                                     for node in tree]))


def binarize(original_rules, tree, head=None):
   if isinstance(tree, str): return tree
   children = []
   binarized_children = tuple((binarize(original_rules, node)
                               for node in tree))
      

   if len(tree) <= 2:
       return Tree(tree.label(), binarized_children)

   if len(tree) >=3:
       rule_num, new_head = original_rules[tree.label().split("^")[0],
                                           tuple([label(node).split("^")[0]
                                                  for node in tree])]
       absolute_head = int(tree.label().split("^")[1])
       parent_nt = tree.label().split("^")[0]
       if head is None:
           head = new_head
       cur = binarized_children[head]
       for i, node in enumerate(tree[head+1:]):
           
          if head + i + 2 == len(tree):
             if not tree[:head]:
                node_label = tree.label()
             else:
                node_label = "Z_(%s_l_%s)^%d"%(parent_nt, clean_label(tree[head-1]), absolute_head)
          else:
             node_label = "Z_(%s_r_%s)^%d"%(parent_nt, clean_label(tree[i + head + 2]), absolute_head)
          cur = Tree(node_label, (cur, binarized_children[head + i + 1]))


       for i, node in enumerate(reversed(tree[:head])):
           #node_label = "Z_(%d,l,%d)^%d"%(rule_num, head - i - 2, absolute_head)
           node_label = "Z_(%s_l_%s)^%d"%(parent_nt, clean_label(tree[head - i - 2]), absolute_head)
           if head - i - 2 == -1:
               node_label = tree.label()
           cur = Tree(node_label, (binarized_children[head - i - 1], cur))

       return cur
