* Overview

Dependency parsers are fast, accurate, and produce easy-to-interpret results, but phrase-structure parses are nice too and are required input for many NLP tasks.

The PAD parser produces phrases-after-dependencies.

* How to Use.

> cat deps.conll | ./pad -m pad.model | head


> ./dep_parser sents.txt | ./pad -m pad.model | head

> ./pad --help

* How to Train

> ./padt -m pad.model --sentences sents.conll --annotations sents.ptb

> ./padt --help

* Details

Cite