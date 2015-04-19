#!/bin/bash

work_dir="old_exp"
mkdir -p ${work_dir}
python python/Generator.py 0 --inputf old_db_dataset/train.withtop.jack.1000 --backoff_rule --hm 0 --vm 0 > ${work_dir}/rules
python python/Generator.py 1 --inputf old_db_dataset/train.withtop.jack.1000 --rulef ${work_dir}/rules --hm 0 --vm 0 > ${work_dir}/parts
cd src
./parser --simple_features --lambda $1 --sentence ../${work_dir}/parts --grammar ../${work_dir}/rules --model ../${work_dir}/model$1fin --epochs=1
./parser --simple_features --sentence ../${work_dir}/parts --sentence_test ../old_db_dataset/red_2labeled_model_k16_dev.parts --grammar ../${work_dir}/rules --test --model ../${work_dir}/model$1fin1 > ../${work_dir}/predict$1pred
../evalb/EVALB/evalb -p ../evalb/EVALB/COLLINS.prm ../old_db_dataset/dev.withtop ../${work_dir}/predict$1pred > ../${work_dir}/predict$1pred.report
