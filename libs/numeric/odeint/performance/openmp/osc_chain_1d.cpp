/* Boost libs/numeric/odeint/performance/openmp/osc_chain_1d.cpp

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Pascal Germroth
 Copyright 2013 Mario Mulansky

 stronlgy nonlinear hamiltonian lattice in 2d

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <vector>

#include <omp.h>
#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/external/openmp/openmp.hpp>

#include <boost/program_options.hpp>
#include <boost/random.hpp>
#include <boost/timer/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include "osc_chain_1d_system.hpp"

using namespace std;
using namespace boost::numeric::odeint;
using namespace boost::accumulators;
using namespace boost::program_options;

using boost::timer::cpu_timer;

const double p_kappa = 3.3;
const double p_lambda = 4.7;

int main( int argc , char* argv[] )
{
    size_t N, blocks, steps, repeat;
    bool split_range, dump;
    options_description desc("Options");
    desc.add_options()
        ("help,h", "show this help")
        ("length", value(&N)->default_value(1024), "length of chain")
        ("steps", value(&steps)->default_value(100), "simulation steps")
        ("blocks", value(&blocks)->default_value(omp_get_max_threads()), "number of blocks (split) or threads (non-split)")
        ("split", bool_switch(&split_range), "split range")
        ("repeat", value(&repeat)->default_value(25), "repeat runs")
        ("dump", bool_switch(&dump), "dump final state to stderr")
        ;
    variables_map vm;
    store(command_line_parser(argc, argv).options(desc).run(), vm);
    notify(vm);
    if(vm.count("help"))
    {
        cerr << desc << endl;
        return EXIT_FAILURE;
    }
    cout << "length\tsteps\tthreads\ttime" << endl;

    accumulator_set< double, stats<tag::mean, tag::median> > acc_time;

    vector<double> p( N ), q( N, 0 );
    boost::random::uniform_real_distribution<double> distribution;
    boost::random::mt19937 engine( 0 );
    generate( p.begin() , p.end() , boost::bind( distribution , engine ) );

    if(split_range) {
        typedef openmp_state<double> state_type;
        typedef symplectic_rkn_sb3a_mclachlan<
                  state_type , state_type , double
                > stepper_type;
        state_type p_split(blocks), q_split(blocks);
        split(p, p_split);
        split(q, q_split);

        for(size_t n_run = 0 ; n_run != repeat ; n_run++) {
            cpu_timer timer;
            integrate_n_steps( stepper_type() , osc_chain( p_kappa , p_lambda ) ,
                               make_pair( boost::ref(q_split) , boost::ref(p_split) ) ,
                               0.0 , 0.01 , steps );
            double run_time = static_cast<double>(timer.elapsed().wall) * 1.0e-9;
            acc_time(run_time);
            cout << N << '\t' << steps << '\t' << blocks << '\t' << run_time << endl;
        }

        if(dump) {
            unsplit(p_split, p);
            copy(p.begin(), p.end(), ostream_iterator<double>(cerr, "\t"));
            cerr << endl;
        }

    } else {
        typedef vector<double> state_type;
        typedef symplectic_rkn_sb3a_mclachlan<
                  state_type , state_type , double ,
                  state_type , state_type , double ,
                  openmp_range_algebra
                > stepper_type;
        omp_set_num_threads(blocks);

        for(size_t n_run = 0 ; n_run != repeat ; n_run++) {
            cpu_timer timer;
            integrate_n_steps( stepper_type() , osc_chain( p_kappa , p_lambda ) ,
                               make_pair( boost::ref(q) , boost::ref(p) ) ,
                               0.0 , 0.01 , steps );
            double run_time = static_cast<double>(timer.elapsed().wall) * 1.0e-9;
            acc_time(run_time);
            cout << N << '\t' << steps << '\t' << blocks << '\t' << run_time << endl;
        }

        if(dump) {
            copy(p.begin(), p.end(), ostream_iterator<double>(cerr, "\t"));
            cerr << endl;
        }

    }

    cout << "# mean=" << mean(acc_time)
         << " median=" << median(acc_time) << endl;

    return 0;
}
