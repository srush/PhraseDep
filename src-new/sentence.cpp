#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "sentence.hpp"

using namespace std;

void annotate_gold(string file, vector<Sentence> *sentences) {
    ifstream in_file;
    in_file.open(file.c_str());
    for (int j = 0; j < sentences->size(); ++j) {
        int n_rules;
        in_file >> n_rules;
        cout << (*sentences)[j].words.size() << " " << n_rules << endl;
        for (int i = 0; i < n_rules; ++i) {
            AppliedRule rule;
            in_file >> rule.i >> rule.j >> rule.k
                    >> rule.h >> rule.m >> rule.rule;
            // assert(rule.h < (*sentences)[j].words.size());
            (*sentences)[j].gold_rules.push_back(rule);
        }
    }
}

vector<Sentence> *read_sentences(string file,
                                 Lexicon *lexicon, Grammar *grammar) {
    ifstream in_file;
    vector<Sentence> *sentences = new vector<Sentence>();
    in_file.open(file.c_str());
    bool first = true;
    Sentence sentence;
    string word, tag, deplabel, lemma, coarse_tag, blank;
    int position, dep;
    while (in_file.good()) {

        in_file >> position >> word >> lemma
                >> coarse_tag >> tag
                >> blank >> dep >> deplabel;

        if (position == 1 && !first) {
            sentences->push_back(sentence);
            sentence = Sentence();
        }
        first = false;
        sentence.words.push_back(word);
        sentence.tags.push_back(tag);

        dep -= 1;
        int root_dep = -1;
        if (dep == -1) {
            if (root_dep == -1) {
                root_dep = position - 1;
            } else {
                dep = root_dep;
            }
        }
        sentence.deps.push_back(dep);
        sentence.deplabels.push_back(deplabel);
    }

    for (auto &sentence : *sentences) {
        lexicon->process_sentence(&sentence, grammar);
    }
    return sentences;
}


void Lexicon::process_sentence(Sentence *sentence, const Grammar *grammar) {
    for (unsigned j = 0; j < sentence->tags.size(); ++j) {
        sentence->int_tags.push_back(
            tag_index.fget(sentence->tags[j]));
        sentence->preterms.push_back(
            grammar->to_nonterm(sentence->tags[j]));
        sentence->int_words.push_back(
            word_index.fget(sentence->words[j]));
        sentence->int_deplabels.push_back(
            deplabel_index.fget(sentence->deplabels[j]));
    }
}
