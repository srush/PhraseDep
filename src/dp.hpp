#include "grammar.hpp"
#include "features.hpp"
#include <vector>
using namespace std;

void cky(const vector<int> &preterms,
         const vector<string> &words,
         const vector<int> &deps,
         const Grammar &grammar,
             const FeatureScorer &scorer,
         vector<AppliedRule> *best_rules,
         bool output);
