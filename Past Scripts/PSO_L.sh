#!/bin/bash
#SBATCH --time=0-10:10:00 
#SBATCH --job-name=PSO_ODE_Linear
#SBATCH --partition=general
#SBATCH --nodes=1
#SBATCH --output=./slurm_outputs/linear_out%j.txt
#SBATCH --ntasks-per-node=8
set -e
echo "PSO_S_ODE_Linear"
./PSO_L