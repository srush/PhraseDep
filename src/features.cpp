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

FeatureGenBackoff::FeatureGenBackoff(const Grammar *grammar, bool delex, bool simple)
        : grammar_(grammar), simple_(simple) {
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
    } else {
        add_template(grammar_->tag_index.size(), grammar_->n_rules, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_nonterms, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_rules, 1);
        add_template(grammar_->tag_index.size(), grammar_->n_nonterms, 1);
        add_template(grammar_->tag_index.size(), grammar_->tag_index.size(), grammar_->n_nonterms);
        add_template(grammar_->tag_index.size(), grammar_->tag_index.size(), grammar_->n_rules);
        add_template(grammar_->n_nonterms, 1, 1);
        add_template(grammar_->n_nonterms, 1, 1);
    }
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
}

void FeatureGenBackoff::backed_off_features(const Sentence &sentence,
                                            const AppliedRule &rule,
                                            int index,
                                            int extra,
                                            FeatureState *state) const {
    int word = get_word_int(grammar_, sentence, index);

    int rule_ = rule.rule;
    int X = grammar_->head_symbol[rule.rule];

    state->inc(word, rule_, extra);
    state->inc(word, X, extra);
    if (!simple_) {
        int tag = get_tag_int(grammar_, sentence, index);
        state->inc(tag, rule_, extra);
        state->inc(tag, X, extra);
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
    long m_deplabel = 1; //sentence.int_deplabels[rule.m];

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
    } else {
        state.inc(h_tag, rule.rule, 1);
        state.inc(h_tag, X, 1);
        state.inc(m_tag, rule.rule, 1);
        state.inc(m_tag, X, 1);
        state.inc(m_tag, h_tag, X);
        state.inc(m_tag, h_tag, rule.rule);
        state.inc(Y, 1);
        state.inc(Z, 1);
    }

    return state.score;
}




// THIS CODE IS DEPRECATED.

FeatureGen::FeatureGen(const Grammar *grammar, bool delex)
        : grammar_(grammar),
          delex_(delex) {

    triples.push_back(Triple(grammar_->n_nonterms, grammar->tag_index.size(), grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_rules, grammar->tag_index.size(), grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_rules, grammar_->n_words, grammar->tag_index.size()));

    doubles.push_back(Double(grammar_->n_nonterms, grammar->tag_index.size()));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_words));
    doubles.push_back(Double(grammar_->n_rules, grammar_->n_words));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(2, grammar->tag_index.size()));
    doubles.push_back(Double(2, grammar_->n_nonterms));
    doubles.push_back(Double(2, grammar_->n_rules));
    doubles.push_back(Double(grammar_->n_rules, grammar->tag_index.size()));
    doubles.push_back(Double(grammar_->n_rules, grammar->tag_index.size()));
    doubles.push_back(Double(2, 2));
    doubles.push_back(Double(grammar_->n_nonterms, 2));

    // Adding features for dependency labels
    doubles.push_back(Double(grammar_->n_nonterms, grammar->n_deplabel));
    doubles.push_back(Double(grammar_->n_nonterms, grammar->n_deplabel));
    doubles.push_back(Double(grammar_->n_nonterms, grammar->n_deplabel));
    doubles.push_back(Double(grammar_->n_rules, grammar->n_deplabel));

    triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar->n_deplabel));
    triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar->n_deplabel));

    // Adding one more feature to be similar to the PCFG part?
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_rules));

    // Adding more surface features
    doubles.push_back(Double(grammar_->n_words, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_rules));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_rules));
    doubles.push_back(Double(9, grammar_->n_nonterms));
    doubles.push_back(Double(9, grammar_->n_rules));

    doubles.push_back(Double(grammar_->n_words, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_rules));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_words, grammar_->n_rules));

}


inline void inc3(const Triple &t, long a, long b, long c, vector<long> *base,
                 long *tally,
                 const vector<double> *weights, double *score)  {
    long app = (a * t._size_b * t._size_c + b * t._size_c + c);
    long index = *tally + app;
    assert(app < t._total_size);

    if (weights != NULL) {
        *score += (*weights)[((long)abs(index)) % n_size];
    } else {
        base->push_back(index);
    }
    (*tally) += t._total_size;
}

inline void inc2(const Double &t, long a, long b,  vector<long> *base,
                 long *tally,
                 const vector<double> *weights, double *score)  {

    long app = (a * t._size_b + b);
    long index = *tally + app;
    assert(app < t._total_size);

    if (weights != NULL) {
        *score += (*weights)[((long)abs(index)) % n_size];
    } else {
        base->push_back(index);
    }
    (*tally) += t._total_size;
}

double FeatureGen::generate(const Sentence &sentence,
                            const AppliedRule &rule,
                            vector<long> *base,
                            const vector<double> *weights) const {


    long X = grammar_->head_symbol[rule.rule];
    long Y = grammar_->left_symbol[rule.rule];
    long Z = grammar_->right_symbol[rule.rule];
    // const NonTerminal &nt_X = grammar_->non_terminals_[X];
    // const NonTerminal &nt_Y = grammar_->non_terminals_[Y];
    // const NonTerminal &nt_Z = grammar_->non_terminals_[Z];

    long m_tag = sentence.int_tags[rule.m];
    long h_tag = sentence.int_tags[rule.h];
    long h_word = sentence.int_words[rule.h];
    long m_word = sentence.int_words[rule.m];

    long m_deplabel = sentence.int_deplabels[rule.m];

    bool is_unary = grammar_->is_unary[rule.rule];
    long sentence_size = sentence.int_tags.size();
    int size = (rule.k - rule.i);

    // int simple_rule = grammar_->sparse_rule_index[rule.rule];
    // int simple_X = nt_X.int_main;
    // int simple_Y = nt_Y.int_main;
    // int simple_Z = nt_Z.int_main;


    // cout << X << " " << Y << " " << Z << " " << nt_X.int_main << " "
    //      << nt_Y.int_main << " " << nt_Z.int_main << " " << h_tag << " " << h_word << " " << m_tag << " " << is_unary << " " << rule.rule << endl;

    long first_word = sentence.int_words[rule.i];
    long last_word = sentence.int_words[rule.k];

    long before_split_word = get_word_int(grammar_, sentence, (rule.j));
    long after_split_word = get_word_int(grammar_, sentence, (rule.j + 1));

    long before_span_word = get_word_int(grammar_, sentence, (rule.i - 1));
    long after_span_word = get_word_int(grammar_, sentence, (rule.k + 1));


    int span_length = rule.k - rule.i;
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

    long tally = 0;
    double score = 0.0;
    inc3(triples[0], X, h_tag, m_tag, base, &tally, weights, &score);
    inc3(triples[1], X, Y, h_tag, base, &tally, weights, &score);
    inc3(triples[2], X, Z, h_tag, base, &tally, weights, &score);
    inc3(triples[3], rule.rule, h_tag, m_tag, base, &tally, weights, &score);
    if (!delex_) {
        inc3(triples[4], rule.rule, h_word, m_tag, base, &tally, weights, &score);
    } else {
        tally += triples[4]._total_size;
    }

    inc2(doubles[0], X, h_tag, base, &tally, weights, &score);
    if (!delex_) {
        inc2(doubles[1], X, h_word, base, &tally, weights, &score);
        inc2(doubles[2], rule.rule, h_word, base, &tally, weights, &score);
    } else {
        tally += doubles[1]._total_size + doubles[2]._total_size;
    }
    inc2(doubles[3], X, Y, base, &tally, weights, &score);
    inc2(doubles[4], X, Z, base, &tally, weights, &score);
    inc2(doubles[5], is_unary, h_tag, base, &tally, weights, &score);
    inc2(doubles[6], is_unary, X, base, &tally, weights, &score);
    inc2(doubles[7], is_unary, rule.rule, base, &tally, weights, &score);


    inc2(doubles[8], rule.rule, h_tag, base, &tally, weights, &score);
    inc2(doubles[9], rule.rule, m_tag, base, &tally, weights, &score);
    inc2(doubles[10], is_unary, (size == 0) ? 1: 0, base, &tally, weights, &score);
    inc2(doubles[11], X, (size == sentence_size)? 1:0, base, &tally, weights, &score);

    // Adding features for dependency labels
    // inc2(doubles[12], X, m_deplabel, base, &tally, weights, &score);
    // inc2(doubles[13], Y, m_deplabel, base, &tally, weights, &score);
    // inc2(doubles[14], Z, m_deplabel, base, &tally, weights, &score);
    // inc2(doubles[15], rule.rule, m_deplabel, base, &tally, weights, &score);

    // inc3(triples[5], X, Y, m_deplabel, base, &tally, weights, &score);
    // inc3(triples[6], X, Z, m_deplabel, base, &tally, weights, &score);

    inc2(doubles[12], X, 1, base, &tally, weights, &score);
    inc2(doubles[13], Y, 1, base, &tally, weights, &score);
    inc2(doubles[14], Z, 1, base, &tally, weights, &score);
    inc2(doubles[15], rule.rule, 1, base, &tally, weights, &score);

    inc3(triples[5], X, Y, 1, base, &tally, weights, &score);
    inc3(triples[6], X, Z, 1, base, &tally, weights, &score);

    // Adding one more feature to be similar to the PCFG part?
    inc2(doubles[16], X, rule.rule, base, &tally, weights, &score);

    // Adding more surface features
    // Span first word, span last word and the span length

    inc2(doubles[17], first_word, X, base, &tally, weights, &score);
    inc2(doubles[18], first_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[19], last_word, X, base, &tally, weights, &score);
    inc2(doubles[20], last_word, rule.rule, base, &tally, weights, &score);

    inc2(doubles[21], bin_span_length, X, base, &tally, weights, &score);
    inc2(doubles[22], bin_span_length, rule.rule, base, &tally, weights, &score);


    inc2(doubles[23], before_span_word, X, base, &tally, weights, &score);
    inc2(doubles[24], before_span_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[25], after_span_word, X, base, &tally, weights, &score);
    inc2(doubles[26], after_span_word, rule.rule, base, &tally, weights, &score);

    inc2(doubles[23], before_split_word, X, base, &tally, weights, &score);
    inc2(doubles[24], before_split_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[25], after_split_word, X, base, &tally, weights, &score);
    inc2(doubles[26], after_split_word, rule.rule, base, &tally, weights, &score);

    // The span shape features leave for future work...
    return score;
}



// END DEPRECATED.
