/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_types
*
* Nota:    Fun��es reescritas em julho/2005
*			Marcos Portnoi
*
* Copyright (C) 2004, 2005, 2006, 2007 Marcos Portnoi, Sergio F. Brito
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

#pragma once

/* Este arquivo contem a especificacao das structs utilizadas pela simulacao */

/* Esta estrutura define v�rios par�metros do simulador TARVOS.  Os diversos m�dulos usam estes par�metros para seu funcionamento.
*/
struct TarvosParam {
	int pathMsgSize; //tamanho em bytes de uma mensagem de controle PATH
	int resvMsgSize; //tamanho em bytes de uma mensagem de controle RESV
	int helloMsgSize; //tamanho em bytes de uma mensagem de controle HELLO
	int ctrlMsgPrio; //prioridade de uma mensagem de controle (PATH, RESV, HELLO, etc.); usada nas filas PQ do simulador (requestp)
	int ttl; //TTL (Time-To-Live inicial dos pacotes gerados)
	double ctrlMsgTimeout; //tempo default para timeout das mensagens de controle do RSVP-TE para um nodo (para que sejam descartadas da fila de mensagens do nodo)
	double LSPtimeout; //tempo default para timeout para as LSPs que partem de um nodo
	double ResvTimeout; //tempo default para timeout de recursos reservados para uma LSP
	double helloMsgTimeout; //tempo default para timeout de uma mensagem HELLO
	double helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunica��o com o nodo)
	double timeoutWatchdog; //intervalo de tempo em que a rotina de verifica��o de timeout (mensagens de controle e LSPs), para todos os nodos, deve aguardar para nova verifica��o; tempo entre escalonamentos do evento timeoutWatchdog
	int ctrlMsgHandlEv; //n�mero do evento default para tratamento das mensagens de controle para um nodo
	int helloMsgGenEv; //n�mero do evento default especificamente para tratamento das mensagems HELLO para um nodo
	double helloInterval; //intervalo de tempo para gera��o das mensagens HELLO a partir de um LSR
	double LSPrefreshInterval; //intervalo de tempo para gera��o de mensagens PATH refresh para as LSPs
	double ResvRefreshInterval; //intervalo de tempo para gera��o de mensagens RESV refresh para as LSPs
	int traceMain; //flag que ativa as impress�es de trace principais; 0 = OFF, 1 = ON.  V�rias fun��es geram trace para arquivos espec�ficos
	int traceDrop; //flag que ativa as impress�es de trace para pacotes descartados; 0 = OFF, 1 = ON.
	int traceSource; //flag que ativa as impress�es de trace para geradores de tr�fego; 0 = OFF, 1 = ON.
	int traceExpoo; //flag que ativa as impress�es de trace para gerador de tr�fego Exponencial On/Off; 0 = OFF, 1 = ON.
	int traceJitterDelayGlobal; //flag que ativa as impress�es de trace para medi��es de Jitter e Delay nodo a nodo, globais; 0 = OFF, 1 = ON.
	int traceJitterDelayAppl; //flag que ativa as impress�es de trace para medi��es de Jitter e Delay nodo a nodo para aplica��es; 0 = OFF, 1 = ON.
	char libFile[50];  //nome do arquivo contendo a LIB para MPLS (para leitura do simulador)
	char sourceTrace[50];  //nome do arquivo que conter� o dump de certas fontes (para debug)
	char expooTrace[50];  //nome do arquivo que conter� o dump das fontes exponenciais on/off (para debug)
	char traceDump[50];  //nome do arquivo que conter� o trace principal
	char libDump[50];  //nome do arquivo que conter� o conte�do da LIB, impresso pelo programa
	char lspTableDump[50]; //nome do arquivo que conter� o conte�do da LSP Table
	char linksDump[50]; //nome do arquivo que conter� a impress�o dos par�metros dos links
	char dropPktTrace[50]; //nome do arquivo que conter� o trace dos pacotes descartados
	char delayNodes[50];  //nome dos arquivos base que conter�o as medi��es Delay de cada nodo
	char jitterNodes[50];  //nome dos arquivos base que conter�o as medi��es Jitter de cada nodo
	char applDelayNodes[50];  //nome dos arquivos base que conter�o as medi��es Delay de cada nodo para Aplica��es
	char applJitterNodes[50];  //nome dos arquivos base que conter�o as medi��es Jitter de cada nodo para Aplica��es
};

//O link entre dois roteadores atende de maneira uniforme os pacotes que chegam a ele (uniforme em funcao da taxa de transmiss�o e tempo de propaga��o)
struct Link {
    int linkNumber;  //especificacao num�rica do link 01, 02, 03, ...  Devem ser �nicos!
	char name[50]; //nome usado para batizar a facility representativa do link
	double bandwidth;  //largura de banda do link de comunicacao entre dois nodos (tipicamente em bps)
	double delay;  //atraso de propaga��o do link, dependente do meio (tipicamente em segundos)
	double delayOther1; //atrasos extras para modelagem de processamento, roteadores, etc. (tipicamente em segundos)
	double delayOther2;
	int src;  //roteador de onde sairao os pacotes
	int dst;  //roteador para onde irao os pacotes
	int facility;  //n�mero da facility associada ao link
	char status[5]; //status do link:  "up" (funcional) ou "down" (n�o-operacional) (� preciso uma posi��o a mais do que a maior palavra a armazenar aqui, sen�o o comando strcpy, que inclui um /0 ao final do string, invadir� o espa�o do pr�ximo campo
	struct PacketsInTransitQueue *packetsInTransitQueue; //apontador para a lista de pacotes em tr�nsito no link
	double availCir; //Committed Information Rate, � a taxa dispon�vel de enchimento do Token Bucket, em bytes por segundo, que este link admite para RSVP (o limite, a princ�pio, � o pr�prio bandwidth)
	double availCbs; //Committed Burst Size, � o tamanho dispon�vel do Token Bucket, em bytes, que este link admite para RSVP
	double availPir; //Peak Information Rate, � o taxa de pico m�xima dispon�vel que este link admite para RSVP (tipicamente, a pr�pria largura de banda do link)
};

//c�lula da lista de pacotes em tr�nsito em um link
struct PacketsInTransitCell {
	int id;  //n�mero do pacote (ou token) em tr�nsito
	struct PacketsInTransitCell *next; //apontador para pr�xima c�lula da lista
};

//estrutura que guardar� dados dos pacotes em tr�nsito (propaga��o) por um link
struct PacketsInTransitQueue {
	struct PacketsInTransitCell *head; //head node da lista de pacotes em tr�nsito
	struct PacketsInTransitCell *last; //�ltimo item da lista de pacotes em tr�nsito
	int NumberInTransit; //quantidade de pacotes em tr�nsito pelo link
};

/* Estrutura que conter� Objeto Rota Expl�cita
*  Este objeto, por sua vez, � composto por uma rota expl�cita (nodos) que o pacote deve seguir, acompanhado pelo �ndice do array associado,
*  e tamb�m um objeto RecordRoute, que � a grava��o da rota seguida por um pacote, acompanhado pelo �ndice do array associado e por uma flag
*  indicativa, para a fun��o de encaminhamento, se a rota deve ser gravada no objeto RecordRoute.
*/
struct ExplicitRoute {
	int *explicitRoute; //apontador para um array, criado pelo usu�rio, contendo as rotas expl�citas para o pacote
	int erNextIndex; //�ndice para a pr�xima posi��o a ler do array de Rota Expl�cita (contendo os n�meros dos nodos a percorrer)
	int recordThisRoute; //flag que indica se a rota percorrida pelo pacote deve ser gravada; 0 para N�O GRAVAR, 1 para GRAVAR
	int *recordRoute; //apontador para um array, criado pelas rotinas do simulador, contendo a rota percorrida pelo pacote e gravada
	int rrNextIndex; //�ndice para a pr�xima posi��o a gravar do array RecordRoute (contendo os n�meros dos nodos percorridos pelo pacote)
};

/*Lista (ou Queue) de Mensagens Pendentes no Nodo
* Mant�m as mensagens de controle (por exemplo, pelo RSVP-TE) geradas pelo nodo, para que possam ser processadas e/ou
* respondidas apropriadamente.  Por exemplo, uma mensagem PATH necessita ser guardada para que a mensagem RESV de resposta
* possa ser combinada com aquela.
* Manter nesta estrutura todos os par�metros necess�rios.
* A estrutura do Nodo dever� conter um ponteiro para ESTA estrutura.
* A t�cnica usada � uma lista duplamente encadeada, circular, com um Head Node (este Head Node � apontado pela estrutura do Nodo).
*/
struct nodeMsgQueue {
	char msgType[30]; //tipo de mensagem
	int msgID;  //ID �nico da mensagem para controle de ACK
	int msgIDack; //ID da mensagem de controle que deve ser respondida por uma mensagem RESV
	int LSPid;  //ID �nico da LSP
	struct ExplicitRoute er; //objeto Rota Expl�cita (cont�m apontador para um array, criado pelo usu�rio, contendo as rotas expl�citas para o pacote)
	int src;  //nodo que originou esta mensagem
	int dst;  //nodo destino desta mensagem
	int iIface; //interface (n�mero do link) de entrada (incoming interface), ou seja, o link ou interface por onde esta mensagem entrou
	int oIface; //interface (n�mero do link) de sa�da (outgoing interface), ou seja, o link ou interface por onde esta mensagem sair� para o pr�ximo nodo
	int iLabel; //guarda o iLabel do pacote (tipicamente o label armazenado atualmente no pacote)
	int oLabel; //guarda o oLabel para o pacote
	double timeout;  //tempo absoluto de timeout para esta mensagem
	int ev; //evento de tratamento para a mensagem de controle; as fun��es de tratamento das msgs de controle, se necess�rio, dever�o escalonar este evento
	int resourcesReserved; //flag que indica se recursos foram reservados pelo processamento desta mensagem; 0 para N�O, 1 para SIM
	struct nodeMsgQueue *previous; //apontador para a c�lula anterior da lista
	struct nodeMsgQueue *next; //apontador para a pr�xima c�lula da lista
};

/* Estrutura com par�metros para os Geradores de Tr�fego ou Fontes
*/
struct Source {
    int packetsGenerated;  //n�mero de pacotes gerados pela fonte
	double expooAbsoluteTurnOffTime; //tempo em que a fonte expoo deve ser desligada (em tempo absoluto do simulador) (s� para fontes expoo)
};

/* Estrutura para acumular estat�sticas em cada nodo ou seu destino final. Ex.:  quantos pacotes chegaram ao
*  destino e quantos ficaram no caminho.  Se os links forem duplex, estas estat�sticas acumular�o para todos os sentidos adequados.
*/
struct Node {
    //este grupo de estat�sticas � global, compreedendo pacotes de aplica��o e de controle
	int packetsReceived;  //n�mero de pacotes que chegaram ao nodo.  Pode-se verificar ent�o quantos ficaram no caminho (descartados).
	double bytesForwarded; //quantidade de bytes que foram encaminhados a partir deste nodo.  Similar ao packetsForwarded abaixo
	int packetsForwarded; //n�mero de pacotes que foram encaminhados a partir deste nodo.  Quando o nodo n�o for o sorvedouro, o pacote ser� encaminhado por outro link, e esta estat�stica dever� ser atualizada.
	int packetsDropped; //n�mero de pacotes perdidos ou descartados no nodo (partindo dele); � o somat�rio dos pacotes perdidos pela facility (transmiss�o) e os perdidos durante propaga��o, para o caso de link down.
	double meanDelay; //atraso m�dio medido para os pacotes recebidos por este nodo
	double delaySum; //somat�rio de todos os atrasos medidos para os pacotes recebidos por este nodo
	double delay; //atraso do �ltimo pacote recebido por este nodo
	double meanJitter; //jitter m�dio para os atrasos dos pacotes recebidos por este nodo
	double jitterSum; //somat�rio de todos os jitters calculados para os pacotes recebidos por este nodo
	double jitter; //�ltimo jitter calculado para o �ltimo pacote recebido por este nodo
	double bytesReceived; //n�mero de bytes recebidos pelo nodo.  � o tamanho de cada pacote recebido, acumulado (admite-se, aqui, pacotes de tamanho diferente).
	//as estat�sticas abaixo s�o exclusivamente para pacotes de aplica��o, ou seja, n�o-controle
	int packetsReceivedAppl;  //n�mero de pacotes que chegaram ao nodo.  Pode-se verificar ent�o quantos ficaram no caminho (descartados).
	double bytesForwardedAppl; //quantidade de bytes que foram encaminhados a partir deste nodo.  Similar ao packetsForwarded abaixo
	int packetsForwardedAppl; //n�mero de pacotes que foram encaminhados a partir deste nodo.  Quando o nodo n�o for o sorvedouro, o pacote ser� encaminhado por outro link, e esta estat�stica dever� ser atualizada.
	double meanDelayAppl; //atraso m�dio medido para os pacotes recebidos por este nodo
	double delaySumAppl; //somat�rio de todos os atrasos medidos para os pacotes recebidos por este nodo
	double delayAppl; //atraso do �ltimo pacote recebido por este nodo
	double meanJitterAppl; //jitter m�dio para os atrasos dos pacotes recebidos por este nodo
	double jitterSumAppl; //somat�rio de todos os jitters calculados para os pacotes recebidos por este nodo
	double jitterAppl; //�ltimo jitter calculado para o �ltimo pacote recebido por este nodo
	double bytesReceivedAppl; //n�mero de bytes recebidos pelo nodo.  � o tamanho de cada pacote recebido, acumulado (admite-se, aqui, pacotes de tamanho diferente).
	struct nodeMsgQueue *nodeMsgQueue; //apontador para a lista (Queue) de mensagens de controle deste nodo; aponta para o Head Node
	int (*nextLabel)[]; /*pr�ximo r�tulo dispon�vel para uso em constru��o de uma LSP para MPLS; ao usar este r�tulo, este campo deve ser
				   incrementado a fim de assegurar r�tulos �nicos por interface no nodo; o �ndice indica o n�mero da interface, que tamb�m � o n�mero do link do modelo*/
	double ctrlMsgTimeout; //valor de timeout (relativo) para mensagens de controle RSVP-TE em segundos
	int ctrlMsgHandlEv; //n�mero do evento para tratamento de mensagens de controle
	int helloMsgGenEv; //n�mero do evento para tratamento de mensagens espec�ficas HELLO
	double LSPtimeout; /*tempo relativo para a extin��o (timeout) de uma LSP que parte deste nodo; as LSPs devem receber REFRESH periodicamente (o RSVP-TE � soft state).
					 A fun��o de cria��o de uma LSP deve recolher este valor de modo a calcular o tempo absoluto de timeout de uma LSP.  Uma fun��o espec�fica de timeout
					 deve ser chamada periodicamente a fim de extinguir as LSPs.*/
	double helloMsgTimeout; //tempo default para timeout do timer de recebimento de uma mensagem HELLO (o estouro indica falha na comunica��o com o nodo)
	double helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunica��o com o nodo)
	double (*helloTimeLimit)[]; //tempo limite para que o nodo na outra ponta do link ligado a este nodo reporte HELLO_ACK (em tempo absoluto simtime())
};

/* A estrutura a seguir representa o header (cabe�alho) para os protocolos de controle de r�tulos (labels)
*/
struct labelHeader {
	int label;  //r�tulo para uso do MPLS
	char msgType[30]; //tipo de mensagem
	char errorCode[50]; //c�digo de erro para PathErr e ResvErr
	char errorValue[50]; //valor ou tipo de erro para PathErr e ResvErr
	int msgID;  //ID �nico desta mensagem para controle de ACK
	int msgIDack; //ID da mensagem de controle para qual esta mensagem serve de ACK ou resposta
	int LSPid;  //ID �nico da LSP por qual trafegar� este pacote
	int priority; //prioridade de tratamento para o pacote; � exatamente a prioridade a ser usada na fun��o requestp do SimM (quanto maior o n�mero, maior a prioridade)
};

/* A estrutura a seguir e a estrutura da token que sera usada ao longo da simulacao. Neste
*  caso usaremos a denominacao packet mas pode ser pensada em uma denominacao mais generica
*  tipo token para facilitar o desacoplamento das funcoes da maquina de simulacao
*/
struct Packet {
    int id;	 //n�mero do pacote, em tempo de cria��o ele leva um n�mero sequencial
	int currentNode;  //nodo atual onde o pacote est�
	int length;  //tamanho do pacote (tipicamente em bytes)
	int ttl; //time-to-live; limite de hops do pacote (mesmo TTL do protocolo IP)
	int outgoingLink;  /*link de sa�da por onde o pacote ir� para o pr�ximo nodo; para MPLS, este n�mero tamb�m indica
				         a interface do nodo (ou roteador); cada interface ter� portanto um n�mero �nico em toda a topologia
						 (n�o pode haver mais de uma interface, independente do nodo, com o mesmo n�mero)*/
	struct labelHeader lblHdr;  //header dos protocolos de label
	int src;  //roteador (nodo) para o qual o pacote foi gerado; origem do pacote
	int dst;  //roteador (nodo) para o qual o pacote devera se dirigir; destino final do pacote
	double generationTime;  //generation time - tempo absoluto em que o pacote foi gerado
	struct ExplicitRoute er; //objeto Rota Expl�cita (cont�m rota expl�cita a ser seguida pelo pacote, como tamb�m a rota gravada)
	struct Packet *previous;  //apontador para o pacote anterior da lista ps (para tratar fragmenta��o?)
	struct Packet *next;  //apontador para o proximo pacote da lista ps (o que � ps?)
};

/* As duas estruturas seguintes implementam a LIB (Label Information Base)
*  a t�cnica � lista din�mica duplamente encadeada, circular, com um Head Node
*/
struct LIBEntry {
	int node; /*nodo atual.  Tecnicamente, cada nodo (roteador) deveria ter sua pr�pria tabela de roteamento,
				mas aqui todas elas s�o concentradas em uma s�.  Portanto, � necess�rio ter um �ndice para os
				nodos. */
	int iIface; //incoming interface; aqui, as interfaces s�o os pr�prios n�meros dos links, que s�o �nicos
	int iLabel; //incoming label
	int oIface; //outgoing interface; aqui, as interfaces s�o os pr�prios n�meros dos links, que s�o �nicos
	int oLabel; //outgoing label
	char status[20]; //status da entrada da LIB; talvez sirva para mapear com o estado da pr�pria FEC; a entrada na LIB � uma FEC?
	int bak; //0 = indica que esta LSP n�o � BACKUP (� working LSP); se != 0, indica que esta LSP � BACKUP
	int LSPid; //�ndice para a tabela de LSPids; este n�mero identifica uma LSP �nica no dom�nio MPLS, e jamais pode ser repetido
	double timeout; //tempo absoluto (no rel�gio de simula��o) de timeout para esta LSP
	double timeoutStamp; //tempo absoluto em que a LSP foi colocada em status timed-out
	struct LIBEntry *previous; //aponta para a c�lula anterior da lista LIB
	struct LIBEntry *next; //aponta para a pr�xima c�lula da lista LIB
};

struct LIB {
	struct LIBEntry *head; //aponta para c�lula Head Node da LIB
	int size; //tamanho da LIB em n�mero de linhas ou c�lulas
};

/* As duas estruturas seguintes implementam a LSP Table - tabela de par�metros de Constraint Routing por LSPid
*  A t�cnica � lista din�mica duplamente encadeada, circular, com um Head Node
*/
struct LSPTableEntry {
	int LSPid; //este n�mero identifica uma LSP �nica no dom�nio MPLS, e jamais pode ser repetido
	int src; //nodo origem deste t�nel LSP
	int dst; //nodo destino deste t�nel LSP
	char trafParam[10];  //par�metros de tr�fego:  Frequency, Weight, PDR, PBS, CDR, CBS, EBS
	int setPrio; //Setup Priority, prioridade (para preemp��o) para aloca��o de recursos da LSP corrente; 0-7, 0 � a mais alta (RFC 3209); esta NUNCA deve ser mais alta que a holding priority
	int holdPrio; //Holding Priority, prioridade (para preemp��o) que a LSP tem de manter os recursos j� reservados; 0-7, 0 � a mais alta (RFC 3209)
	double cir; //Committed Information Rate, em bytes por segundo, par�metro do Token Bucket para RSVP
	double cbs; //Committed Bucket Size, tamanho do Bucket em bytes, par�metro para RSVP
	double pir; //Peak Information Rate, em bytes por segundo, par�metro para RSVP
	double cBucket; //Tamanho do Committed Bucket (tamanho real) em bytes
	double arrivalTime; //Tempo de Chegada absoluto do �ltimo pacote que chegou para o Bucket
	int minPolUnit; //minimum policed unit, o tamanho m�nimo de um datagrama que sofrer� policing.  Qualquer datagrama menor que este ser� considerado como tendo o tamanho m�nimo
	int maxPktSize; //tamanho m�ximo do pacote considerado conforme; pacotes maiores que este valor ser�o imediatamente considerados n�o-conformes
	int tunnelDone; //0 = indica que o LSP tunnel ainda n�o foi completado integralmente (RESV_LABEL_MAPPING n�o chegou no LER de ingresso); 1 = LSP tunnel est� completo
	struct LSPTableEntry *previous; //aponta para a c�lula anterior da lista LSP Table
	struct LSPTableEntry *next; //aponta para a pr�xima c�lula da lista LSP Table
};

struct LSPTable {
	struct LSPTableEntry *head; //aponta para c�lula Head Node da LSP Table
	int size; //tamanho da LSP Table em n�mero de linhas ou c�lulas
};

/* Estrutura que cont�m o modelo para a topologia da rede, composto por Links, Nodos e Fontes, e que ser� definida como global.
*  A estrutura permite empacotar o modelo num s� "namespace".
*/
struct TarvosModel {
	struct Link lnk[LINKS + 1]; //LINKS � o n�mero total de conex�es entre os nodos
	struct Source src[GENERATORS + 1]; //generators � o numero de fontes que gerarao carga no nucleo
	struct Node node[NODES + 1]; //NODES especifica o n�mero de nodos que h� no modelo
};

/* Estruturas de apoio para uma lista duplamente encadeada, circular, com Head Node, para uso da preemp��o de LSPs.  A lista conter� as LSPs, por nodo
*  ordenadas da menor prioridade (n�mero mais alto) para a maior prioridade (n�mero mais baixo de prioridade).
*/
struct LspListItem {
	int LSPid; //ID da LSP
	int setPrio; //Setup Priority [0-7]; n�o deve ser mais alta que Holding Priority
	int holdPrio; //Holding Priority [0-7]
	struct LIBEntry *libEntry; //ponteiro para a entrada na LIB correspondente
	struct LspListItem *previous; //ponteiros para item anterior e pr�ximo da lista
	struct LspListItem *next;
};

/* Lista de LSPs por nodo, contendo as prioridades, para uso das fun��es de preemp��o de LSPs.  A lista ser� ordenada da menor para a maior prioridade.
*  Aqui est�o os ponteiros para o primeiro (HEAD NODE) e �ltimo elemento da lista.
*/
struct LspList {
	struct LspListItem *head;
	int size; //tamanho da lista
};
