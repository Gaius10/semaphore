./main --mode training --commander specimen_model_01 --training_config training.cfg --training_output model.dat
./main --mode performance_stats --commander model_based --model_file model.dat --games 10 --performance_output performance.csv

[ ] Remover carros quando saírem da tela (sem prejudicar calculo das estatisticas)
[ ] Adicionar flag para escolher destino das estatisticas
[ ] Adicionar flag para escolher destino dos modelos treinados
[ ] Adicionar flag para escolher arquivo de entrada do modelo a ser executado
