#!/bin/zsh

export LC_NUMERIC=en_US.UTF-8
declare -A times

export OMP_SCHEDULE=static
export OMP_PROC_BIND=true
repeat=2

function run {
    n=$1
    steps=$2
    printf "# n=$n steps=$steps repeat=$repeat\n"
    printf '"block"'
    for b in gcc icc ; do
        for s in s t ; do
            for t in med mul ; do
                printf "\t\"$b-$s-$t\""
            done
        done
    done
    for block in 1 2 4 8 16 32 64; do
        printf '\n%d' $block
        for build in gcc-4.7 intel-linux ; do
            bench="bin/$build/release/osc_chain_1d"
            for split in 1 0 ; do
                med=$($bench $n $block $steps $repeat $split | tail -1 | awk '{print $4}')
                times[$build-$split-$block]=$med
                speedup=$((${times[$build-$split-1]}/$med))
                printf '\t%f\t%f' $med $speedup
            done
        done
    done
    printf '\n\n\n'
}

run 4096 1024 | tee osc_chain_speedup-short.dat
run 524288 10 | tee osc_chain_speedup-long.dat
