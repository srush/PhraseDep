#ifndef GRAMMAR_H_
#define GRAMMAR_H_

#include <string>
#include <assert.h>
#include <vector>
#include <map>
using namespace std;

struct UnaryRule {
    UnaryRule(int rule_num_, int nt_X_, int nt_Y_)
                : rule_num(rule_num_), nt_X(nt_X_), nt_Y(nt_Y_) {}
    int rule_num;
    int nt_X;
    int nt_Y;
};

struct BinaryRule {
    BinaryRule(int rule_num_, int nt_X_, int nt_Y_, int nt_Z_, int dir_)
            : rule_num(rule_num_), nt_X(nt_X_), nt_Y(nt_Y_), nt_Z(nt_Z_), dir(dir_)
    {}
    int rule_num;
    int nt_X;
    int nt_Y;
    int nt_Z;
    int dir;
};

struct AppliedRule {
    AppliedRule() {}
    AppliedRule(int i_, int j_, int k_, int h_, int m_, int rule_)
            : i(i_), j(j_), k(k_), h(h_), m(m_), rule(rule_) {}

    bool same(const AppliedRule &other) {
        return other.i == i &&
                other.j == j &&
                other.k == k &&
                other.h == h &&
                other.m == m &&
                other.rule == rule;
    }

    int i;
    int j;
    int k;
    int h;
    int m;
    int rule;
};


class Grammar {
  public:

    Grammar() {
        n_nonterms = 0;

    }

    int to_nonterm(string nonterm) {
        if (nonterm_map.find(nonterm) != nonterm_map.end()) {
            return nonterm_map[nonterm];
        } else {
            nonterm_map[nonterm] = n_nonterms;
            rev_nonterm_map[n_nonterms] = nonterm;
            n_nonterms++;
            return n_nonterms-1;
        }
    }

    void add_rule(BinaryRule rule) {
        if (rules_by_first.size() <= n_nonterms) {
            rules_by_first.resize(n_nonterms + 1);
        }
        head_symbol.push_back(rule.nt_X);
        left_symbol.push_back(rule.nt_Y);
        right_symbol.push_back(rule.nt_Z);
        is_unary.push_back(0);
        n_rules++;
        return rules_by_first[rule.nt_Y].push_back(rule);
    }

    double score(const AppliedRule &rule) const {
        return 0.0;
    }

    void add_unary_rule(UnaryRule rule) {
        assert(rule.nt_X < n_nonterms);
        assert(rule.nt_Y < n_nonterms);
        if (unary_rules_by_first.size() <= n_nonterms) {
            unary_rules_by_first.resize(n_nonterms + 1);
        }
        head_symbol.push_back(rule.nt_X);
        left_symbol.push_back(rule.nt_Y);
        right_symbol.push_back(0);
        is_unary.push_back(0);
        n_rules++;
        return unary_rules_by_first[rule.nt_Y].push_back(rule);

    }

    void finish(const vector<int> &roots_) {
        roots = roots_;
        if (rules_by_first.size() <= n_nonterms) {
            rules_by_first.resize(n_nonterms + 1);
        }
        if (unary_rules_by_first.size() <= n_nonterms) {
            unary_rules_by_first.resize(n_nonterms + 1);
        }
    }

     int n_nonterms;
     int n_rules;

    vector<int> roots;

    vector<int> head_symbol;
    vector<int> left_symbol;
    vector<int> right_symbol;
    vector<int> is_unary;

    vector<vector<UnaryRule> > unary_rules_by_first;
    vector<vector<BinaryRule> > rules_by_first;
    map<string, int> nonterm_map;
    map<int, string> rev_nonterm_map;

};


Grammar *read_rule_set(string file);

#endif  // GRAMMAR_H_
