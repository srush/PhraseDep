from tree import *

def test_binarize():
    rules = read_original_rules(open("corpora/orules"))
    tree = Tree('NP^2', ('DT^1', 'NNP^1', 'NNP^1', 'CC^1', 'NNP^1', 'NNP^1'))
    correct = Tree('NP^2',
                   ['DT^1', Tree('Z_(NP_l_DT)^1',
                               ['NNP^1', Tree('Z_(NP_l_NNP)^1',
                                            ['NNP^1', Tree('Z_(NP_l_NNP)^1',
                                                         ['CC^1', Tree('Z_(NP_l_CC)^1', ['NNP^1', 'NNP^1'])])])])])

    bin_tree =  binarize(rules, tree, 4)

    check = unbinarize(bin_tree)
    assert(check == tree)
    assert(bin_tree == correct)

    correct = Tree('NP^2', 
                   [Tree('Z_(NP_r_NNP)^1', 
                         [Tree('Z_(NP_r_NNP)^1', 
                               [Tree('Z_(NP_r_CC)^1', 
                                     [Tree('Z_(NP_r_NNP)^1', ['DT^1', 'NNP^1']), 'NNP^1']), 'CC^1']), 'NNP^1']), 'NNP^1'])
    
    bin_tree =  binarize(rules, tree, 0)
    check = unbinarize(bin_tree)
    assert(check == tree)
    assert(bin_tree == correct)


    correct = Tree('NP^2', ['DT^1', 
                          Tree('Z_(NP_l_DT)^1', 
                               ['NNP^1', Tree('Z_(NP_l_NNP)^1', 
                                            ['NNP^1', Tree('Z_(NP_l_NNP)^1', 
                                                         [Tree('Z_(NP_r_NNP)^1', ['CC^1', 'NNP^1']), 'NNP^1'])])])])
    bin_tree =  binarize(rules, tree, 3)
    print correct
    print bin_tree

    assert(bin_tree == correct)
    check = unbinarize(bin_tree)
    assert(check == tree)
    #bin_tree.draw()
