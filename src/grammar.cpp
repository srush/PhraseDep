#include "grammar.hpp"
#include <iostream>
#include <fstream>
using namespace std;

int Grammar::to_word(string word) {
    if (word_map.find(word) != word_map.end()) {
        return word_map[word];
    } else {
        word_map[word] = n_words;
        n_words++;
        return n_words-1;
    }
}

int Grammar::to_deplabel(string deplabel) {
    if (deplabel_map.find(deplabel) != deplabel_map.end()) {
        return deplabel_map[deplabel];
    } else {
        deplabel_map[deplabel] = n_deplabel;
        n_deplabel++;
        return n_deplabel-1;
    }
}

int Grammar::to_nonterm(string nonterm) {
    if (nonterm_map.find(nonterm) != nonterm_map.end()) {
        return nonterm_map[nonterm];
    } else {
        nonterm_map[nonterm] = n_nonterms;
        rev_nonterm_map[n_nonterms] = nonterm;

        NonTerminal non_terminal = NonTerminal::build(nonterm, &nt_indices);
        non_terminals_.push_back(non_terminal);
        n_nonterms++;
        return n_nonterms-1;
    }
}

void Grammar::add_rule(BinaryRule rule) {
    if (rules_by_first.size() <= n_nonterms) {
        rules_by_first.resize(n_nonterms + 1);
    }
    if (rules_by_second.size() <= n_nonterms) {
        rules_by_second.resize(n_nonterms + 1);
    }

    head_symbol.push_back(rule.nt_X);
    left_symbol.push_back(rule.nt_Y);
    right_symbol.push_back(rule.nt_Z);
    is_unary.push_back(0);

    if (rule.dir == 0) {
        rules_by_first[rule.nt_Y].push_back(rule);
    } else {
        rules_by_second[rule.nt_Z].push_back(rule);
    }

    string simple_rule =
            non_terminals_[rule.nt_X].main + "->" +
            non_terminals_[rule.nt_Y].main + " " +
            non_terminals_[rule.nt_Z].main;
    sparse_rule_index.push_back(sparse_rules.fget(simple_rule));

    n_rules++;
}


void Grammar::add_unary_rule(UnaryRule rule) {
    assert(rule.nt_X < n_nonterms);
    assert(rule.nt_Y < n_nonterms);
    if (unary_rules_by_first.size() <= n_nonterms) {
        unary_rules_by_first.resize(n_nonterms + 1);
    }
    head_symbol.push_back(rule.nt_X);
    left_symbol.push_back(rule.nt_Y);
    right_symbol.push_back(0);
    is_unary.push_back(1);
    unary_rules_by_first[rule.nt_Y].push_back(rule);

    string simple_rule =
            non_terminals_[rule.nt_X].main + "->"
            + non_terminals_[rule.nt_Y].main;
    sparse_rule_index.push_back(sparse_rules.fget(simple_rule));

    n_rules++;
}


void Grammar::finish(const vector<int> &roots_) {
    roots = roots_;
    if (rules_by_first.size() <= n_nonterms) {
        rules_by_first.resize(n_nonterms + 1);
    }

    if (rules_by_second.size() <= n_nonterms) {
        rules_by_second.resize(n_nonterms + 1);
    }

    if (unary_rules_by_first.size() <= n_nonterms) {
        unary_rules_by_first.resize(n_nonterms + 1);
    }
    rule_head_tags.resize(n_rules + 1);
    for (int i = 0; i < n_rules; ++i) {
        rule_head_tags[i].resize(n_nonterms, 0);
    }
}


// void read_pruning(string file, Grammar *grammar) {
//     ifstream in_file;
//     grammar->pruning = true;
//     in_file.open(file.c_str());

//     if (in_file.is_open()) {
//         int rule_num;
//         while (in_file >> rule_num) {
//             int allowed_nt;
//             in_file >> allowed_nt;
//             for (int i = 0; i < allowed_nt; ++i) {
//                 string nt;
//                 in_file >> nt;
//                 grammar->rule_head_tags[rule_num][grammar->to_nonterm(nt)] = 1;
//             }
//         }
//     }
// }


void read_label_pruning(string file, Grammar *grammar) {
    ifstream in_file;
    grammar->label_pruning = true;
    in_file.open(file.c_str());

    if (in_file.is_open()) {
        string label;
        while (in_file >> label) {
            int label_id = grammar->deplabel_map[label];
            int allowed_rules;
            in_file >> allowed_rules;
            for (int i = 0; i < allowed_rules; ++i) {
                int rule_num;
                in_file >> rule_num;
                grammar->label_rule_allowed[label_id][rule_num] = 1;
            }
        }
    }
}


Grammar *read_rule_set(string file) {
    Grammar *grammar = new Grammar();

    ifstream in_file;
    in_file.open(file.c_str());

    if (in_file.is_open()) {
        int rule_num;
        while (in_file >> rule_num) {
            string X, blank, Y, Z;
            int original;
            int min_l, min_r;
            bool is_unary;
            int dir;
            in_file >>  is_unary;
            if (!is_unary) {
                in_file >> X >> Y >> Z >> dir;
                int nt_X = grammar->to_nonterm(X);
                int nt_Y = grammar->to_nonterm(Y);
                int nt_Z = grammar->to_nonterm(Z);
                BinaryRule rule(rule_num, nt_X, nt_Y, nt_Z, dir);
                grammar->add_rule(rule);

            } else {
                in_file >> X >> Y;
                int nt_X = grammar->to_nonterm(X);
                int nt_Y = grammar->to_nonterm(Y);
                UnaryRule rule(rule_num, nt_X, nt_Y);
                grammar->add_unary_rule(rule);
            }
        }
    }
    vector<int> roots;

    roots.push_back(grammar->to_nonterm("TOP"));

    grammar->finish(roots);
    return grammar;
}
