#!/bin/bash
for f in ParSolver/inputs/*.res*; do
	if [ -e $f ]; then
		rm $f
		echo Removed $f
	fi
done

for f in solver_inp/*.res*; do
	if [ -e $f ]; then
		rm $f
		echo Removed $f
	fi
done

echo Done.