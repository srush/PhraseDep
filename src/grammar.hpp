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


struct NonTerminal {
    NonTerminal() {}

    static NonTerminal build(string nt_str) {
        NonTerminal nt;
        nt.full = nt_str;
        nt.removable = false;
        int mode = 0;
        for (int i = 0; i < nt_str.size(); ++i) {
            char cur = nt_str[i];
            if (cur == '#') {
                mode = 1;
            } else if (cur == '|') {
                mode = 2;
                nt.removable = true;
            } else {
                if (mode == 0) {
                    nt.main += cur;
                } else if (mode ==1) {
                    nt.vert_mark += cur;
                } else if (mode ==2) {
                    nt.horiz_mark += cur;
                }
            }
        }
        return nt;
    }

    string full;
    string main;
    string vert_mark;
    string horiz_mark;
    bool removable;
};

class Grammar {
  public:

    Grammar() {
        n_nonterms = 0;
        n_words = 0;
        pruning = false;
    }

    int to_word(string word);

    int to_nonterm(string nonterm);

    void add_rule(BinaryRule rule);

    double score(const AppliedRule &rule) const {
        return 0.0;
    }

    void add_unary_rule(UnaryRule rule);

    void finish(const vector<int> &roots_);

    int n_nonterms;
    int n_rules;
    int n_words;

    vector<int> roots;

    vector<int> head_symbol;
    vector<int> left_symbol;
    vector<int> right_symbol;
    vector<int> is_unary;

    vector<vector<UnaryRule> > unary_rules_by_first;
    vector<vector<BinaryRule> > rules_by_first;
    vector<vector<BinaryRule> > rules_by_second;

    map<string, int> word_map;
    map<string, int> nonterm_map;
    map<int, string> rev_nonterm_map;

    vector<NonTerminal> non_terminals_;

    vector<vector<bool> > rule_head_tags;
    bool pruning;
};


Grammar *read_rule_set(string file);
void read_pruning(string file, Grammar *grammar);

#endif  // GRAMMAR_H_
