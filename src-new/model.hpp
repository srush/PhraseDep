#ifndef MODEL_H_
#define MODEL_H_

class FeatureGen {
  public:
    virtual double generate(const Sentence &sentence,
                            const AppliedRule &rule,
                            vector<long> *base,
                            const vector<double> *weights) const = 0;
};


class Model : public Scorer {
  public:
    Model(bool simple)
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

    AdaGrad adagrad_;
    bool is_cost_augmented_;

    int positive_feature_count_ = 0;
    int full_feature_count_ = 1;

  private:
    FeatureGenBackoff feature_gen_;
    const Sentence *sentence_;

    bool use_positive_features_;
    map<long, int> positive_features_;

    // bool dont_hash_features_;
    mutable map<long, int> all_features_;
    bool simple_;
};

#endif  // MODEL_H_
