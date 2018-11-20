#!/bin/bash

for f in inputs/*txt.*; do
	if [ -e $f ]; then
		rm $f
		echo Removed $f
	fi
done

for f in solver_inp/*txt.*; do
	if [ -e $f ]; then
		rm $f
		echo Removed $f
	fi
done

echo Done.