#!/bin/sh

echo Testing 1 processor
mpirun -np 1 ./bsp_test_send
if test $? -ne 0
  then exit 1
fi

echo Testing 4 processors
mpirun -np 4 ./bsp_test_send
if test $? -ne 0
  then exit 1
fi

echo Testing 7 processors
mpirun -np 7 ./bsp_test_send
if test $? -ne 0
  then exit 1
fi

echo done
