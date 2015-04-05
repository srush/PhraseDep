#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include "util.hpp"

enum optionIndex {
    HELP, MODEL, PRUNING, ORACLE, LABEL_PRUNING, DIR_PRUNING, SENTENCES};

const option::Descriptor usage[] = {
    {ORACLE, 0, "o", "oracle", option::Arg::None,
     "  --oracle, -o  \nRun in oracle mode." },

    {PRUNING, 0, "p", "pruning", option::Arg::None,
     "  --pruning, -p  \n ." },

    {SENTENCES, 0, "s", "sentences", Arg::Required,
     "  --sentences, -g  \nCoNLL sentence file." },

    {LABEL_PRUNING, 0, "l", "label_pruning", Arg::Required,
     "  --label_pruning, -l  \n ." },

    {DIR_PRUNING, 0, "", "dir_pruning", option::Arg::None,
     "  --dir_pruning  \n ." },

    {MODEL, 0, "m", "model", Arg::Required,
     "  --model, -m  \nModel file." },

    {HELP,    0, "" , "help", option::Arg::None,
     "  --help  \tPrint usage and exit." },

    {0, 0, 0, 0, 0, 0}
};



int maino(int argc, char* argv[]) {
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


    FullModel model;

    Pruning *pruner = new Pruning(&model.lexicon,
                                  &model.grammar);

    vector<Sentence> *sentences = read_sentences(
        string(options[SENTENCES].arg),
        &model.lexicon, &model.grammar);

    cout << sentences->size() << endl;

    if (options[LABEL_PRUNING]) {
        pruner->read_label_pruning(
            options[LABEL_PRUNING].arg);
    }

    if (options[PRUNING]) {
        pruner->set_pruning(*sentences);
    }

    if (options[DIR_PRUNING]) {
        pruner->set_dir_pruning(*sentences);
    }


    // Make scorer.
    if (!options[ORACLE]) {
        model.scorer.is_cost_augmented_ = false;

        // Read test sentences.
        vector<Sentence> *sentences =
                read_sentences(string(options[SENTENCES].arg),
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
                read_sentences(string(options[SENTENCES].arg),
                               &model.lexicon, &model.grammar);

        for (auto &sentence : *sentences) {
            OracleScorer oracle(&sentence.gold_rules, &model.grammar);
            Parser parser(&sentence, &model.grammar, &oracle, pruner);
            parser.cky(true, false);
        }
    }
}
