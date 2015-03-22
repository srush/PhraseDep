#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

#include "optionparser.h"
#include "features.hpp"
#include "sentence.hpp"
#include "pruning.hpp"
#include "dp.hpp"
#include "oracle.hpp"

#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>

struct Arg: public option::Arg {
    static option::ArgStatus Required(const option::Option& option, bool msg) {
        if (option.arg != 0)
            return option::ARG_OK;

        if (msg) cerr << "Option '" << option << "' requires an argument\n";
        return option::ARG_ILLEGAL;
    }
    static option::ArgStatus Numeric(const option::Option& option, bool msg) {
        char* endptr = 0;
        if (option.arg != 0 && strtol(option.arg, &endptr, 10)) {}
        if (endptr != option.arg && *endptr == 0)
            return option::ARG_OK;

        if (msg) {
            cerr << "Option '" << option
                 << "' requires a numeric argument" << endl;
        }
        return option::ARG_ILLEGAL;
    }
};

enum optionIndex { HELP, GRAMMAR, SENTENCES, EPOCHS, LAMBDA,
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


struct Model {
    // Model() {}

    Model(const option::Option options[])
            : grammar(*read_rule_set(string(options[GRAMMAR].arg))),
              lexicon(),
              scorer(options[SIMPLE_FEATURES]) {
        scorer.set(&lexicon, &grammar);
    }

    template <class Archive>
    void save(Archive &ar) const {
        ar(grammar, lexicon, scorer);
    }

    template <class Archive>
    void load(Archive &ar) {
        ar(grammar, lexicon, scorer);
        scorer.set(&lexicon, &grammar);
    }


    Grammar grammar;
    Lexicon lexicon;
    FeatureScorer scorer;
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
    Model model(options);
    // Grammar *grammar = read_rule_set(string(options[GRAMMAR].arg));
    // Lexicon *lexicon = new Lexicon();
    Pruning *pruner = new Pruning(&model.lexicon, &model.grammar);

    vector<Sentence> *sentences = read_sentences(string(options[SENTENCES].arg),
                                                 &model.lexicon, &model.grammar);
    cout << sentences->size() << endl;

    // FeatureScorer scorer(grammar, lexicon, false,
    //                      false, false, options[SIMPLE_FEATURES],
    //                      false);

    if (options[LABEL_PRUNING]) {
        pruner->read_label_pruning(options[LABEL_PRUNING].arg);
    }

    if (options[PRUNING]) {
        pruner->set_pruning(*sentences);
    }

    if (options[DIR_PRUNING]) {
        pruner->set_dir_pruning(*sentences);
    }

    // Make scorer.
    if (options[SENTENCE_TEST] && !options[ORACLE]) {
        model.scorer.is_cost_augmented_ = false;

        // Read test sentences.
        vector<Sentence> *sentences =
                read_sentences(string(options[SENTENCE_TEST].arg),
                              &model.lexicon, &model.grammar);

        // Read model. Make function.
        ifstream is(string(options[MODEL].arg),
                    std::ios::binary);
        cereal::BinaryInputArchive iarchive(is);
        iarchive(model);


        clock_t t = clock();

        // Start parsing.
        int total_tokens = 0;
        int total_sentences = 0;

        for (auto &sentence : *sentences) {
            total_tokens += sentence.words.size();
            total_sentences += 1;
            model.scorer.set_sentence(&sentence);
            Parser parser(&sentence, &model.grammar, &model.scorer, pruner);
            parser.cky(true, false);
        }

        float sec = (float)(clock() - t) / CLOCKS_PER_SEC;
        cerr << "(" << sec << ")" << endl;
        cerr << total_tokens / sec << endl;
        cerr << total_sentences / sec << endl;
    } else if (options[ORACLE]) {
        cerr << "ORACLE mode";
        vector<Sentence> *sentences =
                read_sentences(string(options[SENTENCE_TEST].arg),
                               &model.lexicon, &model.grammar);

        for (auto &sentence : *sentences) {
            OracleScorer oracle(&sentence.gold_rules, &model.grammar);
            Parser parser(&sentence, &model.grammar, &oracle, pruner);
            parser.cky(true, false);
        }
    } else {
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
        model.scorer.perceptron_.set_lambda(lambda);

        cerr << "Begining training" << endl;
        cerr << "lambda=" << model.scorer.perceptron_.get_lambda() << endl;

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
                model.scorer.perceptron_.next_round();
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
