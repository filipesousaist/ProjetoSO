	2ª entrega projeto Sistemas Operativos. 
	António Murteira - Filipe Sousa
	Ano letivo 2018/2019

	Estrutura de diretórios:

	proj2/
		inputs/
			generate.py
			random-x32-y32-z3-n64.txt
			random-x32-y32-z3-n96.txt
			random-x48-y48-z3-n48.txt
			random-x48-y48-z3-n64.txt
			random-x64-y64-z3-n48.txt
			random-x64-y64-z3-n64.txt
			random-x128-y128-z3-n64.txt
			random-x128-y128-z3-n128.txt
			random-x128-y128-z5-n128.txt
			random-x256-y256-z3-n256.txt
			random-x256-y256-z5-n256.txt
			random-x512-y512-z7-n512.txt
		lib/
			commandlinereader.c
			commandlinereader.h
			list.c
			list.h
			pair.c
			pair.h
			queue.c
			queue.h
			timer.h
			types.h
			utility.h
			vector.c
			vector.h
		ParSolver/
			CircuitRouter-ParSolver.c
			coordinate.c
			coordinate.h
			grid.c
			grid.h
			maze.c
			maze.h
			router.c
			router.h
			thread.c
			thread.h
			MakeFile
		SeqSolver/
			CircuitRouter-SeqSolver.c
			coordinate.c
			coordinate.h
			grid.c
			grid.h
			maze.c
			maze.h
			router.c
			router.h
			MakeFile
		SimpleShell/
			CircuitRouter-SimpleShell.c
			MakeFile
		doTest.sh
		MakeFile
		.speedups.csv

	Compilação:
		1-Aceder a proj2/.
			~/...$ cd proj2/
		2-Executar o ficheiro de compilação (MakeFile) global.
			~/.../proj2$ make

	Execução:
		1-Aceder a proj2/.
			~/...$ cd proj2/
		2-Executar o executável (doTest) com 2 argumentos, número de threads e nome do ficheiro de input.
			~/.../proj2$ ./doTest.sh [NR_DE_THREADS] [INPUT_FILE_NAME]

	Características do CPU:
		
		


