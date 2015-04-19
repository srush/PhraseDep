#!/bin/bash

work_dir="debug_exp"

#mkdir ${work_dir}

#python python/Generator.py 0 --inputf db_dataset/train.withtop.jack.1000 --backoff_rule --hm 0 --vm 0 > ${work_dir}/rules

#python python/Generator.py 1 --inputf db_dataset/train.withtop.jack.1000 --hm 0 --vm 0 --rulef ${work_dir}/rules > ${work_dir}/parts

cd src-new

./padt --grammar ../${work_dir}/rules --lambda $1 --model ../${work_dir}/model --annotations ../${work_dir}/parts --conll ../db_dataset/train_jack_2label.1000  --epochs 1 --simple_features 1

./pad --model ../${work_dir}/model --sentences ../db_dataset/red_2labeled_model_k16_dev > ../${work_dir}/predict


../evalb/EVALB/evalb -p ../evalb/EVALB/COLLINS.prm ../db_dataset/dev.withtop ../${work_dir}/predict > ../${work_dir}/report
