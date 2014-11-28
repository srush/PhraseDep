
#ifndef FEATURES_H_
#define FEATURES_H_

#include "sentence.hpp"
#include "perceptron.hpp"
#include <cmath>


// const int n_tags = 1000;
const int n_size = 100000000;

class FeatureGen;
class Double;
class Triple;


class Triple {
  public:
    Triple() {}
Triple(const Triple &o) :
        _size_a(o._size_a), _size_b(o._size_b), _size_c(o._size_c), _total_size(o._total_size) {}

    Triple(long size_a, long size_b, long size_c)
            : _size_a(size_a), _size_b(size_b), _size_c(size_c),
              _total_size(size_a * size_b * size_c) {}

    long _size_a;
    long _size_b;
    long _size_c;
    long _total_size;
};

class Double {
  public:
    Double() {}
    Double(const Double &o) :
        _size_a(o._size_a), _size_b(o._size_b), _total_size(o._total_size) {}
    Double(long size_a, long size_b)
            : _size_a(size_a), _size_b(size_b), _total_size(size_a * size_b) {}
    long  _total_size;

    long _size_a;
    long _size_b;

};

// Replicated from python training code.
class FeatureGen {
  public:
    FeatureGen(const Grammar *grammar, bool delex);


    double generate(const Sentence &sentence,
                    const AppliedRule &rule,
                    vector<long> *base,
                    const vector<double> *weights) const;



  private:
    vector <int> singles;
    vector <Triple> triples;
    vector <Double> doubles;
    const Grammar *grammar_;
    bool delex_;
};

class FeatureScorer : public Scorer {
  public:
    FeatureScorer(const Grammar *grammar, bool delex, bool positive_features)
            : perceptron_(n_size), feature_gen_(grammar, delex),
              use_positive_features_(positive_features) {}

    void set_sentence(const Sentence *sentence) {
        sentence_ = sentence;
    }

    double score(const AppliedRule &rule) const;

    void update(const vector<AppliedRule> &good, const vector<AppliedRule> &bad);

    void add_positive_rule(const Sentence &sentence,
                           const AppliedRule &positive_rule) {
        assert(use_positive_features_);
        vector<long> features;

        feature_gen_.generate(sentence, positive_rule, &features, NULL);

        for (int i = 0; i < features.size(); ++i) {
            if (positive_features_.find(features[i]) == positive_features_.end()) {
                positive_features_[features[i]] = positive_feature_count_;
                positive_feature_count_++;
            }
        }
    }

    long hashed_feature(long val) const {
        if (!use_positive_features_) {
            return ((long)abs(val)) % n_size;
        } else {
            map<int, int>::const_iterator iter = positive_features_.find(val);
            if (iter != positive_features_.end()) {
                return iter->second;
            } else {
                int start = positive_feature_count_;
                return start + (((long)abs(val)) % (n_size - start));
            }
        }
    }


    Perceptron perceptron_;
    bool is_cost_augmented_;
    int positive_feature_count_ = 0;

  private:

    FeatureGen feature_gen_;
    const Sentence *sentence_;

    bool use_positive_features_;
    map<int, int> positive_features_;

};


#endif  // FEATURES_H_
