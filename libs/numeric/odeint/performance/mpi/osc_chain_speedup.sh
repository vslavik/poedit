#!/bin/bash

bench="bin/clang-linux-3.2/release/threading-multi/osc_chain_1d"

repeat=5
maxnodes=16

function run {
    n=$1
    steps=$2
    for ((nodes=1 ; nodes < $maxnodes ; nodes++)) ; do
        # swap stderr & stdout
        mpirun -np $nodes $bench $n $steps $repeat 3>&1 1>&2 2>&3
    done
}

function run_all {
    printf "n\tsteps\tnodes\ttime\n"
    run 256 1024
    run 4096 1024
    run 4194304 1
}

run_all | tee osc_chain_speedup.dat
