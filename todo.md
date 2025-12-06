./main --mode training --training_config training.cfg --training_output model.dat
./main --mode performance_stats --commander model_based --model_file model.dat --games 10 --performance_output performance.csv
