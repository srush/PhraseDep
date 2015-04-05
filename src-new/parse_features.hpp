#ifndef PARSE_FEATURES_H_
#define PARSE_FEATURES_H_

class FeatureGenBackoff :public FeatureGen {
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

#endif  // PARSE_FEATURES_H_
