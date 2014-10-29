
#ifndef FEATURES_H_
#define FEATURES_H_

#include "sentence.hpp"
#include "perceptron.hpp"


const int n_tags = 1000;

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
    int _total_size;

    void inc(int a, int b, int c, vector<int> *base, int *tally) const {
        base->push_back(*tally + apply(a, b, c));
        (*tally) += _total_size;
    }

  private:
    int _size_a;
    int _size_b;
    int _size_c;

};

class Double {
  public:
    Double() {}
    Double(int size_a, int size_b)
            : _size_a(size_a), _size_b(size_b), _total_size(size_a * size_b) {}

    int apply(int a, int b) const {
        return a * _size_b + b;
    }

    void inc(int a, int b, vector<int> *base, int *tally) const {
        base->push_back(*tally + apply(a, b));
        (*tally) += _total_size;
    }

    int _total_size;

  private:
    int _size_a;
    int _size_b;

};

// Replicated from python training code.
class FeatureGen {
  public:
    FeatureGen(const Grammar *grammar);

    void generate(const Sentence &sentence,
                  const AppliedRule &rule,
                  vector<int> *base) const;
  private:
    vector <Triple> triples;
    vector <Double> doubles;
    const Grammar *grammar_;
};

class FeatureScorer {
  public:
    FeatureScorer(const Grammar *grammar)
            : perceptron_(1000000), feature_gen_(grammar) {}

    void set_sentence(const Sentence *sentence) {
        sentence_ = sentence;
    }

    double score(const AppliedRule &rule) const;

    void update(const vector<AppliedRule> &rules, int direction);

  private:
    Perceptron perceptron_;
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
