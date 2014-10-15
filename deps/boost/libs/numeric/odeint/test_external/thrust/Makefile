# Copyright 2010-2013 Mario Mulansky
# Copyright 2010-2012 Karsten Ahnert
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or
# copy at http://www.boost.org/LICENSE_1_0.txt)



CUDA_ROOT = /usr/local/cuda-5.0

CC = gcc
CXX = gcc
NVCC = $(CUDA_ROOT)/bin/nvcc

INCLUDES += -I$(BOOST_ROOT) -I$(THRUST_ROOT) -I$(CUDA_ROOT)/include -I../../../../..

NVCCFLAGS = -O3 $(INCLUDES) --compiler-bindir=/opt/gcc4.6.2/bin/

LDLIBS = -lcudart -lstdc++
LDFLAGS = -L$(CUDA_ROOT)/lib64

%.co : %.cu
	$(NVCC) $(NVCCFLAGS) -o $@ -c $<


all : check_thrust


check_thrust : check_thrust.co
	$(CC) -o check_thrust $(LDFLAGS) $(LDLIBS) check_thrust.co
check_thrust.co : check_thrust.cu

clean :
	-rm *~ *.o *.co check_thrust

