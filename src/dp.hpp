#include "grammar.hpp"
#include "features.hpp"
#include <vector>
using namespace std;

double cky(const vector<int> &preterms,
           const vector<string> &words,
           const vector<int> &deps,
           const Grammar &grammar,
           const Scorer &scorer,
           vector<AppliedRule> *best_rules,
           bool output);
