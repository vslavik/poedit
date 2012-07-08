#!/bin/sh

st=0
for i in 0 1 2 3 4 5 6 7 ; do
  ./test-pipe${EXEEXT} $i \
    || { echo test-pipe.sh: iteration $i failed >&2; st=1; }
done
exit $st
