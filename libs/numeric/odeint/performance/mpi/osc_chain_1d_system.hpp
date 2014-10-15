/* Boost libs/numeric/odeint/performance/openmp/osc_chain_1d_system.hpp

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 stronlgy nonlinear hamiltonian lattice

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <vector>
#include <cmath>
#include <iostream>

#include <boost/math/special_functions/sign.hpp>
#include <boost/numeric/odeint/external/mpi/mpi.hpp>

namespace checked_math {
    inline double pow( double x , double y )
    {
        if( x==0.0 )
            // 0**y = 0, don't care for y = 0 or NaN
            return 0.0;
        using std::pow;
        using std::abs;
        return pow( abs(x) , y );
    }
}

double signed_pow( double x , double k )
{
    using boost::math::sign;
    return checked_math::pow( x , k ) * sign(x);
}

struct osc_chain {
    const double m_kap, m_lam;
    osc_chain( const double kap , const double lam )
        : m_kap( kap ) , m_lam( lam ) { }

    void operator()( const boost::numeric::odeint::mpi_state< std::vector<double> > &q ,
                           boost::numeric::odeint::mpi_state< std::vector<double> > &dpdt ) const
    {
        const bool have_left = q.world.rank() > 0;
        const bool have_right = q.world.rank() + 1 < q.world.size();
        double q_left = 0, q_right = 0;
        boost::mpi::request r_left, r_right;
        if(have_left)
        {
            q.world.isend(q.world.rank() - 1, 0, q().front());
            r_left = q.world.irecv(q.world.rank() - 1, 0, q_left);
        }
        if(have_right)
        {
            q.world.isend(q.world.rank() + 1, 0, q().back());
            r_right = q.world.irecv(q.world.rank() + 1, 0, q_right);
        }

        double coupling_lr = 0;
        if(have_left)
        {
            r_left.wait();
            coupling_lr = signed_pow( q_left - q()[0] , m_lam-1 );
        }
        const size_t N = q().size();
        for(size_t i = 0 ; i < N-1 ; ++i)
        {
            dpdt()[i] = -signed_pow( q()[i] , m_kap-1 ) + coupling_lr;
            coupling_lr = signed_pow( q()[i] - q()[i+1] , m_lam-1 );
            dpdt()[i] -= coupling_lr;
        }
        dpdt()[N-1] = -signed_pow( q()[N-1] , m_kap-1 ) + coupling_lr;
        if(have_right)
        {
            r_right.wait();
            dpdt()[N-1] -= signed_pow( q()[N-1] - q_right , m_lam-1 );
        }
    }
    //]
};

#endif
