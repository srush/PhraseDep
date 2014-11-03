
#ifndef FEATURES_H_
#define FEATURES_H_

#include "sentence.hpp"
#include "perceptron.hpp"
#include <cmath>


const int n_tags = 1000;
const int n_size = 10000000;

class FeatureGen;
class Double;
class Triple;



class Triple {
  public:
    Triple() {}
    Triple(int size_a, int size_b, int size_c)
            : _size_a(size_a), _size_b(size_b), _size_c(size_c),
              _total_size(size_a * size_b * size_c) {}

    int apply(int a, int b, int c) const {
        return a * _size_b * _size_c + b * _size_c + c;
    }


    inline void inc(int a, int b, int c, vector<int> *base, int *tally,
                    const vector<double> *weights, double *score) const {
        int index = *tally + apply(a, b, c);
        if (weights != NULL) {
            *score += (*weights)[((int)abs(index)) % n_size];
        } else {
            base->push_back(index);
        }
        (*tally) += _total_size;
//(*tally) += _total_size;

    }

    int _size_a;
    int _size_b;
    int _size_c;
    long int _total_size;
};

class Double {
  public:
    Double() {}
    Double(int size_a, int size_b)
            : _size_a(size_a), _size_b(size_b), _total_size(size_a * size_b) {}

    int apply(int a, int b) const {
        return a * _size_b + b;
    }

    void inc(int a, int b, vector<int> *base, int *tally, const vector<double> *weights, double *score) const {
        int index = *tally + apply(a, b);
        if (weights != NULL) {
            *score += (*weights)[((int)abs(index)) % n_size];
        } else {
            base->push_back(index);
        }
        (*tally) += _total_size;
    }

    long int _total_size;

    int _size_a;
    int _size_b;

};

// Replicated from python training code.
class FeatureGen {
  public:
FeatureGen(const Grammar *grammar, bool delex);

    double generate(const Sentence &sentence,
                        const AppliedRule &rule,
                        vector<int> *base,
                        const vector<double> *weights) const;
  private:
    vector <Triple> triples;
    vector <Double> doubles;
    const Grammar *grammar_;
bool delex_;
};

class FeatureScorer {
  public:
    FeatureScorer(const Grammar *grammar, bool delex)
            : perceptron_(n_size), feature_gen_(grammar, delex) {}

    void set_sentence(const Sentence *sentence) {
        sentence_ = sentence;
    }

    double score(const AppliedRule &rule) const;

    void update(const vector<AppliedRule> &rules, int direction);

    Perceptron perceptron_;

private:

    FeatureGen feature_gen_;
    const Sentence *sentence_;
};




            // base = [// (X, tags[rule.h]], tags[rule.m]]),
        //         // (X, p["TAG"][o[:, HEAD]]),
        //         (X, top),
        //         // (X, Y),
        //         // (X, Z),
        //         (mod_count[o[:, HEAD], 0], o[:, RULE]),
        //         (mod_count[o[:, HEAD], 1], o[:, RULE]),
        //         (first_child, o[:, RULE]),
        //         (last_child, o[:, RULE]),
        //         (is_unary,),
        //         // (is_unary, p["TAG"][o[:, HEAD]]),
        //         // (is_unary, X),
        //         (is_unary, size == 0),
        //         (X, Y, p["TAG"][o[:, HEAD]]),
        //         (X, Z, p["TAG"][o[:, HEAD]]),
        //         (X, p["WORD"][o[:, HEAD]]),
        //         (o[:, RULE], p["TAG"][o[:, MOD]]),
        //         (o[:, RULE], p["TAG"][o[:, HEAD]]),
        //         (o[:, RULE], p["TAG"][o[:, HEAD]], p["WORD"][o[:, MOD]]),
        //         (o[:, RULE], p["WORD"][o[:, HEAD]], p["TAG"][o[:, MOD]]),
        //         (bdist, o[:, RULE]),
        //         (direction, bdist1, bdist2),
        //         (o[:, RULE],)];

    //     double score() {
    //     double score = 0.0;
    //     vector<vector<int> > features;
    //     vector<vector<int> > templates;
    //     generate(&features);
    //     int base = 0;
    //     for (int i = 0; i < features.size(); ++i) {
    //         int val = 0;
    //         int mult = 1;
    //         for (int j = 0; j < features[i].size(); ++j) {
    //             val += features[i][j] * mult;
    //             mult = templates[i][j];
    //         }
    //         score += weights[val + base % 1000000];
    //         base += mult;
    //     }
    //     return base;
    // }
#endif  // FEATURES_H_
