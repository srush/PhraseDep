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
                vector<int> *right,
                vector<vector<int> > *children,
                vector<int> *order) {
    (*left)[h] = h;
    (*right)[h] = h;

    for (int i = 0; i < deps.size(); ++i) {
        if (deps[i] == h) {
            find_spans(i, deps, left, right, children, order);
            (*children)[h].push_back(i);
            if (i < h) {
                (*left)[h] = min((*left)[h], (*left)[i]);
            } else {
                (*right)[h] = max((*right)[h], (*right)[i]);
            }
        }
    }
    order->push_back(h);
}

const int kLayers = 3;

class Chart {
  public:
    Chart (int n, int N, const vector<string> *words) {
        n_ = n;
        N_ = N;
        words_ = words;
    }

    void span_init(const Item &item) {
        // span_nts_[item.i][item.k][item.h][item.layer].push_back(item.nt);
        span_nts_[index_item(item)].push_back(item.nt);
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

    inline bool has_item(const Item &item)  {
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

    const vector<int> & span_nts(const Item &item) {
        return span_nts_[index_item(item)];
    }

    // vector<vector<vector<vector<vector<int> > > > > span_nts;
    map<int, vector<int> > span_nts_;

    // map<vector>
  private:
    int index_item(const Item &item) const {
        // 3 * item.nt
        return kLayers * n_ * n_ *  item.i +  kLayers * n_ * item.k +
                kLayers * item.h +  item.layer;
    }

    int n_;
    int N_;
    map<int, map<int, double> > item_score_;
    map<int, map<int, BackPointer> > bps_;
    const vector<string> *words_;
};

bitset<50000> have_nt;
int add_right(int i, int j, int k, int h, int m,
              const vector<int> &preterms,
              const Grammar &grammar, const Scorer &scorer,
              Chart *chart) {
    have_nt.reset();

    for (int Z : chart->span_nts(Item(j+1, k, m, 0, 2))) {
        have_nt[Z] = 1;
    }
    int total_scored = 0;
    for (int Y : chart->span_nts(Item(i, j, h, 0, 2))) {
        Item item(i, j, h, Y, 2);
        double item_score = chart->score(item);

        for (const BinaryRule &b_rule : grammar.rules_by_first[Y]) {
            int Z = b_rule.nt_Z;
            if (grammar.pruning &&
                !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                continue;

            if (have_nt[Z]) {
                Item other_item(j+1, k, m, Z, 2);
                double other_item_score = chart->score(other_item);
                Item new_item(i, k, h, b_rule.nt_X, 0);
                AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                total_scored++;
                double score = scorer.score(rule);
                chart->update(new_item, score, item, other_item, rule,
                             item_score, other_item_score);
            }
        }
    }
    return total_scored;
}


int add_left(int i, int j, int k, int h, int m,
              const vector<int> &preterms,
              const Grammar &grammar, const Scorer &scorer,
              Chart *chart) {
    int total_scored = 0;
    have_nt.reset();
    for (int Y : chart->span_nts(Item(i, j, m, 0, 2))) {
        have_nt[Y] = 1;
    }
    for (int Z : chart->span_nts(Item(j+1, k, h, 0, 2))) {
        Item item(j+1, k, h, Z, 2);
        double item_score = chart->score(item);
        for (const BinaryRule &b_rule : grammar.rules_by_second[Z]) {

            int Y = b_rule.nt_Y;

            if (grammar.pruning &&
                !grammar.rule_head_tags[b_rule.rule_num][preterms[h]])
                continue;

            if (have_nt[Y]) {
                Item other_item(i, j, m, Y, 2);
                double other_item_score =
                        chart->score(other_item);
                Item new_item(i, k, h, b_rule.nt_X, 0);
                AppliedRule rule(i, j, k, h, m, b_rule.rule_num);
                double score = scorer.score(rule);
                total_scored++;
                chart->update(new_item, score, other_item,
                              item, rule, item_score,
                              other_item_score);
                // assert(r < G);
            }
        }
    }
    return total_scored;
}

bool complete(int i, int k, int h, const vector<int> &preterms,
              const Grammar &grammar, const Scorer &scorer,
              Chart *chart) {
    bool any = false;
    // Fill in the unary rules.
    for (int Y : chart->span_nts(Item(i, k, h, 0, 0))) {
        any = true;
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

    for (int Y : chart->span_nts(Item(i, k, h, 0, 1))) {
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
    return any;
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
    vector<vector<int> > children(n);
    vector<int> order;
    for (int i = 0; i < deps.size(); ++i) {
        if (deps[i] == -1) {
            find_spans(i, deps, &left, &right, &children, &order);
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
                    for (int Z : chart.span_nts(Item(j+1, k, m, 0, 2))) {
                        have_nt[Z] = 1;
                    }

                    for (int Y : chart.span_nts(Item(i, j, h, 0, 2))) {
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
                    for (int Y : chart.span_nts(Item(i, j, m, 0,2))) {
                        have_nt[Y] = 1;
                    }
                    for (int Z : chart.span_nts(Item(j+1, k, h, 0, 2))) {
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

double cky2(const vector<int> &preterms,
            const vector<string> &words,
            const vector<int> &deps,
            const Grammar &grammar,
            const Scorer &scorer,
            vector<AppliedRule> *best_rules,
            bool output,
            bool *success) {
    clock_t start = clock();

    int n = preterms.size();
    int G = grammar.n_rules;
    int N = grammar.n_nonterms;

    // Run the pruning stages.
    // span_pruning(n, deps, &span_pruner);
    vector<int> left(n, -2), right(n, -2);
    vector<vector<int> > children(n);
    vector<int> ordered;
    int root_word;
    for (int i = 0; i < deps.size(); ++i) {
        if (deps[i] == -1) {
            root_word = i;
            find_spans(i, deps, &left, &right, &children, &ordered);
        }
    }



    Chart chart(n, grammar.n_nonterms, &words);
    // chart.reset1(n, grammar.n_nonterms, &words);



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
    int total_combines = 0;
    int total_scored = 0;
    int max_size = 0;
    for (int h : ordered) {
        vector<int> &deps = children[h];
        vector<int> left_children;
        vector<int> right_children;

        for (int dep : deps) {
            if (dep < h) left_children.insert(left_children.begin(), dep);
            else right_children.push_back(dep);
        }

        // Unsigned weirdness.
        int L = left_children.size();
        int R = right_children.size();
        int head_tag = preterms[h];
        int total_taken = 0;
        for (int l = -1; l < L; ++l) {
            for (int r = -1; r < R; ++r) {

                int left_cur = (l != -1) ? left[left_children[l]] : h;
                int right_cur = (r != -1) ? right[right_children[r]] : h;
                bool any = complete(left_cur, right_cur,
                                    h, preterms, grammar, scorer, &chart);
                if (!any) continue;

                int i, j, m, k;

                // Add a right child.
                bool try_right = r < R - 1;
                bool try_left = l < L - 1;

                if (try_right && try_left && grammar.dir_pruning) {
                    // Both possible, but one may be better.
                    int right_tag = preterms[right_children[r+1]];
                    int left_tag = preterms[left_children[l+1]];
                    int left_pre = 0, right_pre = 0;
                    if (l == -1) left_pre = 1;
                    else if (preterms[left_children[l]] == grammar.nonterm_map.at(",")) left_pre = 2;
                    if (r == -1) right_pre = 1;
                    else if (preterms[right_children[r]] == grammar.nonterm_map.at(",")) right_pre = 2;
                    DirPrune prune(head_tag, left_tag, right_tag, left_pre, right_pre);
                    grammar.dir_pick(prune, &try_left, &try_right);
                }

                if (try_right) {
                    i = left_cur;
                    m = right_children[r+1];
                    j = left[m] - 1;
                    k = right[m];
                    total_scored += add_right(i, j, k, h, m, preterms, grammar, scorer, &chart);
                    total_combines++;
                    total_taken++;
                }

                // Add a left child.
                if (try_left) {
                    k = right_cur;
                    m = left_children[l+1];
                    i = left[m];
                    j = right[m];
                    total_scored += add_left(i, j, k, h, m, preterms, grammar, scorer, &chart);
                    total_combines++;
                    total_taken++;
                }
            }
        }
        // if (max_size < total_taken - L - R){
        //     max_size = total_taken - L - R;
        //     if (max_size > 10) {
        //         cerr << grammar.rev_nonterm_map.at(preterms[h]) << " " <<
        //                 L << " " << R << " " << total_taken << endl;
        //         for (int i : left_children) {
        //             cerr << grammar.rev_nonterm_map.at(preterms[i]) << " " << endl;
        //         }
        //         cerr << "MID" << endl;
        //         for (int i : right_children) {
        //             cerr << grammar.rev_nonterm_map.at(preterms[i]) << " " << endl;
        //         }
        //         int lfirst = true;
        //         for (int i : left_children) {
        //             int rfirst = true;
        //             for (int j : right_children) {
        //                 DirPrune prune(head_tag, preterms[i], preterms[j], lfirst, rfirst);
        //                 bool a, b;
        //                 double score = grammar.dir_pick(prune, &a, &b);
        //                 cerr << grammar.rev_nonterm_map.at(preterms[i])
        //                 << " " << grammar.rev_nonterm_map.at(preterms[j])
        //                      << " " << lfirst << " " << rfirst << " " << score <<
        //                         " " << a << " " << b << endl;
        //                 rfirst = false;
        //             }
        //             lfirst = false;
        //         }

    }


    int root = grammar.roots[0];
    Item item(0, n-1, root_word, root, 2);

    stringstream out;
    *success = chart.to_tree(item, grammar, best_rules, output, out);
    if ((*success) && out) {
        cout << out.str();
    }
    clock_t end = clock();
    // cerr << "STATS: " <<  n << " " << total_combines << " " << total_scored << " " << max_size << " " << (end - start) << " " << CLOCKS_PER_SEC << endl;
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
                            for (int Z : chart.span_nts(Item(j+1, k, m, 0, 2))) {
                                have_nt[Z] = 1;
                            }

                            for (int Y : chart.span_nts(Item(i, j, h, 0, 2))) {
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
                            for (int Y : chart.span_nts(Item(i, j, m, 0, 2))) {
                                have_nt[Y] = 1;
                            }
                            for (int Z : chart.span_nts(Item(j+1, k, h, 0, 2))) {
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
    int root = grammar.roots[0];
    int root_word = 1;

    Item item(0, n-1, root_word, root, 2);

    stringstream out;
    // *success = chart.to_tree(item, grammar, best_rules, output, out);
    if ((*success) && out) {
        cout << out.str();
    }
    return 0.0;//chart.score(item);
}
