from StringIO import StringIO
from grammar import *

def test_read_rules():
    rules = StringIO("""VP	-->	VBZ*	UCP		653 0 0 
S	-->	NP*	ADVP		1390 0 0 
Z_(2229,l,0)	-->	PRN	Z_(2229,l,1)*		2229 0 0
Z_(2229,l,1)	-->	VP*	.		2229 0 0 
S	-->	S	Z_(2229,l,0)*		2229 0 0 
Z_(673,l,0)	-->	NNP	NNS*		673 0 0 
A	-->	B		       673 0 0 
""")
    rules = read_rule_set(rules)
    assert rules.rules[0] == BinarizedRule("VP", "VBZ", "UCP", 653, 0, (0, 0))
    assert rules.rules[1] == BinarizedRule("S", "NP", "ADVP", 1390, 0, (0, 0))
    assert rules.rules[2] == BinarizedRule("Z_(2229,l,0)", "PRN", "Z_(2229,l,1)", 2229, 1, (0, 0)), rules.rules[2]
    assert (rules.rule_table == np.array([[0, 1, 2, 0],
                                         [3, 4, 5, 0],
                                         [6, 7, 8, 1],
                                         [8, 0, 9, 0],
                                         [3, 3, 6, 1],
                                         [10, 11, 12, 1]])).all(), rules.rule_table



