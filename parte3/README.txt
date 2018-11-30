3ª entrega - Projeto de Sistemas Operativos 
	António Murteira | Filipe Sousa
	Ano letivo de 2018/2019

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
			shell_inp/
				input1.txt
				input2.txt
				input3.txt
				input4.txt
			solver_inp/
				input1.txt
				input2.txt
				input3.txt
				input4.txt				
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
				Makefile
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
				Makefile
			AdvShell/
				CircuitRouter-AdvShell.c
				CircuitRouter-AdvShell.h
				Makefile
			Client/
				CircuitRouter-Client.c
				MakeFile
			inputs.txt
			cleanRes.sh
			MakeFile
		

	Compilação:
		1-Aceder a proj3/.
			~/...$ cd proj3/
		2-Executar o ficheiro de compilação (MakeFile) global, no modo pretendido.
			2.1-Compilar todos os ficheiros:
				~/.../proj2$ make
			2.2-Compilar todos os ficheiros (em modo debug):
				~/.../proj2$ make debug
			2.3-Eliminar os executáveis e ficheiros com extensão ".o":
				~/.../proj2$ make clean


	Execução (SeqSolver/ParSolver):
		1-Aceder a proj2/.
			~/...$ cd proj2/
		2-Executar o programa pretendido.
			2.1-SeqSolver:
				~/...$ ./CircuitRouter-SeqSolver [INPUT_FILENAME]
			2.2-ParSolver:
				~/...$ ./CircuitRouter-ParSolver [INPUT_FILENAME] -t [NUM_THREADS]


	Execução (testes automáticos):
		1-Aceder a proj2/.
			~/...$ cd proj2/
		2-Executar o executável (doTest.sh) com 2 argumentos - número máximo de threads e nome do ficheiro de input.
			~/.../proj2$ ./doTest.sh [MAX_THREADS] [INPUT_FILENAME]


	Características do CPU:
		4 cores: 
			Intel(R) Core(TM) i7-4750HQ CPU @ 2.00GHz
			Clock rate: 1995.392 MHz
		Sistema opetarivo:
			Linux filipe-VirtualBox 4.15.0-38-generic #41-Ubuntu SMP Wed Oct 10 10:59:38 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux