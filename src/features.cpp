#include "features.hpp"
#include <cmath>
#include <iostream>
#include <map>

double FeatureScorer::score(const AppliedRule &rule) const {
    double score = 0.0;
    vector<long> features;
    if (dont_hash_features_) {
        feature_gen_.generate(*sentence_, rule, &features, NULL);
        for (long i = 0; i < features.size(); ++i) {
            score += perceptron_.weights[hashed_feature(features[i])];
        }
    } else {
        score = feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights);
    }

    if (!is_cost_augmented_) {
        return score;
        // feature_gen_.generate(*sentence_, rule,
        //                              &features, &perceptron_.weights);
    } else {
        double cost = 1;
        for (int j = 0; j < sentence_->gold_rules.size(); ++j) {
            if (rule.same(sentence_->gold_rules[j])) {
                cost = 0;
                break;
            }
        }
        return score + cost;

    }
}

void FeatureScorer::update(const vector<AppliedRule> &good,
                           const vector<AppliedRule> &bad) {
        map<long, int> count_map;
        for (int i = 0; i < good.size(); ++i) {
            vector<long> good_features;
            feature_gen_.generate(*sentence_, good[i], &good_features, NULL);
            for (int j = 0; j < good_features.size(); ++j) {

                if (dont_hash_features_) {
                    add_feature(good_features[j]);
                }
                long index = hashed_feature(good_features[j]);
                double orignal_value = 0.0;
                if (count_map.find(index) != count_map.end()) {
                    orignal_value = count_map[index];
                }
                count_map[index] = orignal_value + 1;
            }
        }
        for (int i = 0; i < bad.size(); ++i) {
            vector<long> bad_features;
            feature_gen_.generate(*sentence_, bad[i], &bad_features, NULL);
            for (int j = 0; j < bad_features.size(); ++j) {
                if (dont_hash_features_) {
                    add_feature(bad_features[j]);
                }
                long index = hashed_feature(bad_features[j]);
                double orignal_value = 0.0;
                if (count_map.find(index) != count_map.end()) {
                    orignal_value = count_map[index];
                }
                count_map[index] = orignal_value - 1;
            }
        }

        for (auto& iter : count_map) {
            // cerr << "update " << (long)iter.first << " with " << (int)iter.second << endl;
            perceptron_.update((long)iter.first,
                               (int)iter.second);
        }
}


inline long get_word_int(const Grammar* grammar,
                         const Sentence &sentence,
                         int index){
    if (index >= 0 && index < sentence.int_words.size()) {
        return sentence.int_words[index];
    } else if (index == -1) {
        return 0;
    } else {
        return 1;
    }
}

inline long get_last_chinese_char(const Grammar* grammar,
                         const Sentence &sentence,
                         int index){
    if (index >= 0 && index < sentence.int_chinese_last_char.size()) {
        return sentence.int_chinese_last_char[index];
    } else if (index == -1) {
        return 0;
    } else {
        return 1;
    }
}

inline long get_tag_int(const Grammar* grammar,
                        const Sentence &sentence,
                        int index){
    if (index >= 0 && index < sentence.int_tags.size()) {
        return sentence.int_tags[index];
    } else if (index == -1) {
        return 0;
    } else {
        return 1;
    }
}



inline int span_length(int span_length) {
    int bin_span_length = 0;
    if (span_length <=5) {
        bin_span_length = span_length;
    } else if (span_length <=10) {
        bin_span_length = 6;
    } else if (span_length <= 20) {
        bin_span_length = 7;
    } else {
        bin_span_length = 8;
    }
    return bin_span_length;
}

FeatureGenBackoff::FeatureGenBackoff(const Grammar *grammar, bool delex, bool simple, bool chinese)
        : grammar_(grammar), simple_(simple), chinese_(chinese) {
    int n_tags = grammar->tag_index.size();
    int n_nonterms = grammar->n_nonterms;
    int n_rules = grammar->n_rules;
    int n_labels = grammar->n_deplabel;

    for (int i = 0; i < 8; ++i) {
        add_backed_off_template(1);
    }
    add_backed_off_template(n_tags);
    add_backed_off_template(n_tags);
    add_backed_off_template(n_nonterms);
    add_backed_off_template(n_nonterms);

    // ad hoc features.
    add_template(n_nonterms, n_nonterms);
    add_template(n_nonterms, n_nonterms);
    add_template(2, n_tags);
    add_template(2, n_nonterms);
    add_template(2, n_rules);

    add_template(10, n_nonterms);
    add_template(10, n_rules);

    if (!simple_) {
        add_template(10, 10, n_nonterms);
        add_template(10, 10, n_rules);


        add_template(n_nonterms, n_labels);
        add_template(n_nonterms, n_labels);
        add_template(n_nonterms, n_labels);
        add_template(n_rules, n_labels);
        add_template(n_nonterms, n_nonterms, n_labels);
        add_template(n_nonterms, n_nonterms, n_labels);
    }
        add_template(grammar_->tag_index.size(), grammar_->n_rules, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_nonterms, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_rules, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_nonterms, 1);
        add_template(grammar_->tag_index.size(), grammar_->tag_index.size(), grammar_->n_nonterms);
        add_template(grammar_->tag_index.size(), grammar_->tag_index.size(), grammar_->n_rules);
        add_template(grammar_->n_nonterms, 1, 1);
        add_template(grammar_->n_nonterms, 1, 1);
    
}

void FeatureGenBackoff::add_template(int a, int b, int c/*1*/) {
    features_.push_back(Triple(a, b, c));
}

void FeatureGenBackoff::add_backed_off_template(int plus) {
        add_template(grammar_->n_words, grammar_->n_rules, plus);
        add_template(grammar_->n_words, grammar_->n_nonterms, plus);
    
    if (!simple_) {
        add_template(grammar_->tag_index.size(), grammar_->n_rules, plus);
        add_template(grammar_->tag_index.size(), grammar_->n_nonterms, plus);
    }
    if (chinese_) {
        // add_template(grammar_->n_last_chinese_char, grammar_->n_rules, plus);
        // add_template(grammar_->n_last_chinese_char, grammar_->n_nonterms, plus);
    }
}

void FeatureGenBackoff::backed_off_features(const Sentence &sentence,
                                            const AppliedRule &rule,
                                            int index,
                                            int extra,
                                            FeatureState *state) const {
    int word = get_word_int(grammar_, sentence, index);
    int chinese_last_char = get_last_chinese_char(grammar_, sentence, index);

    int rule_ = rule.rule;
    int X = grammar_->head_symbol[rule.rule];

        state->inc(word, rule_, extra);
        state->inc(word, X, extra);
    
    if (!simple_) {
        int tag = get_tag_int(grammar_, sentence, index);
        state->inc(tag, rule_, extra);
        state->inc(tag, X, extra);
    }
    if (chinese_){
        // state->inc(chinese_last_char, rule_, extra);
        // state->inc(chinese_last_char, X, extra);
    }
}


double FeatureGenBackoff::generate(const Sentence &sentence,
                                   const AppliedRule &rule,
                                   vector<long> *base,
                                   const vector<double> *weights) const {

    FeatureState state(base, weights);
    state.features = &features_;

    int X = grammar_->head_symbol[rule.rule];
    int Y = grammar_->left_symbol[rule.rule];
    int Z = grammar_->right_symbol[rule.rule];
    int m_tag = sentence.int_tags[rule.m];
    int h_tag = sentence.int_tags[rule.h];
    bool is_unary = grammar_->is_unary[rule.rule];
    long m_deplabel = sentence.int_deplabels[rule.m];

    // 8 backed off position rules.
    vector<int> rules = {rule.i, rule.k, rule.j, rule.j + 1,
                         rule.i - 1, rule.k + 1, rule.h, rule.m};
    for (int i = 0; i < rules.size(); ++i) {
        backed_off_features(sentence, rule, rules[i], 1, &state);
    }

    //
    backed_off_features(sentence, rule, rule.m, h_tag, &state);
    backed_off_features(sentence, rule, rule.h, m_tag, &state);
    backed_off_features(sentence, rule, rule.h, Y, &state);
    backed_off_features(sentence, rule, rule.h, Z, &state);

    state.inc(X, Y);
    state.inc(X, Z);
    state.inc(is_unary, h_tag);
    state.inc(is_unary, X);
    state.inc(is_unary, rule.rule);

    int bin_span_length = span_length(rule.k - rule.i);
    state.inc(bin_span_length, X);
    state.inc(bin_span_length, rule.rule);

    if (!simple_) {
        int bin_span_length1 = span_length(rule.k - rule.j);
        int bin_span_length2 = span_length(rule.j - rule.i);
        state.inc(bin_span_length1, bin_span_length2, X);
        state.inc(bin_span_length1, bin_span_length2, rule.rule);


        // Label features.
        state.inc(X, m_deplabel);
        state.inc(Y, m_deplabel);
        state.inc(Z, m_deplabel);
        state.inc(rule.rule, m_deplabel);
        state.inc(X, Y, m_deplabel);
        state.inc(X, Z, m_deplabel);
    } 
        state.inc(h_tag, rule.rule, 1);
        state.inc(h_tag, X, 1);
        state.inc(m_tag, rule.rule, 1);
        state.inc(m_tag, X, 1);
        state.inc(m_tag, h_tag, X);
        state.inc(m_tag, h_tag, rule.rule);
        state.inc(Y, 1);
        state.inc(Z, 1);
    

    return state.score;
}