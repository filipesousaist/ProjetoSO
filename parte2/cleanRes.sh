#!/bin/bash
if [ -e ParSolver/inputs/*.res* ]
then
	rm ParSolver/inputs/*.res*
	echo Removed .res from ParSolver/inputs.
fi
if [ -e solver_inp/*.res* ]
then
	rm solver_inp/*.res*
	echo Removed .res from solver_inp.
fi
echo Done.