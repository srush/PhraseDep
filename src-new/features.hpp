
#ifndef FEATURES_H_
#define FEATURES_H_

#include "sentence.hpp"
#include "perceptron.hpp"
#include <cmath>

const int n_size = 100000000;

class FeatureGenBackoff;
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

struct FeatureState {
    FeatureState(vector<long> *base_,
                 const vector<double> *weights_)
            : base(base_), weights(weights_) {
        tally = 0;
        score = 0.0;
        feature_num = 0;
    }

    vector<long> *base;
    const vector<double> *weights;
    long tally;
    double score;
    int feature_num;
    const vector<Triple> *features;

    inline void inc(long a, long b, long c=1) {
        const Triple &t = (*features)[feature_num];
        long app = (a * t._size_b * t._size_c + b * t._size_c + c);
        long index = tally + app;
        // assert(app <= t._total_size);

        if (weights != NULL) {
            score += (*weights)[((long)abs(index)) % n_size];
        } else {
            base->push_back(index);
        }
        (tally) += t._total_size;
        feature_num += 1;
    }

};


class FeatureGenBackoff {
  public:
    FeatureGenBackoff() {}
    FeatureGenBackoff(const Lexicon *lexicon,
                      const Grammar *grammar,
                      bool simple);

    double generate(const Sentence &sentence,
                    const AppliedRule &rule,
                    vector<long> *base,
                    const vector<double> *weights) const;

  private:

    void backed_off_features(const Sentence &sentence,
                             const AppliedRule &rule,
                             int index,
                             int extra,
                             FeatureState *state) const;

    void add_template(int a, int b, int c=1);

    void add_backed_off_template(int plus=1);

    vector <Triple> features_;
    const Lexicon *lexicon_;
    const Grammar *grammar_;
    bool simple_;
    bool chinese_;
};


class FeatureScorer : public Scorer {
  public:
    FeatureScorer(bool simple)
            : perceptron_(n_size), simple_(simple) {}

    void set(const Lexicon *lexicon, const Grammar *grammar) {
        feature_gen_ = FeatureGenBackoff(lexicon, grammar, simple_);
    }

    void set_sentence(const Sentence *sentence) {
        sentence_ = sentence;
    }

    template <class Archive>
    void serialize(Archive &ar) {
        ar(perceptron_);
    }

    double score(const AppliedRule &rule) const;

    int update_full(const vector<AppliedRule> &gold_rules,
                    const vector<AppliedRule> &best_rules);

    void update(const vector<AppliedRule> &good,
                const vector<AppliedRule> &bad);

    long hashed_feature(long val) const {
        if (!use_positive_features_) {
            return ((long)abs(val)) % n_size;
        } else {
            map<long, int>::const_iterator iter = positive_features_.find(val);
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
    int full_feature_count_ = 1;

  private:
    FeatureGenBackoff feature_gen_;
    const Sentence *sentence_;

    bool use_positive_features_;
    map<long, int> positive_features_;

    bool dont_hash_features_;
    mutable map<long, int> all_features_;
    bool simple_;
};



#endif  // FEATURES_H_
