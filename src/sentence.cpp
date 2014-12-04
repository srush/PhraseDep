#include <fstream>
#include <iostream>
#include "sentence.hpp"

using namespace std;

void output_sentence(const Sentence &sentence) {
    cout << sentence.words.size() << " " << sentence.gold_rules.size() << endl;

    for (int i = 0; i < sentence.words.size(); ++i) {
        cout << sentence.words[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < sentence.tags.size(); ++i) {
        cout << sentence.tags[i] << " ";
    }
    cout << endl;

    for (int i = 0; i < sentence.deps.size(); ++i) {
        int dep = sentence.deps[i];
        cout << dep << " ";
    }
    cout << endl;

    for (int i = 0; i < sentence.deplabels.size(); ++i) {
        cout << sentence.deplabels[i] << " ";
    }
    cout << endl;
    for (int i = 0; i < sentence.gold_rules.size(); ++i) {
        const AppliedRule &rule = sentence.gold_rules[i];
        cout << rule.i << " " << rule.j << " " << rule.k
             << " " << rule.h << " " << rule.m << " " << rule.rule << endl;
    }
    // cout << endl;
}


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

        int root_dep = -1;
        for (int i = 0; i < n_words; ++i) {
            int dep;
            in_file >> dep;
            if (dep == -1) {
                if (root_dep == -1) {
                    root_dep = i;
                } else {
                    dep = root_dep;
                }
            }
            sentence.deps.push_back(dep);
        }

        // for (int i = 0; i < n_words; ++i) {
        //     int dep;
        //     in_file >> dep;
        //     sentence.deps.push_back(dep);
        // }

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
