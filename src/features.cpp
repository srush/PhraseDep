#include "features.hpp"
#include <cmath>

double FeatureScorer::score(const AppliedRule &rule) const {
    double score = 0.0;
    vector<int> features;
    return feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights);
    // for (int i = 0; i < features.size(); ++i) {
    //     int val = features[i];
    //     score += perceptron_.weights[((int)abs(val)) % 1000000];
    // }
    // return score;
}

void FeatureScorer::update(const vector<AppliedRule> &rules,
                           int direction) {
    for (int i = 0; i < rules.size(); ++i) {
        vector<int> features;
        feature_gen_.generate(*sentence_, rules[i], &features, NULL);
        for (int j = 0; j < features.size(); ++j) {
            int val = features[j];
            perceptron_.update((int)abs(val) % 1000000, direction);
        }
    }
}



FeatureGen::FeatureGen(const Grammar *grammar) : grammar_(grammar) {

    triples.push_back(Triple(grammar_->n_nonterms, n_tags, n_tags));
    triples.push_back(Triple(grammar_->n_rules, n_tags, n_tags));

    doubles.push_back(Double(grammar_->n_nonterms, n_tags));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(2, n_tags));
    doubles.push_back(Double(2, grammar_->n_nonterms));
    doubles.push_back(Double(2, grammar_->n_rules));
    doubles.push_back(Double(grammar_->n_rules, n_tags));
    doubles.push_back(Double(grammar_->n_rules, n_tags));
}


inline void inc3(const Triple &t, int a, int b, int c, vector<int> *base, int *tally,
                 const vector<double> *weights, double *score)  {
    int index = *tally + (a * t._size_b * t._size_c + b * t._size_c + c);
        if (weights != NULL) {
            *score += (*weights)[((int)abs(index)) % 1000000];
        } else {
            base->push_back(index);
        }
        (*tally) += t._total_size;
}

inline void inc2(const Double &t, int a, int b,  vector<int> *base, int *tally,
                 const vector<double> *weights, double *score)  {
    int index = *tally + (a * t._size_b + b);
        if (weights != NULL) {
            *score += (*weights)[((int)abs(index)) % 1000000];
        } else {
            base->push_back(index);
        }
        (*tally) += t._total_size;
}



double FeatureGen::generate(const Sentence &sentence,
                          const AppliedRule &rule,
                          vector<int> *base,
                          const vector<double> *weights) const {

    int X = grammar_->head_symbol[rule.rule];
    int Y = grammar_->left_symbol[rule.rule];
    int Z = grammar_->right_symbol[rule.rule];
    int m_tag = sentence.int_tags[rule.m];
    int h_tag = sentence.int_tags[rule.m];
    bool is_unary = grammar_->is_unary[rule.rule];

    int tally = 0;
    double score = 0.0;
    inc3(triples[0], X, h_tag, m_tag, base, &tally, weights, &score);
    inc3(triples[1], rule.rule, h_tag, m_tag, base, &tally, weights, &score);

    inc2(doubles[0], X, h_tag, base, &tally, weights, &score);
    inc2(doubles[1], X, Y, base, &tally, weights, &score);
    inc2(doubles[2], X, Z, base, &tally, weights, &score);
    inc2(doubles[3], is_unary, h_tag, base, &tally, weights, &score);
    inc2(doubles[4], is_unary, X, base, &tally, weights, &score);
    inc2(doubles[5], is_unary, rule.rule, base, &tally, weights, &score);
    inc2(doubles[6], rule.rule, h_tag, base, &tally, weights, &score);
    inc2(doubles[7], rule.rule, m_tag, base, &tally, weights, &score);
    return score;
}
