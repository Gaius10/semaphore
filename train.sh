#!/bin/bash

for i in $(seq 1 30); do
    ./main --mode training --commander specimen_model_01
    clear
    printf "Sleeping 10 seconds to avoid overheat..."
    sleep 10
done
