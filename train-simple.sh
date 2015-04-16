#!/bin/bash
cd src
./parser --simple_features --lambda $1 --sentence ../final_english_jack/parts --grammar ../final_english_jack/rules --model ../final_english_jack/model$1fin --epochs=5
./parser --simple_features --sentence ../final_english_jack/parts --sentence_test ../testfiles/english_labeled/red_2labeled_model_k16_test.parts --grammar ../final_english_jack/rules --test --model ../final_english_jack/model$1fin5 > ../final_english_jack/predict$1pred
../evalb/EVALB/evalb -p ../evalb/EVALB/COLLINS.prm /media/lingpenk/Data/PTB/penn_tb_3.0_preprocessed/test.withtop ../final_english_jack/predict$1pred > ../final_english_jack/predict$1pred.report
