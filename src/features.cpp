#include "features.hpp"
#include <cmath>
#include <iostream>

double FeatureScorer::score(const AppliedRule &rule) const {
    double score = 0.0;
    vector<long> features;
    return feature_gen_.generate(*sentence_, rule, &features, &perceptron_.weights);
    // for (long i = 0; i < features.size(); ++i) {
    //     long val = features[i];
    //     score += perceptron_.weights[((long)abs(val)) % 1000000];
    // }
    // return score;
}

void FeatureScorer::update(const vector<AppliedRule> &rules,
                           int direction) {
    for (int i = 0; i < rules.size(); ++i) {
        vector<long> features;
        feature_gen_.generate(*sentence_, rules[i], &features, NULL);
        for (int j = 0; j < features.size(); ++j) {
            long val = features[j];
            perceptron_.update((long)abs(val) % n_size, direction);
        }
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

    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_nonterms));
    doubles.push_back(Double(grammar_->n_nonterms, grammar->tag_index.size()));
    doubles.push_back(Double(grammar_->n_nonterms, grammar->tag_index.size()));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_words));
    doubles.push_back(Double(grammar_->n_nonterms, grammar_->n_words));
    doubles.push_back(Double(grammar_->n_nonterms, 2));

    triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));


    triples.push_back(Triple(grammar_->n_rules, grammar->tag_index.size(), grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_rules, grammar->n_words, grammar->tag_index.size()));
    triples.push_back(Triple(grammar_->n_rules, grammar->n_words, grammar->tag_index.size()));

    // triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));
    // triples.push_back(Triple(grammar_->n_nonterms, grammar_->n_nonterms, grammar_->n_nonterms));



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



double FeatureGen::generate(const Sentence &sentence,
                            const AppliedRule &rule,
                            vector<long> *base,
                            const vector<double> *weights) const {


    long X = grammar_->head_symbol[rule.rule];
    long Y = grammar_->left_symbol[rule.rule];
    long Z = grammar_->right_symbol[rule.rule];
    const NonTerminal &nt_X = grammar_->non_terminals_[X];
    const NonTerminal &nt_Y = grammar_->non_terminals_[Y];
    const NonTerminal &nt_Z = grammar_->non_terminals_[Z];


    long m_tag = sentence.int_tags[rule.m];
    long h_tag = sentence.int_tags[rule.h];
    long h_word = sentence.int_words[rule.h];
    long m_word = sentence.int_words[rule.m];

    bool is_unary = grammar_->is_unary[rule.rule];
    long sentence_size = sentence.int_tags.size();
    int size = (rule.k - rule.i);

    int simple_rule = grammar_->sparse_rule_index[rule.rule];
    int simple_X = nt_X.int_main;
    int simple_Y = nt_Y.int_main;
    int simple_Z = nt_Z.int_main;


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
    inc2(doubles[12], simple_X, simple_Y, base, &tally, weights, &score);
    inc2(doubles[13], simple_X, simple_Z, base, &tally, weights, &score);
    inc2(doubles[14], simple_X, h_tag, base, &tally, weights, &score);
    inc2(doubles[15], simple_X, m_tag, base, &tally, weights, &score);
    inc2(doubles[16], simple_X, m_word, base, &tally, weights, &score);
    inc2(doubles[17], simple_X, h_word, base, &tally, weights, &score);
    inc2(doubles[18], simple_X, is_unary, base, &tally, weights, &score);


    inc3(triples[5], nt_X.int_main, nt_Y.int_main, nt_Z.int_main, base, &tally, weights, &score);

    // Sparse rules
    inc3(triples[6], simple_rule, h_tag, m_tag, base, &tally, weights, &score);
    inc3(triples[7], simple_rule, h_word, m_tag, base, &tally, weights, &score);
    inc3(triples[8], simple_rule, m_word, h_tag, base, &tally, weights, &score);
    // inc3(triples[8], nt_X.int_main, nt_Y.int_main, , base, &tally, weights, &score);





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
