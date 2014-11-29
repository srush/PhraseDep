#include "grammar.hpp"
#include "features.hpp"
#include "sentence.hpp"
#include "dp.hpp"
#include "oracle.hpp"
#include "optionparser.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

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
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {}
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) {
            cerr << "Option '" << option << "' requires a numeric argument" << endl;
        }
        return option::ARG_ILLEGAL;
    }
};

enum  optionIndex { UNKNOWN, HELP, GRAMMAR, SENTENCE, EPOCH, LAMBDA,
                    MODEL, TEST, SENTENCE_TEST, PRUNING, DELEX, ORACLE, ORACLE_TREE,
                    LABEL_PRUNING, POSITIVE_FEATURES, NO_HASH, FEATURE_FILE, ITEMS};
const option::Descriptor usage[] =
{
    {UNKNOWN, 0,"" , "", option::Arg::None, "USAGE: example [options]\n\n"
     "Options:" },
    {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
    {GRAMMAR,    0,"g", "grammar", Arg::Required, "  --grammar, -g  \nGrammar file." },
    {SENTENCE,    0,"s", "sentence", Arg::Required, "  --sentence, -g  \nSentence file." },
    {SENTENCE_TEST,    0,"t", "sentence_test", Arg::Required, "  --sentence_test, -t  \nSentence test file." },
    {EPOCH,    0,"e", "epochs", Arg::Numeric, "  --epochs, -e  \nNumber of epochs." },
    {LAMBDA,    0,"l", "lambda", Arg::Required, "  --lambda, -e  \nLambda" },
    {MODEL,    0,"m", "model", Arg::Required, "  --model, -m  \nModel path ." },
    {TEST,    0,"t", "test", option::Arg::None, "  --test, -m  \n ." },
    {PRUNING,    0,"p", "pruning", Arg::Required, "  --pruning, -p  \n ." },
    {LABEL_PRUNING,    0,"l", "label_pruning", Arg::Required, "  --label_pruning, -l  \n ." },
    {DELEX,    0,"p", "delex", option::Arg::None, "  --delex, -d  \n ." },
    {ORACLE,    0,"o", "oracle", option::Arg::None, "  --oracle, -o  \n ." },
    {ORACLE_TREE,    0,"z", "oracle_tree", option::Arg::None, "  --oracle_tree, -z  \n ." },
    {POSITIVE_FEATURES,    0,"f", "positive_features", option::Arg::None, "  --positive_features, -f  \n ." },
    {NO_HASH,    0,"", "no_hash", option::Arg::None, "  --no_hash  \n ." },
    {FEATURE_FILE,    0,"", "feature_file", Arg::Required, "  --feature_file  \n ." },
    {ITEMS,    0,"", "items", option::Arg::None, "  --items  \n ." },
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
    if (options[PRUNING]) {
        read_pruning(options[PRUNING].arg, grammar);
    }

    if (options[LABEL_PRUNING]) {
        read_pruning(options[LABEL_PRUNING].arg, grammar);
    }

    grammar->to_word("#START#");
    grammar->to_word("#END#");

    grammar->tag_index.fget("#START#");
    grammar->tag_index.fget("#END#");

    vector<Sentence> *sentences = read_sentence(string(options[SENTENCE].arg));
    for (int i = 0; i < sentences->size(); ++i) {
        Sentence &sentence = (*sentences)[i];
        for (int j = 0; j < sentence.tags.size(); ++j) {
            sentence.int_tags.push_back(
                grammar->tag_index.fget(sentence.tags[j]));
            sentence.preterms.push_back(
                grammar->to_nonterm(sentence.tags[j]));
            sentence.int_words.push_back(
                grammar->to_word(sentence.words[j]));
            sentence.int_deplabels.push_back(
                grammar->to_deplabel(sentence.deplabels[j]));
            // (*sentences)[i].struct_deplabels.push_back(
            //     DepLabel::build((*sentences)[i].deplabels[j],
            //                     grammar->nt_indices[0]));

        }
    }

    FeatureScorer scorer(grammar, options[DELEX], options[POSITIVE_FEATURES], options[NO_HASH]);
    if (options[POSITIVE_FEATURES]) {
        for (int i = 0; i < sentences->size(); ++i) {
            Sentence &sentence = (*sentences)[i];
            for (int j = 0; j < sentence.gold_rules.size(); ++j) {
                scorer.add_positive_rule(sentence, sentence.gold_rules[j]);
            }
        }
    }

    if (options[NO_HASH] && options[FEATURE_FILE])  {
        scorer.read(options[FEATURE_FILE].arg);
    }


    // Make scorer.

    if (options[TEST]) {
        scorer.is_cost_augmented_ = false;
        vector<Sentence> *sentences =
                read_sentence(string(options[SENTENCE_TEST].arg));
        for (int i = 0; i < sentences->size(); ++i) {
            for (int j = 0; j < (*sentences)[i].tags.size(); ++j) {
                (*sentences)[i].int_tags.push_back(
                    grammar->tag_index.fget((*sentences)[i].tags[j]));
                (*sentences)[i].preterms.push_back(
                    grammar->to_nonterm((*sentences)[i].tags[j]));
                (*sentences)[i].int_words.push_back(
                    grammar->to_word((*sentences)[i].words[j]));
                (*sentences)[i].int_deplabels.push_back(
                    grammar->to_deplabel((*sentences)[i].deplabels[j]));
            }
        }

        ifstream in;
        in.open(options[MODEL].arg);
        for (int i = 0; i < scorer.perceptron_.weights.size(); ++i) {
            in >> scorer.perceptron_.weights[i];
        }
        in.close();

        clock_t t = clock();
        cerr << "start" << endl;
        for (int i = 0; i < sentences->size(); ++i) {
            Sentence *sentence = &(*sentences)[i];
            scorer.set_sentence(sentence);
            vector<AppliedRule> best_rules;
            bool success;
            cky(sentence->preterms, sentence->words, sentence->deps,
                *grammar, scorer, &best_rules, !options[ITEMS], &success);

            if (!options[ITEMS]) {
                cout << endl;
            } else {
                if (success) {
                    sentence->gold_rules = best_rules;
                }
                output_sentence(*sentence);
            }
        }

        t = clock() - t;
        cerr << "(" << ((float)t)/CLOCKS_PER_SEC << ")" << endl;
    } else if (options[ORACLE]) {
        cerr << "ORACLE mode";

        for (int i = 0; i < sentences->size(); ++i) {
            Sentence *sentence = &(*sentences)[i];
            vector<AppliedRule> best_rules;
            OracleScorer oracle(&sentence->gold_rules, grammar);
            bool success;
            if (options[ORACLE_TREE]) {
                cky(sentence->preterms, sentence->words, sentence->deps,
                    *grammar, oracle, &best_rules, true, &success);
                cout << endl;
            } else {
                cky(sentence->preterms, sentence->words, sentence->deps,
                    *grammar, oracle, &best_rules, false, &success);
                if (success) {
                    sentence->gold_rules = best_rules;
                }
                output_sentence(*sentence);
            }
        }
    } else {
        scorer.is_cost_augmented_ = true;
        char *temp = 0;
        double lambda = strtod(options[LAMBDA].arg, &temp);
        scorer.perceptron_.set_lambda(lambda);
        cerr << "begin training" << endl;
        cerr << "lambda " << scorer.perceptron_.get_lambda();

        char *temp2 = 0;
        int epochs = strtol(options[EPOCH].arg, &temp2, 10);
        char epoch_char = '1';
        for (int epoch = 0; epoch < epochs; ++epoch) {
            double total_score = 0.0;
            double total_score2 = 0.0;
            int total = 0;
            for (int i = 0; i < sentences->size(); ++i) {
                const Sentence *sentence = &(*sentences)[i];
                scorer.set_sentence(sentence);
                vector<AppliedRule> best_rules;
                bool success;
                double dp_score =
                        cky(sentence->preterms, sentence->words, sentence->deps,
                            *grammar, scorer, &best_rules, false, &success);

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

                scorer.update(good, bad);

                double tscore;
                if (best_rules.size() > 0 && sentence->gold_rules.size() > 0) {
                    total_score += correct2 / (double)sentence->gold_rules.size();
                    total_score2 += correct / (double)best_rules.size();
                    total += 1;
                    tscore = correct / (double)best_rules.size();
                    if (tscore < 0.15) cout << "bad score "
                                            << best_rules.size()
                                            << " "
                                            << correct
                                            << " "
                                            << sentence->words[0]
                                            << endl;
                }


                if (i % 100 == 0) {
                    cout << i << " " << scorer.full_feature_count_ << endl;
                }
                scorer.perceptron_.next_round();
            }
            // scorer.perceptron_.finish();
            cout << "EPOCH: " << epoch << " " << total_score / (float)total
                 << " " << total_score2 / (float)total << endl;
            scorer.perceptron_.write(string(options[MODEL].arg)
                                     + (char)(epoch_char + epoch));

            scorer.write(string(options[MODEL].arg)
                         + (char)(epoch_char + epoch) + ".features");

            // scorer.perceptron_.write(string(options[MODEL].arg)
            //                          + (char)(epoch_char + epoch) + ".average", true);
        }

        // Output model.
        scorer.perceptron_.write(string(options[MODEL].arg));
        scorer.write(string(options[MODEL].arg) + ".features");

    }
}
