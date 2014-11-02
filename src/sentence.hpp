#ifndef SENTENCE_H_
#define SENTENCE_H_

#include "grammar.hpp"
#include <vector>

using namespace std;

struct Sentence {
  public:
    vector<string> tags;
    vector<int> int_tags;
    vector<string> words;
    vector<int> int_words;
    vector<int> deps;
    vector<AppliedRule> gold_rules;
};

vector<Sentence> *read_sentence(string file);

#endif  // SENTENCE_H_
