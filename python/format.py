from collections import namedtuple, defaultdict
import numpy as np


class BinarizedNonTerm(namedtuple("BinarizedNonTerm", ["name"])):
    pass


class BinarizedRule(namedtuple("BinarizedRule",
                               ["X", "Y", "Z",
                                "original", "direction",
                                "mins"])):
    pass

class UnaryRule(namedtuple("UnaryRule",
                           ["X", "Y"])):
    pass

class RuleSet(object):
    def __init__(self):
        self.nonterms = {}
        # self.preterms = {}
        self.rules = []
        self.unary_rules = []

    def add_nonterm(self, nonterm):
        if nonterm not in self.nonterms:
            self.nonterms[nonterm] = len(self.nonterms)

    # def add_preterm(self, preterm):
    #     if preterm not in self.preterms:
    #         self.preterms[preterm] = len(self.preterms)

    def add(self, rule):
        self.add_nonterm(rule.X)
        self.add_nonterm(rule.Y)
        self.add_nonterm(rule.Z)
        self.rules.append(rule)

    def add_unary(self, rule):
        self.add_nonterm(rule.X)
        self.add_nonterm(rule.Y)
        self.unary_rules.append(rule)

    def finish(self):
        self.rule_table = []
        self.rule_limits = []
        self.unary_table = []
        self.unary_rev_indices = {}
        self.unary_indices = {}
        self.rule_indices = {}
        self.rule_rev_indices = {}


        for i, rule in enumerate(self.rules):
            X_n = self.nonterms[rule.X]
            Y_n = self.nonterms[rule.Y]
            Z_n = self.nonterms[rule.Z]
            self.rule_indices[rule.X, rule.Y, rule.Z] = i
            self.rule_rev_indices[i] = (rule.X, rule.Y, rule.Z)
            self.rule_table.append([X_n, Y_n, Z_n,
                                    rule.direction])
            self.rule_limits.append(rule.mins)

        # Start the enumeration at the end of rules.
        for i, rule in enumerate(self.unary_rules, len(self.rules)):
            X_n = self.nonterms[rule.X]
            Y_n = self.nonterms[rule.Y]
            self.unary_indices[rule.X, rule.Y] = i
            self.unary_rev_indices[i] = (rule.X, rule.Y)
            self.unary_table.append([X_n, Y_n])

            self.rule_indices[rule.X, rule.Y, 0] = i
            self.rule_rev_indices[i] = (rule.X, rule.Y, 0)
            self.rule_table.append([X_n, Y_n, 0, 0])
            self.rule_limits.append((0, 0))

        self.rule_table = np.array(self.rule_table)
        self.unary_table = np.array(self.unary_table)
        self.rule_limits = np.array(self.rule_limits)

        # Build indices for the rules.
        self.rules_by_nonterm = defaultdict(lambda: [])
        self.rules_by_first = defaultdict(lambda: [])
        self.rules_by_second = defaultdict(lambda: [])
        self.rules_by_both = defaultdict(lambda: [])
        self.rules_by_nonterm_dir = defaultdict(lambda: [])
        self.rules_by_first_nonterm_dir = defaultdict(lambda: [])
        self.rules_by_both_dir = defaultdict(lambda: [])
        for r, rule in enumerate(self.rule_table):
            self.rules_by_nonterm_dir[rule[0], rule[3]].append((r,rule[1], rule[2]))
            self.rules_by_nonterm[rule[0]].append((r,rule[1], rule[2], rule[3]))
            self.rules_by_first[rule[1]].append((r, rule[0], rule[2], rule[3]))
            self.rules_by_second[rule[2]].append((r, rule[0], rule[1], rule[3]))
            self.rules_by_both[rule[1], rule[2]].append((r, rule[0], rule[3]))
            self.rules_by_both_dir[rule[1], rule[2], rule[3]].append((r, rule[0]))

        self.rule_vec_first = {}
        self.rule_vec_second = {}
        for X in range(len(self.nonterms)):
            self.rule_vec_first[X] = \
                {rule[0]
                 for rule in self.rules_by_first[X]}
            self.rule_vec_second[X] = \
                {rule[0]
                 for rule in self.rules_by_second[X]}

def read_rule_set(handle):
    rules = RuleSet()

    for l in handle:
        if not l.strip(): continue
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

            rules.add(BinarizedRule(X, Y, Z, original, direction, (min_l, min_r)))
    rules.finish()
    return rules
