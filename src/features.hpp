
#ifndef FEATURES_H_
#define FEATURES_H_

#include "sentence.hpp"
#include "perceptron.hpp"
#include <cmath>


// const int n_tags = 1000;
const int n_size = 100000000;

class FeatureGen;
class FeatureGenBackoff;
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
        assert(app <= t._total_size);

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
    FeatureGenBackoff(const Grammar *grammar, bool delex, bool simple);

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
    const Grammar *grammar_;
    bool simple_;
};


class FeatureScorer : public Scorer {
  public:
    FeatureScorer(const Grammar *grammar, bool delex, bool positive_features,
                  bool dont_hash_features, bool simple)
            : perceptron_(n_size),
              feature_gen_(grammar, delex, simple),
              use_positive_features_(positive_features),
              dont_hash_features_(dont_hash_features) {}

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

    void add_feature(long val) {
        assert(dont_hash_features_);

        map<long, int>::const_iterator iter = all_features_.find(val);
        if (iter == all_features_.end()) {
            int start = full_feature_count_;
            all_features_[val] = full_feature_count_;
            full_feature_count_++;
        }
    }

    long hashed_feature(long val) const {
        if (dont_hash_features_) {
            map<long, int>::const_iterator iter = all_features_.find(val);
            if (iter != all_features_.end()) {
                return iter->second;
            } else {
                return 0;
            }
        } else if (!use_positive_features_) {
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

    void write(string file) {
        if (dont_hash_features_) {
            ofstream out;
            out.open(file.c_str());
            for (auto iter : all_features_) {
                out << iter.first << " " << iter.second << endl;
            }
            out << endl;
            out.close();
        }
    }

    void read(string file) {
        if (dont_hash_features_) {

            ifstream in;
            in.open(file);
            while (in.good()) {
                long index;
                int feature;
                in >> index >> feature;
                all_features_[index] = feature;
                full_feature_count_ = max(full_feature_count_, feature + 1);
            }
            in.close();
            cout << "Reading Features :  ";
            cout << full_feature_count_ << endl;
        }
    }


    Perceptron perceptron_;
    bool is_cost_augmented_;

    int positive_feature_count_ = 0;
    int full_feature_count_ = 1;

  private:

    // FeatureGen feature_gen_;
    FeatureGenBackoff feature_gen_;
    const Sentence *sentence_;

    bool use_positive_features_;
    map<long, int> positive_features_;


    bool dont_hash_features_;
    mutable map<long, int> all_features_;
};



#endif  // FEATURES_H_
