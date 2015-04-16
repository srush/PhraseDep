#!/bin/bash

work_dir="full_exp"
lbd=$1

mkdir ${work_dir}

#python python/Generator.py 0 --inputf db_dataset/train.withtop.jack --backoff_rule --hm 0 --vm 0 > ${work_dir}/rules

#python python/Generator.py 1 --inputf db_dataset/train.withtop.jack --hm 0 --vm 0 --rulef ${work_dir}/rules > ${work_dir}/parts

cd src-new

./padt --lambda ${lbd} --grammar ../${work_dir}/rules --model ../${work_dir}/model${lbd} --annotations ../${work_dir}/parts --conll ../db_dataset/train_jack_2label  --epochs 5 --simple_features 1

./pad --model ../${work_dir}/model${lbd} --sentences ../db_dataset/red_2labeled_model_k16_test > ../${work_dir}/predict_test${lbd}
./pad --model ../${work_dir}/model${lbd} --sentences ../db_dataset/red_2labeled_model_k16_dev > ../${work_dir}/predict_dev${lbd}


../evalb/EVALB/evalb -p ../evalb/EVALB/COLLINS.prm ../db_dataset/test.withtop ../${work_dir}/predict_test${lbd} > ../${work_dir}/report_test${lbd}
../evalb/EVALB/evalb -p ../evalb/EVALB/COLLINS.prm ../db_dataset/dev.withtop ../${work_dir}/predict_dev${lbd} > ../${work_dir}/report_dev${lbd}
