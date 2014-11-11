/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_types
*
* Nota:    Funções reescritas em julho/2005
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

/* Esta estrutura define vários parâmetros do simulador TARVOS.  Os diversos módulos usam estes parâmetros para seu funcionamento.
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
	double helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunicação com o nodo)
	double timeoutWatchdog; //intervalo de tempo em que a rotina de verificação de timeout (mensagens de controle e LSPs), para todos os nodos, deve aguardar para nova verificação; tempo entre escalonamentos do evento timeoutWatchdog
	int ctrlMsgHandlEv; //número do evento default para tratamento das mensagens de controle para um nodo
	int helloMsgGenEv; //número do evento default especificamente para tratamento das mensagems HELLO para um nodo
	double helloInterval; //intervalo de tempo para geração das mensagens HELLO a partir de um LSR
	double LSPrefreshInterval; //intervalo de tempo para geração de mensagens PATH refresh para as LSPs
	double ResvRefreshInterval; //intervalo de tempo para geração de mensagens RESV refresh para as LSPs
	int traceMain; //flag que ativa as impressões de trace principais; 0 = OFF, 1 = ON.  Várias funções geram trace para arquivos específicos
	int traceDrop; //flag que ativa as impressões de trace para pacotes descartados; 0 = OFF, 1 = ON.
	int traceSource; //flag que ativa as impressões de trace para geradores de tráfego; 0 = OFF, 1 = ON.
	int traceExpoo; //flag que ativa as impressões de trace para gerador de tráfego Exponencial On/Off; 0 = OFF, 1 = ON.
	int traceJitterDelayGlobal; //flag que ativa as impressões de trace para medições de Jitter e Delay nodo a nodo, globais; 0 = OFF, 1 = ON.
	int traceJitterDelayAppl; //flag que ativa as impressões de trace para medições de Jitter e Delay nodo a nodo para aplicações; 0 = OFF, 1 = ON.
	char libFile[50];  //nome do arquivo contendo a LIB para MPLS (para leitura do simulador)
	char sourceTrace[50];  //nome do arquivo que conterá o dump de certas fontes (para debug)
	char expooTrace[50];  //nome do arquivo que conterá o dump das fontes exponenciais on/off (para debug)
	char traceDump[50];  //nome do arquivo que conterá o trace principal
	char libDump[50];  //nome do arquivo que conterá o conteúdo da LIB, impresso pelo programa
	char lspTableDump[50]; //nome do arquivo que conterá o conteúdo da LSP Table
	char linksDump[50]; //nome do arquivo que conterá a impressão dos parâmetros dos links
	char dropPktTrace[50]; //nome do arquivo que conterá o trace dos pacotes descartados
	char delayNodes[50];  //nome dos arquivos base que conterão as medições Delay de cada nodo
	char jitterNodes[50];  //nome dos arquivos base que conterão as medições Jitter de cada nodo
	char applDelayNodes[50];  //nome dos arquivos base que conterão as medições Delay de cada nodo para Aplicações
	char applJitterNodes[50];  //nome dos arquivos base que conterão as medições Jitter de cada nodo para Aplicações
};

//O link entre dois roteadores atende de maneira uniforme os pacotes que chegam a ele (uniforme em funcao da taxa de transmissão e tempo de propagação)
struct Link {
    int linkNumber;  //especificacao numérica do link 01, 02, 03, ...  Devem ser únicos!
	char name[50]; //nome usado para batizar a facility representativa do link
	double bandwidth;  //largura de banda do link de comunicacao entre dois nodos (tipicamente em bps)
	double delay;  //atraso de propagação do link, dependente do meio (tipicamente em segundos)
	double delayOther1; //atrasos extras para modelagem de processamento, roteadores, etc. (tipicamente em segundos)
	double delayOther2;
	int src;  //roteador de onde sairao os pacotes
	int dst;  //roteador para onde irao os pacotes
	int facility;  //número da facility associada ao link
	char status[5]; //status do link:  "up" (funcional) ou "down" (não-operacional) (é preciso uma posição a mais do que a maior palavra a armazenar aqui, senão o comando strcpy, que inclui um /0 ao final do string, invadirá o espaço do próximo campo
	struct PacketsInTransitQueue *packetsInTransitQueue; //apontador para a lista de pacotes em trânsito no link
	double availCir; //Committed Information Rate, é a taxa disponível de enchimento do Token Bucket, em bytes por segundo, que este link admite para RSVP (o limite, a princípio, é o próprio bandwidth)
	double availCbs; //Committed Burst Size, é o tamanho disponível do Token Bucket, em bytes, que este link admite para RSVP
	double availPir; //Peak Information Rate, é o taxa de pico máxima disponível que este link admite para RSVP (tipicamente, a própria largura de banda do link)
};

//célula da lista de pacotes em trânsito em um link
struct PacketsInTransitCell {
	int id;  //número do pacote (ou token) em trânsito
	struct PacketsInTransitCell *next; //apontador para próxima célula da lista
};

//estrutura que guardará dados dos pacotes em trânsito (propagação) por um link
struct PacketsInTransitQueue {
	struct PacketsInTransitCell *head; //head node da lista de pacotes em trânsito
	struct PacketsInTransitCell *last; //último item da lista de pacotes em trânsito
	int NumberInTransit; //quantidade de pacotes em trânsito pelo link
};

/* Estrutura que conterá Objeto Rota Explícita
*  Este objeto, por sua vez, é composto por uma rota explícita (nodos) que o pacote deve seguir, acompanhado pelo índice do array associado,
*  e também um objeto RecordRoute, que é a gravação da rota seguida por um pacote, acompanhado pelo índice do array associado e por uma flag
*  indicativa, para a função de encaminhamento, se a rota deve ser gravada no objeto RecordRoute.
*/
struct ExplicitRoute {
	int *explicitRoute; //apontador para um array, criado pelo usuário, contendo as rotas explícitas para o pacote
	int erNextIndex; //índice para a próxima posição a ler do array de Rota Explícita (contendo os números dos nodos a percorrer)
	int recordThisRoute; //flag que indica se a rota percorrida pelo pacote deve ser gravada; 0 para NÃO GRAVAR, 1 para GRAVAR
	int *recordRoute; //apontador para um array, criado pelas rotinas do simulador, contendo a rota percorrida pelo pacote e gravada
	int rrNextIndex; //índice para a próxima posição a gravar do array RecordRoute (contendo os números dos nodos percorridos pelo pacote)
};

/*Lista (ou Queue) de Mensagens Pendentes no Nodo
* Mantém as mensagens de controle (por exemplo, pelo RSVP-TE) geradas pelo nodo, para que possam ser processadas e/ou
* respondidas apropriadamente.  Por exemplo, uma mensagem PATH necessita ser guardada para que a mensagem RESV de resposta
* possa ser combinada com aquela.
* Manter nesta estrutura todos os parâmetros necessários.
* A estrutura do Nodo deverá conter um ponteiro para ESTA estrutura.
* A técnica usada é uma lista duplamente encadeada, circular, com um Head Node (este Head Node é apontado pela estrutura do Nodo).
*/
struct nodeMsgQueue {
	char msgType[30]; //tipo de mensagem
	int msgID;  //ID único da mensagem para controle de ACK
	int msgIDack; //ID da mensagem de controle que deve ser respondida por uma mensagem RESV
	int LSPid;  //ID único da LSP
	struct ExplicitRoute er; //objeto Rota Explícita (contém apontador para um array, criado pelo usuário, contendo as rotas explícitas para o pacote)
	int src;  //nodo que originou esta mensagem
	int dst;  //nodo destino desta mensagem
	int iIface; //interface (número do link) de entrada (incoming interface), ou seja, o link ou interface por onde esta mensagem entrou
	int oIface; //interface (número do link) de saída (outgoing interface), ou seja, o link ou interface por onde esta mensagem sairá para o próximo nodo
	int iLabel; //guarda o iLabel do pacote (tipicamente o label armazenado atualmente no pacote)
	int oLabel; //guarda o oLabel para o pacote
	double timeout;  //tempo absoluto de timeout para esta mensagem
	int ev; //evento de tratamento para a mensagem de controle; as funções de tratamento das msgs de controle, se necessário, deverão escalonar este evento
	int resourcesReserved; //flag que indica se recursos foram reservados pelo processamento desta mensagem; 0 para NÃO, 1 para SIM
	struct nodeMsgQueue *previous; //apontador para a célula anterior da lista
	struct nodeMsgQueue *next; //apontador para a próxima célula da lista
};

/* Estrutura com parâmetros para os Geradores de Tráfego ou Fontes
*/
struct Source {
    int packetsGenerated;  //número de pacotes gerados pela fonte
	double expooAbsoluteTurnOffTime; //tempo em que a fonte expoo deve ser desligada (em tempo absoluto do simulador) (só para fontes expoo)
};

/* Estrutura para acumular estatísticas em cada nodo ou seu destino final. Ex.:  quantos pacotes chegaram ao
*  destino e quantos ficaram no caminho.  Se os links forem duplex, estas estatísticas acumularão para todos os sentidos adequados.
*/
struct Node {
    //este grupo de estatísticas é global, compreedendo pacotes de aplicação e de controle
	int packetsReceived;  //número de pacotes que chegaram ao nodo.  Pode-se verificar então quantos ficaram no caminho (descartados).
	double bytesForwarded; //quantidade de bytes que foram encaminhados a partir deste nodo.  Similar ao packetsForwarded abaixo
	int packetsForwarded; //número de pacotes que foram encaminhados a partir deste nodo.  Quando o nodo não for o sorvedouro, o pacote será encaminhado por outro link, e esta estatística deverá ser atualizada.
	int packetsDropped; //número de pacotes perdidos ou descartados no nodo (partindo dele); é o somatório dos pacotes perdidos pela facility (transmissão) e os perdidos durante propagação, para o caso de link down.
	double meanDelay; //atraso médio medido para os pacotes recebidos por este nodo
	double delaySum; //somatório de todos os atrasos medidos para os pacotes recebidos por este nodo
	double delay; //atraso do último pacote recebido por este nodo
	double meanJitter; //jitter médio para os atrasos dos pacotes recebidos por este nodo
	double jitterSum; //somatório de todos os jitters calculados para os pacotes recebidos por este nodo
	double jitter; //último jitter calculado para o último pacote recebido por este nodo
	double bytesReceived; //número de bytes recebidos pelo nodo.  É o tamanho de cada pacote recebido, acumulado (admite-se, aqui, pacotes de tamanho diferente).
	//as estatísticas abaixo são exclusivamente para pacotes de aplicação, ou seja, não-controle
	int packetsReceivedAppl;  //número de pacotes que chegaram ao nodo.  Pode-se verificar então quantos ficaram no caminho (descartados).
	double bytesForwardedAppl; //quantidade de bytes que foram encaminhados a partir deste nodo.  Similar ao packetsForwarded abaixo
	int packetsForwardedAppl; //número de pacotes que foram encaminhados a partir deste nodo.  Quando o nodo não for o sorvedouro, o pacote será encaminhado por outro link, e esta estatística deverá ser atualizada.
	double meanDelayAppl; //atraso médio medido para os pacotes recebidos por este nodo
	double delaySumAppl; //somatório de todos os atrasos medidos para os pacotes recebidos por este nodo
	double delayAppl; //atraso do último pacote recebido por este nodo
	double meanJitterAppl; //jitter médio para os atrasos dos pacotes recebidos por este nodo
	double jitterSumAppl; //somatório de todos os jitters calculados para os pacotes recebidos por este nodo
	double jitterAppl; //último jitter calculado para o último pacote recebido por este nodo
	double bytesReceivedAppl; //número de bytes recebidos pelo nodo.  É o tamanho de cada pacote recebido, acumulado (admite-se, aqui, pacotes de tamanho diferente).
	struct nodeMsgQueue *nodeMsgQueue; //apontador para a lista (Queue) de mensagens de controle deste nodo; aponta para o Head Node
	int (*nextLabel)[]; /*próximo rótulo disponível para uso em construção de uma LSP para MPLS; ao usar este rótulo, este campo deve ser
				   incrementado a fim de assegurar rótulos únicos por interface no nodo; o índice indica o número da interface, que também é o número do link do modelo*/
	double ctrlMsgTimeout; //valor de timeout (relativo) para mensagens de controle RSVP-TE em segundos
	int ctrlMsgHandlEv; //número do evento para tratamento de mensagens de controle
	int helloMsgGenEv; //número do evento para tratamento de mensagens específicas HELLO
	double LSPtimeout; /*tempo relativo para a extinção (timeout) de uma LSP que parte deste nodo; as LSPs devem receber REFRESH periodicamente (o RSVP-TE é soft state).
					 A função de criação de uma LSP deve recolher este valor de modo a calcular o tempo absoluto de timeout de uma LSP.  Uma função específica de timeout
					 deve ser chamada periodicamente a fim de extinguir as LSPs.*/
	double helloMsgTimeout; //tempo default para timeout do timer de recebimento de uma mensagem HELLO (o estouro indica falha na comunicação com o nodo)
	double helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunicação com o nodo)
	double (*helloTimeLimit)[]; //tempo limite para que o nodo na outra ponta do link ligado a este nodo reporte HELLO_ACK (em tempo absoluto simtime())
};

/* A estrutura a seguir representa o header (cabeçalho) para os protocolos de controle de rótulos (labels)
*/
struct labelHeader {
	int label;  //rótulo para uso do MPLS
	char msgType[30]; //tipo de mensagem
	char errorCode[50]; //código de erro para PathErr e ResvErr
	char errorValue[50]; //valor ou tipo de erro para PathErr e ResvErr
	int msgID;  //ID único desta mensagem para controle de ACK
	int msgIDack; //ID da mensagem de controle para qual esta mensagem serve de ACK ou resposta
	int LSPid;  //ID único da LSP por qual trafegará este pacote
	int priority; //prioridade de tratamento para o pacote; é exatamente a prioridade a ser usada na função requestp do SimM (quanto maior o número, maior a prioridade)
};

/* A estrutura a seguir e a estrutura da token que sera usada ao longo da simulacao. Neste
*  caso usaremos a denominacao packet mas pode ser pensada em uma denominacao mais generica
*  tipo token para facilitar o desacoplamento das funcoes da maquina de simulacao
*/
struct Packet {
    int id;	 //número do pacote, em tempo de criação ele leva um número sequencial
	int currentNode;  //nodo atual onde o pacote está
	int length;  //tamanho do pacote (tipicamente em bytes)
	int ttl; //time-to-live; limite de hops do pacote (mesmo TTL do protocolo IP)
	int outgoingLink;  /*link de saída por onde o pacote irá para o próximo nodo; para MPLS, este número também indica
				         a interface do nodo (ou roteador); cada interface terá portanto um número único em toda a topologia
						 (não pode haver mais de uma interface, independente do nodo, com o mesmo número)*/
	struct labelHeader lblHdr;  //header dos protocolos de label
	int src;  //roteador (nodo) para o qual o pacote foi gerado; origem do pacote
	int dst;  //roteador (nodo) para o qual o pacote devera se dirigir; destino final do pacote
	double generationTime;  //generation time - tempo absoluto em que o pacote foi gerado
	struct ExplicitRoute er; //objeto Rota Explícita (contém rota explícita a ser seguida pelo pacote, como também a rota gravada)
	struct Packet *previous;  //apontador para o pacote anterior da lista ps (para tratar fragmentação?)
	struct Packet *next;  //apontador para o proximo pacote da lista ps (o que é ps?)
};

/* As duas estruturas seguintes implementam a LIB (Label Information Base)
*  a técnica é lista dinâmica duplamente encadeada, circular, com um Head Node
*/
struct LIBEntry {
	int node; /*nodo atual.  Tecnicamente, cada nodo (roteador) deveria ter sua própria tabela de roteamento,
				mas aqui todas elas são concentradas em uma só.  Portanto, é necessário ter um índice para os
				nodos. */
	int iIface; //incoming interface; aqui, as interfaces são os próprios números dos links, que são únicos
	int iLabel; //incoming label
	int oIface; //outgoing interface; aqui, as interfaces são os próprios números dos links, que são únicos
	int oLabel; //outgoing label
	char status[20]; //status da entrada da LIB; talvez sirva para mapear com o estado da própria FEC; a entrada na LIB é uma FEC?
	int bak; //0 = indica que esta LSP não é BACKUP (é working LSP); se != 0, indica que esta LSP é BACKUP
	int LSPid; //índice para a tabela de LSPids; este número identifica uma LSP única no domínio MPLS, e jamais pode ser repetido
	double timeout; //tempo absoluto (no relógio de simulação) de timeout para esta LSP
	double timeoutStamp; //tempo absoluto em que a LSP foi colocada em status timed-out
	struct LIBEntry *previous; //aponta para a célula anterior da lista LIB
	struct LIBEntry *next; //aponta para a próxima célula da lista LIB
};

struct LIB {
	struct LIBEntry *head; //aponta para célula Head Node da LIB
	int size; //tamanho da LIB em número de linhas ou células
};

/* As duas estruturas seguintes implementam a LSP Table - tabela de parâmetros de Constraint Routing por LSPid
*  A técnica é lista dinâmica duplamente encadeada, circular, com um Head Node
*/
struct LSPTableEntry {
	int LSPid; //este número identifica uma LSP única no domínio MPLS, e jamais pode ser repetido
	int src; //nodo origem deste túnel LSP
	int dst; //nodo destino deste túnel LSP
	char trafParam[10];  //parâmetros de tráfego:  Frequency, Weight, PDR, PBS, CDR, CBS, EBS
	int setPrio; //Setup Priority, prioridade (para preempção) para alocação de recursos da LSP corrente; 0-7, 0 é a mais alta (RFC 3209); esta NUNCA deve ser mais alta que a holding priority
	int holdPrio; //Holding Priority, prioridade (para preempção) que a LSP tem de manter os recursos já reservados; 0-7, 0 é a mais alta (RFC 3209)
	double cir; //Committed Information Rate, em bytes por segundo, parâmetro do Token Bucket para RSVP
	double cbs; //Committed Bucket Size, tamanho do Bucket em bytes, parâmetro para RSVP
	double pir; //Peak Information Rate, em bytes por segundo, parâmetro para RSVP
	double cBucket; //Tamanho do Committed Bucket (tamanho real) em bytes
	double arrivalTime; //Tempo de Chegada absoluto do último pacote que chegou para o Bucket
	int minPolUnit; //minimum policed unit, o tamanho mínimo de um datagrama que sofrerá policing.  Qualquer datagrama menor que este será considerado como tendo o tamanho mínimo
	int maxPktSize; //tamanho máximo do pacote considerado conforme; pacotes maiores que este valor serão imediatamente considerados não-conformes
	int tunnelDone; //0 = indica que o LSP tunnel ainda não foi completado integralmente (RESV_LABEL_MAPPING não chegou no LER de ingresso); 1 = LSP tunnel está completo
	struct LSPTableEntry *previous; //aponta para a célula anterior da lista LSP Table
	struct LSPTableEntry *next; //aponta para a próxima célula da lista LSP Table
};

struct LSPTable {
	struct LSPTableEntry *head; //aponta para célula Head Node da LSP Table
	int size; //tamanho da LSP Table em número de linhas ou células
};

/* Estrutura que contém o modelo para a topologia da rede, composto por Links, Nodos e Fontes, e que será definida como global.
*  A estrutura permite empacotar o modelo num só "namespace".
*/
struct TarvosModel {
	struct Link lnk[LINKS + 1]; //LINKS é o número total de conexões entre os nodos
	struct Source src[GENERATORS + 1]; //generators é o numero de fontes que gerarao carga no nucleo
	struct Node node[NODES + 1]; //NODES especifica o número de nodos que há no modelo
};

/* Estruturas de apoio para uma lista duplamente encadeada, circular, com Head Node, para uso da preempção de LSPs.  A lista conterá as LSPs, por nodo
*  ordenadas da menor prioridade (número mais alto) para a maior prioridade (número mais baixo de prioridade).
*/
struct LspListItem {
	int LSPid; //ID da LSP
	int setPrio; //Setup Priority [0-7]; não deve ser mais alta que Holding Priority
	int holdPrio; //Holding Priority [0-7]
	struct LIBEntry *libEntry; //ponteiro para a entrada na LIB correspondente
	struct LspListItem *previous; //ponteiros para item anterior e próximo da lista
	struct LspListItem *next;
};

/* Lista de LSPs por nodo, contendo as prioridades, para uso das funções de preempção de LSPs.  A lista será ordenada da menor para a maior prioridade.
*  Aqui estão os ponteiros para o primeiro (HEAD NODE) e último elemento da lista.
*/
struct LspList {
	struct LspListItem *head;
	int size; //tamanho da lista
};
