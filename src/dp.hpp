#include "grammar.hpp"
#include "features.hpp"
#include <vector>
using namespace std;

struct Item {
    Item() : used(false) {}
    Item(int i_, int k_, int h_, int nt_, int layer_)
            : i(i_), k(k_), h(h_), nt(nt_), layer(layer_), used(true) {}
    int i;
    int k;
    int h;
    int nt;
    int layer;
    bool used;
};


struct BackPointer {
    BackPointer() : terminal(false), single(false), promotion(false) {}
    bool terminal;
    bool single;
    bool promotion;
    Item item1;
    Item item2;
    AppliedRule rule;
};


double cky(const vector<int> &preterms,
           const vector<string> &words,
           const vector<int> &deps,
           const Grammar &grammar,
           const Scorer &scorer,
           vector<AppliedRule> *best_rules,
           bool output,
           bool *success);

double cky2(const vector<int> &preterms,
            const vector<string> &words,
            const vector<int> &deps,
            const Grammar &grammar,
            const Scorer &scorer,
            vector<AppliedRule> *best_rules,
            bool output,
            bool *success,
            bool no_prune);

double cky_full(const vector<int> &preterms,
                const vector<string> &words,
                const Grammar &grammar,
                const Scorer &scorer,
                vector<AppliedRule> *best_rules,
                bool output,
                bool *success);
