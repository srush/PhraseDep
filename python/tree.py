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
        # if (X, rhs) not in d or d[X, rhs][0] > rule_num:
        d[X, rhs, head] = (rule_num, head)
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
    return label(tree).split("^")[0]

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
                              head+1)

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
        return clean_label(tree)
    return Tree(clean_label(tree),
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


def binarize(original_rules, deps, tree, fixed_head=None, start=0):
    if terminal(tree):
        return tree, start, start + 1

    binarized_children = []
    head_children = []
    pstart = start


    for node in tree:
        c, head, pstart = binarize(original_rules, deps, node, start=pstart)
        binarized_children.append(c)
        head_children.append(head)
    absolute_head = deps[start, pstart]
    for i in range(len(head_children)):
        if head_children[i] == absolute_head:
            relative_head = i


    binarized_children = tuple(binarized_children)
    parent_nt, _ = annotated_label(tree)

    original_rule = (parent_nt, map_t(clean_label, tree), relative_head)

    if original_rule in original_rules:
        _, new_head = original_rules[original_rule]

        assert len(original_rule[1]) == len(head_children)
    else:
        # make head last word...
        new_head = relative_head #len(original_rule[1]) - 1

    if fixed_head is None:
        head = new_head
    else:
        head = fixed_head

    #absolute_head2 = head_children[head]
    cur = binarized_children[head]
    _, absolute_head2 = annotated_label(cur)
    # absolute_head = deps[start, pstart]
    # assert absolute_head == absolute_head2, "%s %s %s %s"%(absolute_head, absolute_head2, start, pstart)
    tree_label = annotate_label(parent_nt, absolute_head)
    if len(tree) <= 2:
        return Tree(tree_label, binarized_children), absolute_head, pstart

    if len(tree) >=3:
        cur = binarized_children[head]
        #_, absolute_head = annotated_label(cur)

        #assert absolute_head1 == absolute_head, "%s %s"%(absolute_head, absolute_head1)
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

        return cur, absolute_head, pstart

def make_spans(tree):
    all = set()
    def make_span(node, i):
        if terminal(node):
            return
        cur = i
        span = (label(node), i, i + len(node.leaves()))
        all.add(span)
        for n in node:
            make_span(n, cur)
            if terminal(n):
                cur += 1
            else:
                cur += len(n.leaves())
    make_span(tree, 0)
    return all
