/*
*          Sistema Simulation Machine - Simulacao de Filas
*
*          Versao modificada do smpl usando a filosofia de
*          construcao de modelos de forma reestruturada para
*          utilizar mais adequadamente os recursos computacionais
*
*                  Arquivo <simm_kernel>
*                                        
*  Nota:  fun��o fname, que retorna nome da facility, inclu�da 
*         julho/2005 Marcos Portnoi
*		  
*		  fun��o mname, que retorna o nome do modelo, inclu�da
*		  julho/2005 Marcos Portnoi
*
*		  fun��o getFacMaxQueueSize, que retorna tamanho m�ximo da fila da
*		  facility (para fila �nica), inclu�da
*		  julho/2005 Marcos Portnoi
*
*		  fun��o enqueuep modificada para enfileirar os tokens por ordem
*		  de prioridade (prioridade mais alta primeiro)
*		  10.Nov.2005 Marcos Portnoi
*
*		  fun��o setFacUp, que coloca a facility em estado operacional (up), inclu�da
*		  28.Dez.2005 Marcos Portnoi
*
*	  	  fun��o setFacDown, que coloca a facility em estado n�o-operacional (down), inclu�da
*		  28.Dez.2005 Marcos Portnoi
*
*		  fun��o getFacUpStatus, que retorna o estado operacional ou n�o da facility, inclu�da
*		  28.Dez.2005 Marcos Portnoi
*
*		  fun��o purgeFacQueue, que esvazia a fila de uma facility, inclu�da
*		  04.Jan.2006 Marcos Portnoi
*
*		  fun��o causep:  teste para cadeia de eventos vazia inclu�do
*		  05.Jan.2006 Marcos Portnoi
*
*		  fun��o preemptp corrigida de forma que tokens bloqueadas sejam colocadas em fila ao final
*		  de outras tokens de mesma prioridade; tokens suspensas s�o colocadas antes de outras tokens
*		  de mesma prioridade.  O incremento para o contador de preemp��es tamb�m foi inclu�do.
*		  08.Jan.2006 Marcos Portnoi
*
*		  fun��es requestp e preemptp alteradas para checar o estado da facility (up/down) antes de
*		  processar a token; retornam (0) se facility tem servidor livre; (1) se token foi colocada em
*		  fila; (2) se facility est� down.
*		  Tamb�m incrementam o contador de tokens descartadas para cada chamada em que a facility estiver down.
*		  10.Jan.2006 Marcos Portnoi
*
*		  fun��es facility, enqueuep_preempt, enqueuep e schedulep modificadas; todas as inst�ncias de malloc tiveram
*		  o argumento alterado para indicar apenas o conte�do da pr�pria vari�vel tipo ponteiro (ex: aux = malloc(sizeof *aux)).  Para compacta��o do c�digo
*		  24.Abr.2006 Marcos Portnoi
*
*		  fun��o resetf() corrigida para inicializar todos os contadores e acumuladores estat�sticos das facilities e servidores (checar detalhes na fun��o)
*		  05.Nov.2006 Marcos Portnoi
*
*		  fun��o cancelp_tkn modificada:  se a token passada como par�metro n�o for encontrada, fun��o retorna NULL, e n�o mais interrompe execu��o com erro.
*		  10.Nov.2006 Marcos Portnoi
*
*		  fun��o cancelp_ev inclu�da, que � basicamente uma c�pia da fun��o cancelp_tkn, mas recebendo como par�metro o n�mero do evento.
*		  10.Nov.2006 Marcos Portnoi
*
*
*		  alguns nomes de ponteiros alterados de 'prior' para 'previous',
*		  a fim de n�o causar confus�o com "prioridade".
*
* Copyright (C) 2002, 2005, 2006, 2007 Marcos Portnoi, Sergio F. Brito
*
* This file is part of TARVOS Computer Networks Simulator.
*
* TARVOS Computer Networks Simulator is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* TARVOS Computer Networks Simulator is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with TARVOS Computer Networks Simulator.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Este arquivo contem as funcoes principais para a simulacao de modelos
   conforme a estrategia de simulacao do smpl. Os nomes originais das
   funcoes smpl sao preservados no sentido de permitir que modelos
   ja executados no smpl sejam tambem executados atraves desta nova
   versao
*/

/* Um recurso excluido desta nova versao foi o trace, que antigamente
   era ativado pela variavel mtr. A razao para isto fundamenta-se em que
   atualmente a maioria dos ambientes de desenvolvimento tem a
   possibilidade de acompanhamento passo-a-passo
*/

/* Um ponto importante e a construcao das estruturas de simulacao
   atraves de armazenamento dinamico, diferentemente da versao anterior
   que basicamente tratava todo o armazenamento de forma estatica
   matricialmente.
*/

/* Assim serao definidas as seguintes estruturas para manipulacao da simulacao:
		1. Uma lista simplesmente encadeada para as facilities
		2. Uma lista duplamente encadeada para os eventos (tokens)
*/

/* Neste sentido a cadeia de eventos e uma lista duplamente encadeada
   onde o inicio da lista � o proximo tempo a ser processado e novas
   tokens serao inseridas a partir do final da lista minizando a busca.
   Ou seja, nesta versao a busca nao e heap. Mas como a busca comeca
   pelo final provavelmente ela sera rapida. Pode ser testado atraves de
   um contador verificando a media de buscas feitas para insercao do novo
   tempo */

/* Este programa trabalha com um sistema de prioridade de processamento da
   token onde o maior valor num�rico de prioridade tem tambem a maior prioridade de
   execu��o. Assim, uma token com prioridade 5 devera ser processada antes de tokens com prioridade
   menores do que cinco. */

/* Uma mudanca filosofica no funcionamento do simulador foi fundamental para
   adapta-lo aos objetivos de simulacao contemplando um apontador da estrutura
   da token contendo informacoes de processamento ao longo de seu encaminhamento
   Na versao do smpl o processamento do Release era efetuado da seguinte forma:
	 1. Libera o servidor e atualiza estatisticas
	 2. Verifica se tem fila
	 3. Se sim retira a primeira token da fila e atualiza as informacoes da facility
	    sobre a fila
	 4. Coloca a token retirada da fila como o primeiro evento da cadeia para ser
	    tratado novamente desta vez entrando na facility direto pois ela estara vazia.
		Para tanto utiliza uma estrategia (muito interessante) de armazenamento do evento
		associado a esta token, a recuperacao dele e a continuidade do processamento
		como se nada tivesse ocorrido.
		Problema:  se as informacoes da estrutura da token forem atualizadas como
		se ela ja tivesse passado pela fila na verdade havera um novo processamento
		de consequ�ncias imprevis�veis.
	 Na nova versao 'simm' o processamento do Release acontece da seguinte forma:
	 1. Libera o servidor e atualiza estatisticas
	 2. Verifica se tem fila
	 3. Se sim retira a primeira token da fila e atualiza as informacoes da facility e
	    do servidor; coloca a token imediatamente em processamento, diretamente da fun��o.
	 4. Escalona o termino do servico normalmente atraves de informacoes contidas
	    na propria token.
	 Consequencia: ao ser chamada a funcao requestp, mais informa��es ter�o de ser passadas para ela
	 do que a fun��o request original (SMPL); se esta token precisar ser enfileirada, ao ser
	 retirada da fila o seu processamento dever� continuar normalmente.
*/

#include "simm_globals.h"

#define pl 58        /* printer page length   (lines used   */
#define sl 23        /* screen page length     by 'smpl')   */
#define FF 12        /* form feed                           */

//Prototypes das fun��es locais static
static void resetf();
static void enqueuep (int f, int tkn, TOKEN *tkp, int pri, double te, int ev);
static void enqueuep_preempt (int f, int tkn, TOKEN *tkp, int pri, double te, int ev);
static struct facilit *rept_page(struct facilit *f);
static int purgeFacQueue(int f);

//Variaveis de processamento locais

static int sn = 0;	/* sn -> simulation number
	     Esta variavel � o indexador da estrutura simul que define
		 a parametrizacao de cada simulacao onde permitira que se
		 execute mais de uma simulacao por modelo. A ideia e que
		 a cada nova simulacao a estrutura de numeros aleatorios
		 seja modificada para gerara resultados diferentes. O
		 numero de simulacoes e limitado pelo MAX_SIMULATIONS */

static int
	event,		/* current simulation event (???) possivelmente sem uso */
    token,		/* last token dispatched */
    tr,	    	/* event trace flag                    */
    mr,			/* monitor activation flag             */
    lft=sl;		/* lines left on current page/screen   */

static double
	clock,	    /* current simulation time */
	start;		/* simulation interval start time */

static FILE
  //these definitions are not compatible with some compilers;
  //put them inside a function, such as simm()
  //*display=stdout,	/* screen display file                 */
  //*opf=stdout;	    /* current output destination          */
  *display,	/* screen display file                 */
  *opf;	    /* current output destination          */

FILE *fp;	/* fp � para o depurador*/

/*---------------  INITIALIZE SIMULATION SUBSYSTEM  ------------------
*
* � poss�vel haver v�rias inst�ncias de simula��o para um mesmo modelo, bastando
* para isso chamar a fun��o simm para cada inst�ncia.  A fun��o simm cria um vetor de
* simula��es, e o �ndice deste vetor � incrementado para cada chamada da fun��o.
* Esta � na verdade a implementa��o, em c�digo procedural, de cada simula��o como
* um objeto da classe sim (ou simul), onde aqui o sim ou simul � uma estrutura de dados
* contendo os ponteiros necess�rios para cada simula��o individual.
* A gera��o das estat�sticas combinadas de todas as simula��es (com intervalo de
* confian�a) n�o est� ainda implementado de modo autom�tico, portanto cabe ao usu�rio
* faz�-lo.
* Por default, a primeira simula��o configura o seed para o gerador de n�meros aleat�rios
* como 1.  As simula��es subsequentes, se definidas v�rias inst�ncias do simm, usam
* seeds consecutivos.
* O seed inicial pode ser definido pelo usu�rio, bastando chamar a fun��o stream(n) depois
* de chamada a fun��o simm.
*
* Como alternativa, pode-se usar uma �nica inst�ncia de simula��o e execut�-la v�rias vezes,
* mudando o seed do gerador de n�meros aleat�rios manualmente a fim de obter amostras diferentes.
*
*/
void simm(int m, char *s)
{
	static int rns = 1;	/* rns define o numero do stream aleatorio que sera usado.
						   Como rns e uma variavel tipo static ela so eh inicializada
						   uma vez, ou seja, se o smpl for chamada de novo, a rotina
						   que incrementa rns sera ativada e o simulador executara
						   com uma nova sequencia aleatoria */

	display=stdout;	//screen display file
	opf=stdout;	    //current output destination
	
	sn = sn + 1;		/* A simulacao comeca com o elemento 1 do vetor de simula��es.  A cada
						   vez que a rotina simm for chamada, o �ndice � incrementado
						   e cria-se uma nova inst�ncia de simula��o */

	if ( sn > MAX_SIMULATIONS )
	{
		printf ("\nError - simm - number of simulations exceeds MAX_SIMULATIONS");
		exit (1);
	}

	/* Inicializando as estruturas de cadeia de eventos e de faciltys e apontando para o primeiro
	   elemento */

	strcpy(sim[sn].name, s);

	sim[sn].evc_begin = NULL;
	sim[sn].evc_end = NULL;
	sim[sn].fct_begin = NULL;
	sim[sn].fct_number = 0;

	clock=start= 0.0;		/* Tempo de simulacao e intervalo serao iguais s o start nao for
												 modificado ao longo da simulacao */
	event = 0;

	rns = stream(rns); rns = ++rns>15? 1:rns; /* set random no stream */
}

/*---------------  INITIALIZE SIMULATION SUBSYSTEM:  TARVOS version  ------------------
*
* Esta fun��o simplesmente chama a fun��o similar simm(), do kernel SimM.  Est� aqui para fins de compatibilidade.
*
* � poss�vel haver v�rias inst�ncias de simula��o para um mesmo modelo, bastando
* para isso chamar a fun��o simm para cada inst�ncia.  A fun��o simm cria um vetor de
* simula��es, e o �ndice deste vetor � incrementado para cada chamada da fun��o.
* Esta � na verdade a implementa��o, em c�digo procedural, de cada simula��o como
* um objeto da classe sim (ou simul), onde aqui o sim ou simul � uma estrutura de dados
* contendo os ponteiros necess�rios para cada simula��o individual.
* A gera��o das estat�sticas combinadas de todas as simula��es (com intervalo de
* confian�a) n�o est� ainda implementado de modo autom�tico, portanto cabe ao usu�rio
* faz�-lo.
* Por default, a primeira simula��o configura o seed para o gerador de n�meros aleat�rios
* como 1.  As simula��es subsequentes, se definidas v�rias inst�ncias do simm, usam
* seeds consecutivos.
* O seed inicial pode ser definido pelo usu�rio, bastando chamar a fun��o stream(n) depois
* de chamada a fun��o simm.
*
* Como alternativa, pode-se usar uma �nica inst�ncia de simula��o e execut�-la v�rias vezes,
* mudando o seed do gerador de n�meros aleat�rios manualmente a fim de obter amostras diferentes.
*
*/
void tarvos(int m, char *s)
{
	simm(m, s);
}

/*-----------------------  RESET MEASUREMENTS  -----------------------*/
void reset()  					/* limpa os contadores e acumuladores das medicoes */
{
    resetf();
	start = clock;					/* Ao final clock-start apresenta o intervalo em que as medicoes
															 ocorreram */

    /* #if MODIFY
	     nstrtwrp=nclkwrp;	   ??? verificar para que serve esta variavel e para salvar
														     estados ?
       #endif */
}

/* RESET FACILITY & QUEUE MEASUREMENTS
*
*  Limpa as facilities e as medi��es das filas de modo que o sistema j� esteja populado e as estat�sticas reflitam o estado permanente de
*  funcionamento (ou seja, elimina o transit�rio) a partir da chamada desta fun��o.
*  O in�cio de uma simula��o representa um transit�rio, pois as filas ainda est�o vazias e as estat�sticas acumular�o esta situa��o.  Uma boa
*  pr�tica � deixar a simula��o rodar por um certo tempo e ent�o chamar reset().  As estat�sticas ser�o ent�o tomadas a partir deste ponto,
*  que eliminar� o transit�rio.
*
*  Modifica��es e reparos em Nov2006 Marcos Portnoi:
*
*  . Originalmente, n�o estava limpando apropriadamente as estat�sticas dos servidores das facilities, nem a �ltima facility.  Reparado com altera��o
*		dos dois while
*  . Inclu�da a limpeza do contador de releases para a facility (f_release_count) (existe um contador para cada servidor e um global para a facility).
*    Quest�o:  deve ser limpo tamb�m o contador de tamanho m�ximo de fila?
*  . Campo f_max_queue (tamanho m�ximo de fila) � igualado ao tamanho atual da fila.
*  . Campo f_busy_time, que � o somat�rio, para a facility, de todos os busy times dos servidores (fs_busy_time) tamb�m � zerado.
*  . Campo f_tkn_dropped tamb�m � zerado.
*
*/
static void resetf() {
	struct facilit *fct;
	struct fserv *fct_serv;

	fct = sim[sn].fct_begin;

	//while (fct->fct_next != NULL) Consertada conforme abaixo, para que a �ltima facility seja tamb�m processada.
	while (fct != NULL) { //fa�a enquanto houver facilities na lista
		fct->f_exit_count_q = 0;
		fct->f_preempt_count = 0;
		fct->length_time_prod_sum = 0.;
		fct->f_release_count = 0; //este campo � um somat�rio dos fs_release_count de cada servidor; ent�o, zerar tamb�m
		fct->f_busy_time = 0.; //zerar este acumulador, que � o somat�rio de todos os fs_busy_time dos servidores
		fct->f_max_queue = fct->f_n_length_q; //iguale o tamanho m�ximo de fila ao tamanho atual da fila
		fct->f_tkn_dropped = 0; //zera contador de tokens descartados

		fct_serv = fct->f_serv; //coleta apontador para a fila de servidores da facility

		//while (fct_serv->fs_next != NULL) Consertado conforme abaixo, pois se s� houver um servidor, este n�o estava sendo limpo.
		while (fct_serv != NULL) { //fa�a enquanto houver servidores na lista da facility
			fct->f_serv->fs_release_count = 0;
			fct->f_serv->fs_busy_time = 0.;
			fct_serv = fct_serv->fs_next; //coleta o pr�ximo servidor
		}

		fct = fct->fct_next; //coleta a pr�xima facility
	}

	/* ATENCAO AJUSTAR ESTA FUNCAO POIS A ULTIMA FACILITY NAO ESTA SENDO RESETADA */
	/* Na verdade, nem os servidores de cada facility, nem a �ltima facility, est�o sendo limpas. */
}

/*-------------------------  DEFINE FACILITY  ------------------------
* As facilities podem ser colocadas como operacionais (UP) ou n�o-operacionais (DOWN).
* Pode-se assim simular facilities que podem falhar.
* Todas as facilities s�o criadas, por default, como UP (operacionais)
* Implementado em 28.Dez.2005 Marcos Portnoi
*/
int facility(char *s, int n)
{
	struct facilit *fct, *fct_end;
	struct fserv *srv;
	struct fserv *srv_previous;

	int i;

	if (n < 1)
	{
		printf ("\nError - facility - incorrect number of servers");
		exit (1);
	}

	fct = (facilit*)malloc(sizeof *fct);
	if (fct == NULL)
	{
		printf ("\nError - facility - insufficient memory to allocate for facility");
		exit (1);
	}

	sim[sn].fct_number = sim[sn].fct_number + 1; //incrementa fct_number para que indique o pr�ximo n�mero a criar

	strcpy(fct->f_name, s);
	fct->f_number = sim[sn].fct_number;	/* As facilities come�am em 1, 2, 3, ... sim[sn].fct_number cont�m o �ltimo n�mero de facility criado */
	fct->f_n_serv = n;
	fct->f_n_busy_serv = 0;	/* Se n_busy_serv = n, todos os servidores estao ocupados; a token deve ser enfileirada */
	fct->f_n_length_q = 0;
	fct->f_exit_count_q = 0;
	fct->f_last_ch_time_q = 0.0;
	fct->f_max_queue = 0;
	fct->f_preempt_count = 0;
	fct->length_time_prod_sum = 0.0;
	fct->f_busy_time = 0.0;
	fct->f_release_count = 0;
	fct->f_serv = NULL; //apontador para a lista de servidores
	fct->fct_next = NULL; //apontador para a defini��o do pr�ximo facility
	fct->f_queue = NULL; //Inicia com NULL; quando as tokens chegarem, serao enfileiradas
	fct->f_up = 1; //facility est� operacional (UP) por default (0 = DOWN, 1 = UP)
	fct->f_tkn_dropped = 0; //contador de tokens descartadas (tokens s�o descartadas, por exemplo, quando a facility est� down)
	for (i = 0; i < n; i++)
	{
		srv = (fserv*)malloc(sizeof *srv);
		if (srv == NULL)
		{
			printf ("\nError - facility - insufficient memory to allocate for server");
			exit (1);
		}
		srv->fs_number = i;		// IMPORTANTE - o numero do servidor comeca com zero e nao com 1
		srv->fs_tkn = 0;
		srv->fs_p_tkn = 0;
		srv->fs_release_count = 0;
		srv->fs_start = 0.0;
		srv->fs_busy_time = 0.0;
		srv->fs_next = NULL;

		if (fct->f_serv == NULL) //s� entra aqui se for o primeiro servidor criado para a facility
		{
			fct->f_serv = srv;	// Campo f_serv da facility aponta para lista de servs
			srv_previous = srv;
		}
		else //j� h� outros servidores criados para esta facility; adicione este servidor � lista
		{
			srv_previous->fs_next = srv;
			srv_previous = srv;
		}
	}

	if (sim[sn].fct_begin == NULL)	/* S� entra aqui se a lista de facilities estiver vazia */
	{
		sim[sn].fct_begin = fct;
	}
	else
	{
		fct_end = sim[sn].fct_begin;
		while (fct_end->fct_next != NULL) //percorre a lista de facilities em busca da �ltima posi��o; lista de facilities � simplesmente encadeada
		{
			fct_end = fct_end->fct_next;
		}
		fct_end->fct_next = fct; //adiciona facility rec�m criada ao final da lista
		fct->fct_next = NULL; //certifica que este apontador next, no �ltimo elemento da fila, � NULL
	}
  return(sim[sn].fct_number);
}


/*-------REQUEST FACILITY WITH TOKEN POINTER - FC requestp ----------------
*
* A funcao request foi modificada com o acrescimo de um apontador associado
* a tkn para que na area apontada tenha mais informacoes sobre a tkn. Desta
* maneira pode-se acrescentar caracteristicas da token. Neste caso criou-se
* uma variavel tkp - token pointer com o tipo da estrutura do pacote.
*
* Em relacao ao smpl original algumas mudancas filosoficas foram realizadas.
* Agora a request solicita o tempo de servico, ja calculado no momento em que o
* request e efetuado, e o evento que a token devera realizar apos ser processada.
* Este ev e o mesmo designado da chamada schedule que vem logo apos o request.
* A ideia, e que, se ela for enfileirada, ao ser retirada da fila pela
* fc - release ela devera ter informacoes suficientes para ser colocada em
* processamento dentro da propria funcao release e tambem escalonada
* imediatamente.
*
* Par�metros:  f:  facility number
*			   tkn:  token number
*			   pri:  priority
*			   ev:  event number
*			   te:  interevent time
*			   tkp:  token pointer
*
* A fun��o retorna:
*  0 se a facility tem servidor dispon�vel e o token foi colocado em servi�o;
*  1 se a facility n�o tem servidor dispon�vel; token colocada na fila
*  2 se a facility est� down (n�o-operacional); nada foi feito com a token
* O contador de tokens descartados tamb�m � incrementado para o caso 2.
* (estas amplia��es de estado da facility 10.Jan.2006 Marcos Portnoi)
*/
int requestp (int f, int tkn, int pri, int ev, double te, TOKEN *tkp) {
	int r;
	struct facilit *fct;
	struct fserv *srv;

	if ( tkn == 0 )
	{
		printf ("\nError - requestp - token = 0 ");
		exit (1);
	}

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - requestp - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	//se a facility estiver down, retorne imediatamente com 2
	//O programa do usu�rio deve decidir o que fazer com a token (descartar ou n�o)
	if (getFacUpStatus(f)==0)
	{
		fct->f_tkn_dropped++; //incremente contador de tokens descartadas; se isso n�o for interessante para o usu�rio, ent�o esta linha deve ser editada
		return 2;
	}

	if ( fct->f_n_busy_serv < fct->f_n_serv )	/* Ainda tem servidor livre */
	{
			srv = fct->f_serv;
			while (srv->fs_tkn != 0)
			{
				srv=srv->fs_next;
			}
			srv->fs_tkn = tkn;
			srv->fs_p_tkn = pri;
			srv->fs_start = clock;						/* Comeco de novo servico */
			fct->f_n_busy_serv++;
			r = 0;
	}
	else			/* facility busy - enqueue token marked w/event, priority */
	{
		enqueuep(f, tkn, tkp, pri, te, ev);
		r=1;
	}
	return(r);
}

/*-------PREEMPT FACILITY WITH TOKEN POINTER - FC preemptp ----------------
* A funcao preempt foi modificada com o acrescimo de um apontador associado
* a tkn para que na area apontada tenha mais informacoes sobre a tkn. Desta
* maneira pode-se acrescentar caracteristicas da token. Neste caso criou-se
* uma variavel tkp - token pointer com o tipo da estrutura do pacote.
* Este programa trabalha com um sistema de prioridade de processamento da
* token onde o maior valor numerico de prioridade tem tambem a maior prioridade de
* execucao.
* Assim, uma token com prioridade 5 devera ser processada antes de tokens com prioridade
* menor do que cinco.

* Coment�rio:  na fun��o originalmente implementada, caso as tokens em servi�o tenham
* prioridade igual ou menor que a token que chama a preemp��o, ent�o n�o haver� preemp��o;
* esta fun��o chama pois a fun��o enqueuep_preempt, que coloca a token em quest�o na fila
* da facility em ordem de prioridade, mas � frente de outras tokens de mesma prioridade.
* N�o seria portanto uma a��o id�ntica � requestp, como prev� MacDougall para o SMPL.
* Em havendo de fato a preemp��o, a token que foi retirada de servi�o vai para a fila em
* ordem de prioridade, e � frente de outras tokens de mesma prioridade, podendo assim
* voltar a servi�o imediatamente � frente das outras tokens enfileiradas de mesma prioridade.
* O funcionamento original, portanto, n�o est� conforme previsto por MacDougall no SMPL.  Se n�o
* houver preemp��o e preemptp deve funcionar como um requestp neste caso, ent�o a token
* deve ser enfileirada por ordem de prioridade em ao fim das tokens de mesma prioridade
* (deve obedecer ao esquema FIFO dentre a mesma prioridade).  Uma chamada � fun��o enqueuep,
* ao inv�s de enqueuep_preempt, para o caso de n�o haver preemp��o, deve resolver a quest�o.
* (08.Jan.2006 Marcos Portnoi)
*
* Note: this function, as it is, is *not* incrementing # of releases upon a successful preemption.
*       This is in contrast with SMPL approach. Although the current approach will yield a correct
*       Mean of Busy Time calculation, problems might arise if a preempted token never goes back to service.
*       If a preempted token never goes back to service, then the service time is has received so far did
*       go into Sum of Busy Time, but a # release count will never be accumulated for this token. Therefore,
*       the effect is adding time to the Sum of Busy Time for no specific token, causing an "error" in the
*       computation of the Mean of Busy Time.
*
*       (more)
*		This SMPL behavior has effects on the correct calculation of certain statistics,
*		namely Sum of Busy Time and Mean Busy Period. Mean Busy Period relies on # of releases to calculate the mean.
*		A preempted token moving back into service, and then released, can be technically considered as two tokens:
*		one is serviced up to time t and then "released"; the second is serviced for the remaining service time and then
*		released. Sum of Busy Time accumulates both parts of the service (before preemption, and after resuming service), 
*		and Number of Releases accumulates two releases for the same token. The final Mean Busy Period will be slightly different
*		than accumulating one whole service time (the sum of the two parts of the service for the preempted token) and dividing by
*		*one* single release for the same token.
*		The larger the value of Sum of Busy Time and # of Releases, the less the difference, but it exists.
*		SMPL treatment also causes problems if # of Releases is used as # of Tokens serviced. A preempted token will be counted twice.
*		A simple fix is to obtain the # of Serviced tokens as # of Releases - # of Preemptions.
*		And, perhaps, use, for calculating the Mean Busy Period, the # of Serviced Tokens, and not # of Releases.
*		(01.August.2013 Marcos Portnoi)
*
* A fun��o retorna:
*  0 se a facility tem servidor dispon�vel e o token foi colocado em servi�o;
*  1 se a facility n�o tem servidor dispon�vel; token colocada na fila
*  2 se a facility est� down (n�o-operacional); nada foi feito com a token
* O contador de tokens descartados tamb�m � incrementado para o caso 2.
* (estas amplia��es de estado da facility 10.Jan.2006 Marcos Portnoi)
*/
int preemptp (int f, int tkn, int pri, int ev, double te, TOKEN *tkp) {
	int r, tkn_srv, tkn_pri_srv, ev_srv, menor_pri;
	struct facilit *fct;
	struct fserv *srv, *srv_menor_pri;
	struct evchain *evc_tkn_srv;
	TOKEN *tkp_srv;
	double te_srv;

	if ( tkn == 0 )
	{
		printf ("\nError - preemptp - token = 0 ");
		exit (1);
	}

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - preemptp - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	//se a facility estiver down, retorne imediatamente com 2
	//O programa do usu�rio deve decidir o que fazer com a token (descartar ou n�o)
	if (getFacUpStatus(f)==0)
	{
		fct->f_tkn_dropped++; //incremente contador de tokens descartadas; se isso n�o for interessante para o usu�rio, ent�o esta linha deve ser editada
		return 2;
	}

	/*Situacoes poss�veis na preemp��o:
     1. Se tiver servidor livre aloca o servidor com a token e retorna 0 da funcao
	 2. Se todos os servidores estiverem ocupados verifica-se as prioridades das tokens em servico
			a. Se as tokens em servico tiverem prioridade maior ou igual a token que chama a preemp��o, entao a
			   token que chama a preemp��o eh colocada em fila como qualquer outra token e 1 eh retornado ou seja
				 o preempt funciona exatamente como o request.
			b. Se a token em servico tiver prioridade menor que do que a token atual, retira a token
			   de servico fazendo todas as atualizacoes necessarias -
				 . retira da cadeia de eventos o termino do servico desta token (caso ela nao encontre
				   esta token com termino de servico um erro e retornado
				 . acrescenta a utilizacao na facility e no servidor do tempo que ela passou em servico
				   este tempo eh o clock atual subtraido do tempo em que ela entrou em servico
					 pois de toda forma ele ja usou os recursos de servico
				 . coloca entao a token retirada em servico na fila de espera onde o tempo que estara
				   associado ao termino de servico da token e o tempo final de servico do evento
					 retirado da cadeia de eventos diminuido do clock atual
				 . a insercao da token na fila de espera respeitara a seguinte condicao: ela sera
				   colocada para execucao antes das tokens de mesma prioridade. Assim, dentro das
					 tokens de mesma prioridade ela sera a primeira a ser servida.
				 . coloca a token que chama a preemp��o em servico
				 . retorna 0, pois a token foi colocada em servi�o
	*/

	if ( fct->f_n_busy_serv < fct->f_n_serv )	/* Ainda tem servidor livre */
	{
			srv = fct->f_serv;
			while (srv->fs_tkn != 0)
			{
				srv=srv->fs_next;
			}
			srv->fs_tkn = tkn;
			srv->fs_p_tkn = pri;
			srv->fs_start = clock;				/* Comeco de novo servico */
			fct->f_n_busy_serv++;
			r = 0;
			return (r);
	}

	/* Se ele chegar aqui siginifica que a Facility esta busy - assim deve-se
	   verificar os servidores que estao ocupados para verificar prioridade da
	   token em servico e efetuar os processamentos 2.a ou 2.b */

	srv = fct->f_serv;
	srv_menor_pri = NULL;
	menor_pri = pri;
	while (srv != NULL)
	{
		if (menor_pri > srv->fs_p_tkn)
		{
			menor_pri = srv->fs_p_tkn;
			srv_menor_pri = srv;
		}
		srv=srv->fs_next;
	}

	if (srv_menor_pri == NULL)			/* Processamento 2.a inicio */

	/* Significa que ele nao achou um servidor com tkn em servico com prioridade menor
	   do que a token que chama a preemp��o. Ou seja, as tokens tem prioridade maior ou igual a token que chama a preemp��o.
	   N�o haver�, pois, suspens�o de tokens em servi�o.*/
	{
	    /*(Coment�rio:  Sergio Brito) Neste ponto discordo do MacDougall pois ele diz que, para a token que chama a preemp��o, se forem achados
		  servidores com token em servico com prioridade maior ou igual � dessa, a token que chama a preemp��o deve ser
		  enfileirada como � feito em um request tradicional (deve ficar atr�s, na fila, de tokens com mesma prioridade).
		  Eu acho que ela deve ser enfileirada com o mesmo esquema de prioridade que eh feito no caso 2.b, assim ao
		  inves de usar a funcao - enqueuep(f, tkn, tkp, pri, te, ev); - deve usar a funcao: enqueuep_preempt */
		/* O problema aqui � que, assim, a token que n�o causou preemp��o vai para fila � frente de outras tokens de mesma
		   prioridade... isso � desej�vel?  O comportamento neste caso assemelha-se a eleva��o da prioridade da token que chamou a fun��o
		   (08.Jan.2006 Marcos Portnoi)*/
		/* Fa�amos como o MacDougall:  colocar a token bloqueada ap�s as tokens de mesma prioridade
		enqueuep_preempt(f, tkn, tkp, pri, te, ev);*/
		enqueuep(f, tkn, tkp, pri, te, ev); //coloca a token em fila em ordem de prioridade, ap�s as tokens de mesma prioridade
		r = 1;
		return (r);
	}       					/* Processamento 2.a final */

	srv = srv_menor_pri;		/* Processamento 2.b inicio:  aqui, efetivamente, haver� preemp��o (suspens�o de token em servi�o) */

	/* faz o servidor atual ser o com tkn de menor prioridade */
	/* retira a token da cadeia de eventos com o tempo de termino de servico
	   deve retornar apos o processamento o apontador para o elemento da cadeia
	   de eventos que contem as informacoes de processamento da token */

	evc_tkn_srv = cancelp_tkn(srv->fs_tkn);

	if (evc_tkn_srv == NULL) { //se for NULL, indica que o token n�o foi encontrado na cadeia de eventos
		printf("\nError - preemptp - token to be preempted not found in event chain (possible release event)");
		exit(1);
	}

	/* obtem as informacoes da token em servico para que ela possa ser enfileirada */

	if ( evc_tkn_srv->ev_tkn != srv->fs_tkn ) /* Testa se realmente a tkn obtida eh a em srv */
	{
		printf ("\nError - preemptp - token removed from event chain is different from token in service");
		printf ("\nError - preemptp - token removed from event chain is different from token in service");
		exit (1);
	}

	tkn_srv = evc_tkn_srv->ev_tkn;
	tkp_srv = evc_tkn_srv->ev_tkn_p;
	te_srv = (evc_tkn_srv->ev_time - clock);
	tkn_pri_srv = srv->fs_p_tkn;
	ev_srv = evc_tkn_srv->ev_type;

	/* coloca a token em servico na fila de espera da facility como a primeira que sera
	   servida em relacao as tokens de mesma prioridade */

	enqueuep_preempt(f, tkn_srv, tkp_srv, tkn_pri_srv, te_srv, ev_srv);

	/* atualiza estatisticas do servidor apos saida da tkn que estava em servico */
	srv->fs_tkn = 0;
	srv->fs_p_tkn = 0;
	  /* srv->fs_release_count++;nao deve ser contabilizado mais um release pois senao
		 um job criado na fonte seria representado por varios pedacos.
		 Entao, so deve ocorrer a contabilizacao do release, quando ele
		 for retirado de servico pela funcao release.  (Sergio Brito)
		 Observar que o SMPL, do MacDougall, contabiliza o release na suspens�o de servi�o
		 (preemp��o).  Isso de fato criaria jobs fragmentados.  hmmmmm... (08.Jan.2006 Marcos Portnoi)*/
	  /* This SMPL behavior has effects on the correct calculation of certain statistics,
	     namely Sum of Busy Time and Mean Busy Period. Mean Busy Period relies on # of releases to calculate the mean.
		 A preempted token moving back into service, and then released, can be technically considered as two tokens:
		 one is serviced up to time t and then "released"; the second is serviced for the remaining service time and then
		 released. Sum of Busy Time accumulates both parts of the service (before preemption, and after resuming service), 
		 and Number of Releases accumulates two releases for the same token. The final Mean Busy Period will be slightly different
		 than accumulating one whole service time (the sum of the two parts of the service for the preempted token) and dividing by
		 *one* single release for the same token.
		 The larger the value of Sum of Busy Time and # of Releases, the less the difference, but it exists.
		 SMPL treatment also causes problems if # of Releases is used as # of Tokens serviced. A preempted token will be counted twice.
		 A simple fix is to obtain the # of Serviced tokens as # of Releases - # of Preemptions.
		 And, perhaps, use, for calculating the Mean Busy Period, the # of Serviced Tokens, and not # of Releases.
		 (01.August.2013 Marcos Portnoi)
		 */
	srv->fs_busy_time = srv->fs_busy_time + ( clock - srv->fs_start ); /*Acumula tempo uso serv */

	/* atualiza estatisticas da facility apos saida do servico */
	fct->f_busy_time = fct->f_busy_time + ( clock - srv->fs_start );
	/* fct->f_release_count++; idem srv->fs_release_cont++ */
	fct->f_n_busy_serv--; //agora h� um servidor livre
	fct->f_preempt_count++; //incrementa contador de preemp��es, pois aqui efetivamente ocorreu uma

	/* apos a utilizacao de todas as informacoes da token que foi tirada da cadeia e do
	   servico e foi enfileirada na facility, podemos liberar a area de memoria que
	   o elemento da cadeia estava usando */

	free(evc_tkn_srv);

	/* coloca a token que chama a preemp��o em servico */

	srv->fs_tkn = tkn;
	srv->fs_p_tkn = pri;
	srv->fs_start = clock;		/* Comeco de novo servico */
	fct->f_n_busy_serv++;
	r = 0;						/* retorna 0 pois a solicitacao de servico foi aceita
								   para que seja escalonado o termino de servico na
								   rotina que a chamou */

	return (r);					/* Processamento 2.b final */
}

/*-------CANCEL TOKEN WITH TOKEN POINTER - FC cancelp_tkn ---------------------
*
* Esta rotina cancela uma token da cadeia de eventos retornando um apontador para
* o elemento da cadeia que foi retirado.  Recebe como par�metro o n�mero do token.
* Ela s� funciona bem se o n�mero de token for �nico durante toda a simula��o; caso
* contr�rio, o primeiro token (com tempo de ocorr�ncia mais pr�ximo) com aquele n�mero
* ser� retirado da cadeia, podendo ser o token desejado, ou n�o.
*
* Retorna ponteiro para o elemento da cadeia de eventos que foi cancelado, ou NULL se n�o
* encontrado.  A fun��o que chama deve cuidar de testar para NULL e eliminar o elemento da
* mem�ria com free, se necess�rio.
*
*/
struct evchain *cancelp_tkn(int tkn)
{

	struct evchain *evc, *evc_tkn_srv;

	if ( tkn == 0 )
	{
		printf ("\nError - cancelp_tkn - token = 0 ");
		exit (1);
	}

	if ( (sim[sn].evc_begin == NULL) && (sim[sn].evc_end == NULL) )	/* Cadeia de eventos vazia */
	{
		printf ("\nError - cancelp_tkn - empty event chain");
		exit (1);
	}

	evc = sim[sn].evc_begin;
	evc_tkn_srv = NULL;

	while (evc != NULL)
	{
		if ( evc->ev_tkn == tkn)
		{
			evc_tkn_srv = evc;
			break;
		}
		evc = evc->ev_next;
	}

	if ( evc_tkn_srv == NULL )	/* Nao achou tkn em servico na cadeia de eventos */
	{
		/*printf ("\nErro - cancelp_tkn - tkn em servico nao encontrada na cadeia de eventos");
		exit (1);*/
		return NULL;
	}

	/* O elemento eh o primeiro e unico da cadeia de eventos */
	if ((evc_tkn_srv->ev_previous == NULL) && (evc_tkn_srv->ev_next == NULL))
	{
		sim[sn].evc_begin = NULL;
		sim[sn].evc_end = NULL;
		return (evc_tkn_srv);
	}

	/* O elemento eh o primeiro e nao eh o unico da cadeia de eventos */
	if ((evc_tkn_srv->ev_previous == NULL) && (evc_tkn_srv->ev_next != NULL))
	{
		sim[sn].evc_begin = evc_tkn_srv->ev_next;
		evc_tkn_srv->ev_next->ev_previous = NULL;
		return (evc_tkn_srv);
	}

	/* O elemento eh o ultimo da cadeia de eventos */
	if ((evc_tkn_srv->ev_previous != NULL) && (evc_tkn_srv->ev_next == NULL))
	{
		sim[sn].evc_end = evc_tkn_srv->ev_previous;
		evc_tkn_srv->ev_previous->ev_next = NULL;
		return (evc_tkn_srv);
	}

	/* O elemento estah no meio da cadeia de eventos */
	if ((evc_tkn_srv->ev_previous != NULL) && (evc_tkn_srv->ev_next != NULL))
	{
		evc_tkn_srv->ev_previous->ev_next = evc_tkn_srv->ev_next;
		evc_tkn_srv->ev_next->ev_previous = evc_tkn_srv->ev_previous;
		return (evc_tkn_srv);
	}

  /* Se chegar neste ponto siginifica que um erro imprevisto ocorreu e tem que ser analisado
	   em detalhe */
	printf ("\nError - cancelp_tkn - element could not be removed");
	exit (1);

}

/*-------CANCEL TOKEN WITH TOKEN POINTER - FC cancelp_ev ---------------------
*
* Esta rotina cancela uma token da cadeia de eventos retornando um apontador para
* o elemento da cadeia que foi retirado.  Recebe como par�metro o n�mero do evento.
* A fun��o retirar� a primeira ocorr�ncia do evento na cadeia, independente do n�mero do
* token envolvido.
*
* Retorna ponteiro para o elemento da cadeia de eventos que foi cancelado, ou NULL se n�o
* encontrado.  A fun��o que chama deve cuidar de testar para NULL e eliminar o elemento da
* mem�ria com free, se necess�rio.
*
* Nov2006 Marcos Portnoi.
*/
struct evchain *cancelp_ev(int ev)
{

	struct evchain *evc, *evc_elem;

	if ( (sim[sn].evc_begin == NULL) && (sim[sn].evc_end == NULL) )	/* Cadeia de eventos vazia */
	{
		printf ("\nError - cancelp_ev - empty event chain");
		exit (1);
	}

	evc = sim[sn].evc_begin;
	evc_elem = NULL;

	while (evc != NULL) //percorre a cadeia de eventos em busca da primeira ocorr�ncia de ev
	{
		if ( evc->ev_type == ev)
		{
			evc_elem = evc;
			break;
		}
		evc = evc->ev_next;
	}

	if ( evc_elem == NULL )	/* Nao achou elemento na cadeia com evento especificado */
	{
		return NULL;
	}

	/* O elemento eh o primeiro e unico da cadeia de eventos */
	if ((evc_elem->ev_previous == NULL) && (evc_elem->ev_next == NULL))
	{
		sim[sn].evc_begin = NULL;
		sim[sn].evc_end = NULL;
		return (evc_elem);
	}

	/* O elemento eh o primeiro e nao eh o unico da cadeia de eventos */
	if ((evc_elem->ev_previous == NULL) && (evc_elem->ev_next != NULL))
	{
		sim[sn].evc_begin = evc_elem->ev_next;
		evc_elem->ev_next->ev_previous = NULL;
		return (evc_elem);
	}

	/* O elemento eh o ultimo da cadeia de eventos */
	if ((evc_elem->ev_previous != NULL) && (evc_elem->ev_next == NULL))
	{
		sim[sn].evc_end = evc_elem->ev_previous;
		evc_elem->ev_previous->ev_next = NULL;
		return (evc_elem);
	}

	/* O elemento estah no meio da cadeia de eventos */
	if ((evc_elem->ev_previous != NULL) && (evc_elem->ev_next != NULL))
	{
		evc_elem->ev_previous->ev_next = evc_elem->ev_next;
		evc_elem->ev_next->ev_previous = evc_elem->ev_previous;
		return (evc_elem);
	}

  /* Se chegar neste ponto siginifica que um erro imprevisto ocorreu e tem que ser analisado
	   em detalhe */
	printf ("\nError - cancelp_ev - element could not be removed");
	exit (1);

}

/*-------ENQUEUE TOKEN PREEMPTED WITH TOKEN POINTER - FC enqueuep_preempt ---------------------*/
/* Esta fun��o efetua a inser��o do elemento na fila priorizando a token em relacao �s
   demais e em relacao a tokens que tenham a mesma prioridade; ela ser� servida antes das
   demais com prioridade menor.
   A prioridade segue a ordem n�merica:  quanto maior o n�mero, maior a prioridade.
   Assim, uma token com prioridade 5 dever� ser desenfileirada antes de tokens com
   prioridade menor ou igual a 5.
 */

static void enqueuep_preempt (int f, int tkn, TOKEN *tkp, int pri, double te, int ev)
{
	struct facilit *fct;
	struct fqueue *que, *que_actual, *que_previous;

	if (tkn == 0)
	{
		printf ("\nError - enqueue_preempt - token = 0 ");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	fct->length_time_prod_sum += fct->f_n_length_q * (clock - fct->f_last_ch_time_q);
	fct->f_n_length_q++;

	/* O caso de uma token que foi retirada de servi�o por uma token que chama a preemp��o
	   e foi colocada em fila; por esta raz�o, dever� normalmente alterar as estat�sticas
	   da fila (como est� ocorrendo aqui), pois efetivamente o efeito da preemp��o foi
	   o enfileiramento e deve ent�o ser reportado em termos do comportamento da fila da facility */

	if (fct->f_n_length_q > fct->f_max_queue)				/* Atualiza tamanho m�ximo da fila*/
	{
		fct->f_max_queue = fct->f_n_length_q;
	}

	fct->f_last_ch_time_q = clock;

	que = (fqueue*)malloc(sizeof *que);
	if (que == NULL)
	{
		printf ("\nError - enqueue - insufficient memory to allocate for facility queue");
		exit (1);
	}

	que->fq_ev	= ev;
	que->fq_tkn = tkn;
	que->fq_tkp = tkp;
	que->fq_pri = pri;
	que->fq_time = te;
	que->fq_next = NULL;

	/* Agora a rotina de inser��o eh diferente do enqueuep pois na realidade ela devera
	   ser inserida antes de uma token com menor prioridade ou igual a da token  que
	   esta sendo inserida na fila */

	//Coment�rio:  ap�s modificada, a diferen�a entre as fun��es enqueuep e enqueuep_preempt ficou:
	//enqueuep:  insere na fila por ordem de prioridade, e depois dos tokens com mesma prioridade;
	//enqueuep_preempt:  insere na fila por ordem de prioridade, e antes dos tokens com mesma prioridade
	//H� alguma situa��o onde a prioridade deve ser desconsiderada para enfileirar? (Marcos Portnoi em 21-Dez-2005)

	if (fct->f_queue == NULL) //fila da facility est� vazia; o token enfileirado ser� o primeiro, retorne da fun��o imediatamente
	{
		fct->f_queue = que; /* Campo f_queue da facility aponta para a queue */
		return;
	}

	//h� fila na facility; vejamos em que posi��o deveremos inserir o novo token
	que_actual = fct->f_queue;
	que_previous = NULL;

	while (que_actual != NULL) /*faz uma busca na fila existente por prioridade:  ao achar uma token com prioridade maior ou igual � da
							   da token a enfileirar, p�ra a busca; a token dever� ser inserida antes de que_actual.*/
	{
		if (( pri > que_actual->fq_pri) || (pri == que_actual->fq_pri)) break;
		que_previous = que_actual;
		que_actual = que_actual->fq_next;
	}

	/* Situacoes poss�veis:
	 . Estar no inicio da fila
	 . Estar no meio da fila
	 . Estar no fim da fila */

	/* Estando no inicio da fila */

	if (que_previous == NULL)
	{
		fct->f_queue = que;
		que->fq_next = que_actual;
		return;
	}

	/* Estando no fim da fila */

	if (que_actual == NULL)
	{
		que_previous->fq_next = que;
		return;
	}

	/* Estando no meio da fila */

	if ( (que_actual != NULL) && (que_previous != NULL) )
	{
		que_previous->fq_next = que;
		que->fq_next = que_actual;
		return;
	}

	printf ("\nError - enqueuep_preempt - element could not be inserted into facility queue");
	exit (1);
}

/*-------ENQUEUE TOKEN WITH TOKEN POINTER - FC enqueuep---------------------*/
/* A funcao enqueue foi modificada com o acrescimo de um apontador associado
   a tkn para que na area apontada tenha mais informacoes sobre a tkn. Desta
   maneira pode-se acrescentar caracteristicas da token. Neste caso criou-se
   uma variavel tkp - token pointer com o tipo da estrutura do pacote.
	 
   Fun��o modificada a fim de inserir os tokens na fila em ordem de priori-
   dade, as maiores prioridades primeiro.  11-Nov-2005 Marcos Portnoi
   Coment�rio:  ap�s modificada, a diferen�a entre as fun��es enqueuep e enqueuep_preempt ficou:
   enqueuep:  insere na fila por ordem de prioridade, e depois dos tokens com mesma prioridade;
   enqueuep_preempt:  insere na fila por ordem de prioridade, e antes dos tokens com mesma prioridade
*/

static void enqueuep (int f, int tkn, TOKEN *tkp, int pri, double te, int ev)
{
	struct facilit *fct;
	struct fqueue *que, *que_aux, *que_aux_next;

	if ( tkn == 0 )
	{
		printf ("\nError - enqueuep - token = 0 ");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	fct->length_time_prod_sum += fct->f_n_length_q * (clock - fct->f_last_ch_time_q);
	fct->f_n_length_q++;

	if (fct->f_n_length_q > fct->f_max_queue)	/* Atualiza tamanho maximo da fila*/
	{
		fct->f_max_queue = fct->f_n_length_q;
	}

	fct->f_last_ch_time_q = clock;

	que = (fqueue*)malloc(sizeof *que);
	if (que == NULL)
	{
		printf ("\nError - enqueue - insufficient memory to allocate for facility queue");
		exit (1);
	}

	que->fq_ev	= ev;
	que->fq_tkn = tkn;
	que->fq_tkp = tkp;
	que->fq_pri = pri;
	que->fq_time = te;
	que->fq_next = NULL;

	//Fun��o modificada a partir daqui, a fim de inserir tokens em ordem de prioridade
	//Insere token na fila em ordem de prioridade, ou seja, antes do token com prioridade menor que ele
	//Coment�rio:  ap�s modificada, a diferen�a entre as fun��es enqueuep e enqueuep_preempt ficou:
	//enqueuep:  insere na fila por ordem de prioridade, e depois dos tokens com mesma prioridade;
	//enqueuep_preempt:  insere na fila por ordem de prioridade, e antes dos tokens com mesma prioridade
	
	//H� alguma situa��o onde a prioridade deve ser desconsiderada no enfileiramento?

	if (fct->f_queue == NULL) //N�o h� nenhuma fila ainda; ent�o crie a fila com o novo token e saia.
	{
		fct->f_queue = que;						// Campo f_queue da facility aponta para a queue
	}
	else
	{
		//Aqui, que_aux indicar� o elemento imediatamente anterior ao ponto de inser��o, e que_aux_next, o elemento logo ap�s (ou NULL)
		que_aux = fct->f_queue;
		que_aux_next = que_aux->fq_next;
		while (que_aux_next != NULL)
		{
			if (que_aux_next->fq_pri >= que->fq_pri) {
				que_aux=que_aux_next;
				que_aux_next=que_aux_next->fq_next;
			}
			else break;
		}
		//O ponteiro que_aux indica a posi��o imediatamente anterior aonde deve ser inserido o novo token na fila,
		//exceto se houver apenas 1 elemento na fila.  Neste caso, o teste seguinte resolver� se o novo elemento deve
		//ser inserido antes ou depois de que_aux.
		if (que_aux->fq_pri < que->fq_pri) { //Se o teste for positivo, certamente que_aux � o �nico elemento na fila
			que->fq_next=que_aux;
			fct->f_queue=que; //j� que que_aux era o �nico elemento na fila, atualize o ponteiro para o primeiro elemento (que, inserido antes de que_aux)
		} else { //teste negativo; insira ent�o DEPOIS de que_aux (aqui � irrelevante a fila s� ter um elemento ou n�o)
			que->fq_next = que_aux->fq_next;
			que_aux->fq_next=que;
		}
	}
	return;
}


/*-------RELEASE FACILITY  WITH TOKEN POINTER - FC releasep ----------------*/
void releasep (int f, int tkn)
{
	struct facilit *fct;
	struct fserv *srv;
	struct fqueue *que;
	int tkn_not_serv;

	if ( tkn == 0 )
	{
		printf ("\nError - releasep - token = 0 ");
		exit (1);
	}

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - releasep - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	srv = fct->f_serv;
	tkn_not_serv = 1;

	while (srv != NULL)
	{
		if (srv->fs_tkn == tkn)
		{
			tkn_not_serv = 0;
			break;
		}
		srv = srv->fs_next;
	}

	if (tkn_not_serv)
	{
		printf ("\nError - releasep - token to be released not in service in this server");
		exit (1);
	}

	/* Atualiza estatisticas do servidor */
	srv->fs_tkn = 0;
	srv->fs_p_tkn = 0;
	srv->fs_release_count++;
	srv->fs_busy_time = srv->fs_busy_time + ( clock - srv->fs_start ); /*Acumula tempo uso serv */

	/* Atualiza estatisticas da facility */
	fct->f_busy_time = fct->f_busy_time + ( clock - srv->fs_start );
	fct->f_release_count++;
	fct->f_n_busy_serv--;

	if ( fct->f_n_length_q > 0 )
	{
    /* queue not empty:  dequeue request & update queue measures */
		que = fct->f_queue;

		fct->f_queue = que->fq_next;
		fct->length_time_prod_sum += fct->f_n_length_q * (clock - fct->f_last_ch_time_q);
		fct->f_n_length_q--;
		fct->f_exit_count_q++;
		fct->f_last_ch_time_q = clock;

		/* Seria bom uma modifica��o aqui?  � conveniente que a token em fila seja colocada no in�cio da cadeia de eventos, e
		*  n�o imediatamente escalonada para servi�o.  Assim, permite-se que a rotina de tratamento de eventos trate as tokens
		*  em fila (por exemplo, para descartar tokens em fila quando a facility estiver em modo down ou n�o-operacional).  Se as 
		*  tokens em fila forem escalonadas para servi�o diretamente aqui, n�o � poss�vel interceptar ent�o a token que est� saindo
		*  da fila antes que entre em servi�o.
		*  Para viabilizar esta modifica��o, a inser��o no in�cio da cadeia de eventos ter� de ser feita aqui (com todos
		*  os testes necess�rios, como cadeia vazia, etc.) e a fun��o requestp poder� ser simplificada, sendo desnecess�rio
		*  passar para ela como par�metros o evento posterior e tempo interevento (ficar� mais semelhante � fun��o original do
		*  MacDougall).  (Marcos Portnoi em 21-Dez-2005)
		*  O MacDougall, entretanto, trata este assunto de duas maneiras diferentes:  se o token em fila estiver bloqueado (n�o
		*  foi interrompido por preempt), o simulador coloca no in�cio da cadeia de eventos, permitindo assim o controle pela
		*  rotina de tratamento de eventos do usu�rio.  Mas, se o token retirado da fila for um retorno de servi�o interrompido
		*  (preempt), ent�o o simulador do MacDougall faz o escalonamento diretamente.
		*  Talvez a implementa��o mais racional seria colocar um tipo de teste na fun��o releasep, de modo que esta somente
		*  fa�a o desenfileiramento se a facility estiver operacional.  Uma flag tamb�m teria de ser implementada na estrutura da
		*  facility.
		*/

		/* Neste ponto diferencia-se o processamento do MacDougal, ao ter um
		   servidor livre e existir fila, imediatamente este servidor eh ocupado
		   e eh escalonado o tempo de termino de servico desta token */

		// Atualiza servidor que pega a token que estava na fila
		srv->fs_tkn = que->fq_tkn;
		srv->fs_p_tkn = que->fq_pri;
		srv->fs_start = clock;

		/* Atualiza a facility associada ao servidor */
		fct->f_n_busy_serv++;
		/* fct->f_last_ch_time_q = clock+que->fq_time;  ATENCAO ver com calma este procedimento */

		schedulep(que->fq_ev, que->fq_time, que->fq_tkn, que->fq_tkp); /* Escalona termino do servico */

		free(que);
	}
}

/*-------SCHEDULE EVENT WITH TOKEN POINTER - FC schedulep ------------------*/
/* A funcao schedule sera modificada com o acrescimo de um apontador associado
   a tkn para que na area apontada tenha mais informacoes sobre a tkn. Desta
	 maneira pode-se acrescentar caracteristicas da token */
/* Atencao - Verificar mais tarde como generalizar este procedimento para
   que nao seja necessario toda vez que for compilar o programa  definir
	 o tipo da variavel que a funcao recebera criar um typedef ou coisa do tipo */
/* Neste caso criou-se uma variavel tkp - token pointer com o tipo da estrutura
   do pacote. O ideal e isto ser generalizado para se evitar ter que se colocar
	 esta informacao. Ou ainda deixar a rotina schedule normal e uma schedule com
	 apontador. Neste segundo caso a vantagem e rodar scripts smpl antigos sem
	 problemas */
/* Basicamente precisam de apenas duas mudancas */
void schedulep(int ev, double te, int tkn, TOKEN *tkp)
/* void schedule(int ev, double te, int tkn)           MUDANCA 01/02 */
{
	struct evchain *evc, *evc_aux;
	double st;														/* simulation time tempo de ocorrencia do evento */

	st = clock + te;

	if ( tkn == 0 )
	{
		printf ("\nError - schedulep - token = 0 ");
		exit (1);
	}

	if (te < 0.0)
	{
		printf ("\nError - schedulep - simulated time less than zero");
		exit (1);
	}

	evc = (evchain*)malloc(sizeof *evc);
	if (evc == NULL)
	{
		printf ("\nError - schedulep - insufficient memory to allocate for event");
		exit (1);
	}

	evc->ev_time = st;
	evc->ev_tkn = tkn;
	evc->ev_tkn_p = tkp;							/* Insercao desta linha MUDANCA 02/02 */
	evc->ev_type = ev;

	if ( (sim[sn].evc_begin == NULL) && (sim[sn].evc_end == NULL) )	/* Cadeia de eventos vazia */
	{
		sim[sn].evc_begin = evc;
		sim[sn].evc_end = evc;
		evc->ev_next = NULL;
		evc->ev_previous = NULL;
		return;
	}

    /************************************************/
	/* A sequencia de testes a seguir eh mandatoria */
	/************************************************/


	evc_aux = sim[sn].evc_end;

	if (( st > evc_aux->ev_time ) || ( st == evc_aux->ev_time ))	/* Tempo atual maior ou igual */
	{																															/* do que o ultimo tempo da 	*/
		evc_aux->ev_next = evc;										/* cadeia acrescenta no final */
		evc->ev_previous = evc_aux;							    	/* da cadeia automaticamente  */
		evc->ev_next = NULL;
		sim[sn].evc_end = evc;
		return;
	}


	evc_aux = sim[sn].evc_begin;

	if ( st < evc_aux->ev_time )					/* Tempo atual menor que o primeiro tempo da cadeia
													   acrescenta no inicio da cadeia automaticamente */
	{
		evc_aux->ev_previous = evc;
		evc->ev_previous = NULL;
		evc->ev_next = evc_aux;
		sim[sn].evc_begin = evc;
		return;
	}

	if ( st == evc_aux->ev_time )					/* Tempo atual igual ao primeiro tempo da cadeia
													   como ele foi gerado depois ele ficara logo apos o primeiro */
	{
		evc->ev_next = evc_aux->ev_next;
		evc->ev_previous = evc_aux;
		evc_aux->ev_next->ev_previous = evc;
		evc_aux->ev_next = evc;
		return;
	}

	/* Tempo atual menor ou igual do que o ultimo  tempo da cadeia procura dentro da lista
		 ponto de insercao. A busca para insercao comeca do final para o inicio pois a
	   perspectiva e que o novo sn esteja mais proximo do final do que do inicio e isto
	   agiliza a insercao. Pensar em colocar um contador para verificar esta velocidade */


	evc_aux = sim[sn].evc_end;

	while (1)		/* Insere na lista mas nao e o primeiro */
	{
		if (( st > evc_aux->ev_time ) || ( st == evc_aux->ev_time ))
		{
			evc->ev_next = evc_aux->ev_next;
			evc->ev_previous = evc_aux;
			evc_aux->ev_next->ev_previous = evc;
			evc_aux->ev_next = evc;
			return;
		}
		evc_aux = evc_aux->ev_previous;
	}

	/* Se chegar a este ponto significa que ocorreu algum erro na insercao do tempo */

	printf ("\nError - schedulep - could not insert time into event chain");
	exit (1);

}

/*---------- CAUSE EVENT WITH TOKEN POINTER ---------------------------*/
/* Foram feitas duas mudancas em relacao ao processamento original       */
TOKEN *causep(int *ev, int *tkn)
{
	struct evchain *evc;
	TOKEN *tkp;											/* Insercao desta linha MUDANCA 01/03 */

	evc = sim[sn].evc_begin;
	//testa se cadeia de eventos est� vazia; se estiver, mostra mensagem de erro e sai do programa
	//isto � necess�rio, caso contr�rio as instru��es subsequentes causar�o erro
	//a cadeia de eventos pode ficar vazia se o t�rmino da simula��o for controlado por tempo,
	//mas a gera��o de novos eventos terminar antes do tempo m�ximo limite. 05.Jan.2006 Marcos Portnoi
	if (evChainIsEmpty()) {
		printf("\nError - causep - empty event chain");
		exit(1);
	}

	*tkn = token = evc->ev_tkn;
	*ev = event = evc->ev_type;		/* este event servira para o enqueue token colocar o
									   evento que devera ser processado associado a token
									   retirada da fila da facility */
	clock = evc->ev_time;
	tkp = evc->ev_tkn_p;			/* Insercao desta linha ao inves de return puro MUDANCA 02/03  */

	/* Apos a retirada do evento libera-se a area deste evento */

	sim[sn].evc_begin = evc->ev_next;

	if (evc->ev_next == NULL) //cadeia de eventos ficou vazia; atualizar indicadores para NULL
	{
		sim[sn].evc_begin = NULL;
		sim[sn].evc_end = NULL;
	}
	else
	{
		evc->ev_next->ev_previous = NULL;  //este � o primeiro elemento da cadeia; o ponteiro previous aponta para NULL, ent�o.
	}


	free(evc);

	/* retorna o apontador da token */

	return (tkp);	/* Insercao desta linha ao inves de return puro MUDANCA 03/03  */

}

/* CHECKS WHETHER EVENT CHAIN IS EMPTY
*
*  Testa se a cadeia de eventos est� vazia; retorna 0 se contiver eventos, 1 se estiver vazia
*  05.Jan.2006 Marcos Portnoi
*/
int evChainIsEmpty()
{
	return (sim[sn].evc_begin == NULL? 1:0);
}

/*----------  CAUSE EVENT  -------------------------------------------*/

void cause(int *ev, int *tkn)
{
	struct evchain *evc;
	evc = sim[sn].evc_begin;

	*tkn = token = evc->ev_tkn;
	*ev = event = evc->ev_type;			/* este event servira para o enqueue token colocar o
																	   evento que devera ser processado associado a token
																		 retirada da fila da facilty */
	clock = evc->ev_time;

	/* Apos a retirada do evento libera-se a area deste evento */

	sim[sn].evc_begin = evc->ev_next;

	if (evc->ev_next == NULL)
	{
		sim[sn].evc_begin = NULL;
		sim[sn].evc_end = NULL;
	}
	else
	{
		evc->ev_next->ev_previous = NULL;
	}


	free(evc);

	/* retorna o apontador da token */

	return;
}

/*--------------------------  RETURN TIME  ---------------------------*/
double simtime()
{
    return(clock);
}

/*-----------------------  GET FACILITY BUSY STATUS  ----------------------*/
int status(int f)
{
	struct facilit *fct;

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - status - facility number does not exist");
		exit (1);
	}
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	return(fct->f_n_serv == fct->f_n_busy_serv? 1:0);
}

/*-----------------------  SET FACILITY STATUS UP ----------------------*/
void setFacUp(int f)
{
	struct facilit *fct;

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - setFacUp - facility number does not exist");
		exit (1);
	}
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	fct->f_up = 1;
}

/*-----------------------  SET FACILITY STATUS DOWN ----------------------
*
* This function sets the facility status down and purges its queue, returning
* the number of tokens purged.
*
* Notice that a token currently is service is not dropped. This has unwanted consequences: for example,
* even when the facility is down, the token currently in service will finish service and will be released
* as scheduled. In a network link, PDUs which have began transmission before the link went down will continue
* being transmitted. This results in transmission finishing and the PDU being propagated through the link, even
* when the link is already down (since the link transmission and link propagate functions are not properly
* handling down links with tokens already in service).
* Must fix this.
*
*/
int setFacDown(int f)
{
	struct facilit *fct;

	if (f > sim[sn].fct_number)
	{
		printf ("\nError - setFacDown - facility number does not exist");
		exit (1);
	}
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	fct->f_up = 0;
	//Now, purge the facility queue; if this is not desired, comment the remaining code
	return(purgeFacQueue(f));
}
/*-----------------------  GET FACILITY UP/DOWN STATUS  ----------------------*/
int getFacUpStatus(int f)
{
    struct facilit *fct;

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - getFacUpStatus - facility number does not exist");
		exit (1);
	}
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	return(fct->f_up);
}

/*---------- ESVAZIA A FILA DE UMA FACILITY - DESCARTA TODOS OS TOKENS ENFILEIRADOS ----------
*
* A fun��o recebe o n�mero da facility cuja fila ser� limpa, e devolve o n�mero de tokens
* descartados (ou seja, o tamanho da fila no momento do esvaziamento).
*
* Esta fun��o foi desenhada a princ�pio para apenas uma fila por facility.
* O objetivo �, para o caso de uma facility entrar em estado down (n�o-operacional),
* haver o descarte de todos os tokens ainda enfileirados para servi�o nesta facility.
* Isto permite simular corretamente, por exemplo, um link que est� down, causando o
* descarte dos pacotes em fila para transmiss�o.
* Observar que, para o caso de um link de rede, tamb�m � necess�rio descartar os pacotes
* em tr�nsito link (em propaga��o).  Idealmente, isto deve ser tratado em outra fun��o.
* 04.Jan.2006 Marcos Portnoi
*
*/
static int purgeFacQueue(int f)
{
	struct facilit *fct;
	struct fqueue *que;
	int i=0; //contador para posi��es da fila descartadas
	
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
		if (fct == NULL) {
			printf("\nError - purgeFacQueue - facility number does not exist");
			exit(1);
		}
	}
	while (fct->f_n_length_q > 0)
	{
    	//fila n�o est� vazia; descarte as tokens em fila, mas atualize as estat�sticas
		que = fct->f_queue;
		fct->f_queue = que->fq_next;
		fct->length_time_prod_sum += fct->f_n_length_q * (clock - fct->f_last_ch_time_q);
		fct->f_n_length_q--;
		//fct->f_exit_count_q++;  //token descartada; ent�o n�o seria uma token dequeued; n�o atualizar esta estat�stica (CORRETO?)
		fct->f_last_ch_time_q = clock;
		i++; //mais uma posi��o descartada:  atualize o contador de descartes para esta fun��o
		fct->f_tkn_dropped++; //atualize o contador de descartes para toda a facility
		free(que->fq_tkp); //elimine o token da mem�ria
		free(que); //descarte a posi��o da fila
	}
	fct->f_queue=NULL; //assegura que fila da facility est� agora vazia
	return i;  //devolve o n�mero de posi��es eliminadas da fila nesta opera��o
}

/*--------------------  GET CURRENT QUEUE LENGTH  --------------------*/
int inq(int f)
{
  struct facilit *fct;
	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - inq - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	return(fct->f_n_length_q);
}

/*--------------------  GET FACILITY UTILIZATION  --------------------*/
double U(int f)
{
  struct facilit *fct;
	double util = 0.0;
	double interval = clock - start;
	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - U (utilization) - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	if ( interval > 0.0 )
	{
		util = fct->f_busy_time / (float) interval;
	}
	return (util);
}

/*----------------------  GET MEAN BUSY PERIOD  ----------------------*/
double B(int f)
{
  struct facilit *fct;
	int n = 0;
	double mbp = 0.0;
	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - B (mean busy period) - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}

	if (fct->f_release_count > 0)
	{
		mbp = fct->f_busy_time / (float) fct->f_release_count;
		return (mbp);
	}
	else
	{
		return (fct->f_busy_time);
	}
}

/*--------------------  GET AVERAGE QUEUE LENGTH  --------------------*/
double Lq(int f)
{
	struct facilit *fct;
	double interval = clock-start;

	if ( f > sim[sn].fct_number)
	{
		printf ("\nError - Lq (average queue length) - facility number does not exist");
		exit (1);
	}

	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
	}
	return((interval>0.0)? (fct->length_time_prod_sum / interval):0.0);
}

/*------------------------  GENERATE REPORT  -------------------------*/
void report()
{
  newpage();
  reportf();
  endpage();
}

/*--------------------  GENERATE FACILITY REPORT  --------------------*/
void reportf()
{
	struct facilit *f;
  if (( f=sim[sn].fct_begin ) == NULL)
	{
		fprintf(opf,"\nno facilities defined:  report abandoned\n");
	}
	else
  { /* f = 0 at end of facility chain */
    while(f)
		{
			f=rept_page(f);
			if (f>0) endpage();
		}
  }
}

/*----------------------  GENERATE REPORT PAGE  ----------------------*/
static struct facilit *rept_page(struct facilit *f)
{

	char fn[19];
  static char *s[7]= {
                      "simm SIMULATION REPORT", " MODEL: ", "TIME: ", "INTERVAL: ",
                      "MEAN BUSY     MEAN QUEUE        OPERATION COUNTS",
                      " FACILITY          UTIL.    ",
                      " PERIOD        LENGTH     RELEASE   PREEMPT   QUEUE" };
  fprintf(opf,"\n%51s\n\n\n",s[0]);
  fprintf(opf,"%-s%-54s%-s%11.3f\n",s[1],sim[sn].name,s[2],clock);
  fprintf(opf,"%68s%11.3f\n\n",s[3],clock-start);
  fprintf(opf,"%75s\n",s[4]);
  fprintf(opf,"%s%s\n",s[5],s[6]);
	lft-=8;
  while(f && lft--)
  {
		if (f->f_n_serv==1)
		{
			sprintf(fn,"%s",f->f_name);
		}
		else
		{
			sprintf(fn,"%s[%d]",f->f_name,f->f_n_serv);
		}
		fprintf(opf," %-17s%6.4f %10.3f %13.3f %11d %9d %7d\n",
    fn,U(f->f_number),B(f->f_number),Lq(f->f_number), f->f_release_count,
		f->f_preempt_count, f->f_exit_count_q);

		f=f->fct_next;
	}
  return(f);
}

/*---------------------------  COUNT LINES  --------------------------*/
int lns(int i)
{
	lft-=i;
	if (lft<=0) endpage();
  return(lft);
}

/*----------------------------  END PAGE  ----------------------------*/
void endpage()
{
	if (opf==display)
  { /* screen output: push to top of screen & pause */
		while(lft>0)
		{
			putc('\n',opf); lft--;
		}
		printf("\n[ENTER] to continue:");
		getchar();
    /* if (mr) then clr_scr(); else */ printf("\n\n");
	}
  else if (lft<pl) putc(FF,opf);
  newpage();
}

/*----------------------------  NEW PAGE  ----------------------------*/
void newpage()
{ /* set line count to top of page/screen after page change/screen  */
  /* clear by 'simm', another SimM module, or simulation program    */
  lft=(opf==display)? sl:pl;
}

/*------------------------  REDIRECT OUTPUT  -------------------------*/
FILE *sendto(FILE *dest)
{
  if (dest!=NULL) opf=dest;
  return(opf);
}
/*------------------  DISPLAY ERROR MESSAGE & EXIT  ------------------*/
void error(int n, char *s)
{
	FILE *dest;
  static char
  *m[8]= { "Simulation Error at Time ",
           "Empty Element Pool",
           "Empty Name Space",
           "Facility Defined After Queue/Schedule",
           "Negative Event Time",
           "Empty Event List",
           "Preempted Token Not in Event List",
	         "Release of Idle/Unowned Facility"
				  };
  dest=opf;
  while(1)
	{ /* send messages to both printer and screen */
    fprintf(dest,"\n**** %s%.3f\n",m[0],clock);
    if (n) fprintf(dest,"     %s\n",m[n]);
    if (s!=NULL) fprintf(dest,"     %s\n",s);
    if (dest==display) break;
		else
		{
			dest=display;
		}
	}
  if (opf!=display) report();
  /* if (mr) then mtr(0,1); */
  exit(0);
}

/*------------------------  GET FACILITY NAME  -----------------------
* Included in July 2005 by Marcos Portnoi
* returns facility name as char pointer, given the facility number assigned
* by the 'facility' function call (which creates the facility)
*/
char *fname(int f)
{
	struct facilit *fct;
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
		if (fct == NULL) {
			printf("\nError - fname - facility number does not exist");
			exit(1);
		}
	}
    return(fct->f_name);  //retorna endere�o do array char contendo o nome; n�o seria preciso usar &, correto?
}

/*---------- GET MODEL NAME ----------
*  Included in July 2005 by Marcos Portnoi
*  returns model name as char pointer
*/
char *mname()
{
    return(sim[sn].name);
}

/*----------------------  GET FACILITY MAXIMAL QUEUE SIZE  ---------------------
* Included in July 2005 by Marcos Portnoi
* returns the facility maximal queue size reached during simulation (for a single queue),
* given the facility number
*/
int getFacMaxQueueSize(int f)
{
	struct facilit *fct;
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
		if (fct == NULL) {
			printf("\nError - getFacMaxQueue - facility number does not exist");
			exit(1);
		}
	}
    return(fct->f_max_queue);
}

/*----------------------  GET FACILITY DROPPED TOKENS COUNT  ---------------------
* Included in 05.Jan.2006 by Marcos Portnoi
* returns the facility dropped token count (for a single queue), given the facility number
*/
int getFacDropTokenCount(int f)
{
	struct facilit *fct;
	fct = sim[sn].fct_begin;
	while (fct->f_number != f)
	{
		fct=fct->fct_next;
		if (fct == NULL) {
			printf("\nError - getFacDropTokenCount - facility number does not exist");
			exit(1);
		}
	}
	return(fct->f_tkn_dropped);
}

/**********************************************************************/
/*          Funcoes para depuracao do programa				                */
/*																	                                  */
/**********************************************************************/

/* Sequencia de teste do programa CR_02
O programa te a seguinte sequencia de execucao:

	1. Faz o cause() obtendo o novo tempo, a token, o tipo do evento e avancado o relogio
	2. Se o tipo for 1 ele escalona como tipo 2 e gera um novo evento tipo 1
	3. Se o tipo for 2 ele encaminha o pkt para a facility designada como proximo nodo pelo pkt
	4. Se o tipo for 3 se nodo destino ele envia para o sink caso nao manda para proxima fct

Entao um acompanhamento da situa��o de simulacao inicial pode ser o seguinte:

	a. Lista a cadeia de eventos e o tempo atual

	1.

	b. Lista a cadeia de eventos e o tempo atual

	2. c - Lista a cadeia de eventos depois de executar o passo 2

	3. d - Lista a cadeia de eventos e a situa��o das facilitys

	4. e - Lista a situa��o dos sinks

*/

void dbg_init()
{

	fp = fopen("debug_cr.txt","w");

	fprintf(fp, "\n -----------------------------------------------------------------------");
	fprintf(fp, "\n Depuracao do Programa CR - Core Routing");
	fprintf(fp, "\n Dia/Hora de Geracao:");

	fclose(fp);
}

void dbg_cab()
{

	int n = 1;

	fp = fopen("debug_cr.txt","a");

	fprintf(fp, "\n\n-----------------------------------------------------------------------");
	fprintf(fp, "\nCadeia:  tempo final - num da tkn - tipo do evento");
	/* fprintf(fp, "\nCadeia:  tempo final - num da tkn - tipo do evento -
	                           nodo em que o pkt se encontra."); */
	fprintf(fp, "\nFacility: nome - num servs - 0/1 (sem/com serv)");
	fprintf(fp, " - fila (0 - vazia), uso acumulado fct");
	fprintf(fp, "\nFacility Servers: num do serv - num tkn (0 sem tkn em servico)");
	fprintf(fp, " - prioridade da tkn, uso acumulado srv");
	fprintf(fp, "\nFacility Queues: prioridade tkn - num tkn - tempo inter evento tkn -");
	fprintf(fp, " - evento associado a tkn\n");

	fclose(fp);
}

void dbg_cause(double tempo, int pkt, int ev)
{
	fp = fopen("debug_cr.txt","a");
	fprintf(fp, "\n[Cause] Clock - %6.2f, Pkt: %4d, Tipo Ev: %2d", tempo, pkt, ev);
	fclose(fp);
}

void dbg_tp_1()
{
	fp = fopen("debug_cr.txt","a");
	fprintf(fp, "\n[Tipo 1] - Escalona pkt tipo 2 e gera um novo pkt tipo 1");
/*fprintf(fp, "\n-------------------------------------------------------"); */
	fclose(fp);
}

void dbg_tp_2()
{
	fp = fopen("debug_cr.txt","a");
	fprintf(fp, "\n[Tipo 2] Encaminha o pkt para a facility designada como proximo nodo pelo pkt");
/*fprintf(fp, "\n----------------------------------------------------------------------------"); */
	fclose(fp);
}

void dbg_tp_3()
{
	fp = fopen("debug_cr.txt","a");
	fprintf(fp, "\n[Tipo 3] Se nodo destino ele envia para o sink caso nao manda para proxima fct");
/*fprintf(fp, "\n-----------------------------------------------------------------------------"); */
	fclose(fp);
}

void dbg_evc()
{
	struct evchain *evc;
	int n = 0;

	fp = fopen("debug_cr.txt","a");

	evc = sim[sn].evc_begin;

	if (evc == NULL)
	{
		fprintf(fp, "\n\n[Cadeia atual]:");
		fprintf(fp, "Cadeia vazia\n");
		return;
	}

	fprintf(fp, "\n\n[Cadeia atual]:");
/*fprintf(fp, "\n --------------\n"); */
	while (evc != NULL)
	{
	 	if ( (n % 3) == 0 )	fprintf (fp, "\n");
		/* Imprime a sequencia por elemento da cadeia: tempo; token; tp evento */
		fprintf(fp,"    [%2d] - %6.2f; %2d; %2d; **",
		            n, evc->ev_time, evc->ev_tkn, evc->ev_type);
		/* Imprime a sequencia por elemento da cadeia: tempo; pacote; onde se encontra; tp evento
		fprintf(fp,"    [%2d] - %2d; %6.2f; %2d; %2d **",
		            n, evc->ev_type, evc->ev_time, evc->ev_tkn, evc->ev_tkn_p->currentNode) */;
		n++;
		evc = evc->ev_next;
	}
	fclose(fp);
}


void dbg_fct()
{
	struct facilit *fct;
	int n = 0;

	fp = fopen("debug_cr.txt","a");

	fct = sim[sn].fct_begin;

	fprintf(fp, "\n[Facilitys]:");
	/*	fprintf(fp, "\n -----------"); */
	while (fct != NULL)
	{
		/* Imprime a sequencia por facility: nome - num de servidores -
		- 0/1 (sem servico ou com servico) - tam da fila (0 sem fila) - uso acumulado */
		fprintf(fp,"\n %s - %2d; %2d; %2d; %2d; %6.2f",
		            fct->f_name, fct->f_n_serv, fct->f_n_busy_serv, fct->f_serv->fs_tkn,
								fct->f_n_length_q, fct->f_busy_time);
		fct = fct->fct_next;
	}
	fclose(fp);
}

void dbg_fct_srv()
{
	struct facilit *fct;
	struct fserv *srv;
	int n = 0;

	fp = fopen("debug_cr.txt","a");

	fprintf(fp, "\n\n[Facilitys - Servidores]:");
/*fprintf(fp, "\n --------------\n"); */

	fct = sim[sn].fct_begin;
	while (fct != NULL)
	{
		fprintf(fp,"\n %s:", fct->f_name);
		srv = fct->f_serv;
		n = 0;
		while (srv != NULL)
		{
	 	if ( (n % 3) == 0 )	fprintf (fp, "\n");
			/* Imprime a sequencia por servidor: num do servidor - num da token em servico
			   (0 sem tkn em servico) - prioridade da token - uso acumulado do servidor */
			fprintf(fp,"    [%2d] - %2d; %2d; %6.2f; **",
	            srv->fs_number, srv->fs_tkn, srv->fs_p_tkn, srv->fs_busy_time);
			n++;
			srv = srv->fs_next;
		}
		fct = fct->fct_next;
	}
	fclose(fp);
}

void dbg_fct_queue()
{
	struct facilit *fct;
	struct fqueue *que;
	int n = 0;

	fp = fopen("debug_cr.txt","a");

	fprintf(fp, "\n\n[Facilitys - Filas]:");
/*fprintf(fp, "\n --------------\n"); */

	fct = sim[sn].fct_begin;
	while (fct != NULL)
	{
		fprintf(fp,"\n %s:", fct->f_name);
		if (fct->f_n_length_q > 0 )
		{
			n = 0;
			que = fct->f_queue;
			while (que != NULL)
			{
	 			if ( (n % 3) == 0 )	fprintf (fp, "\n");
				/* Imprime a sequencia por facility das informacoes da fila se houver:
					 prioridade da tkn - num do tkn - tempo de proc do tkn - evento associado a tkn */
				fprintf(fp,"    [%2d] - %2d; %2d; %6.2f; %2d **",
		            n, que->fq_pri, que->fq_tkn, que->fq_time, que->fq_ev);
				n++;
				que = que->fq_next;
			}
		}
		else
		{
			fprintf(fp,"\n    Fila vazia");
		}

		fct = fct->fct_next;
	}
	fclose(fp);
}


/****************************************************
FUNCOES DO SMPL ANTIGO PARA PODER RODAR MODELOS SMPL
****************************************************/




/*------------------------  REQUEST FACILITY  ------------------------*/
/* ATENCAO EM FUNCAO DA MUDANCA FILOSOFICA ABORDADA NO INICIO TALVEZ
   PRECISE SER FEITA UMA REAVALIACAO DESTA E DAS OUTRAS FUNCOES RELACIONADAS
	 AO PROCESSAMENTO DA FACILITY
	 ENQUEUE
	 RELEASE
 */


/*-------------------------  ENQUEUE TOKEN  --------------------------*/


/*-------RELEASE FACILITY  WITH TOKEN POINTER - FC releasep ----------------*/
/* A funcao facility foi modificada com o acrescimo de um apontador associado
   a tkn para que na area apontada tenha mais informacoes sobre a tkn. Desta
	 maneira pode-se acrescentar caracteristicas da token. Neste caso criou-se
	 uma variavel tkp - token pointer com o tipo da estrutura do pacote. */


/*-----------------------SCHEDULE ---------------------------------------------*/
/* Esta e a funcao original do smpl que nao utiliza o apontador da token */



/*---------------------------------- CAUSE  ---------------------------*/
