#include "features.hpp"
#include <cmath>

double FeatureScorer::score(const AppliedRule &rule) const {
    double score = 0.0;
    vector<int> features;
    feature_gen_.generate(*sentence_, rule, &features);
    for (int i = 0; i < features.size(); ++i) {
        int val = features[i];
        score += perceptron_.weights[((int)abs(val)) % 1000000];
    }
    return score;
}

void FeatureScorer::update(const vector<AppliedRule> &rules,
                           int direction) {
    for (int i = 0; i < rules.size(); ++i) {
        vector<int> features;
        feature_gen_.generate(*sentence_, rules[i], &features);
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



void FeatureGen::generate(const Sentence &sentence,
              const AppliedRule &rule,
              vector<int> *base) const {

    int X = grammar_->head_symbol[rule.rule];
    int Y = grammar_->left_symbol[rule.rule];
    int Z = grammar_->right_symbol[rule.rule];
    bool is_unary = grammar_->is_unary[rule.rule];

    int tally = 0;
    triples[0].inc(X, sentence.int_tags[rule.h], sentence.int_tags[rule.m], base, &tally);
    triples[1].inc(rule.rule, sentence.int_tags[rule.h], sentence.int_tags[rule.m], base, &tally);

    doubles[0].inc(X, sentence.int_tags[rule.h], base, &tally);
    doubles[1].inc(X, Y, base, &tally);
    doubles[2].inc(X, Z, base, &tally);
    doubles[3].inc(is_unary, sentence.int_tags[rule.h], base, &tally);
    doubles[4].inc(is_unary, X, base, &tally);
    doubles[5].inc(is_unary, rule.rule, base, &tally);
    doubles[6].inc(rule.rule, sentence.int_tags[rule.h], base, &tally);
    doubles[7].inc(rule.rule, sentence.int_tags[rule.m], base, &tally);
}
