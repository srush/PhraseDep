"""
This file implements tree operations. Mainly binarizing
and unbinarizing.
"""

from nltk import ImmutableTree as Tree

def read_original_rules(handle):
    """
    Read in the original, unbinarized rules.
    """
    d = {}
    for l in handle:
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

def terminal(node):
    "Is node terminal?"
    return isinstance(node, str) 

def label(tree):
    "Get tree label"
    if terminal(tree): return tree
    return tree.label()

def annotated_label(tree):
    "Get tree label and absolute head position."
    l = label(tree)
    X, h = l.split("^")
    return X, int(h) - 1

def clean_label(tree):
    "Get tree label without head position."
    return annotated_label(tree)[0]

def annotate_label(symbol, head):
    "Construct a tree annotation"
    return "%s^%d"%(symbol, head + 1)

def is_bin_nonterm(tree):
    "Is the label a binarized nonterminal?"
    return label(tree)[:2] == "Z_"

def make_bin_nonterm(parent_nt, prev_label, dir, head):
    """
    Make a binarized nonterminal.
    
    Parameters
    ----------
    parent_nt : str
       Parent non-terminal label.

    prev_label : str
       Previous (horizontal markov) label.

    dir : int
       Direction (0 left, 1 right)

    head : int
       Absolute head.

    """
    return "Z_(%s_%s_%s)^%d"%(parent_nt, 
                              "l" if dir == 0 else "r",
                              clean_label(prev_label), 
                              head)

def _un_z(tree):
    """
    Remove binarization from tree. 
    """
    if terminal(tree): 
        return [tree]
    if is_bin_nonterm(tree):
        return tuple([flat
                      for node in tree
                      for flat in _un_z(node)])
    else:
        return (unbinarize(tree),)

def remove_head(tree):
    "Recursively remove head annotations from a tree"
    if terminal(tree): 
        return annotated_label(tree)[0]
    return Tree(annotated_label(tree)[0],
                map_t(remove_head, tree))

def map_t(f, ls):
    return tuple(map(f, ls))
    
def unbinarize(tree):
    if terminal(tree): 
        return tree
    tree_label = label(tree)

    if is_bin_nonterm(tree[0]):
        return Tree(tree_label, 
                    _un_z(tree[0]) + (unbinarize(tree[1]),))

    if len(tree) > 1 and is_bin_nonterm(tree[1]):
        return Tree(tree_label,
                    (unbinarize(tree[0]),) + _un_z(tree[1]))

    return Tree(tree_label, map_t(unbinarize, tree))


def binarize(original_rules, tree, head=None):
    if terminal(tree): 
        return tree
    children = []
    binarized_children = tuple([binarize(original_rules, node)
                                for node in tree])
    tree_label = label(tree)

    if len(tree) <= 2:
        return Tree(tree_label, binarized_children)

    if len(tree) >=3:
        parent_nt, absolute_head = annotated_label(tree)
        rule_num, new_head = original_rules[
            parent_nt,
            map_t(clean_label, tree)]

        if head is None:
            head = new_head
            
        cur = binarized_children[head]
        for pos, bin_node in enumerate(binarized_children[head+1:], head+1):
            if pos == len(tree) - 1:
                if head == 0:
                    node_label = tree_label
                else:
                    node_label = make_bin_nonterm(parent_nt, 
                                                  tree[head-1], 0, 
                                                  absolute_head)
            else:
                node_label = make_bin_nonterm(parent_nt, 
                                              tree[pos + 1], 
                                              1, absolute_head)
            cur = Tree(node_label, (cur, bin_node))

        for i, bin_node in enumerate(reversed(binarized_children[:head])):
            pos = head - 1 - i
            node_label = make_bin_nonterm(parent_nt, 
                                          tree[pos - 1], 
                                          0, absolute_head)
            if pos == 0:
                node_label = tree_label
            cur = Tree(node_label, 
                       (bin_node, cur))
            
        return cur
