#include "grammar.hpp"
#include "features.hpp"
#include "sentence.hpp"
#include "dp.hpp"
#include "optionparser.h"
#include <iostream>

struct Arg: public option::Arg
{
    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) cerr << "Option '" << option << "' requires an argument\n";
        return option::ARG_ILLEGAL;
    }
};

enum  optionIndex { UNKNOWN, HELP, GRAMMAR, SENTENCE };
const option::Descriptor usage[] =
{
    {UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: example [options]\n\n"
     "Options:" },
    {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
    {GRAMMAR,    0,"g", "grammar", Arg::Required, "  --grammar, -g  \nGrammar file." },
    {SENTENCE,    0,"s", "sentence", Arg::Required, "  --sentence, -g  \nSentence file." },
    {UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
                                                  "  example --unknown -- --this_is_no_option\n"
     "  example -unk --plus -ppp file1 file2\n" },
    {0,0,0,0,0,0}
};

int main(int argc, char* argv[])
{
    argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
    option::Stats  stats(usage, argc, argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    if (parse.error())
        return 1;

    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }


    Grammar *grammar = read_rule_set(string(options[GRAMMAR].arg));
    FeatureScorer scorer(grammar);
    vector<Sentence> *sentences = read_sentence(string(options[SENTENCE].arg));
    for (int i = 0; i < sentences->size(); ++i) {
        for (int j = 0; j < (*sentences)[i].tags.size(); ++j) {
            (*sentences)[i].int_tags.push_back(grammar->to_nonterm((*sentences)[i].tags[j]));
        }
    }

    for (int epoch = 0; epoch < 40; ++epoch) {
        double total_score = 0.0;
        for (int i = 0; i < sentences->size(); ++i) {
            const Sentence *sentence = &(*sentences)[i];
            scorer.set_sentence(sentence);
            vector<AppliedRule> best_rules;
            cky(sentence->int_tags, sentence->deps, *grammar, scorer, &best_rules);

            scorer.update(best_rules, -1);
            // cout << "RULES: " << best_rules.size() << endl;

            int correct = 0;
            double score;

            for (int i = 0; i < best_rules.size(); ++i) {
                for (int j = 0; j < sentence->gold_rules.size(); ++j) {
                    if (best_rules[i].same(sentence->gold_rules[j])) {
                        correct += 1;
                        break;
                    }
                }
            }
            score = correct / (double)best_rules.size();
            total_score += score;
            // cout << score << " " << best_rules.size() << " " << sentence->gold_rules.size() << endl;
            // cout << correct << " " << best_rules.size() << " " << score << endl;

            // cout << "BEST" << endl;
            // for (int i = 0; i < best_rules.size(); ++i) {
            //     cout << best_rules[i].i << " " << best_rules[i].j <<  " " << best_rules[i].k << endl;
            // }
            // cout << "GOLD" << endl;

            // for (int i = 0; i < sentence->gold_rules.size(); ++i) {
            //     cout << sentence->gold_rules[i].i << " " << sentence->gold_rules[i].j <<  " " << sentence->gold_rules[i].k << endl;
            // }

            scorer.update(sentence->gold_rules, 1);
            if (i % 100 == 0) {
                cout << i << endl;
            }
        }
        cout << "EPOCH: " << epoch << " " << total_score / (float)sentences->size() << endl;
    }
}
