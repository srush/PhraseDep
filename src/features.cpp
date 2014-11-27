#include "features.hpp"
#include <cmath>
#include <iostream>
#include <map>

double FeatureScorer::score(const AppliedRule &rule) const{
    double score = 0.0;
    vector<long> features;
    if (!is_cost_augmented_){
        return feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights);
    }else{
        double cost = 1;
        for (int j = 0; j < sentence_->gold_rules.size(); ++j) {
            if (rule.same( sentence_->gold_rules[j] ) ) {
                cost = 0;
                break;
            }
        }
        // if(cost == 0){
        //     cerr << "fin " << feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights) << "cost " << cost << endl;
        // }
        return feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights) + cost;  
    }
    // for (long i = 0; i < features.size(); ++i) {
    //     long val = features[i];
    //     score += perceptron_.weights[((long)abs(val)) % 1000000];
    // }
    // return score;
    
}

// void FeatureScorer::update(const vector<AppliedRule> &rules,
//                            int direction) {
//     for (int i = 0; i < rules.size(); ++i) {
//         vector<long> features;
//         feature_gen_.generate(*sentence_, rules[i], &features, NULL);
//         for (int j = 0; j < features.size(); ++j) {
//             long val = features[j];
//             perceptron_.update((long)abs(val) % n_size, direction);
//         }
//     }
// }

void FeatureScorer::update(const vector<AppliedRule> &good, const vector<AppliedRule> &bad){
        map<long, int> count_map;
        for (int i = 0; i < good.size(); ++i) {
            vector<long> good_features;
            feature_gen_.generate(*sentence_, good[i], &good_features, NULL);
            for (int j = 0; j < good_features.size(); ++j) {
                long val = good_features[j];
                long index = (long)abs(val) % n_size;
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
                long val = bad_features[j];
                long index = (long)abs(val) % n_size;
                double orignal_value = 0.0;
                if (count_map.find(index) != count_map.end()) {
                    orignal_value = count_map[index];
                }
                count_map[index] = orignal_value - 1;
            }
        }

        for(auto& iter : count_map) {
            // cerr << "update " << (long)iter.first << " with " << (int)iter.second << endl;
            perceptron_.update((long)iter.first, (int)iter.second);
        }
}

FeatureGen::FeatureGen(const Grammar *grammar, bool delex) : grammar_(grammar), delex_(delex) {

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

    // doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    // doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    // doubles.push_back(Double(grammar_->n_nonterms, grammar->tag_index.size()));
    // doubles.push_back(Double(grammar_->n_nonterms, grammar->tag_index.size()));
    // doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_words));
    // doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_words));
    // doubles.push_back(Double(grammar_->n_nonterms, 2));

    // triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));


    // triples.push_back(Triple(grammar_->n_rules, grammar->tag_index.size(), grammar->tag_index.size()));
    // triples.push_back(Triple(grammar_->n_rules, grammar->n_words, grammar->tag_index.size()));
    // triples.push_back(Triple(grammar_->n_rules, grammar->n_words, grammar->tag_index.size()));

    // triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));
    // triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));

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


inline void inc3(const Triple &t, long a, long b, long c, vector<long> *base, long *tally,
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

inline void inc2(const Double &t, long a, long b,  vector<long> *base, long *tally,
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

// inline void inc1(long total_size, long a, vector<long> *base, long *tally,
//                  const vector<double> *weights, double *score)  {

//     long app = (a);
//     long index = *tally + app;
//     assert(app < total_size);

//     if (weights != NULL) {
//         *score += (*weights)[((long)abs(index)) % n_size];
//     } else {
//         base->push_back(index);
//     }
//     (*tally) += total_size;
// }

inline long get_word_int(const Grammar* grammar, const Sentence &sentence, int index){
    if (index >= 0 && index <= sentence.int_words.size()){
        return sentence.int_words[index];
    }else if(index == -1){
        return 0;
    }else {
        return 1;
    }
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

    // New features.
    // inc2(doubles[12], simple_X, simple_Y, base, &tally, weights, &score);
    // inc2(doubles[13], simple_X, simple_Z, base, &tally, weights, &score);
    // inc2(doubles[14], simple_X, h_tag, base, &tally, weights, &score);
    // inc2(doubles[15], simple_X, m_tag, base, &tally, weights, &score);
    // inc2(doubles[16], simple_X, m_word, base, &tally, weights, &score);
    // inc2(doubles[17], simple_X, h_word, base, &tally, weights, &score);
    // inc2(doubles[18], simple_X, is_unary, base, &tally, weights, &score);


    // inc3(triples[5], nt_X.int_main, nt_Y.int_main, nt_Z.int_main, base, &tally, weights, &score);

    // // Sparse rules
    // inc3(triples[6], simple_rule, h_tag, m_tag, base, &tally, weights, &score);
    // inc3(triples[7], simple_rule, h_word, m_tag, base, &tally, weights, &score);
    // inc3(triples[8], simple_rule, m_word, h_tag, base, &tally, weights, &score);
    // inc3(triples[8], nt_X.int_main, nt_Y.int_main, , base, &tally, weights, &score);

    // Adding features for dependency labels
    inc2(doubles[12], X, m_deplabel, base, &tally, weights, &score);
    inc2(doubles[13], Y, m_deplabel, base, &tally, weights, &score);
    inc2(doubles[14], Z, m_deplabel, base, &tally, weights, &score);
    inc2(doubles[15], rule.rule, m_deplabel, base, &tally, weights, &score);

    inc3(triples[5], X, Y, m_deplabel, base, &tally, weights, &score);
    inc3(triples[6], X, Z, m_deplabel, base, &tally, weights, &score);

    // Adding one more feature to be similar to the PCFG part?
    inc2(doubles[16], X, rule.rule, base, &tally, weights, &score);

    // Adding more surface features
    // Span first word, span last word and the span length

    long first_word = sentence.int_words[rule.i];
    long last_word = sentence.int_words[rule.k];

    int span_length = rule.k - rule.i;
    int bin_span_length = 0;
    if (span_length <=5) {
        bin_span_length = span_length;
    }else if (span_length <=10) {
        bin_span_length = 6;
    }else if (span_length <= 20) {
        bin_span_length = 7;
    }else{
        bin_span_length = 8;
    }

    inc2(doubles[17], first_word, X, base, &tally, weights, &score);
    inc2(doubles[18], first_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[19], last_word, X, base, &tally, weights, &score);
    inc2(doubles[20], last_word, rule.rule, base, &tally, weights, &score);

    inc2(doubles[21], bin_span_length, X, base, &tally, weights, &score);
    inc2(doubles[22], bin_span_length, rule.rule, base, &tally, weights, &score);


    long before_span_word = get_word_int(grammar_, sentence, (rule.i - 1));
    long after_span_word = get_word_int(grammar_, sentence, (rule.k + 1));

    inc2(doubles[23], before_span_word, X, base, &tally, weights, &score);
    inc2(doubles[24], before_span_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[25], after_span_word, X, base, &tally, weights, &score);
    inc2(doubles[26], after_span_word, rule.rule, base, &tally, weights, &score);

    long before_split_word = get_word_int(grammar_, sentence, (rule.j));
    long after_split_word = get_word_int(grammar_, sentence, (rule.j + 1));

    inc2(doubles[23], before_split_word, X, base, &tally, weights, &score);
    inc2(doubles[24], before_split_word, rule.rule, base, &tally, weights, &score);
    inc2(doubles[25], after_split_word, X, base, &tally, weights, &score);
    inc2(doubles[26], after_split_word, rule.rule, base, &tally, weights, &score);

    // The span shape features leave for future work...
    return score;
}

        // D       [(X, p["TAG"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
        // D        (X, p["TAG"][o[:, HEAD]]),
        //         (X, top),
        // D        (X, Y),
        // D        (X, Z),
        //         (mod_count[o[:, HEAD], 0], o[:, RULE]),
        //         (mod_count[o[:, HEAD], 1], o[:, RULE]),
        //         (first_child, o[:, RULE]),
        //         (last_child, o[:, RULE]),
        //         (is_unary,),
        // D        (is_unary, p["TAG"][o[:, HEAD]]),
        // D        (is_unary, X),
        //         (is_unary, size == 0),
        // D        (X, Y, p["TAG"][o[:, HEAD]]),
        // D        (X, Z, p["TAG"][o[:, HEAD]]),
        // D        (X, p["WORD"][o[:, HEAD]]),
        // D        (o[:, RULE], p["TAG"][o[:, MOD]]),
        // D        (o[:, RULE], p["TAG"][o[:, HEAD]]),
        //         (o[:, RULE], p["TAG"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
        // D        (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
        //         (bdist, o[:, RULE]),
        //         (direction, bdist1, bdist2),
        //         (o[:, RULE],)]
