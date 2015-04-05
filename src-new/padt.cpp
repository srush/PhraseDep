#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

#include "optionparser.h"
#include "model.hpp"
#include "parse_features.hpp"
#include "sentence.hpp"
#include "pruning.hpp"
#include "inference.hpp"
#include "oracle.hpp"

#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include "util.hpp"

enum optionIndex {
    HELP, GRAMMAR, SENTENCES, EPOCHS, LAMBDA,
    ANNOTATIONS, MODEL, SENTENCE_TEST, PRUNING,
    ORACLE, LABEL_PRUNING, SIMPLE_FEATURES, DIR_PRUNING};

const option::Descriptor usage[] = {
    {GRAMMAR, 0, "g", "grammar", Arg::Required,
     "  --grammar, -g  \nGrammar file." },

    {SENTENCES, 0, "s", "sentences", Arg::Required,
     "  --sentences, -g  \nCoNLL sentence file." },

    {ANNOTATIONS, 0, "s", "annotations", Arg::Required,
     "  --annotations, -g  \nGold annotation file." },

    {SENTENCE_TEST, 0, "t", "sentence_test", Arg::Required,
     "  --sentence_test, -t  \nSentence test file." },

    {EPOCHS, 0, "e", "epochs", Arg::Numeric,
     "  --epochs[=10], -e  \nNumber of epochs." },

    {LAMBDA, 0, "l", "lambda", Arg::Required,
     "  --lambda[=0.0001]  \nL1 Regularization constant. " },

    {MODEL, 0, "m", "model", Arg::Required,
     "  --model, -m  \nModel file." },

    {ORACLE, 0, "o", "oracle", option::Arg::None,
     "  --oracle, -o  \nRun in oracle mode." },

    {SIMPLE_FEATURES, 0, "", "simple_features", option::Arg::None,
     "  --simple_features  \nUse simple set of features (debugging)." },

    {PRUNING, 0, "p", "pruning", option::Arg::None,
     "  --pruning, -p  \n ." },

    {LABEL_PRUNING, 0, "l", "label_pruning", Arg::Required,
     "  --label_pruning, -l  \n ." },

    {DIR_PRUNING, 0, "", "dir_pruning", option::Arg::None,
     "  --dir_pruning  \n ." },

    {HELP,    0, "" , "help", option::Arg::None,
     "  --help  \tPrint usage and exit." },

    {0, 0, 0, 0, 0, 0}
};

int main(int argc, char* argv[]) {
    // For arg parsing.
    argc -= (argc>0);
    argv += (argc>0);
    char *temp = 0;

    option::Stats  stats(usage, argc, argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    if (parse.error())
        return 1;

    if (options[HELP] || argc == 0) {
        option::printUsage(std::cout, usage);
        return 0;
    }

    FullModel model(string(options[GRAMMAR].arg),
                    options[SIMPLE_FEATURES]);
    Pruning *pruner = new Pruning(&model.lexicon, &model.grammar);


    vector<Sentence> *sentences = read_sentences(string(options[SENTENCES].arg),
                                                 &model.lexicon, &model.grammar);
    cout << sentences->size() << endl;

    if (options[LABEL_PRUNING]) {
        pruner->read_label_pruning(options[LABEL_PRUNING].arg);
    }

    if (options[PRUNING]) {
        pruner->set_pruning(*sentences);
    }

    if (options[DIR_PRUNING]) {
        pruner->set_dir_pruning(*sentences);
    }

    if (true) {
        model.scorer.is_cost_augmented_ = true;

        assert(options[ANNOTATIONS]);
        annotate_gold(options[ANNOTATIONS].arg, sentences);

        // Read in epochs and lambda.
        double lambda = 0.0001;
        if (options[LAMBDA]) {
           lambda = strtod(options[LAMBDA].arg, &temp);
        }

        int epochs = 10;
        if (options[EPOCHS]) {
            epochs = strtol(options[EPOCHS].arg, &temp, 10);
        }
        model.scorer.adagrad_.set_lambda(lambda);

        cerr << "Begining training" << endl;
        cerr << "lambda=" << model.scorer.adagrad_.get_lambda() << endl;

        char epoch_char = '1';
        for (int epoch = 0; epoch < epochs; ++epoch) {
            double total_score = 0.0;
            int total = 0;

            for (auto &sentence : *sentences) {
                if (sentence.words.size() <= 5) continue;
                model.scorer.set_sentence(&sentence);
                Parser parser(&sentence, &model.grammar, &model.scorer, pruner);
                parser.cky(false, false);

                // Compute difference.
                int correct2 = model.scorer.update_full(sentence.gold_rules,
                                                        parser.best_rules);
                total_score += correct2 /
                        static_cast<double>(sentence.gold_rules.size());
                total += 1;
                model.scorer.adagrad_.next_round();
            }
            cout << "EPOCH: " << epoch << " "
                 << total_score / (float)total
                 << " " << endl;

            ofstream os(string(options[MODEL].arg) + (char)(epoch_char + epoch),
                        std::ios::binary);
            cereal::BinaryOutputArchive archive(os);
            archive(model);
        }

        // Output model.
        ofstream os(string(options[MODEL].arg),
                    std::ios::binary);
        cereal::BinaryOutputArchive archive(os);
        archive(model);
    }
}
