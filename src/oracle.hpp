#ifndef ORACLE_H_
#define ORACLE_H_
#include <iostream>
#include <set>

class OracleScorer : public Scorer {
  public:
    OracleScorer(const vector<AppliedRule> *best_rules)
            : best_rules_(best_rules) {}

    double score(const AppliedRule &rule) const {
        //cerr << "SCORE" << endl;
        for (int i = 0; i < best_rules_->size(); ++i) {
            if ((*best_rules_)[i].same(rule)) {
                return 1;
            }
        }
        return -1;
    }

  private:
    const vector<AppliedRule> *best_rules_;

};


#endif  // ORACLE_H_
