To run experiments:

Bother me if there are any issues!

0) Make sure you have the latest pydecode "pip install -U pydecode"
1) Create a directory called experiments/ 
2) Copy ini files over "cp samples/* experiments"
3) Replace {vars} with correct paths.
4) In experiments/ "mkdir Data" and "mkdir Data/{exname}".
5) Run "python ../python/run.py config.init.ini {exname}"
6) Run "python ../python/run.py config.train.ini {exname}"
7) Run "python ../python/run.py config.test.ini {exname}"
8) Run "python ../python/run.py config.eval.ini {exname}"


To setup virtualenv
