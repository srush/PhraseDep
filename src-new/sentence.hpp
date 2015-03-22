#ifndef SENTENCE_H_
#define SENTENCE_H_

#include "index.hpp"
#include "grammar.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>

using namespace std;

class AppliedRule;
class Grammar;

struct Sentence {
  public:
    vector<string> tags;
    vector<int> int_tags;

    vector<string> words;
    vector<int> int_words;

    vector<string> deplabels;
    vector<int> int_deplabels;

    vector<int> deps;
    vector<AppliedRule> gold_rules;
    vector<int> preterms;
};

class Lexicon {
  public:
    Lexicon() {
        word_index.fget("#START#");
        word_index.fget("#END#");

        tag_index.fget("#START#");
        tag_index.fget("#END#");
    }

    template <class Archive>
    void serialize(Archive &ar) {
        ar(tag_index, word_index, deplabel_index);
    }

    void process_sentence(Sentence *sentence,
                          const Grammar *grammar);

    Index tag_index;
    Index word_index;
    Index deplabel_index;
};

vector<Sentence> *read_sentences(string file, Lexicon *lexicon, Grammar *grammar);
void annotate_gold(string file, vector<Sentence> *sentences);

#endif  // SENTENCE_H_
