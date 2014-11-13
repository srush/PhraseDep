#include <fstream>
#include "sentence.hpp"

using namespace std;

vector<Sentence> *read_sentence(string file) {
    ifstream in_file;
    vector<Sentence> *sentences = new vector<Sentence>();
    in_file.open(file.c_str());
    while(in_file.good()) {
        Sentence sentence;

        int n_words, n_rules;
        in_file >> n_words >> n_rules;

        for (int i = 0; i < n_words; ++i) {
            string word;
            in_file >> word;
            sentence.words.push_back(word);
        }

        for (int i = 0; i < n_words; ++i) {
            string tag;
            in_file >> tag;
            sentence.tags.push_back(tag);
        }

        for (int i = 0; i < n_words; ++i) {
            int dep;
            in_file >> dep;
            sentence.deps.push_back(dep);
        }

        for (int i = 0; i < n_words; ++i) {
            string deplabel;
            in_file >> deplabel;
            sentence.deplabels.push_back(deplabel);
        }

        for (int i = 0; i < n_rules; ++i) {
            AppliedRule rule;
            in_file >> rule.i >> rule.j >> rule.k >> rule.h >> rule.m >> rule.rule;
            sentence.gold_rules.push_back(rule);
        }
        sentences->push_back(sentence);
    }
    sentences->pop_back();
    return sentences;
}
