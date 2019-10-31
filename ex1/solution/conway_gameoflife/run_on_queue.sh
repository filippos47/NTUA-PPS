#!/bin/bash

## Give the Job a descriptive name
#PBS -N run_conway_gameoflife

## Output and error files
#PBS -o run_conway_gameoflife.out
#PBS -e run_conway_gameoflife.err

## How many machines should we get? 
#PBS -l nodes=1:ppn=8

##How long should the job run for?
#PBS -l walltime=00:10:00

## Start 
## Run make in the src folder (modify properly)

module load openmp
cd /home/parallel/parlab13/vitsalis/conway_gameoflife

nthreads=( 1 2 4 6 8 )
sizes=( 64 1024 4096 )

for nthread in "${nthreads[@]}";
do
	for size in "${sizes[@]}";
	do
		export OMP_NUM_THREADS=${nthread};
		./Game_Of_Life ${size} 1000;
	done
done

