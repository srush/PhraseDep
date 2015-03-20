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
    vector<int> int_chinese_last_char;

    vector<string> deplabels;
    vector<int> int_deplabels;
    vector<DepLabel> struct_deplabels;

    vector<int> deps;
    vector<AppliedRule> gold_rules;
    vector<int> preterms;
};

vector<Sentence> *read_sentence(string file);
void output_sentence(const Sentence &sentence);

#endif  // SENTENCE_H_
