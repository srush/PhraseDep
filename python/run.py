import argparse
import logging
from pystruct.learners import StructuredPerceptron
import json
import sys
import os
import numpy as np
import pydecode
import argparse_config
import train, tree
from grammar import read_rule_set
from tree import read_original_rules
from encoder import LexicalizedCFGEncoder
import pydecode.model

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
    parser.add_argument('--training_ps', type=str,
                        help='Lexicalized phrase structure file.')
    parser.add_argument('--training_dep', type=str,
                        help='Dependency parse file.')
    parser.add_argument('--store_hypergraph_dir', type=str,
                        help='Directory to store/load hypergraphs.')
    parser.add_argument('--save_hypergraph', type=bool,
                        help='Construct and save hypergraphs.')
    parser.add_argument('--limit', type=int, help='Amount of sentences to use.')
    parser.add_argument('--test_file', type=str, help='Test files.')
    parser.add_argument('--gold_file', type=str, help='Gold file.')
    parser.add_argument('--model', type=str, help='Weight model.')
    parser.add_argument('--test_limit', type=int, help='Number of sentences to test on.')
    parser.add_argument('--run_eval', default=False, type=bool, help='')
    parser.add_argument('--test_load', default=False, type=bool, help='')

    parser.add_argument('--debugger', default=False, type=bool, help='')
    parser.add_argument('--oracle', default=False, type=bool, help='run oracle experiments')



    parser.add_argument('config', type=str)
    parser.add_argument('label', type=str)

    print >>sys.stderr, open(sys.argv[1]).read()
    argparse_config.read_config_file(parser, sys.argv[1])

    args = parser.parse_args()
    print args

    if args.debugger:
        from IPython.core import ultratb
        sys.excepthook = ultratb.FormattedTB(color_scheme='Linux', call_pdb=1)

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
    if args.training_dep:
        X, Y = train.read_data_set(args.training_dep,
                                   args.training_ps,
                                   args.limit)

        orules = tree.read_original_rules(open(args.original_rules))
        grammar = read_rule_set(open(args.binarized_rules))
        # for rule in grammar.unary_rules:
        #     print rule
        X, Y = zip(*[(x, y) for x, y in zip(X, Y)
                     if len(x.words) >= 5])
        binarized_Y = [tree.binarize(orules, y) for y in Y]



        model = train.ReconstructionModel(feature_hash=int(1e8),
                                          joint_feature_format="fast",
                                          joint_feature_cache=False,
                                          part_feature_cache=False)
        model.set_grammar(grammar)



        model.initialize(X, binarized_Y)

    if args.test_load:
        print "LOAD"
        graphs = []
        start = memory()

        for i in range(1000 -1):
            if len(X[i].words) < 5:
                continue
            x = X[i]
            path = "%s/graphs%s.graph"%(args.store_hypergraph_dir, i)
            
            encoder = LexicalizedCFGEncoder(x.words, x.tags, grammar)
            pre = memory()
            graph = pydecode.load(path)
            print i, memory() - pre, len(graph.edges), len(X[i].words), memory() - start
            pre = memory()
            encoder.load("%s/encoder%s.pickle"%(
                    args.store_hypergraph_dir, i), graph)
            print i, memory() - pre
            graphs.append((graph, encoder))

    elif args.save_hypergraph:
        print "SAVING"
        import time
        model.set_from_disk(None)
        for i in range(40000):
            if len(X[i].words) < 5:
                continue
            # if len(X[i].words) > 15: continue
            graph, encoder = model.dynamic_program(X[i])

            # Sanity Check

            # print binarized_Y[i]
            # print encoder.structure_path(graph, binarized_Y[i])
            if i % 100 == 0:
                print i
            pydecode.save("%s/graphs%s.graph"%(
                    args.store_hypergraph_dir, X[i].index),
                          graph)
            encoder.save("%s/encoder%s.pickle"%(
                    args.store_hypergraph_dir, X[i].index), graph)
            del graph
            del encoder
    elif args.oracle:
        print "ORACLE"
        trees_out = open(os.path.join(output_dir, "oracle.txt"), 'w')
        model = train.ReconstructionModel(feature_hash=int(1e8), part_feature_cache=False,
                                          joint_feature_cache=False,
                                          joint_feature_format="sparse")
        model.set_grammar(grammar)
        model.initialize(X, binarized_Y)
        model.set_from_disk(None)
        X_test, Y_test = train.read_data_set(
            args.test_file, args.gold_file, args.test_limit)

        w = np.load(args.model)

        # GOLD TREES
        binarized_Y_test = []
        
        for x, orig_y in zip(X_test, Y_test):
            y = tree.binarize(orules, orig_y)
            
            try:
                graph, encoder = model.dynamic_program(x)
                label_values = np.zeros(np.max(graph.labeling) + 1)
                label_values.fill(-1)

                possible = 0
                brackets = set()
                for part in encoder.transform_structure(y):
                    X = grammar.rule_nonterms(part[5])[0]
                    brackets.add((part[0], part[2], X))
                    #print part
                    if tuple(part) in encoder.encoder: 
                        label = encoder.encoder[tuple(part)]
                        label_values[label] = 10.0
                        possible += 1
                print "transform"

                label_weights = np.zeros(len(graph.labeling))
                graph_labels = graph.labeling[graph.labeling != -1]
                parts = encoder.transform_labels(graph_labels)
                weights = []
                for part in parts:
                    X = grammar.rule_nonterms(part[5])[0]
                    if part[1] != part[2] and X[0] != "Z":
                        if (part[0], part[2], X) in brackets:
                            weights.append(2.0)
                        else:
                            weights.append(-2.0)
                    else:
                        weights.append(0.0)
                label_weights = np.zeros(len(graph.labeling))
                label_weights[graph.labeling != -1] = np.array(weights)

                # graph_labels = graph.labeling[graph.labeling != -1]
                # parts = encoder.transform_labels(graph_labels)
                # parts_features = model.parts_features(x, parts)
                # feature_indices = pydecode.model.sparse_feature_indices(parts_features,
                #                                          model.temp_shape,
                #                                          model.offsets,
                #                                          model.feature_hash)
                
                # # Sum the feature weights for the features in each label row.
                # label_weights = np.zeros(len(graph.labeling))
                # label_weights[graph.labeling != -1] = \
                #     np.sum(np.take(w, feature_indices, mode="clip"), axis=1)



                oracle_weights = pydecode.transform(graph, label_values)
                path = pydecode.best_path(graph, oracle_weights + label_weights)
                print "Match", oracle_weights.T * path.v, possible
                y_hat = encoder.transform_path(path)
                print >>trees_out, tree.remove_head(tree.unbinarize(y_hat)) \
                                       .pprint(100000)

            except:
                print >>trees_out, ""
                print "error"
                continue

    elif args.test_file:
        print "TESTING"
        trees_out = open(os.path.join(output_dir, "trees.txt"), 'w')
        model = train.ReconstructionModel(feature_hash=int(1e8), part_feature_cache=False,
                                          joint_feature_cache=False,
                                          joint_feature_format="sparse")
        model.set_grammar(grammar)
        model.initialize(X, binarized_Y)
        model.set_from_disk(None)
        X_test, Y_test = train.read_data_set(
            args.test_file, args.gold_file, args.test_limit)
        w = np.load(args.model)
        # binarized_Y_test = []
        # for i, y in enumerate(Y_test):
        #     print i
        #     binarized_Y_test.append(tree.binarize(orules, y))
        # for x, y in zip(X_test, binarized_Y_test):
        for x in X_test:
            try:
                graph, encoder = model.dynamic_program(x)

                y_hat = model.inference(x, w)
                for part in encoder.transform_structure(y_hat):
                    print part, grammar.rule_nonterms(part[-1]), model.score_part(x, w, part)
                a = w.T * model.joint_feature(x, y_hat)
                # b = w.T * model.joint_feature(x, y)
                # print a, b
                # if b > a: print "FAIL"

                print
                print tree.remove_head(y_hat)

                print
                print tree.remove_head(tree.unbinarize(y_hat))\
                          .pprint()

                # print tree.remove_head(tree.unbinarize(y))\
                #           .pprint()
                # print
                #)\tree.remove_head(
                print >>trees_out, tree.remove_head(tree.unbinarize(y_hat)) \
                                       .pprint(100000)
            except:
                print "error"
                print >>trees_out, ""

    elif args.run_eval:
        test_file = os.path.join(output_dir, "oracle.txt")
        gold_file = args.gold_file
        print "Evaling", test_file, gold_file
        os.system("../evalb/EVALB/evalb  -p ../evalb/EVALB/COLLINS.prm %s %s"%(gold_file, test_file))
    else:
        print "TRAINING"
        model.set_from_disk(args.store_hypergraph_dir)
        sp = StructuredPerceptron(model, verbose=1, max_iter=5,
                                  average=True)
        import warnings
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            sp.fit(X, binarized_Y)
        np.save(os.path.join(output_dir, "params"), sp.w)
        w = sp.w


########MEMORY
import os
_proc_status = '/proc/%d/status' % os.getpid()

_scale = {'kB': 1024.0, 'mB': 1024.0*1024.0,
          'KB': 1024.0, 'MB': 1024.0*1024.0}

def _VmB(VmKey):
    '''Private.
    '''
    global _proc_status, _scale
     # get pseudo file  /proc/<pid>/status
    try:
        t = open(_proc_status)
        v = t.read()
        t.close()
    except:
        return 0.0  # non-Linux?
     # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
    i = v.index(VmKey)
    v = v[i:].split(None, 3)  # whitespace
    if len(v) < 3:
        return 0.0  # invalid format?
     # convert Vm value to bytes
    return float(v[1]) * _scale[v[2]]


def memory(since=0.0):
    '''Return memory usage in bytes.
    '''
    return _VmB('VmSize:') - since


def resident(since=0.0):
    '''Return resident memory usage in bytes.
    '''
    return _VmB('VmRSS:') - since


def stacksize(since=0.0):
    '''Return stack size in bytes.
    '''
    return _VmB('VmStk:') - since

if __name__ == "__main__":
    main()
