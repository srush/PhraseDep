#include "dp.hpp"
#include "grammar.hpp"
#include <vector>
using namespace std;

void test_dp() {
    Grammar test_grammar;
    test_grammar.n_rules = 3;

    vector<int> test_preterms;
    test_preterms.push_back(test_grammar.to_nonterm("C"));
    test_preterms.push_back(test_grammar.to_nonterm("A"));
    test_preterms.push_back(test_grammar.to_nonterm("B"));

    vector<int> test_deps;
    test_deps.push_back(-1);
    test_deps.push_back(0);
    test_deps.push_back(1);






    test_grammar.add_rule(BinaryRule(0, test_grammar.to_nonterm("D"),
                                     test_grammar.to_nonterm("A"), test_grammar.to_nonterm("B"), 0));
    test_grammar.add_rule(BinaryRule(1, test_grammar.to_nonterm("F"), test_grammar.to_nonterm("C") , test_grammar.to_nonterm("D"), 0));
    test_grammar.add_unary_rule(UnaryRule(2, test_grammar.to_nonterm("F"), test_grammar.to_nonterm("F")));
    test_grammar.root = test_grammar.to_nonterm("F");
    test_grammar.finish();

    vector<AppliedRule> best;
    cky(test_preterms, test_deps, test_grammar, &best);
}

int main() {
    test_dp();
}
