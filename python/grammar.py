from collections import namedtuple, defaultdict
import numpy as np


class BinarizedRule(namedtuple("BinarizedRule",
                               ["X", "Y", "Z",
                                "original", "direction",
                                "mins"])):
    """
    A binarized rule consists of a lexicalized rule X->Y Z
    and possibly information about the original rule it 
    was derived from. 

    Attributes
    -----------
    X, Y, Z : strings
       The non-terminals of the rule X -> Y Z

    original : int
       The id of the original rule. 
    
    direction : int
       The head of the rule. 0 for Y, 1 for z.

    mins : pair of int
       The minimal left/right spans of X.  
    """
    pass

class UnaryRule(namedtuple("UnaryRule",
                           ["X", "Y"])):
    """
    A unary rule consisting of the form X->Y.
    """
    pass

class RuleSet(object):
    """
    The complete binarized rule set. 

    Attributes
    ----------
    nonterms, rev_nonterms : dict
        Mapping from non-terminal to id and reverse.

    rules, unary_rules : dict
        Mapping from id to rules and unary rules. 
    
    rule_table : ndarray
        A Gx4 matrix. The first three columns are X, Y, Z, 
        and the last column is a direction bit. 

    unary_table : ndarray
        A Gx2 matrix. The columns are X, Y.

    rules_by_* : dict
        Cached lookup tables to find rules by combinations of 
        X,Y,Z and direction.

    """

    def __init__(self):
        self.nonterms = {"*BLANK*" : 0}
        self.rev_nonterms = {0 : "*BLANK*"}

        self.rules = []
        self.unary_rules = []

        self.rule_table = []
        self.unary_table = []


        self._unary_rev_indices = {}
        self._unary_indices = {}
        self._rule_indices = {}
        self._rule_rev_indices = {}

        self.root = None

    def _add_nonterm(self, nonterm):
        if nonterm not in self.nonterms:
            i = len(self.nonterms)
            self.nonterms[nonterm] = i
            self.rev_nonterms[i] = nonterm

    def add(self, rule):
        self._add_nonterm(rule.X)
        self._add_nonterm(rule.Y)
        self._add_nonterm(rule.Z)
        self.rules.append(rule)

    def add_unary(self, rule):
        self._add_nonterm(rule.X)
        self._add_nonterm(rule.Y)
        self.unary_rules.append(rule)

    def nonterm_name(self, index):
        return self.rev_nonterms[int(index)]

    def nonterm_index(self, name):
        return self.nonterms[name]

    def rule_index(self, X, Y, Z=None):
        """
        Returns the index i based on the rule X -> Y Z.
        """

        if Z is not None:
            return self._rule_indices[X, Y, Z]
        else:
            return self._unary_indices[X, Y]

    def rule_nonterms(self, index):
        """
        Returns the rules X -> Y Z based on index.
        """
        if index < len(self.rules):
            return self._rule_rev_indices[index]
        else:
            return self._unary_rev_indices[index]

    def finish(self):
        self.root = self.nonterms["S"]


        for i, rule in enumerate(self.rules):
            X_n = self.nonterms[rule.X]
            Y_n = self.nonterms[rule.Y]
            Z_n = self.nonterms[rule.Z]
            self._rule_indices[rule.X, rule.Y, rule.Z] = i
            self._rule_rev_indices[i] = (rule.X, rule.Y, rule.Z)
            self.rule_table.append([X_n, Y_n, Z_n,
                                    rule.direction])

        # Start the enumeration at the end of rules.
        for i, rule in enumerate(self.unary_rules, len(self.rules)):
            X_n = self.nonterms[rule.X]
            Y_n = self.nonterms[rule.Y]
            self._unary_indices[rule.X, rule.Y] = i
            self._unary_rev_indices[i] = (rule.X, rule.Y)
            self.unary_table.append([X_n, Y_n])

            # self.rule_indices[rule.X, rule.Y, 0] = i
            # self.rule_rev_indices[i] = (rule.X, rule.Y, 0)
            self.rule_table.append([X_n, Y_n, 0, 0])

        self.rule_table = np.array(self.rule_table)
        self.unary_table = np.array(self.unary_table)

        # Build indices for the rules.
        self.rules_by_nonterm = defaultdict(lambda: [])
        self.rules_by_first = defaultdict(lambda: [])
        self.rules_by_second = defaultdict(lambda: [])
        self.rules_by_both = defaultdict(lambda: [])
        self.rules_by_nonterm_dir = defaultdict(lambda: [])
        self.rules_by_first_nonterm_dir = defaultdict(lambda: [])
        self.rules_by_both_dir = defaultdict(lambda: [])

        for r, rule in enumerate(self.rule_table[:len(self.rules)]):
            self.rules_by_nonterm_dir[rule[0], rule[3]].append(
                (r,rule[1], rule[2]))
            self.rules_by_nonterm[rule[0]].append(
                (r,rule[1], rule[2], rule[3]))
            self.rules_by_first[rule[1]].append((r, rule[0], rule[2], rule[3]))
            self.rules_by_second[rule[2]].append((r, rule[0], rule[1], rule[3]))
            self.rules_by_both[rule[1], rule[2]].append((r, rule[0], rule[3]))
            self.rules_by_both_dir[rule[1], rule[2], rule[3]].append(
                (r, rule[0]))


def read_rule_set(handle):
    """
    Read the rule set from a file handle.

    Returns
    -------
    RuleSet : 
    """
    rules = RuleSet()

    for l in handle:
        if not l.strip(): 
            continue
        t = l.split()
        if len(t) == 6:
            X = t[0].strip()
            Y = t[2].strip()[:-1]
            rules.add_unary(UnaryRule(X, Y))
        else:
            X = t[0].strip()
            Y = t[2].strip()
            Z = t[3].strip()

            original = int(t[4])
            if Y[-1] == "*":
                Y = Y[:-1]
                direction = 0
            elif Z[-1] == "*":
                Z = Z[:-1]
                direction = 1
            else:
                assert False, "Need a head."
            min_l = int(t[5].strip())
            min_r = int(t[6].strip())

            rules.add(BinarizedRule(X, Y, Z, 
                                    original, direction, (min_l, min_r)))
    rules.finish()
    return rules
