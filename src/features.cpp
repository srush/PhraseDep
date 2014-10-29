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


// void FeatureGen::initialize(const Grammar *grammar) {
//     grammar_ = grammar;
//     triples.resize(2);
//     doubles.resize(5);

//     triples[0] = Triple(grammar_->n_nonterms, n_tags, n_tags);
//     triples[1] = Triple(grammar_->n_rules, n_tags, n_tags);

//     doubles[0] = Double(grammar_->n_nonterms, n_tags);
//     doubles[1] = Double(grammar_->n_nonterms, grammar_->n_nonterms);
//     doubles[2] = Double(grammar_->n_nonterms, grammar_->n_nonterms);
//     doubles[3] = Double(2, n_tags);
//     doubles[4] = Double(2, grammar_->n_nonterms);

// }


void FeatureGen::generate(const Sentence &sentence,
              const AppliedRule &rule,
              vector<int> *base) const {

    int X = grammar_->head_symbol[rule.rule];
    int Y = grammar_->left_symbol[rule.rule];
    int Z = grammar_->right_symbol[rule.rule];
    bool is_unary = grammar_->is_unary[rule.rule];

    int tally = 0;
    base->push_back(tally+triples[0].apply(X, sentence.int_tags[rule.h], sentence.int_tags[rule.m]));
    tally += triples[0]._total_size;
    base->push_back(tally+triples[1].apply(rule.rule, sentence.int_tags[rule.h], sentence.int_tags[rule.m]));
    tally += triples[1]._total_size;

    base->push_back(tally+doubles[0].apply(X, sentence.int_tags[rule.h]));
    tally += doubles[0]._total_size;
    base->push_back(tally+doubles[1].apply(X, Y));
    tally += doubles[1]._total_size;
    base->push_back(tally+doubles[2].apply(X, Z));
    tally += doubles[2]._total_size;
    base->push_back(tally+doubles[3].apply(is_unary, sentence.int_tags[rule.h]));
    tally += doubles[3]._total_size;
    base->push_back(tally+doubles[4].apply(is_unary, X));
}
