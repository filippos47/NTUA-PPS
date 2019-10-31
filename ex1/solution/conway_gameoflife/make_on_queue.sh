#!/bin/bash

## Give the Job a descriptive name
#PBS -N make_conway_gameoflife

## Output and error files
#PBS -o make_conway_gameoflife.out
#PBS -e make_conway_gameoflife.err

## How many machines should we get? 
#PBS -l nodes=1:ppn=1

##How long should the job run for?
#PBS -l walltime=00:10:00

## Start 
## Run make in the src folder (modify properly)

module load openmp
cd /home/parallel/parlab13/vitsalis/conway_gameoflife/
make clean
make

