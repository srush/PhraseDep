#include <vector>
#include <iostream>
#include <assert.h>
#include <sstream>
#include <bitset>
#include "grammar.hpp"
#include "features.hpp"
#include "dp.hpp"

using namespace std;

void find_spans(int h, const vector<int> &deps,
                vector<int> *left,
                vector<int> *right) {
    (*left)[h] = h;
    (*right)[h] = h;
    for (int i = 0; i < deps.size(); ++i) {
        if (deps[i] == h) {
            find_spans(i, deps, left, right);
            if (i < h) {
                (*left)[h] = min((*left)[h], (*left)[i]);
            } else {
                (*right)[h] = max((*right)[h], (*right)[i]);
            }
        }
    }
}

const int kLayers = 3;

class Chart {
  public:
    Chart (int n, int N, const vector<string> *words)
            : n_(n), N_(N), words_(words) {
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

        double new_score = item_score_[index1][item1.nt];
        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() || new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.promotion = true;
            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else {
                iter->second = new_score;
            }
        }

    }

    void update(const Item &item, double score,
                const Item &item1, const Item &item2,
                const AppliedRule &rule, double score1, double score2) {

        int index = index_item(item);
        double new_score = score + score1 + score2;

        assert(has_item(item1) && has_item(item2));

        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() || new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.item2 = item2;
            bp.rule = rule;

            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else {
                iter->second = new_score;
            }
        }
    }

    void update(const Item &item, double score,
                const Item &item1, const AppliedRule &rule,
                double score1) {

        assert(has_item(item1));

        int index = index_item(item);
        double new_score = score + score1;
        map<int, double>::iterator iter = item_score_[index].find(item.nt);
        if (iter == item_score_[index].end() || new_score > iter->second) {
            BackPointer &bp = bps_[index][item.nt];
            bp.item1 = item1;
            bp.single = true;
            bp.rule = rule;
            if (iter == item_score_[index].end()) {
                item_score_[index][item.nt] = new_score;
                span_init(item);
            } else {
                iter->second = new_score;
            }
        }
    }

    inline bool has_item(const Item &item) const {
        int index = index_item(item);
        return item_score_[index].find(item.nt) != item_score_[index].end();
    }

    inline double score(const Item &item) {
        int index = index_item(item);
        return item_score_[index][item.nt];
    }


    bool to_tree(const Item &item, const Grammar &grammar,
                 vector<AppliedRule> *best_rules, bool output,
                 stringstream &out) {
        int index = index_item(item);
        BackPointer &bp = bps_[index][item.nt];
        bool success = true;
        if (bp.terminal) {
            assert(item.i == item.k);
            if (item.i != item.k) {
                cerr << "FAIL" << endl;
                return false;
            }
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
            if (!item.used || !bp.item1.used) {
                cerr << "FAIL" << endl;
                return false;
            }

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
        return kLayers * n_ * n_ *  item.i +  kLayers * n_ * item.k +
                kLayers * item.h +  item.layer;
    }

    int n_;
    int N_;
    vector<map<int, double> > item_score_;
    vector<map<int, BackPointer> > bps_;
    const vector<string> *words_;
};

void complete(int i, int k, int h, const vector<int> &preterms,
              const Grammar &grammar, const Scorer &scorer,
              Chart *chart) {
    // Fill in the unary rules.
    for (int Y : chart->span_nts[i][k][h][0]) {
        Item item(i, k, h, Y, 0);
        Item item_prom(i, k, h, Y, 2);
        assert(chart->has_item(item));
        chart->promote(item_prom, item);

        double item_score = chart->score(item);

        for (const UnaryRule &u_rule : grammar.unary_rules_by_first[Y]) {
            if (grammar.pruning &&
                !grammar.rule_head_tags[u_rule.rule_num][preterms[h]])
                continue;

            AppliedRule rule(i, k, k, h, h, u_rule.rule_num);
            Item item2(i, k, h, u_rule.nt_X, 1);
            chart->update(item2, scorer.score(rule), item, rule, item_score);

            assert(chart->has_item(item2));
            Item item2_prom(i, k, h, u_rule.nt_X, 2);
            chart->promote(item2_prom, item2);
        }
    }

    for (int Y : chart->span_nts[i][k][h][1]) {
        Item item(i, k, h, Y, 1);
        double item_score = chart->score(item);

        for (const UnaryRule &u_rule : grammar.unary_rules_by_first[Y]) {
            if (grammar.pruning &&
                !grammar.rule_head_tags[u_rule.rule_num][preterms[h]])
                continue;

            AppliedRule rule(i, k, k, h, h, u_rule.rule_num);
            Item item2(i, k, h, u_rule.nt_X, 2);
            chart->update(item2, scorer.score(rule), item, rule, item_score);
        }
    }
}

double cky(const vector<int> &preterms,
           const vector<string> &words,
           const vector<int> &deps,
           const Grammar &grammar,
           const Scorer &scorer,
           vector<AppliedRule> *best_rules,
           bool output,
           bool *success) {
    int n = preterms.size();
    int G = grammar.n_rules;
    int N = grammar.n_nonterms;

    // Run the pruning stages.
    // span_pruning(n, deps, &span_pruner);
    vector<int> left(n, -2), right(n, -2);
    for (int i = 0; i < deps.size(); ++i) {
        if (deps[i] == -1) {
            find_spans(i, deps, &left, &right);
        }
    }

    Chart chart(n, grammar.n_nonterms, &words);

    // Initialize the chart.
    for (int i = 0; i < n; ++i) {
        int Y = preterms[i];
        Item item(i, i, i, Y, 0);
        chart.init(item);
        complete(i, i, i, preterms, grammar, scorer, &chart);
    }

    // Main loop.
    //vector<bool> have_nt(N, 0);
    bitset<50000> have_nt;
    for (int d = 1; d < n; ++d) {
        for (int i = 0; i < n; ++i) {
            int k = i + d;
            if (k >= n) continue;
            for (int m = i; m <= k; ++m) {
                int h = deps[m];
                if (h < i || h > k) continue;
                if (h < m && right[m] == k) {
                    int j = left[m] - 1;
                    have_nt.reset();
                    for (int Z : chart.span_nts[j+1][k][m][2]) {
                        have_nt[Z] = 1;
                    }

                    for (int Y : chart.span_nts[i][j][h][2]) {
                        Item item(i, j, h, Y, 2);
                        double item_score = chart.score(item);

                        for (const BinaryRule &b_rule : grammar.rules_by_first[Y]) {
                            int Z = b_rule.nt_Z;
                            if (grammar.pruning &&
                                !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                                continue;

                            // if (grammar.label_pruning &&
                            //     !grammar.rule_head_tags[rule.rule_num][preterms[h]])
                            //     continue;

                            if (have_nt[Z]) {
                                Item other_item(j+1, k, m, Z, 2);
                                double other_item_score = chart.score(other_item);
                                Item new_item(i, k, h, b_rule.nt_X, 0);
                                AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                                double score = scorer.score(rule);
                                chart.update(new_item, score, item, other_item, rule,
                                             item_score, other_item_score);
                            }
                        }
                    }
                }
                if (m < h && left[m] == i) {
                    int j = right[m];

                    have_nt.reset();
                    for (int Y : chart.span_nts[i][j][m][2]) {
                        have_nt[Y] = 1;
                    }
                    for (int Z : chart.span_nts[j+1][k][h][2]) {
                        Item item(j+1, k, h, Z, 2);
                        double item_score = chart.score(item);
                        for (const BinaryRule &b_rule : grammar.rules_by_second[Z]) {

                            int Y = b_rule.nt_Y;

                            if (grammar.pruning &&
                                !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                                continue;

                            if (have_nt[Y]) {
                                Item other_item(i, j, m, Y, 2);
                                double other_item_score =
                                        chart.score(other_item);
                                Item new_item(i, k, h, b_rule.nt_X, 0);
                                AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                                double score = scorer.score(rule);

                                chart.update(new_item, score, other_item,
                                             item, rule, item_score,
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

    int root = grammar.roots[0];
    Item item(0, n-1, root_word, root, 2);

    stringstream out;
    *success = chart.to_tree(item, grammar, best_rules, output, out);
    if ((*success) && out) {
        cout << out.str();
    }
    return chart.score(item);
}



double cky_full(const vector<int> &preterms,
           const vector<string> &words,
           const Grammar &grammar,
           const Scorer &scorer,
           vector<AppliedRule> *best_rules,
           bool output,
           bool *success) {
    int n = preterms.size();
    int G = grammar.n_rules;
    int N = grammar.n_nonterms;

    Chart chart(n, grammar.n_nonterms, &words);

    // Initialize the chart.
    for (int i = 0; i < n; ++i) {
        int Y = preterms[i];
        Item item(i, i, i, Y, 0);
        chart.init(item);
        complete(i, i, i, preterms, grammar, scorer, &chart);
    }

    // Main loop.
    //vector<bool> have_nt(N, 0);
    bitset<50000> have_nt;
    for (int d = 1; d < n; ++d) {
        for (int i = 0; i < n; ++i) {
            int k = i + d;
            if (k >= n) continue;
            for (int m = i; m <= k; ++m) {
                for (int h = i; h <= k; ++h) {
                    if (h == m) continue;

                    if (h < m) {
                        for (int j = h; j < m; ++j) {
                            have_nt.reset();
                            for (int Z : chart.span_nts[j+1][k][m][2]) {
                                have_nt[Z] = 1;
                            }

                            for (int Y : chart.span_nts[i][j][h][2]) {
                                Item item(i, j, h, Y, 2);
                                double item_score = chart.score(item);

                                for (const BinaryRule &b_rule : grammar.rules_by_first[Y]) {
                                    int Z = b_rule.nt_Z;
                                    if (grammar.pruning &&
                                        !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                                        continue;

                                    // if (grammar.label_pruning &&
                                    //     !grammar.rule_head_tags[rule.rule_num][preterms[h]])
                                    //     continue;

                                    if (have_nt[Z]) {
                                        Item other_item(j+1, k, m, Z, 2);
                                        double other_item_score = chart.score(other_item);
                                        Item new_item(i, k, h, b_rule.nt_X, 0);
                                        AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                                        double score = scorer.score(rule);
                                        chart.update(new_item, score, item, other_item, rule,
                                                     item_score, other_item_score);
                                    }
                                }
                            }
                        }
                    }
                    if (m < h) {
                        for (int j = m; j < h; ++j) {

                            have_nt.reset();
                            for (int Y : chart.span_nts[i][j][m][2]) {
                                have_nt[Y] = 1;
                            }
                            for (int Z : chart.span_nts[j+1][k][h][2]) {
                                Item item(j+1, k, h, Z, 2);
                                double item_score = chart.score(item);
                                for (const BinaryRule &b_rule : grammar.rules_by_second[Z]) {

                                    int Y = b_rule.nt_Y;

                                    if (grammar.pruning &&
                                    !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                                        continue;

                                    if (have_nt[Y]) {
                                        Item other_item(i, j, m, Y, 2);
                                        double other_item_score =
                                                chart.score(other_item);
                                        Item new_item(i, k, h, b_rule.nt_X, 0);
                                        AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                                        double score = scorer.score(rule);

                                        chart.update(new_item, score, other_item,
                                                     item, rule, item_score,
                                                     other_item_score);
                                        // assert(r < G);
                                    }
                                }
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
    int root_word = 1;

    int root = grammar.roots[0];
    Item item(0, n-1, root_word, root, 2);

    stringstream out;
    *success = chart.to_tree(item, grammar, best_rules, output, out);
    if ((*success) && out) {
        cout << out.str();
    }
    return chart.score(item);
}
