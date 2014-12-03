#ifndef GRAMMAR_H_
#define GRAMMAR_H_

#include <string>
#include <assert.h>
#include <vector>
#include <map>
#include <iostream>
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

    bool same_span(const AppliedRule &other) const{
        return other.i == i &&
                other.j == j &&
                other.k == k;
    }

    bool same_top(const AppliedRule &other) const{
        return other.i == i &&
                other.k == k;
    }


    bool same_non_head(const AppliedRule &other) const{
        return other.i == i &&
                other.j == j &&
                other.k == k &&
                other.rule == rule;
    }


    bool same(const AppliedRule &other) const{
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



class Scorer {
  public:
    virtual double score(const AppliedRule &rule) const = 0;
};

struct Index {
    Index() : cur_index(0) {}

    int fget(string item) {
        if (fmap.find(item) != fmap.end()) {
            return fmap[item];
        } else {
            fmap[item] = cur_index;
            rmap[cur_index] = item;
            cur_index++;
            return cur_index - 1;
        }
    }

    int fget(string item) const {
        return fmap.at(item);
    }

    string rget(int index) const {
        return rmap.at(index);
    }

    int size() const {
        return cur_index;
    }

    int cur_index;
    map<string, int> fmap;
    map<int, string> rmap;
};

struct DepLabel {
  DepLabel() {}

    static DepLabel build(string label, const Index &nt_index) {
        // Label parts are of the form S+NP+VP

        vector<string> label_parts(3);
        int p = 0;
        for (int i = 0; i < label.size(); ++i) {
            if (label[i] == '+') {
                p += 1;
            } else {
                label_parts[p] += label;
            }
        }
        DepLabel out;

        out.rule_label = label_parts[0];
        out.head_label = label_parts[1];
        out.mod_label = label_parts[2];

        out.rule_label_id = nt_index.fget(out.rule_label);
        out.head_label_id = nt_index.fget(out.head_label);
        out.mod_label_id = nt_index.fget(out.mod_label);

        return out;
    }

    string rule_label;
    string head_label;
    string mod_label;

    int rule_label_id;
    int head_label_id;
    int mod_label_id;
};


struct NonTerminal {
    NonTerminal() {}

    static NonTerminal build(string nt_str, vector<Index> *nt_indices) {
        NonTerminal nt;
        nt.full = nt_str;
        nt.removable = false;
        nt.split.resize(1);
        int split = 0;
        int mode = 0;
        for (int i = 0; i < nt_str.size(); ++i) {
            char cur = nt_str[i];
            if (cur == '#') {
                mode = 1;
            } else if (cur == '|') {
                mode = 2;
                nt.removable = true;
                nt.main += '|';
            } else {
                if (mode == 0) {
                    nt.main += cur;
                    if (cur == '+') {
                        split += 1;
                        nt.split.resize(split + 1);
                    } else {
                        nt.split[split] += cur;
                    }
                } else if (mode ==1) {
                    nt.vert_mark += cur;
                } else if (mode ==2) {
                    nt.horiz_mark += cur;
                    nt.main += cur;
                }
            }
        }
        nt.int_main = (*nt_indices)[0].fget(nt.main);
        nt.int_vert_mark = (*nt_indices)[1].fget(nt.vert_mark);
        nt.int_horiz_mark = (*nt_indices)[2].fget(nt.horiz_mark);

        return nt;
    }

    string full;

    string main;
    int int_main;

    string vert_mark;
    int int_vert_mark;

    string horiz_mark;
    int int_horiz_mark;

    vector<string> split;

    bool removable;
};

struct DirPrune {
    DirPrune(int head_, int left_, int right_, int left_first_, int right_first_)
            : head(head_), left(left_), right(right_),
              left_first(left_first_), right_first(right_first_) {}

    int head;
    int left;
    int right;
    int left_first;
    int right_first;
};

inline bool operator==(const DirPrune& left, const DirPrune& right) {
    return (left.head == right.head) &&
            (left.right == right.right) &&
            (left.left == right.left) &&
            (left.left_first == right.left_first) &&
            (left.right_first == right.right_first);
}

inline bool operator<(const DirPrune& left, const DirPrune& right) {
    if (left.head < right.head) { return true; }
    if (left.head > right.head) { return false; }

    if (left.right < right.right) { return true; }
    if (left.right > right.right) {return false; }

    if (left.left < right.left) { return true; }
    if (left.left > right.left) { return false; }

    if (left.left_first < right.left_first) { return true; }
    if (left.left_first > right.left_first) { return false; }

    if (left.right_first < right.right_first) { return true; }
    if (left.right_first > right.right_first) { return false; }

    return false;
}


class Grammar {
  public:

    Grammar() : nt_indices(3) {
        n_nonterms = 0;
        n_words = 0;

        pruning = false;
        label_pruning = false;
        dir_pruning = false;
    }

    int to_word(string word);
    int to_deplabel(string deplabel);

    int to_nonterm(string nonterm);

    void add_rule(BinaryRule rule);

    double score(const AppliedRule &rule) const {
        return 0.0;
    }

    void add_unary_rule(UnaryRule rule);

    void finish(const vector<int> &roots_);

    double dir_pick(const DirPrune &prune,
                    bool *try_left, bool *try_right) const;

    Index tag_index;

    int n_nonterms;
    int n_rules;
    int n_words;
    int n_deplabel;

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

    map<string, int> deplabel_map;

    vector<Index> nt_indices;

    vector<NonTerminal> non_terminals_;

    vector<vector<bool> > rule_head_tags;
    vector<vector<bool> > label_rule_allowed;
    bool pruning;
    bool label_pruning;
    bool dir_pruning;

    map<DirPrune, pair<int, int> > dir_pruner;

    vector<int> sparse_rule_index;
    Index sparse_rules;
};


Grammar *read_rule_set(string file);
// void read_pruning(string file, Grammar *grammar);
void read_label_pruning(string file, Grammar *grammar);

#endif  // GRAMMAR_H_
