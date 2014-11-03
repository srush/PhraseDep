#include "grammar.hpp"
#include "features.hpp"
#include "sentence.hpp"
#include "dp.hpp"
#include "optionparser.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>

struct Arg: public option::Arg
{
    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) cerr << "Option '" << option << "' requires an argument\n";
        return option::ARG_ILLEGAL;
    }
    static option::ArgStatus Numeric(const option::Option& option, bool msg)
    {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {};
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) cerr << "Option '" << option << "' requires a numeric argument\n";
        return option::ARG_ILLEGAL;
    }
};

enum  optionIndex { UNKNOWN, HELP, GRAMMAR, SENTENCE, EPOCH, MODEL, TEST, SENTENCE_TEST };
const option::Descriptor usage[] =
{
    {UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: example [options]\n\n"
     "Options:" },
    {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
    {GRAMMAR,    0,"g", "grammar", Arg::Required, "  --grammar, -g  \nGrammar file." },
    {SENTENCE,    0,"s", "sentence", Arg::Required, "  --sentence, -g  \nSentence file." },
    {SENTENCE_TEST,    0,"t", "sentence_test", Arg::Required, "  --sentence_test, -t  \nSentence test file." },
    {EPOCH,    0,"e", "epochs", Arg::Numeric, "  --epochs, -e  \nNumber of epochs." },
    {MODEL,    0,"m", "model", Arg::Required, "  --model, -m  \nModel path ." },
    {TEST,    0,"t", "test", option::Arg::None, "  --test, -m  \n ." },
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
    vector<Sentence> *sentences = read_sentence(string(options[SENTENCE].arg));
    for (int i = 0; i < sentences->size(); ++i) {
        for (int j = 0; j < (*sentences)[i].tags.size(); ++j) {
            (*sentences)[i].int_tags.push_back(grammar->to_nonterm((*sentences)[i].tags[j]));
            (*sentences)[i].int_words.push_back(grammar->to_word((*sentences)[i].words[j]));
        }
    }

    // Make scorer.
    FeatureScorer scorer(grammar);
    if (options[TEST]) {
        vector<Sentence> *sentences = read_sentence(string(options[SENTENCE_TEST].arg));
        for (int i = 0; i < sentences->size(); ++i) {
            for (int j = 0; j < (*sentences)[i].tags.size(); ++j) {
                (*sentences)[i].int_tags.push_back(grammar->to_nonterm((*sentences)[i].tags[j]));
                (*sentences)[i].int_words.push_back(grammar->to_word((*sentences)[i].words[j]));
            }
        }

        ifstream in;
        in.open(options[MODEL].arg);
        for (int i = 0; i < scorer.perceptron_.weights.size(); ++i) {
            in >> scorer.perceptron_.weights[i];
        }
        in.close();

        for (int i = 0; i < sentences->size(); ++i) {
            const Sentence *sentence = &(*sentences)[i];
            scorer.set_sentence(sentence);
            vector<AppliedRule> best_rules;
            cky(sentence->int_tags, sentence->words, sentence->deps, *grammar, scorer, &best_rules, true);
            cout << endl;
        }

    } else {

        char *temp = 0;
        int epochs = strtol(options[EPOCH].arg, &temp, 10);
        char epoch_char = '1';
        for (int epoch = 0; epoch < epochs; ++epoch) {
            double total_score = 0.0;
            double total_score2 = 0.0;
            int total = 0;
            for (int i = 0; i < sentences->size(); ++i) {
                const Sentence *sentence = &(*sentences)[i];
                scorer.set_sentence(sentence);
                vector<AppliedRule> best_rules;
                double dp_score =
                        cky(sentence->int_tags, sentence->words, sentence->deps, *grammar, scorer, &best_rules, false);

                // cout << "RULES: " << best_rules.size() << endl;

                int correct = 0, correct2 = 0;
                double score;

                vector<AppliedRule> bad, good;
                double best_score = 0.0;
                for (int i = 0; i < best_rules.size(); ++i) {
                    bool have = false;
                    best_score += scorer.score(best_rules[i]);
                    for (int j = 0; j < sentence->gold_rules.size(); ++j) {
                        if (best_rules[i].same(sentence->gold_rules[j])) {
                            correct += 1;
                            have = true;
                            break;
                        }
                    }
                    if (!have) bad.push_back(best_rules[i]);
                }

                assert(fabs(best_score - dp_score) < 1e-4);

                double gold_score = 0.0;
                for (int j = 0; j < sentence->gold_rules.size(); ++j) {
                    bool have = false;
                    gold_score += scorer.score(sentence->gold_rules[j]);
                    for (int i = 0; i < best_rules.size(); ++i) {
                        if (best_rules[i].same(sentence->gold_rules[j])) {
                            correct2 += 1;
                            have = true;
                            break;
                        }
                    }
                    if (!have) good.push_back(sentence->gold_rules[j]);
                }
                assert(best_score >= gold_score - 1e-4);
                scorer.update(good, 1);
                scorer.update(bad, -1);

                double tscore;
                if (best_rules.size() > 0 && sentence->gold_rules.size() > 0) {
                    total_score += correct2 / (double)sentence->gold_rules.size();
                    total_score2 += correct / (double)best_rules.size();
                    total += 1;
                    tscore = correct / (double)best_rules.size();
                    if (tscore < 0.05) cout << "bad score" << endl;
                }


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


                if (i % 100 == 0) {
                    cout << i << endl;
                }
                scorer.perceptron_.next_round();
            }
            scorer.perceptron_.finish();
            cout << "EPOCH: " << epoch << " " << total_score / (float)total << " " << total_score2 / (float)total << endl;
            scorer.perceptron_.write(string(options[MODEL].arg) + (char)(epoch_char + epoch), false);
            scorer.perceptron_.write(string(options[MODEL].arg) + (char)(epoch_char + epoch) + ".average", true);
        }

        // Output model.
        scorer.perceptron_.write(string(options[MODEL].arg) + ".average", true);
    }
}
