#!/usr/bin/env gnuplot

set terminal pngcairo size 1000,1000
set output "osc_chain_speedup.png"

set multiplot layout 2,2

set key left

set xrange [1:64]
set x2range [1:64]
set x2tics 8 format ""
set grid x2tics
set yrange [0:8]

set title "short: speedup"
plot \
    "osc_chain_speedup-short.dat" i 0 u "block":"gcc-s-mul" w lp t "gcc (split)" , \
    "osc_chain_speedup-short.dat" i 0 u "block":"gcc-t-mul" w lp t "gcc (simple)", \
    "osc_chain_speedup-short.dat" i 0 u "block":"icc-s-mul" w lp t "icc (split)" , \
    "osc_chain_speedup-short.dat" i 0 u "block":"icc-t-mul" w lp t "icc (simple)", \
    (x < 4 ? x : 4) lc 0 lt 0 t "target"

unset key

set title "long: speedup"
plot \
    "osc_chain_speedup-long.dat" i 0 u "block":"gcc-s-mul" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"gcc-t-mul" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"icc-s-mul" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"icc-t-mul" w lp, \
    (x < 4 ? x : 4) lc 0 lt 0

set yrange [0:*]

set title "short: time[s]"
plot \
    "osc_chain_speedup-short.dat" i 0 u "block":"gcc-s-med" w lp, \
    "osc_chain_speedup-short.dat" i 0 u "block":"gcc-t-med" w lp, \
    "osc_chain_speedup-short.dat" i 0 u "block":"icc-s-med" w lp, \
    "osc_chain_speedup-short.dat" i 0 u "block":"icc-t-med" w lp

set title "long: time[s]"
plot \
    "osc_chain_speedup-long.dat" i 0 u "block":"gcc-s-med" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"gcc-t-med" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"icc-s-med" w lp, \
    "osc_chain_speedup-long.dat" i 0 u "block":"icc-t-med" w lp

unset multiplot
