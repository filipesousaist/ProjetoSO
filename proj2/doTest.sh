#!/bin/bash

# -- doTest.sh --

MAX_THREADS=${1}
FILENAME=${2}
OUTPUTFILE=${FILENAME}.speedups.csv

echo "#threads,exec_time,speedup" > ${OUTPUTFILE}

./CircuitRouter-SeqSolver ${FILENAME}
seqTime=`grep "Elapsed time" ${FILENAME}.res | cut -d'=' -f2 | cut -d' ' -f2`
echo 1S,${seqTime},1 >> ${OUTPUTFILE}

for ((th = 1; th < $MAX_THREADS + 1; th ++))
do
	./CircuitRouter-ParSolver -t ${th} ${FILENAME}
	time=`grep "Elapsed time" ${FILENAME}.res | cut -d'=' -f2 | cut -d' ' -f2`
	speedup=`echo "scale=6; ${seqTime}/${time}" | bc`
	echo ${th},${time},${speedup} >> ${OUTPUTFILE}
done
