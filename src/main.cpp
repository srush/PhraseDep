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
                    LABEL_PRUNING, POSITIVE_FEATURES, NO_HASH, FEATURE_FILE, ITEMS,
                    NO_DEP, LIMIT, LOWER_LIMIT, SIMPLE_FEATURES, DIR_PRUNING};
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
    {PRUNING,    0,"p", "pruning", option::Arg::None, "  --pruning, -p  \n ." },
    {LABEL_PRUNING,    0,"l", "label_pruning", Arg::Required, "  --label_pruning, -l  \n ." },
    {DELEX,    0,"p", "delex", option::Arg::None, "  --delex, -d  \n ." },
    {ORACLE,    0,"o", "oracle", option::Arg::None, "  --oracle, -o  \n ." },
    {ORACLE_TREE,    0,"z", "oracle_tree", option::Arg::None, "  --oracle_tree, -z  \n ." },
    {POSITIVE_FEATURES,    0,"f", "positive_features", option::Arg::None, "  --positive_features, -f  \n ." },
    {NO_HASH,    0,"", "no_hash", option::Arg::None, "  --no_hash  \n ." },
    {FEATURE_FILE,    0,"", "feature_file", Arg::Required, "  --feature_file  \n ." },
    {ITEMS,    0,"", "items", option::Arg::None, "  --items  \n ." },
    {NO_DEP,    0,"", "no_dep", option::Arg::None, "  --no_dep  \n ." },
    {LIMIT,    0,"", "limit", Arg::Numeric, "  --limit  \n ." },
    {LOWER_LIMIT,    0,"", "lower_limit", Arg::Numeric, "  --lower_limit  \n ." },
    {SIMPLE_FEATURES,    0,"", "simple_features", option::Arg::None, "  --simple_features  \n ." },
    {DIR_PRUNING,    0,"", "dir_pruning", option::Arg::None, "  --dir_pruning  \n ." },
    {UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
                                                  "  example --unknown -- --this_is_no_option\n"
     "  example -unk --plus -ppp file1 file2\n" },
    {0,0,0,0,0,0}
};


void process_sentence(Sentence *sentence,
                      Grammar *grammar) {
    for (int j = 0; j < sentence->tags.size(); ++j) {
        sentence->int_tags.push_back(
            grammar->tag_index.fget(sentence->tags[j]));
        sentence->preterms.push_back(
            grammar->to_nonterm(sentence->tags[j]));
        sentence->int_words.push_back(
            grammar->to_word(sentence->words[j]));
        sentence->int_deplabels.push_back(
            grammar->to_deplabel(sentence->deplabels[j]));
    }
}

int main(int argc, char* argv[]) {
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
    if (options[LABEL_PRUNING]) {
        read_label_pruning(options[LABEL_PRUNING].arg, grammar);
    }

    grammar->to_word("#START#");
    grammar->to_word("#END#");

    grammar->tag_index.fget("#START#");
    grammar->tag_index.fget("#END#");

    vector<Sentence> *sentences = read_sentence(string(options[SENTENCE].arg));
    for (int i = 0; i < sentences->size(); ++i) {
        process_sentence(&(*sentences)[i], grammar);
    }

    FeatureScorer scorer(grammar, options[DELEX],
                         options[POSITIVE_FEATURES], options[NO_HASH], options[SIMPLE_FEATURES]);
    if (options[POSITIVE_FEATURES]) {
        for (auto sentence : *sentences) {
            for (auto rule : sentence.gold_rules) {
                scorer.add_positive_rule(sentence, rule);
            }
        }
    }


    if (options[PRUNING]) {
        cerr << "PRUNING" << endl;
        grammar->pruning = true;
        for (auto sentence : *sentences) {
            for (auto rule : sentence.gold_rules) {
                int head = sentence.preterms[rule.h];
                grammar->rule_head_tags[rule.rule][head] = 1;
            }
        }
    }

    if (options[DIR_PRUNING]) {
        cerr << "DIR PRUNING" << endl;
        grammar->dir_pruning = true;

        for (auto sentence : *sentences) {
            int n = sentence.words.size();
            vector<vector<int>> right_deps(n), left_deps(n);
            vector<int> seen_left(n, 0);
            vector<int> seen_right(n, 0);
            for (int i = 0; i < n; ++i) {
                int d = sentence.deps[i];
                if (d == -1) continue;
                if (i < d)
                    left_deps[d].insert(left_deps[d].begin(), i);
                else
                    right_deps[d].push_back(i);
            }

            for (auto rule : sentence.gold_rules) {
                int head = sentence.preterms[rule.h];
                // Skip unary.
                if (rule.h == rule.m) continue;

                int mod = sentence.preterms[rule.m];

                int left_prev=0, right_prev=0;
                if (seen_left[rule.h] == 0) {
                    left_prev = 1;
                } else if (sentence.preterms[left_deps[rule.h][seen_left[rule.h]-1]] ==
                           grammar->to_nonterm(",")) {
                    left_prev = 2;
                }

                if (seen_right[rule.h] == 0) {
                    right_prev = 1;
                } else if (sentence.preterms[right_deps[rule.h][seen_right[rule.h]-1]] ==
                           grammar->to_nonterm(",")) {
                    right_prev = 2;
                }

                if (rule.m < rule.h) {
                    if (seen_right[rule.h] == right_deps[rule.h].size())
                        continue;
                    int right_mod = sentence.preterms[right_deps[rule.h][seen_right[rule.h]]];
                    DirPrune prune(head, mod, right_mod,
                                   (seen_left[rule.h] == 0),
                                   (seen_right[rule.h] == 0));
                    grammar->dir_pruner[prune].first += 1;
                    seen_left[rule.h] += 1;
                } else {
                    if (seen_left[rule.h] == left_deps[rule.h].size())
                        continue;
                    int left_mod = sentence.preterms[left_deps[rule.h][seen_left[rule.h]]];
                    DirPrune prune(head, left_mod, mod,
                                   (seen_left[rule.h] == 0),
                                   (seen_right[rule.h] == 0));
                    grammar->dir_pruner[prune].second += 1;
                    seen_right[rule.h] += 1;
                }
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
            process_sentence(&(*sentences)[i], grammar);
        }

        ifstream in;
        in.open(options[MODEL].arg);
        for (int i = 0; i < scorer.perceptron_.weights.size(); ++i) {
            in >> scorer.perceptron_.weights[i];
        }
        in.close();

        clock_t t = clock();
        int total_tokens = 0;
        int total_sentences = 0;
        cerr << "start " << sentences->size() << endl;
        for (int i = 0; i < sentences->size(); ++i) {
            Sentence *sentence = &(*sentences)[i];
            if (options[LIMIT]) {
                char *temp2 = 0;
                int limit = strtol(options[LIMIT].arg, &temp2, 10);
                if (sentence->words.size() > limit) {
                    continue;
                }
            }

            if (options[LOWER_LIMIT]) {
                char *temp2 = 0;
                int limit = strtol(options[LOWER_LIMIT].arg, &temp2, 10);
                if (sentence->words.size() < limit) {
                    continue;
                }
            }
            total_tokens += sentence->words.size();
            total_sentences += 1;
            scorer.set_sentence(sentence);
            vector<AppliedRule> best_rules;
            bool success;
            if (options[NO_DEP]) {
                cky_full(sentence->preterms, sentence->words,
                         *grammar, scorer, &best_rules, false,
                         &success);
            } else {
                cky2(sentence->preterms, sentence->words, sentence->deps,
                     *grammar, scorer, &best_rules, !options[ITEMS], &success, false);
            }

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
        float sec = ((float)t)/CLOCKS_PER_SEC;
        cerr << "(" << sec << ")" << endl;
        cerr << total_tokens / (float)sec << endl;
        cerr << total_sentences / (float)sec << endl;
    } else if (options[ORACLE]) {
        cerr << "ORACLE mode";

        vector<Sentence> *sentences =
                read_sentence(string(options[SENTENCE_TEST].arg));
        for (int i = 0; i < sentences->size(); ++i) {
            process_sentence(&(*sentences)[i], grammar);
        }

        for (int i = 0; i < sentences->size(); ++i) {
            Sentence *sentence = &(*sentences)[i];
            vector<AppliedRule> best_rules;
            OracleScorer oracle(&sentence->gold_rules, grammar);
            bool success;
            if (options[NO_DEP]) {
                cky_full(sentence->preterms, sentence->words,
                         *grammar, oracle, &best_rules, options[ORACLE_TREE],
                         &success);

            } else {
                cky2(sentence->preterms, sentence->words, sentence->deps,
                    *grammar, oracle, &best_rules, options[ORACLE_TREE],
                     &success, false);
            }

            if (options[ORACLE_TREE]) {
                cout << endl;
            } else {
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
                if (sentence->words.size() <= 5) continue;
                scorer.set_sentence(sentence);
                vector<AppliedRule> best_rules;
                bool success;
                double dp_score;
                dp_score = cky(sentence->preterms, sentence->words, sentence->deps,
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
