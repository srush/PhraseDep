#ifndef PRUNING_H_
#define PRUNING_H_

#include <cereal/types/vector.hpp>
//#include <cereal/types/map.hpp>
#include <map>

#include "sentence.hpp"
#include "grammar.hpp"

using namespace std;

struct DirPrune {
    DirPrune(int head_, int left_, int right_,
             int left_first_, int right_first_)
            : head(head_), left(left_), right(right_),
              left_first(left_first_), right_first(right_first_) {}

    int head;
    int left;
    int right;
    int left_first;
    int right_first;

    template <class Archive>
    void serialize(Archive &ar) {
        ar(head, left, right, left_first, right_first);
    }
};

inline bool operator==(const DirPrune& left, const DirPrune& right) {
    return (left.head == right.head) &&
            (left.right == right.right) &&
            (left.left == right.left) &&
            (left.left_first == right.left_first) &&
            (left.right_first == right.right_first);
}

inline bool operator<(const DirPrune& left, const DirPrune& right) {
    if (left.head < right.head) { return true; }
    if (left.head > right.head) { return false; }

    if (left.right < right.right) { return true; }
    if (left.right > right.right) {return false; }

    if (left.left < right.left) { return true; }
    if (left.left > right.left) { return false; }

    if (left.left_first < right.left_first) { return true; }
    if (left.left_first > right.left_first) { return false; }

    if (left.right_first < right.right_first) { return true; }
    if (left.right_first > right.right_first) { return false; }

    return false;
}


struct Pair {
    int first;
    int second;
    template <class Archive>
    void serialize(Archive &ar) {
        ar(first, second);
    }
};

class Pruning {
  public:
    Pruning(const Lexicon *lexicon,
            const Grammar *grammar) : lexicon_(lexicon),
                                      grammar_(grammar) {
        pruning = false;
        label_pruning = false;
        dir_pruning = false;
    }

    void read_label_pruning(string file);

    bool prune(int rule_num, int preterm) const {
        return pruning && !rule_head_tags[rule_num][preterm];
    }

    void set_pruning(const vector<Sentence> &sentences);

    void set_dir_pruning(const vector<Sentence> &sentences);

    double dir_pick(const DirPrune &prune, bool *try_left,
                        bool *try_right) const;

    template <class Archive>
    void serialize(Archive &ar) {
        ar(rule_head_tags, label_rule_allowed, pruning,
           label_pruning,
           dir_pruner);
    }

    bool dir_pruning;

  private:

    vector<vector<bool> > rule_head_tags;
    vector<vector<bool> > label_rule_allowed;

    bool pruning;
    bool label_pruning;

    map<DirPrune, Pair > dir_pruner;

    const Lexicon *lexicon_;
    const Grammar *grammar_;
};



#endif  // PRUNING_H_
