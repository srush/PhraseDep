import argparse
import logging
from pystruct.learners import StructuredPerceptron
import json
import sys
import os
import numpy as np

import gzip
import pydecode
import argparse_config
import warnings

import train, lex, format, tree

logger = logging.getLogger("parse")

INF = 1e9

class StructuredMessage(object):
    def __init__(self, message, **kwargs):
        self.message = message
        self.kwargs = kwargs

    def __str__(self):
        return '%s >>> %s' % (self.message, json.dumps(self.kwargs))

def main():
    parser = argparse.ArgumentParser(description='Run parsing experiments.')
    parser.add_argument('--original_rules', type=str, help='Original rule file')
    parser.add_argument('--binarized_rules', type=str, help='Binarized rules')
    parser.add_argument('--training_ps', type=str, help='Lexicalized phrase structure file.')
    parser.add_argument('--training_dep', type=str, help='Dependency parse file.')
    parser.add_argument('--store_hypergraph_dir', type=str, help='Directory to store/load hypergraphs.')
    parser.add_argument('--save_hypergraph', type=bool, help='Construct and save hypergraphs.')
    parser.add_argument('--limit', type=int, help='Amount of sentences to use.')
    parser.add_argument('--test_file', type=str, help='Test files.')
    parser.add_argument('--gold_file', type=str, help='Gold file.')
    parser.add_argument('--model', type=str, help='Weight model.')


    parser.add_argument('config', type=str)
    parser.add_argument('label', type=str)

    print >>sys.stderr, open(sys.argv[1]).read()
    argparse_config.read_config_file(parser, sys.argv[1])

    args = parser.parse_args()
    print args

    output_dir = os.path.join("Data", args.label)
    data_out = os.path.join(output_dir, "mydata.txt")
    print >>sys.stderr, data_out

    # Set up logging.
    logger.setLevel(logging.DEBUG)
    handler = logging.StreamHandler(open(data_out, 'w'))
    logger.addHandler(handler)

    # Load data. 
    print args.training_dep
    print args.training_ps
    X, Y = train.read_data_set(args.training_dep, args.training_ps, args.limit)

    orules = tree.read_original_rules(args.original_rules)
    grammar = format.read_rule_set(open(args.binarized_rules))

    X, Y = zip(*[(x, y) for x, y in zip(X, Y)     
                 if len(x.words) >= 5])
    binarized_Y = [tree.binarize(orules, y) for y in Y]
    model = train.ReconstructionModel(feature_hash=int(1e7),
                                      joint_feature_format="fast")
    model.set_grammar(grammar)


    
    model.initialize(X, binarized_Y)
    if args.save_hypergraph:
        model.set_from_disk(None)
        for i in range(40000):
            print i,
            if len(X[i].words) < 5: continue
            graph, encoder = model.dynamic_program(X[i])

            pydecode.save(args.store_hypergraph_dir+"/graphs%s.graph"%i, graph)
            encoder.save(args.store_hypergraph_dir+"/encoder%s.pickle"%i)

    elif args.test_file:
        model.set_from_disk(None)
        X_test, Y_test = train.read_data_set(args.test_file, args.gold_file, 100)
        w = np.load(args.model)
        for x, y in zip(X_test, Y_test):
            y_hat = model.inference(x, w)
            print tree.unbinarize(y_hat).pprint(1000000)
            print tree.unbinarize(y).pprint(1000000)


    else:
        print "train"
        model.set_from_disk(args.store_hypergraph_dir)
        sp = StructuredPerceptron(model, verbose=1, max_iter=10,
                                  average=False)
        sp.fit(X, binarized_Y)
        np.save(os.path.join(output_dir, "params"), sp.w)
        w = sp.w

        # for i in range(40000):
        #     print i,
        
    # if not args.test_model:
    #     with warnings.catch_warnings():
    #         warnings.simplefilter("ignore")
    #         data = args.corpus
    #         X, Y = read_parse(data, limit=args.limit, length=args.length)

    #         model = FirstOrderModel(feature_hash=int(1e7),
    #                                 joint_feature_format="fast",
    #                                 part_feature_cache=True,
    #                                 joint_feature_cache=True)
    #         sp = StructuredPerceptron(model, verbose=1, max_iter=10,
    #                                   average=True, logger=logger)
    #         sp.fit(X, Y)
    #         np.save(os.path.join(output_dir, "params"),
    #                 sp.w)
    #         w = sp.w
    # else:
    #     X, Y = read_parse(args.corpus,
    #                       limit=args.limit, length=args.length)

    #     model = FirstOrderModel(feature_hash=int(1e7), joint_feature_format="fast",
    #                             use_cache=False)
    #     model.initialize(X, Y)
    #     w = np.load(args.test_model)

    # if args.test_corpus:
    #     X_test, Y_test = read_parse(args.test_corpus, limit=args.limit,
    #                                 length=args.length)
    #     total = 0.0
    #     right = 0.0
    #     for x, y in zip(X_test, Y_test):
    #         y_hat = model.inference(x, w)
    #         total += len(y)
    #         right += np.sum(y == y_hat)

    #         if args.debug:
    #             graph, encoder = model.dynamic_program(x)
    #             features = model.joint_feature(x, y_hat)
    #             print np.sum(w[features.d.keys()])
    #             features = model.joint_feature(x, y)
    #             parts = encoder.transform_structure(y_hat)

    #             for part in parts:
    #                 print part
    #                 parts_features = model.parts_features(x,
    #                                                 np.array([part]))
    #                 assert(parts_features >=0)
    #                 indices = pydecode.model.sparse_feature_indices(parts_features, model.templates(), int(1e7))

    #                 print "scores"
    #                 print indices
    #                 print w[indices]
    #                 print
    #             print features
    #             print np.sum(w[features.d.keys()])
    #             print x
    #             print model._preprocess(x)
    #             print y
    #             print y_hat
    #             print
    #         # # print model.loss(y, y_hat)
    #         # print
    #         #model.joint_feature(y_hat)
    #         # loss += model.loss(y, y_hat)
    #         # total += model.max_loss(y)
    #     print right / float(total)



if __name__ == "__main__":
    main()
