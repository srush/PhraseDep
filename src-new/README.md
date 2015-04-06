- Overview

Dependency parsers are fast, accurate, and produce easy-to-interpret results, but phrase-structure parses are nice too and are required input for many NLP tasks.

The PAD parser produces phrases-after-dependencies. Give it the output of a dependency parser and it will produce the optimal constrained phrase-structure parse.

- How to Use.

> cat deps.conll | ./pad -m pad.model | head


> ./dep_parser sents.txt | ./pad -m pad.model | head

> ./pad --help

PAD: Phrases After Dependencies
USAGE: pad [options]

Options:
--help:              Print this message and exit.
--model, -m:         (Required) Model file.
--sentences, -g:     CoNLL sentence file.
--oracle, -o:        Run in oracle mode.
--pruning, -p:        .
--dir_pruning:        .

- How to Train

> ./padt -m pad.model --sentences sents.conll --annotations sents.ptb

> ./padt --help

PADt: Phrases After Dependencies trainer
USAGE: padt [options]

Options:
--help:             Print this message and exit.
--grammar, -g:      (Required) Grammar file.
--conll, -c:        (Required) CoNLL sentence file.
--model, -m:        (Required) Model file to output.
--annotations, -a   (Required) Gold phrase structure file.
--epochs[=10], -e:  Number of epochs.
--lambda[=0.0001]:  L1 Regularization constant.
--simple_features   Use simple set of features (debugging).

- Details

Cite

- Format

The grammar file has two types of lines. For unary rules:

RULE# 0 X Y 0

For binary rules:

RULE# 1 X Y Z HEAD

The annotation file is only required for training. Each line is of the form:

#RULES
i j k h m r

There is no line break between sentences.
