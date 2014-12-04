#ifndef ORACLE_H_
#define ORACLE_H_
#include <iostream>
#include <set>

class OracleScorer : public Scorer {
  public:
    OracleScorer(const vector<AppliedRule> *best_rules,
                 const Grammar *grammar)
            : best_rules_(best_rules), grammar_(grammar) {}

    double score(const AppliedRule &rule) const {
        //cerr << "SCORE" << endl;
        for (int i = 0; i < best_rules_->size(); ++i) {
            if ((*best_rules_)[i].same(rule)) {
                return 2;
            }
        }

        for (int i = 0; i < best_rules_->size(); ++i) {
            if ((*best_rules_)[i].same_span(rule) &&
                grammar_->head_symbol[rule.rule] == grammar_->head_symbol[(*best_rules_)[i].rule]) {
                return 1;
            }
        }

        // for (int i = 0; i < best_rules_->size(); ++i) {
        //     if ((*best_rules_)[i].same_top(rule) &&
        //         grammar_->head_symbol[rule.rule] == grammar_->head_symbol[(*best_rules_)[i].rule]) {
        //         return 0.5;
        //     }
        // }
        return -5;
    }

  private:
    const vector<AppliedRule> *best_rules_;
    const Grammar *grammar_;
};


#endif  // ORACLE_H_
