#!/bin/bash
MAX_Z=5

for file in inputs/*.txt
do
	zSize=`echo ${file} | cut -d'-' -f4 | cut -c2-`
	if [ ${zSize} -le ${MAX_Z} ]
	then
		./doTest.sh 8 ${file}
		echo Finished testing ${file}.
	fi
done
echo Done testing.
