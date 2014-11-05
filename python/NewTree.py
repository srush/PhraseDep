from nltk import Tree
from copy import deepcopy
import sys
# http://www.nltk.org/_modules/nltk/treetransforms.html#chomsky_normal_form

HORZMARKOV = 1
VERTMARKOV = 1

def read_from_ptb(filename):
    ptb_file = open(filename, 'r')
    for l in ptb_file:
        tree = Tree.fromstring(l)
        # for st in tree.subtrees():
        #     print "*" + st.pprint()
        original = deepcopy(tree)
        #print tree.pprint()
        chomsky_normal_form(tree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV, childChar='|', parentChar='#')
        #un_chomsky_normal_form(tree)
        #print tree.pprint()
        #print original.pprint() == tree.pprint()
    ptb_file.close()

# Check the head rules to match in each case
# (Now a re-implementation of (Collins, 1999) thesis)
# Lingpeng Kong, lingpenk@cs.cmu.edu

PUNCTSET = set([".", ",", ":", "``", "''"])

def remove_labelChar(n, labelChar = '^'):
    # print n
    # print type(n).__name__
    if isinstance(n, str):
        return n[:(n.rfind(labelChar) if n.rfind(labelChar) > 0 else len(n))]
    else:
        return [remove_labelChar(x) for x in n]

# Parent is a string, child_list is a list of string
def findHead(parent,child_list, labelChar='^'):
    r_parent = remove_labelChar(parent, labelChar)
    r_child_list = remove_labelChar(child_list, labelChar)
    #print r_child_list
    if len(r_child_list) == 1:
        #Unary Rule -> the head must be the only one
        return 0
    normalHead = findHeaderNormal(r_parent, r_child_list)
    return findHeaderCoord(r_parent, r_child_list, normalHead);

def firstNoPunctChild(child_list):
    for i in xrange(len(child_list)):
        if child_list[i] not in PUNCTSET:
            return i
    return 0

def lastNoPunctChild(child_list):
    for i in reversed(xrange(len(child_list))):
        if child_list[i] not in PUNCTSET:
            return i
    return -1

def test():
    print findHead("NP",["DT", "NNP", "CC", "NNP"])
    print findHead("VP",["ADVP", "VBN", "PP", "NP"])
    print findHead("NP",["NP", "NP",",","NP",",","NP","CC","NP"])

def findHeaderNormal(parent, child_list):
        # Rules for NPs
        if parent == "NP":
            # If the last word is tagged POS return (last word)
            if child_list[lastNoPunctChild(child_list)] == "POS":
                return lastNoPunctChild(child_list)

            # Else search from right to left for the first child which is an
            # NN, NNP, NNPS, NNS, NX, POS or JJR
            s = set(["NN", "NNP", "NNPS", "NNS", "NX", "POS", "JJR"])

            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else search from left to right for the first child which is an NP
            for i in xrange(len(child_list)):
                if child_list[i] == "NP":
                    return i

            # Else search from right to left for the first child which is a $,
            # ADJP or PRN.
            s = set(["$", "ADJP", "PRN"])
            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else search from right to left for the first child which is a CD.
            for i in reversed(xrange(len(child_list))):
                if child_list[i] == ("CD"):
                    return i

            # Else search from right to left for the first child which is a JJ,
            # JJS, RB or QP.
            s = set(["JJ", "JJS", "RB", "QP"])
            for i in reversed(xrange(len(child_list))):
                if child_list[i] in s:
                    return i

            # Else return the last word.
            return lastNoPunctChild(child_list)


        # ADJP -- Left -- NNS QP NN $ ADVP JJ VBN VBG ADJP JJR NP JJS DT FW RBR
        # RBS SBAR RB
        if parent == "ADJP":
            plist = ["NNS", "QP", "NN", "$", "ADVP", "JJ", "VBN", "VBG", "ADJP", "JJR", "NP", "JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # ADVP -- Right -- RB RBR RBS FW ADVP TO CD JJR JJ IN NP JJS NN
        if parent == "ADVP":
            plist = ["RB", "RBR", "RBS", "FW", "ADVP", "TO", "CD", "JJR", "JJ", "IN", "NP", "JJS", "NN"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # CONJP -- Right -- CC RB IN
        if parent == "CONJP":
            plist = ["CC", "RB", "IN"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # FRAG -- Right
        if parent == "FRAG":
            return lastNoPunctChild(child_list)

        # INTJ -- Left
        if parent == "INTJ":
            return firstNoPunctChild(child_list)

        # LST -- Right -- LS :
        if parent == "LST":
            plist = ["LS", ":"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # NAC -- Left -- NN NNS NNP NNPS NP NAC EX $ CD QP PRP VBG JJ JJS JJR
        # ADJP FW
        if parent == "NAC":
            plist = ["NN", "NNS", "NNP", "NNPS", "NP", "NAC", "EX", "$", "CD", "QP", "PRP", "VBG", "JJ", "JJS", "JJR", "ADJP", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # PP -- Right -- IN TO VBG VBN RP FW
        if parent == "PP":
            plist = ["IN", "TO", "VBG", "VBN", "RP", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # PRN -- Left
        if parent == "PRN":
            return firstNoPunctChild(child_list)

        # PRT -- Right -- RP
        if parent == "PRT":
            plist = ["RP"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # QP -- Left -- $ IN NNS NN JJ RB DT CD NCD QP JJR JJS
        if parent == "QP":
            plist = ["$", "IN", "NNS", "NN", "JJ", "RB", "DT", "CD", "NCD", "QP", "JJR", "JJS"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # RRC -- Right -- VP NP ADVP ADJP PP
        if parent == "RRC":
            plist = ["VP","NP","ADVP","ADJP","PP"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i

            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # S -- Left -- TO IN VP S SBAR ADJP UCP NP
        if parent == "S":
            plist = ["TO", "IN", "VP", "S", "SBAR", "ADJP", "UCP", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SBAR -- Left -- WHNP WHPP WHADVP WHADJP IN DT S SQ SINV SBAR FRAG
        if parent == "SBAR":
            plist = ["WHNP", "WHPP", "WHADVP", "WHADJP", "IN", "DT", "S", "SQ", "SINV", "SBAR", "FRAG"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SBARQ -- Left -- SQ S SINV SBARQ FRAG
        if parent == "SBARQ":
            plist = ["SQ", "S", "SINV", "SBARQ", "FRAG"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SINV -- Left -- VBZ VBD VBP VB MD VP S SINV ADJP NP
        if parent == "SINV":
            plist = ["VBZ", "VBD", "VBP", "VB", "MD", "VP", "S", "SINV", "ADJP", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # SQ -- Left -- VBZ VBD VBP VB MD VP SQ
        if parent == "SQ":
            plist = ["VBZ", "VBD", "VBP", "VB", "MD", "VP", "SQ"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # UCP -- Right
        if parent == "UCP":
            return lastNoPunctChild(child_list)

        # VP -- Left -- TO VBD VBN MD VBZ VB VBG VBP VP ADJP NN NNS NP
        if parent == "VP":
            plist = ["TO", "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "ADJP", "NN", "NNS", "NP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHADJP -- Left -- CC WRB JJ ADJP
        if parent == "WHADJP":
            plist = ["CC", "WRB", "JJ", "ADJP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHADVP -- Right -- CC WRB
        if parent == "WHADVP":
            plist = ["CC", "WRB"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list);

        # WHNP -- Left -- WDT WP WP$ WHADJP WHPP WHNP
        if parent == "WHNP":
            plist = ["WDT", "WP", "WP$", "WHADJP", "WHPP", "WHNP"]
            for pe in plist:
                for i in xrange(len(child_list)):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return leftmost
            return firstNoPunctChild(child_list)

        # WHPP -- Right -- IN TO FW
        if parent == "WHPP":
            plist = ["IN", "TO", "FW"]
            for pe in plist:
                for i in reversed(xrange(len(child_list))):
                    if child_list[i] == pe:
                        return i
            # Nothing found, return rightmost
            return lastNoPunctChild(child_list)

        # No rule found, return leftmost
        return firstNoPunctChild(child_list)

def findHeaderCoord(parent, child_list, normalHead):
        # Rules for Coordinated Phrases
        h = normalHead
        if h < len(child_list) - 2:
            if child_list[h+1] == "CC":
                # Y_h Y_h+1 Y_h+2 forms a triple of non-terminals in a coordinating relationship,
                # But since Y_h already the head, nothing happened in unlabeled parsing.
                return normalHead

        if h > 1:
            if child_list[h-1] == "CC":
                # Y_h-2 Y_h-1 Y_h forms a triple of non-terminals in a coordinating relationship,
                # the head is modified to be Y_h-2 in this case

                # Make sure punctuation not selected as the head
                if child_list[h-2] not in PUNCTSET:
                    return h-2

        return normalHead

def lexLabel(tree, labelChar="^"):
    # a little hacky way to label the leaves using the index
    preterminals = [t for t in tree.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        preterminals[i][0] = preterminals[i][0] + labelChar + str(i+1)
    #     leaves[i] = leaves[i] + "-" + str(i+1)
    # print leaves

    for pos in tree.treepositions(order='postorder'):
        st = tree[pos]
        # print st
        # print type(st).__name__
        if isinstance(st, str):
            continue
        else:
            # print "*" + str(st)
            if len(st) == 1:
                # print st.label() + labelChar + findIndex(st[0])
                st.set_label(st.label() + labelChar + findIndex(st[0]))
            else:
                child_list_str = [n.label() if isinstance(n, Tree) else n for n in st]
                child_list = [n for n in st]
                index = findIndex(child_list[findHead(st.label(), child_list_str)])
                st.set_label(st.label() + labelChar + findIndex(index))

    # for t in tree.subtrees():
    #     if isinstance(t, str):
    #         continue
    #     else:
    #         t.set_label(t.label() + labelChar + getLexLabelNT(t))
    #     #print "*" + str(tree)

# return a string representing a int as index
# def getLexLabelNT(node, labelChar='^'):
#     # print "we are at " + str(node)
#     if isinstance(node, str):
#         return findIndex(node, labelChar)
#     if isinstance(node,Tree):
#         if node.height() == 2:
#             return findIndex(node[0])
#         else:
#             child_list_str = [n.label() if isinstance(n, Tree) else n for n in node]
#             child_list = [n for n in node]
#             return getLexLabelNT(child_list[findHead(node.label(), child_list_str)], labelChar)

# e.g. for "word^3" return '3'
def findIndex(s, labelChar='^'):
    if isinstance(s, str):
        return s[s.rfind(labelChar)+1:]
    else:
        m = s.label()
        return m[m.rfind(labelChar)+1:]

def getParentDic(lexTree, labelChar='^'):
    # build a parent set
    dep_set = {}
    # add root to the tree
    dep_set[findIndex(lexTree.label())] = '0'
    for nt in lexTree.subtrees():
        if isinstance(nt, Tree):
            ind_p = findIndex(nt.label())
            for c in nt:
                ind_c = findIndex(c.label() if isinstance(c, Tree) else c)
                if not ind_c == ind_p:
                    dep_set[ind_c] = ind_p
    return dep_set

def generateDep(ot, labelChar='^'):
    lt = deepcopy(ot)
    lexLabel(lt, labelChar)
    dep_set = getParentDic(lt, labelChar)

    conll_lines = []
    preterminals = [t for t in lt.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        word = remove_labelChar(preterminals[i][0], labelChar)
        pos = remove_labelChar(preterminals[i].label(), labelChar)
        index = findIndex(preterminals[i][0], labelChar)
        parent = dep_set[index]
        conll_lines.append([index, word, pos, parent])
    return conll_lines

def print_conll_lines(clines, wt):
    # 1 Influential _   JJ  JJ  _   2   _   _
    for l in clines:
        wt.write(l[0] + "\t" + l[1] + "\t_\t" + l[2] + "\t" + l[2] + "\t_\t" + l[3] + "\t_\t_\n")

def chomsky_normal_form(tree, horzMarkov=None, vertMarkov=0, childChar="|", parentChar="#"):
    # assume all subtrees have homogeneous children
    # assume all terminals have no siblings

    # A semi-hack to have elegant looking code below.  As a result,
    # any subtree with a branching factor greater than 999 will be incorrectly truncated.
    if horzMarkov is None: horzMarkov = 999

    # Traverse the tree depth-first keeping a list of ancestor nodes to the root.
    # I chose not to use the tree.treepositions() method since it requires
    # two traversals of the tree (one to get the positions, one to iterate
    # over them) and node access time is proportional to the height of the node.
    # This method is 7x faster which helps when parsing 40,000 sentences.

    nodeList = [(tree, [tree.label()])]
    # print nodeList

    while nodeList != []:
        node, parent = nodeList.pop()

        if isinstance(node,Tree):
            # for the node and the parent, we want to know which is the index of the head in Collins head rule
            # which basically is the break point of the binarization
            child_list = [n.label() for n in node if isinstance(n, Tree)]
            if len(child_list) == 0:
                continue

            # print parent
            # print str(node.label()) + "\t-->\t" + "\t".join(child_list)

            # The head postion is determined by the collins rule
            head_postion = findHead(node.label(),child_list)

            #print child_list[head_postion]

            # parent annotation
            parentString = ""
            originalNode = node.label()
            if vertMarkov != 0 and node != tree and isinstance(node[0],Tree):
                parentString = "%s<%s>" % (parentChar, "-".join(parent))
                node.set_label(node.label() + parentString)
                # Attach the higher level parent to the parent list and then pass the into the agenda later
                # The originalNode here is the direct parent of the child node
                parent = [originalNode] + parent[:vertMarkov - 1]

            # add children to the agenda before we mess with them
            for child in node:
                nodeList.append((child, parent))

            # chomsky normal form factorization
            if len(node) > 2:
                childNodes = [child.label() for child in node]
                nodeCopy = node.copy()
                node[0:] = [] # delete the children

                curNode = node
                numChildren = len(nodeCopy)

                for i in range(1,numChildren - 1):
                    #if factor == "right":
                    next_step_right = ((i+1) < head_postion)
                    if i < head_postion:
                        #every time we going to the right, the left context in are consumed by one
                        del childNodes[0]
                        #newHead = "%s%s<%s>%s" % (originalNode, childChar, "-".join(childNodes[i:min([i+horzMarkov,numChildren])]),parentString) # create new head
                        if next_step_right:
                            newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "l-") + "-".join(childNodes[:min(len(childNodes),horzMarkov)]),parentString) # create new head
                        else:
                            newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "r-") + "-".join(childNodes[max(0,len(childNodes)-horzMarkov):]),parentString) # create new head
                        newNode = Tree(newHead, [])
                        curNode[0:] = [nodeCopy.pop(0), newNode]

                    else:
                        del childNodes[-1]
                        #newHead = "%s%s<%s>%s" % (originalNode, childChar, "-".join(childNodes[max([numChildren-i-horzMarkov,0]):-i]),parentString)
                        if next_step_right:
                            newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "l-") + "-".join(childNodes[:min(len(childNodes),horzMarkov)]),parentString) # create new head
                        else:
                            newHead = "%s%s<%s>%s" % (originalNode, childChar, ("" if len(childNodes)<=2 else "r-") + "-".join(childNodes[max(0,len(childNodes)-horzMarkov):]),parentString) # create new head
                        newNode = Tree(newHead, [])
                        curNode[0:] = [newNode, nodeCopy.pop()]


                    curNode = newNode

                curNode[0:] = [child for child in nodeCopy]


def un_chomsky_normal_form(tree, expandUnary = True, childChar = "|", parentChar = "#", unaryChar = "+"):
    # Traverse the tree-depth first keeping a pointer to the parent for modification purposes.
    nodeList = [(tree,[])]
    while nodeList != []:
        node,parent = nodeList.pop()
        if isinstance(node,Tree):
            # if the node contains the 'childChar' character it means that
            # it is an artificial node and can be removed, although we still need
            # to move its children to its parent
            childIndex = node.label().find(childChar)
            if childIndex != -1:
                nodeIndex = parent.index(node)
                parent.remove(parent[nodeIndex])
                # Generated node was on the left if the nodeIndex is 0 which
                # means the grammar was left factored.  We must insert the children
                # at the beginning of the parent's children
                # if nodeIndex == 0:
                parent.insert(nodeIndex,node[0])
                parent.insert(nodeIndex+1,node[1])
                # else:
                #     parent.extend([node[0],node[1]])

                # parent is now the current node so the children of parent will be added to the agenda
                node = parent
            else:
                parentIndex = node.label().find(parentChar)
                if parentIndex != -1:
                    # strip the node name of the parent annotation
                    node.set_label(node.label()[:parentIndex])

                # expand collapsed unary productions
                if expandUnary == True:
                    unaryIndex = node.label().find(unaryChar)
                    if unaryIndex != -1:
                        newNode = Tree(node.label()[unaryIndex + 1:], [i for i in node])
                        node.set_label(node.label()[:unaryIndex])
                        node[0:] = [newNode]

            for child in node:
                nodeList.append((child,node))
        #print "#" + str(tree)


def collapse_unary(tree, collapsePOS = False, collapseRoot = True, joinChar = "+"):
    """
    Collapse subtrees with a single child (ie. unary productions)
    into a new non-terminal (Tree node) joined by 'joinChar'.
    This is useful when working with algorithms that do not allow
    unary productions, and completely removing the unary productions
    would require loss of useful information.  The Tree is modified
    directly (since it is passed by reference) and no value is returned.

    :param tree: The Tree to be collapsed
    :type  tree: Tree
    :param collapsePOS: 'False' (default) will not collapse the parent of leaf nodes (ie.
                        Part-of-Speech tags) since they are always unary productions
    :type  collapsePOS: bool
    :param collapseRoot: 'False' (default) will not modify the root production
                         if it is unary.  For the Penn WSJ treebank corpus, this corresponds
                         to the TOP -> productions.
    :type collapseRoot: bool
    :param joinChar: A string used to connect collapsed node values (default = "+")
    :type  joinChar: str
    """

    if collapseRoot == False and isinstance(tree, Tree) and len(tree) == 1:
        nodeList = [tree[0]]
    else:
        nodeList = [tree]

    # depth-first traversal of tree
    while nodeList != []:
        node = nodeList.pop()
        if isinstance(node,Tree):
            if len(node) == 1 and isinstance(node[0], Tree) and (collapsePOS == True or isinstance(node[0,0], Tree)):
                node.set_label(node.label() + joinChar + node[0].label())
                node[0:] = [child for child in node[0]]
                # since we assigned the child's children to the current node,
                # evaluate the current node again
                nodeList.append(node)
            else:
                for child in node:
                    nodeList.append(child)

def flat_print(t):
    # print the tree in one single line
    return t._pprint_flat(nodesep='', parens='()', quotes=False)

# this actually assumes that every bin rule contains one dep, like we assumed in the paper.
def get_binarize_lex(otree, labelChar='^'):
    work_tree = deepcopy(otree)
    lexLabel(work_tree)
    # print work_tree
    parent_dic = getParentDic(work_tree)
    # for x in parent_dic:
    #     print x, parent_dic[x]
    work_tree = deepcopy(otree)
    chomsky_normal_form(work_tree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV)


    preterminals = [t for t in work_tree.subtrees(lambda t: t.height() == 2)]
    for i in xrange(len(preterminals)):
        preterminals[i][0] = preterminals[i][0] + labelChar + str(i+1)
    # print work_tree

    for pos in work_tree.treepositions(order='postorder'):
        t = work_tree[pos]
        if isinstance(t, str):
            continue
        else:
            if len(t) == 1:
                t.set_label(t.label() + labelChar + findIndex(t[0]))
            else:
                x_ind = findIndex(t[0])
                y_ind = findIndex(t[1])
                if parent_dic[x_ind] == y_ind:
                    t.set_label(t.label() + labelChar + y_ind)
                else:
                    t.set_label(t.label() + labelChar + x_ind)

    # for t in work_tree.subtrees():
    #     if isinstance(t, str):
    #         continue
    #     else:
    #         t.set_label(t.label() + labelChar + find_lex_head_bin(t, parent_dic))
    return work_tree

def find_lex_head_bin(nt, parent_dic, labelChar='^'):
    if isinstance(nt, str):
        return findIndex(nt)
    if isinstance(nt, Tree):
        if nt.height() == 2:
            return findIndex(nt[0])
        else:
            if len(nt) == 1:
                return find_lex_head_bin(nt[0], parent_dic)
            else:
                if find_lex_head_bin(nt[0], parent_dic) in parent_dic:
                    if parent_dic[find_lex_head_bin(nt[0], parent_dic)] == find_lex_head_bin(nt[1], parent_dic):
                        return find_lex_head_bin(nt[1], parent_dic)
                    else:
                        return find_lex_head_bin(nt[0], parent_dic)
                else:
                    return find_lex_head_bin(nt[1], parent_dic)

# The function take a lex bin tree and return all the bin (and unary) rules as a set
# the rule is represented as {unary?} {X} {Y} {Z} {DIR}, we will add the {id} later
def generate_rules(lex_bin_tree):
    rule_set = set([])
    for st in lex_bin_tree.subtrees():
        if isinstance(st, str):
            # these are leaves, just skip
            continue
        if isinstance(st, Tree):
            if st.height() == 2:
                # preterminals also don't generate rules
                continue
            else:
                if len(st) == 1:
                    # unary rule
                    X = remove_labelChar(st.label())
                    Y = remove_labelChar(st[0].label())
                    rule = "1 " + X + " " + Y 
                    rule_set.add(rule)
                else:
                    # not unary rule
                    X = remove_labelChar(st.label())
                    Y = remove_labelChar(st[0].label())
                    Z = remove_labelChar(st[1].label())

                    X_ind = findIndex(st.label())
                    Y_ind = findIndex(st[0].label())

                    d = ('0' if X_ind == Y_ind else '1')
                    rule = "0 " + X + " " + Y + " " + Z + " " + d
                    rule_set.add(rule)
    return rule_set





#################################################################
# Demonstration
#################################################################

def demo():
    """
    A demonstration showing how each tree transform can be used.
    """

    # from nltk.draw.tree import draw_trees
    from nltk import tree, treetransforms
    from copy import deepcopy

    # original tree from WSJ bracketed text
    sentence = """(S
  (S
    (VP
      (VBN Turned)
      (ADVP (RB loose))
      (PP
        (IN in)
        (NP
          (NP (NNP Shane) (NNP Longman) (POS 's))
          (NN trading)
          (NN room)))))
  (, ,)
  (NP (DT the) (NN yuppie) (NNS dealers))
  (VP (AUX do) (NP (NP (RB little)) (ADJP (RB right))))
  (. .))"""

    sentence = """(S (NP (NP (JJ Influential) (NNS members)) (PP (IN of) (NP (DT the) (NNP House) (NNP Ways) (CC and) (NNP Means) (NNP Committee)))) (VP (VBD introduced) (NP (NP (NN legislation)) (SBAR (WHNP (WDT that)) (S (VP (MD would) (VP (VB restrict) (SBAR (WHADVP (WRB how)) (S (NP (DT the) (JJ new) (NN savings-and-loan) (NN bailout) (NN agency)) (VP (MD can) (VP (VB raise) (NP (NN capital)))))) (, ,) (S (VP (VBG creating) (NP (NP (DT another) (JJ potential) (NN obstacle)) (PP (TO to) (NP (NP (NP (DT the) (NN government) (POS 's)) (NN sale)) (PP (IN of) (NP (JJ sick) (NNS thrifts)))))))))))))) (. .))"""
    # sentence = """(M (S (A (B (B1 b1) (B2 b2) (B3 (BB1 bb1) (BB2 bb2) (BB3 bb3) ) (B4 b4)) (C (C1 c1) (C2 c2)  ) (D d) (E e) (F f) (G g)) (W w)))"""
    t = tree.Tree.fromstring(sentence, remove_empty_top_bracketing=True)

    # collapse subtrees with only one child
    lTree = deepcopy(t)
    lexLabel(lTree)
    print flat_print(lTree)

    deps = generateDep(t)
    print_conll_lines(deps,sys.stdout)


    collapsedTree = deepcopy(t)
    collapse_unary(collapsedTree)

    # convert the tree to CNF
    # cnfTree = deepcopy(collapsedTree)
    # chomsky_normal_form(cnfTree)

    # convert the tree to CNF with parent annotation (one level) and horizontal smoothing of order two
    parentTree = deepcopy(collapsedTree)
    chomsky_normal_form(parentTree, horzMarkov=HORZMARKOV, vertMarkov=VERTMARKOV)

    # convert the tree back to its original form (used to make CYK results comparable)
    original = deepcopy(parentTree)
    un_chomsky_normal_form(original)

    # convert tree back to bracketed text
    sentence2 = original.pprint()
    print(t.pprint())
    print(sentence2)
    print("Sentences the same? ", str(sentence2) == str(t.pprint()))

    # draw_trees(t, collapsedTree, cnfTree, parentTree, original)
    #print t, collapsedTree, cnfTree, parentTree, original
    print parentTree

    collapse_unary(t)
    t = get_binarize_lex(t)
    print "*" + str(t)
    rule_set = generate_rules(t)
    for x in rule_set:
        print x

if __name__ == '__main__':
    demo()
    #read_from_ptb('/media/lingpenk/Data/PhraseDep/treebank/dev.1.notraces')
