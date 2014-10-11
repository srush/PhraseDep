from nltk import Tree
import random
# http://www.nltk.org/_modules/nltk/treetransforms.html#chomsky_normal_form 

def read_from_ptb(filename):
    ptb_file = open(filename, 'r')
    for l in ptb_file:
        tree = Tree.fromstring(l)
        # for st in tree.subtrees():
        #     print "*" + st.pprint()
        print "*" + tree.pprint()
        tree.chomsky_normal_form(factor='right', horzMarkov=2, vertMarkov=2, childChar='|', parentChar='^')
        print "+" + tree.pprint()
    ptb_file.close()

def chomsky_normal_form(tree, factor="right", horzMarkov=None, vertMarkov=0, childChar="|", parentChar="^"):
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
    # print "***0"
    # print nodeList
    
    while nodeList != []:
        node, parent = nodeList.pop()

        if isinstance(node,Tree):
            # for the node and the parent, we want to know which is the index of the head in Collins head rule
            # which basically is the break point of the binarization
            child_list = [n.label() for n in node if isinstance(n, Tree)]
            if len(child_list) == 0:
                continue

            print parent
            print str(node.label()) + "\t-->\t" + "\t".join(child_list)

            # The head postion is determined by the collins rule
            head_postion = random.randint(0, len(child_list))

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


def un_chomsky_normal_form(tree, expandUnary = True, childChar = "|", parentChar = "^", unaryChar = "+"):
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


def collapse_unary(tree, collapsePOS = False, collapseRoot = False, joinChar = "+"):
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

#################################################################
# Demonstration
#################################################################

def demo():
    """
    A demonstration showing how each tree transform can be used.
    """

    from nltk.draw.tree import draw_trees
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
    sentence = """(S (A (B (B1 b1) (B2 b2) (B3 (BB1 bb1) (BB2 bb2) (BB3 bb3) ) (B4 b4)) (C (C1 c1) (C2 c2)  ) (D d) (E e) (F f) (G g)) (W w))"""
    t = tree.Tree.fromstring(sentence, remove_empty_top_bracketing=True)

    # collapse subtrees with only one child
    # collapsedTree = deepcopy(t)
    # collapse_unary(collapsedTree)

    # convert the tree to CNF
    # cnfTree = deepcopy(collapsedTree)
    # chomsky_normal_form(cnfTree)

    # convert the tree to CNF with parent annotation (one level) and horizontal smoothing of order two
    parentTree = deepcopy(t)
    chomsky_normal_form(parentTree, horzMarkov=2, vertMarkov=2)

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
    

if __name__ == '__main__':
    demo()
    # read_from_ptb('/media/lingpenk/Data/PhraseDep/treebank/dev.1.notraces')
