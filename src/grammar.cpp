#include "grammar.hpp"
#include <iostream>
#include <fstream>
using namespace std;

void read_pruning(string file, Grammar *grammar) {
    ifstream in_file;
    grammar->pruning = true;
    in_file.open(file.c_str());

    if (in_file.is_open()) {
        int rule_num;
        while (in_file >> rule_num) {
            int allowed_nt;
            in_file >> allowed_nt;
            for (int i = 0; i < allowed_nt; ++i) {
                string nt;
                in_file >> nt;
                grammar->rule_head_tags[rule_num][grammar->to_nonterm(nt)] = 1;
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
    roots.push_back(grammar->to_nonterm("S"));
    roots.push_back(grammar->to_nonterm("SQ"));
    roots.push_back(grammar->to_nonterm("SBARQ"));
    roots.push_back(grammar->to_nonterm("FRAG"));
    roots.push_back(grammar->to_nonterm("SINV"));
    roots.push_back(grammar->to_nonterm("NP"));
    roots.push_back(grammar->to_nonterm("UCP"));

    grammar->finish(roots);
    return grammar;
}
