#!/usr/bin/env gnuplot

set terminal pngcairo size 1000,1000
set output "osc_chain_speedup.png"

set multiplot layout 2,2

set key left

set xrange [1:16]
set x2range [1:16]
set x2tics 8 format ""
set grid x2tics
set yrange [0:8]

set title "short: speedup"
plot \
    "osc_chain_speedup-short.dat" i 0 u "block":"mul" w lp t "MPI" , \
    (x < 4 ? x : 4) lc 0 lt 0 t "target"

unset key

set title "long: speedup"
plot \
    "osc_chain_speedup-long.dat" i 0 u "block":"mul" w lp, \
    (x < 4 ? x : 4) lc 0 lt 0

set yrange [0:*]

set title "short: time[s]"
plot \
    "osc_chain_speedup-short.dat" i 0 u "block":"med" w lp

set title "long: time[s]"
plot \
    "osc_chain_speedup-long.dat" i 0 u "block":"med" w lp

unset multiplot
