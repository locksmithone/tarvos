Include the following lines in the file which contains the main() function:

	#define MAIN_MODULE //defines variables and structs (and not only declare them as extern)
	#include "simm_globals.h"
	#include "tarvos_globals.h"

Generally, within other user files that do not have the main() function, add the following lines:

	#include "simm_globals.h" //in case Tarvos' Kernel functions are used
	#include "tarvos_globals.h" //in case Tarvos' Shell 1 and/or Shell 2 functions are used

------------


Colocar as seguintes linhas no arquivo que cont�m a fun��o main() no programa:

	#define MAIN_MODULE //define as vari�veis e estruturas, e n�o apenas as declara como extern
	#include "simm_globals.h"
	#include "tarvos_globals.h"

Via de regra, em outros arquivos do usu�rio que n�o contenham a fun��o main(), adicionar as seguintes linhas:

	#include "simm_globals.h" //em caso de serem usadas fun��es do Kernel do Tarvos
	#include "tarvos_globals.h" //em caso de serem usadas fun��es do Shell 1 e/ou Shell 2 do Tarvos