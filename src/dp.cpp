#include <vector>
#include <iostream>
#include <assert.h>
#include <sstream>
#include "grammar.hpp"
#include "features.hpp"

using namespace std;

struct DepSplit {
    DepSplit(int head_, int mod_, int split_)
            : head(head_), mod(mod_), split(split_) {}

    int head;
    int mod;
    int split;
};


struct RuleSplit {
    RuleSplit() {}
    RuleSplit(int rule_index_, int nt1_, int nt2_)
            : rule(rule_index_), nt1(nt1_), nt2(nt2_) {}

    int rule;
    int nt1;
    int nt2;
};

struct Item {

    Item() {}
    Item(int i_, int k_, int h_, int nt_, int layer_)
            : i(i_), k(k_), h(h_), nt(nt_), layer(layer_){}
    int i;
    int k;
    int h;
    int nt;
    int layer;

};



void span_pruning(const int n, const vector<int> &deps,
                  vector<vector<vector<DepSplit> > > *chart) {

    // List of DepSplits  at each span.
    chart->resize(n);

    // List of head positions at each span.
    vector<vector<vector<int> > > heads(n);
    vector<vector<vector<int> > > has_head(n);

    // Initialize.
    for (int i = 0; i < n; ++i) {
        heads[i].resize(n);
        (*chart)[i].resize(n);
        (*chart)[i][i].push_back(DepSplit(i, i, i));
        heads[i][i].push_back(i);
        has_head[i].resize(n);
        for (int j = 0; j < n; ++j) {
            has_head[i][j].resize(n, 0);
        }
    }

    for (int d = 1; d < n; ++d) {
        for (int i = 0; i < n; ++i) {
            int k = i + d;
            if (k >= n) continue;

            for (int j = i; j < k; ++j) {
                for (int h1_index = 0; h1_index < heads[i][j].size(); ++h1_index) {
                    int h1 = heads[i][j][h1_index];
                    for (int h2_index = 0; h2_index < heads[j+1][k].size(); ++h2_index) {
                        int h2 = heads[j+1][k][h2_index];
                        if (deps[h2] == h1) {
                            (*chart)[i][k].push_back(DepSplit(h1, h2, j));
                            if (!has_head[i][k][h1]) {
                                heads[i][k].push_back(h1);
                                has_head[i][k][h1] = 1;
                            }
                        }
                        if (deps[h1] == h2) {
                            (*chart)[i][k].push_back(DepSplit(h2, h1, j));
                            if (!has_head[i][k][h2]) {
                                heads[i][k].push_back(h2);
                                has_head[i][k][h2] = 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

struct BackPointer {
    BackPointer() : terminal(true) {}
    bool terminal;
    bool single;
    bool promotion;
    Item item1;
    Item item2;
    AppliedRule rule;
};

const int kLayers = 3;

class Chart {
  public:
    Chart (int n, int N, const vector<string> *words) : n_(n), N_(N), words_(words) {
        int total_size = n * n * n * kLayers;
        bps_.resize(total_size);
        item_score_.resize(total_size);

        span_nts.resize(n);
        for (int i = 0; i < n; ++i) {
            span_nts[i].resize(n);
            for (int j = 0; j < n; ++j) {
                span_nts[i][j].resize(n);
                for (int h = 0; h < n; ++h) {
                    span_nts[i][j][h].resize(kLayers);
                }
            }
        }
    }

    void span_init(const Item &item) {
        span_nts[item.i][item.k][item.h][item.layer].push_back(item.nt);
    }

    void init(const Item &item) {
        span_init(item);
        int index = index_item(item);
        BackPointer &bp = bps_[index][item.nt];
        bp.terminal = true;
        item_score_[index][item.nt] = 0.0;
    }

    void promote(const Item &item, const Item &item1) {
        assert(has_item(item1));

        int index = index_item(item);
        int index1 = index_item(item1);
        //assert(has_item_[index1][item1.nt]);
        double new_score = item_score_[index1][item1.nt];
        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() or new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.promotion = true;
            bp.single = false;
            bp.terminal = false;

            // iter->second = new_score;

            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else{
                iter->second = new_score;
            }

            // item_score_[index][item.nt] = new_score;
        }

    }

    void update(const Item &item, double score, const Item &item1, const AppliedRule &rule,
                double score1) {

        assert(has_item(item1));

        int index = index_item(item);
        double new_score = score + score1;
        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() or new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.single = true;
            bp.terminal = false;
            bp.promotion = false;
            bp.rule = rule;
            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else{
                iter->second = new_score;
            }
        }
    }

    bool has_item(const Item &item) const {
        int index = index_item(item);
        return item_score_[index].find(item.nt) != item_score_[index].end();
    }

    double score(const Item &item) {
        int index = index_item(item);
        return item_score_[index][item.nt];
    }

    void update(const Item &item, double score, const Item &item1, const Item &item2,
                const AppliedRule &rule, double score1, double score2) {

        int index = index_item(item);
        double new_score = score + score1 + score2;

        assert(has_item(item1) && has_item(item2));

        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() or new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.item2 = item2;
            bp.single = false;
            bp.terminal = false;
            bp.promotion = false;
            bp.rule = rule;

            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else{
                iter->second = new_score;
            }
        }
    }

    bool to_tree(const Item &item, const Grammar &grammar,
                 vector<AppliedRule> *best_rules, bool output,
                 stringstream &out) {
        int index = index_item(item);
        BackPointer &bp = bps_[index][item.nt];
        bool success = true;
        if (bp.terminal) {
            assert(item.i == item.k);
            if (item.i != item.k) return false;
            NonTerminal nt = grammar.non_terminals_[item.nt];
            if (output)
                out << " (" << grammar.non_terminals_[item.nt].main
                     << " " << (*words_)[item.i] << ") ";

        } else if (bp.single) {
            best_rules->push_back(bp.rule);
            NonTerminal nt = grammar.non_terminals_[item.nt];
            if (output) {
                for (int i = 0; i < nt.split.size(); ++i) {
                    out << " (" << nt.split[i] << " ";
                }
            }
            success &= to_tree(bp.item1, grammar, best_rules, output, out);
            if (output) {
                for (int i = 0; i < nt.split.size(); ++i) {
                    out << ") ";
                }
            }
        } else if (bp.promotion) {
            success &= to_tree(bp.item1, grammar, best_rules, output, out);
        } else {
            best_rules->push_back(bp.rule);
            NonTerminal nt = grammar.non_terminals_[item.nt];

            if (output && !nt.removable) {
                for (int i = 0; i < nt.split.size(); ++i) {
                    out << " (" << nt.split[i] << " ";
                }
            }

            success &= to_tree(bp.item1, grammar, best_rules, output, out);
            if (output)
                out << " ";
            success &= to_tree(bp.item2, grammar, best_rules, output, out);
            if (output && !nt.removable) {
                for (int i = 0; i < nt.split.size(); ++i) {
                    out << ") ";
                }
            }
        }
        return success;

    }

    vector<vector<vector<vector<vector<int> > > > > span_nts;


  private:
    int index_item(const Item &item) const {
        // 3 * item.nt
        return kLayers * n_ * n_ *  item.i +  kLayers * n_ * item.k + kLayers * item.h +  item.layer;
    }

    int n_;
    int N_;
    // vector<map<int, bool> > has_item_;
    vector<map<int, double> > item_score_;
    vector<map<int, BackPointer> > bps_;
    const vector<string> *words_;
};





void complete(int i, int k, int h, const vector<int> &preterms,
              const Grammar &grammar, const FeatureScorer &scorer,  Chart *chart) {
    // Fill in the unary rules.
    for (int index = 0; index < chart->span_nts[i][k][h][0].size(); ++index) {
        int Y = chart->span_nts[i][k][h][0][index];
        Item item(i, k, h, Y, 0);
        Item item_prom(i, k, h, Y, 2);
        assert(chart->has_item(item));
        chart->promote(item_prom, item);

        double item_score = chart->score(item);


        for (int index2 = 0; index2 < grammar.unary_rules_by_first[Y].size(); ++index2) {
            int X = grammar.unary_rules_by_first[Y][index2].nt_X;
            int r = grammar.unary_rules_by_first[Y][index2].rule_num;
            if (grammar.pruning && !grammar.rule_head_tags[r][preterms[h]])
                continue;

            AppliedRule rule(i, k, k, h, h, r);
            Item item2(i, k, h, X, 1);
            chart->update(item2, scorer.score(rule), item, rule, item_score);

            assert(chart->has_item(item2));
            Item item2_prom(i, k, h, X, 2);
            chart->promote(item2_prom, item2);
        }
    }

    for (int index = 0; index < chart->span_nts[i][k][h][1].size(); ++index) {
        int Y = chart->span_nts[i][k][h][1][index];
        Item item(i, k, h, Y, 1);
        double item_score = chart->score(item);

        for (int index2 = 0; index2 < grammar.unary_rules_by_first[Y].size(); ++index2) {
            int X = grammar.unary_rules_by_first[Y][index2].nt_X;
            int r = grammar.unary_rules_by_first[Y][index2].rule_num;
            if (grammar.pruning && !grammar.rule_head_tags[r][preterms[h]])
                continue;

            AppliedRule rule(i, k, k, h, h, r);
            Item item2(i, k, h, X, 2);
            chart->update(item2, scorer.score(rule), item, rule, item_score);
        }
    }
}

double cky(const vector<int> &preterms,
           const vector<string> &words,
           const vector<int> &deps,
           const Grammar &grammar,
           const FeatureScorer &scorer,
           vector<AppliedRule> *best_rules,
           bool output) {
    int n = preterms.size();
    vector<vector<vector<DepSplit> > > span_pruner;
    // vector<vector<vector<vector<vector <RuleSplit> > > > > cell_rules;
    int G = grammar.n_rules;
    int N = grammar.n_nonterms;

    // Run the pruning stages.
    span_pruning(n, deps, &span_pruner);
    // grammar_pruning(preterms, grammar, span_pruner, &cell_rules);

    Chart chart(n, grammar.n_nonterms, &words);

    // Initialize the chart.
    for (int i = 0; i < n; ++i) {
        int Y = preterms[i];
        Item item(i, i, i, Y, 0);
        chart.init(item);
        complete(i, i, i, preterms, grammar, scorer, &chart);
    }

    // Main loop.
    for (int d = 1; d < n; ++d) {
        for (int i = 0; i < n; ++i) {
            int k = i + d;
            if (k >= n) continue;

            for (int span = 0; span < span_pruner[i][k].size(); ++span) {
                int h = span_pruner[i][k][span].head;
                int m = span_pruner[i][k][span].mod;
                int j = span_pruner[i][k][span].split;


                if (h <= j) {
                    vector<bool> have_nt(N, 0);
                    //Item other_item(j+1, k, m, Z, 2);
                    for (int index = 0; index < chart.span_nts[j+1][k][m][2].size(); ++index) {
                        int Z = chart.span_nts[j+1][k][m][2][index];
                        have_nt[Z] = 1;
                    }

                    for (int index = 0; index < chart.span_nts[i][j][h][2].size(); ++index) {
                        int Y = chart.span_nts[i][j][h][2][index];
                        Item item(i, j, h, Y, 2);
                        double item_score = chart.score(item);

                        for (int r_index = 0; r_index < grammar.rules_by_first[Y].size(); ++r_index) {
                            const BinaryRule &rule = grammar.rules_by_first[Y][r_index];
                            int r = rule.rule_num;
                            int X = rule.nt_X;
                            int Z = rule.nt_Z;
                            if (grammar.pruning && !grammar.rule_head_tags[rule.rule_num][preterms[h]])
                                continue;

                            if (have_nt[Z]) {
                                Item other_item(j+1, k, m, Z, 2);
                                double other_item_score = chart.score(other_item);
                                Item new_item(i, k, h, X, 0);
                                AppliedRule rule(i, j, k, h, m, r);
                                double score = scorer.score(rule);
                                chart.update(new_item, score, item, other_item, rule,
                                             item_score, other_item_score);
                                // assert(r < G);
                            }
                        }
                    }
                }
                if (h > j) {
                    vector<bool> have_nt(N, 0);
                    for (int index = 0; index < chart.span_nts[i][j][m][2].size(); ++index) {
                        int Y = chart.span_nts[i][j][m][2][index];
                        have_nt[Y] = 1;
                    }

                    for (int index = 0; index < chart.span_nts[j+1][k][h][2].size(); ++index) {
                        int Z = chart.span_nts[j+1][k][h][2][index];
                        Item item(j+1, k, h, Z, 2);
                        double item_score = chart.score(item);

                        for (int r_index = 0; r_index < grammar.rules_by_second[Z].size(); ++r_index) {
                            const BinaryRule &rule = grammar.rules_by_second[Z][r_index];
                            int r = rule.rule_num;
                            int X = rule.nt_X;
                            int Y = rule.nt_Y;
                            if (grammar.pruning && !grammar.rule_head_tags[rule.rule_num][preterms[h]])
                                continue;

                            if (have_nt[Y]) {
                                Item other_item(i, j, m, Y, 2);
                                double other_item_score = chart.score(other_item);
                                Item new_item(i, k, h, X, 0);
                                AppliedRule rule(i, j, k, h, m, r);
                                double score = scorer.score(rule);

                                chart.update(new_item, score, other_item, item, rule, item_score,
                                             other_item_score);
                                // assert(r < G);
                            }
                        }
                    }
                }
            }
            for (int h = i; h <= k; ++h) {
                complete(i, k, h, preterms, grammar, scorer, &chart);
            }
        }
    }
    int root_word = 0;
    for (int i = 0; i < n; ++i) {
        if (deps[i] == -1) {
            root_word = i;
        }
    }
    Item item(0, n-1, root_word, 0, 2);
    for (int i = 0; i < grammar.roots.size(); ++i) {
        int root = grammar.roots[i];
        Item item1(0, n-1, root_word, root, 2);
        if (chart.has_item(item1)) {
            chart.promote(item, item1);
        }
    }
    stringstream out;
    bool success = chart.to_tree(item, grammar, best_rules, output, out);
    if (success && out) {
        cout << out.str();
    }
    return chart.score(item);
}



// void initialize_grammar_pruning(int n, int N,
//                                 vector<vector<vector<bool> > > *has_item,
//                                 vector<vector<vector<int > > > *cell_nts,
//                                 vector<vector<vector<vector<vector <RuleSplit> > > > >  * cell_rules) {

//     has_item->resize(n);
//     cell_nts->resize(n);
//     cell_rules->resize(n);
//     for (int i = 0; i < n; ++i) {
//         (*has_item)[i].resize(n);
//         (*cell_nts)[i].resize(n);
//         (*cell_rules)[i].resize(n);
//         for (int j = 0; j < n; ++j) {
//             (*has_item)[i][j].resize(N);
//             (*cell_rules)[i][j].resize(N);
//             for (int r = 0; r < N; ++r) {
//                 (*cell_rules)[i][j][r].resize(2);
//             }
//         }
//     }

// }

// void grammar_pruning(const vector<int> &preterms,
//                      const Grammar &grammar,
//                      const vector<vector<vector<DepSplit> > > &span_pruner,
//                      vector<vector<vector<vector<vector <RuleSplit> > > > >  * cell_rules) {
//     int N = grammar.n_nonterms;
//     int n = preterms.size();
//     int G = grammar.n_rules;

//     // Indicator of cell in span.
//     vector<vector<vector<bool> > > has_item;
//     vector<vector<vector<int > > > cell_nts;
//     initialize_grammar_pruning(n, N, &has_item, &cell_nts, cell_rules);

//     for (int i = 0; i < n; ++i) {
//         int Y = preterms[i];
//         has_item[i][i][Y] = 1;
//         cell_nts[i][i].push_back(preterms[i]);
//         for (int r = 0; r < grammar.unary_rules_by_first[Y].size(); ++r) {
//             int X = grammar.unary_rules_by_first[Y][r].nt_X;
//             if (!has_item[i][i][X]) {
//                 has_item[i][i][X] = 1;
//                 cell_nts[i][i].push_back(X);
//             }
//             for (int r2 = 0; r2 < grammar.unary_rules_by_first[X].size(); ++r2) {
//                 int X_up = grammar.unary_rules_by_first[X][r2].nt_X;
//                 if (not has_item[i][i][X_up]) {
//                     has_item[i][i][X_up] = 1;
//                     cell_nts[i][i].push_back(X_up);
//                 }
//             }
//         }
//     }

//     for (int d = 1; d < n; ++d) {
//         for (int i =0; i < n; ++i) {
//             int k = i + d;
//             if (k >= n) continue;

//             for (int span = 0; span < span_pruner[i][k].size(); ++span) {
//                 int h = span_pruner[i][k][span].head;
//                 int j = span_pruner[i][k][span].split;
//                 int req_dir_ = (h <= j) ? 0 : 1;
//                 for (int Y_index = 0; Y_index < cell_nts[i][j].size(); ++Y_index) {
//                     int Y = cell_nts[i][j][Y_index];
//                     for (int r_index = 0; r_index < grammar.rules_by_first[Y].size(); ++r_index) {
//                         const BinaryRule &rule = grammar.rules_by_first[Y][r_index];
//                         int r = rule.rule_num;
//                         int X = rule.nt_X;
//                         int Z = rule.nt_Z;
//                         int dir_ = rule.dir;
//                         if (dir_ != req_dir_) continue;

//                         if (!has_item[i][k][X]) {
//                             has_item[i][k][X] = 1;
//                             cell_nts[i][k].push_back(X);
//                             for (int r2 = 0; r2 < grammar.unary_rules_by_first[X].size(); ++r2) {
//                                 int X_up = grammar.unary_rules_by_first[X][r2].nt_X;
//                                 if (!has_item[i][k][X_up]) {
//                                     has_item[i][k][X_up] = 1;
//                                     cell_nts[i][k].push_back(X_up);
//                                 }

//                                 for (int r3 = 0; r3 < grammar.unary_rules_by_first[X_up].size(); ++r3) {
//                                     int X_up2 = grammar.unary_rules_by_first[X_up][r3].nt_X;
//                                     if (!has_item[i][k][X_up2]) {
//                                         has_item[i][k][X_up2] = 1;
//                                         cell_nts[i][k].push_back(X_up2);
//                                     }
//                                 }
//                             }
//                         }

//                         assert(r < G);
//                         if (dir_ == 0) {
//                             (*cell_rules)[i][k][Y][0].push_back(RuleSplit(r, X, Z));
//                         }  else {
//                             (*cell_rules)[i][k][Z][1].push_back(RuleSplit(r, X, Y));
//                         }
//                     }
//                 }
//             }
//         }
//     }
// }
