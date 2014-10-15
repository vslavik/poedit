/* Boost libs/numeric/odeint/performance/openmp/osc_chain_1d.cpp

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 stronlgy nonlinear hamiltonian lattice in 2d

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>
#include <vector>

#include <boost/numeric/odeint.hpp>
#include <boost/numeric/odeint/external/mpi/mpi.hpp>

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
    boost::mpi::environment env(argc, argv);
    boost::mpi::communicator world;

    size_t N, steps, repeat;
    bool dump;
    options_description desc("Options");
    desc.add_options()
        ("help,h", "show this help")
        ("length", value(&N)->default_value(1024), "length of chain")
        ("steps", value(&steps)->default_value(100), "simulation steps")
        ("repeat", value(&repeat)->default_value(25), "repeat runs")
        ("dump", bool_switch(&dump), "dump final state to stderr (on node 0)")
        ;
    variables_map vm;
    store(command_line_parser(argc, argv).options(desc).run(), vm);
    notify(vm);
    if(vm.count("help"))
    {
        if(world.rank() == 0)
            cerr << desc << endl;
        return EXIT_FAILURE;
    }
    cout << "length\tsteps\tthreads\ttime" << endl;

    accumulator_set< double, stats<tag::mean, tag::median> > acc_time;

    vector<double> p( N ), q( N, 0 );
    if(world.rank() == 0) {
        boost::random::uniform_real_distribution<double> distribution;
        boost::random::mt19937 engine( 0 );
        generate( p.begin() , p.end() , boost::bind( distribution , engine ) );
    }

    typedef vector<double> inner_state_type;
    typedef mpi_state< inner_state_type > state_type;
    typedef symplectic_rkn_sb3a_mclachlan<
              state_type , state_type , double
            > stepper_type;
    state_type p_split( world ), q_split( world );
    split(p, p_split);
    split(q, q_split);

    for(size_t n_run = 0 ; n_run != repeat ; n_run++) {
        cpu_timer timer;
        world.barrier();
        integrate_n_steps( stepper_type() , osc_chain( p_kappa , p_lambda ) ,
                           make_pair( boost::ref(q_split) , boost::ref(p_split) ) ,
                           0.0 , 0.01 , steps );
        world.barrier();
        if(world.rank() == 0) {
            double run_time = static_cast<double>(timer.elapsed().wall) * 1.0e-9;
            acc_time(run_time);
            cout << N << '\t' << steps << '\t' << world.size() << '\t' << run_time << endl;
        }
    }

    if(dump) {
        unsplit(p_split, p);
        if(world.rank() == 0) {
            copy(p.begin(), p.end(), ostream_iterator<double>(cerr, "\t"));
            cerr << endl;
        }
    }

    if(world.rank() == 0)
        cout << "# mean=" << mean(acc_time)
             << " median=" << median(acc_time) << endl;

    return 0;
}

