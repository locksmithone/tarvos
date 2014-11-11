/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_node
*
* Funções que serão usadas pelo programa de roteamento do núcleo para criação, parametrização,
* inicialização, operação dos nodos.
*
* Copyright (C) 2005, 2006, 2007 Marcos Portnoi
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

#include "simm_globals.h" //para uso da função nodeDropPacket, que faz uma chamada a simtime().  Se esta chamada for dispensada, pode-se apagar esta linha
#include "tarvos_globals.h"

//Prototypes das funções locais (static)
static struct nodeMsgQueue *createNodeMsgQueue(int n_node);
static int nodeReceiveCtrlMsg(struct Packet *pkt);
static int nodeCreateLabel(int n_node, int iFace);
static int nodeProcessPathLabelRequest(struct Packet *pkt);
static int nodeProcessResvLabelMapping(struct Packet *pkt);
static int nodeProcessPathRefreshNoMerge(struct Packet *pkt);
static int nodeProcessResvRefreshNoMerge(struct Packet *pkt);
static int nodeProcessPathRefresh(struct Packet *pkt);
static int nodeProcessResvRefresh(struct Packet *pkt);
static int nodeProcessHello(struct Packet *pkt);
static int nodeProcessHelloAck(struct Packet *pkt);
static int nodeProcessPathDetour(struct Packet *pkt);
static int nodeProcessResvDetourMapping(struct Packet *pkt);
static int nodeProcessPathErr(struct Packet *pkt);
static int nodeProcessResvErr(struct Packet *pkt);
static void nodeUpdateStats(struct Packet *pkt);
static void nodeUpdateForwardStats(struct Packet *pkt);
static int nodeProcessPathPreempt(struct Packet *pkt);
static int nodeProcessResvPreempt(struct Packet *pkt);

/* CRIAÇÃO DOS NODOS
*
*  Cria a Estrutura dos Nodos, que conterá campos para armazenagem de estatísticas e outras coisas.
*  Os parâmetros a passar são o número do nodo a criar (que é o índice do vetor de estruturas de nodos) e o número de interfaces que o nodo
*  conterá.  Cada número de interface, neste simulador, coincidirá necessariamente com o número único do link a que a interface está conectada.
*  O vetor de interfaces será criado dinamicamente.
*/
void createNode(int n_node) {
    //Inicializa estatísticas dos nodos
	int i, links;
	
	tarvosModel.node[n_node].packetsReceived = 0; //Número de pacotes que chegaram a este nodo
	tarvosModel.node[n_node].packetsForwarded = 0; //Número de pacotes que foram encaminhados a partir deste nodo
	tarvosModel.node[n_node].packetsDropped = 0; //Número de pacotes descartados ou perdidos no nodo (sempre partindo do nodo)(transmissão + propagação)
	tarvosModel.node[n_node].bytesReceived = 0; //Quantidade de bytes recebidos pelo nodo (admite-se pacotes de tamanho diferente)
	tarvosModel.node[n_node].bytesForwarded = 0; //quantidade de bytes encaminhados a partir deste nodo
	tarvosModel.node[n_node].delay=0; //atraso medido para o último pacote recebido pelo nodo
	tarvosModel.node[n_node].delaySum=0; //somatório dos atrasos medidos para pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanDelay=0; //atraso médio medido para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].jitter=0; //último jitter medido para o último pacote recebido por este nodo
	tarvosModel.node[n_node].jitterSum=0; //somatório dos jitters medidos para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanJitter=0; //jitter médio medido para os pacotes recebidos por este nodo
	//abaixo, estatísticas para pacotes não-controle
	tarvosModel.node[n_node].packetsReceivedAppl = 0; //Número de pacotes que chegaram a este nodo
	tarvosModel.node[n_node].packetsForwardedAppl = 0; //Número de pacotes que foram encaminhados a partir deste nodo
	tarvosModel.node[n_node].bytesReceivedAppl = 0; //Quantidade de bytes recebidos pelo nodo (admite-se pacotes de tamanho diferente)
	tarvosModel.node[n_node].bytesForwardedAppl = 0; //quantidade de bytes encaminhados a partir deste nodo
	tarvosModel.node[n_node].delayAppl=0; //atraso medido para o último pacote recebido pelo nodo
	tarvosModel.node[n_node].delaySumAppl=0; //somatório dos atrasos medidos para pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanDelayAppl=0; //atraso médio medido para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].jitterAppl=0; //último jitter medido para o último pacote recebido por este nodo
	tarvosModel.node[n_node].jitterSumAppl=0; //somatório dos jitters medidos para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanJitterAppl=0; //jitter médio medido para os pacotes recebidos por este nodo
	links=(sizeof tarvosModel.lnk / sizeof *(tarvosModel.lnk)); //tamanho do vetor de links (que é o número real de links + 1)
	tarvosModel.node[n_node].nextLabel=(int(*)[])malloc(links * sizeof(**(tarvosModel.node[n_node].nextLabel))); //cria vetor com [número de links] posições
	if (tarvosModel.node[n_node].nextLabel==NULL) {
		printf("\nError - createNode - insufficient memory to allocate for label pool");
		exit(1);
	}
	(*(tarvosModel.node[n_node].nextLabel))[0]=1; //a posição zero do vetor serve para pacotes gerados no próprio nodo; use rótulo inicial 1
	for(i=1; i<links; i++) { /*a primeira posição do vetor, zero, é usada para indicar pacote sendo gerado no próprio nodo;
							   as interfaces começam de 1 (para seguir o padrão dos links, fontes, nodos, etc.*/
		(*(tarvosModel.node[n_node].nextLabel))[i]=i*100; /*próximo rótulo disponível para uso em construção de uma LSP para MPLS; ao usar este rótulo, este campo deve ser
														 incrementado a fim de assegurar rótulos únicos por interface.
														 Rótulo zero significa, no módulo TARVOS, que o pacote não deve ser encaminhado por rótulo (está saindo de um domínio MPLS).*/
	}
	tarvosModel.node[n_node].helloTimeLimit=(double(*)[])malloc(links * sizeof(**(tarvosModel.node[n_node].helloTimeLimit))); //cria vetor para limites de tempo para recebimento de HELLO-ACKS (um time stamp para cada link)
	if (tarvosModel.node[n_node].helloTimeLimit==NULL) {
		printf("\nError - createNode - insufficient memory to allocate for helloTimeLimit");
		exit(1);
	}
	for(i=0; i<links; i++) {
		(*(tarvosModel.node[n_node].helloTimeLimit))[i]=0; //inicializa todos os limites (inclusive o índice zero, não usado) para zero
	}
	tarvosModel.node[n_node].ctrlMsgHandlEv = tarvosParam.ctrlMsgHandlEv; //seta evento default (evento que conterá o tratamento de mensagens de controle)
	tarvosModel.node[n_node].helloMsgGenEv = tarvosParam.helloMsgGenEv; //seta evento default para tratar mensagens HELLO
	tarvosModel.node[n_node].ctrlMsgTimeout = tarvosParam.ctrlMsgTimeout; //define timeout default para mensagens de controle do RSVP-TE (tipicamente PATH)
	tarvosModel.node[n_node].LSPtimeout = tarvosParam.LSPtimeout; //define timeout default para as LSPs que partem deste nodo
	tarvosModel.node[n_node].helloMsgTimeout = tarvosParam.helloMsgTimeout; //define timeout default para as mensagens HELLO geradas a partir deste nodo
    tarvosModel.node[n_node].helloTimeout = tarvosParam.helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunicação com o nodo)
	createNodeMsgQueue(n_node);
}

/* CRIAÇÃO DA LISTA DE MENSAGENS DE CONTROLE DO NODO
*
*  Cria basicamente o Head Node.
*/
static struct nodeMsgQueue *createNodeMsgQueue(int n_node) {
	tarvosModel.node[n_node].nodeMsgQueue = (nodeMsgQueue*)malloc(sizeof *(tarvosModel.node[n_node].nodeMsgQueue)); //cria Head Node
	if (tarvosModel.node[n_node].nodeMsgQueue==NULL) {
		printf("\nError - createNodeMsgQueue - insufficient memory to allocate for Node Control Message Queue");
		exit(1);
	}
	tarvosModel.node[n_node].nodeMsgQueue->previous = tarvosModel.node[n_node].nodeMsgQueue;
	tarvosModel.node[n_node].nodeMsgQueue->next = tarvosModel.node[n_node].nodeMsgQueue; //perfaz a característica circular da lista
	return tarvosModel.node[n_node].nodeMsgQueue;
}

/* INSERE NOVO ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO
*  (nota:  a Lista de Mensagens é uma lista dinâmica duplamente encadeada circular com Head Node)
*  Passar todo o conteúdo de uma linha da lista como parâmetros
*/
void insertInNodeMsgQueue(int n_node, char *msgType, int msgID, int msgIDack, int LSPid, int er[], int erIndex, int source, int dst, double timeout,
						  int ev, int iIface, int oIface, int iLabel, int oLabel, int resourcesReserved) {
	struct nodeMsgQueue *p; //variável tipo apontador auxiliar
	char traceString[255];

	p=tarvosModel.node[n_node].nodeMsgQueue;
	p->previous->next = (nodeMsgQueue*)malloc(sizeof *(p->previous->next)); //cria mais um nó ao final da lista (lista duplamente encadeada)
	if (p->previous->next==NULL) {
		printf("\nError - insertInNodeMsgQueue - insufficient memory to allocate for Node Control Message Queue");
		exit(1);
	}
	p->previous->next->previous=p->previous; //atualiza ponteiro previous do novo nó da lista
	p->previous->next->next=p; //atualiza ponteiro next do novo nó da lista
	p->previous=p->previous->next; //atualiza ponteiro previous do Head Node
	p->previous->previous->next=p->previous; //atualiza ponteiro next do penúltimo nó
	strcpy(p->previous->msgType, msgType);
	p->previous->msgID=msgID;
	p->previous->msgIDack=msgIDack;
	p->previous->LSPid=LSPid;
	p->previous->er.explicitRoute=er;
	p->previous->er.erNextIndex=erIndex; //??
	p->previous->src=source;
	p->previous->dst=dst;
	p->previous->iIface=iIface;
	p->previous->oIface=oIface;
	p->previous->iLabel=iLabel;
	p->previous->oLabel=oLabel;
	p->previous->resourcesReserved=resourcesReserved;
	p->previous->timeout=timeout; //insere o tempo absoluto de timeout para esta mensagem
	p->previous->ev=ev; /*insere o evento de tratamento informado pelo usuário (pode ser, por exemplo, um evento de transmissão do pacote;
						um LER de destino recebe uma mensagem PATH e gera automaticamente uma mensagem RESV para o caminho inverso; deve então
						escalonar este evento para tratar a mensagem RESV recém-criada*/
	sprintf(traceString, "CtrlMsg inserted in node Queue: node:  %d  msgID:  %d  LSPid:  %d  src:  %d  dst:  %d  iLabel:  %d\n", n_node, msgID, LSPid, source, dst, iLabel);
	mainTrace(traceString);
}

/* BUSCA ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO POR LSP_ID
*  (nota:  a Lista de Mensagens é uma lista dinâmica duplamente encadeada circular com Head Node)
*  A chave de busca é o LSPid para um determinado nodo.  Retorna posição do item buscado, se encontrado;
*  caso contrário, retorna NULL.  Função que chama deve testar isso.
*/
struct nodeMsgQueue *searchInNodeMsgQueueLSPid(int n_node, int LSPid) {
	struct nodeMsgQueue *p;
	
	p=tarvosModel.node[n_node].nodeMsgQueue->next;
	tarvosModel.node[n_node].nodeMsgQueue->LSPid=LSPid;  //coloca item buscado no Head Node
	while (p->LSPid != LSPid) {
		p=p->next; //se item não existir, busca termina no HEAD NODE (lista circular)
	}
	if (p==tarvosModel.node[n_node].nodeMsgQueue) //se p for o Head Node (guardado na estrutura 'node'), item buscado não foi encontrado
		return NULL;
	else
		return p; //item foi encontrado; retorne sua posição
}

/* BUSCA ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO POR MSG_ID_ACK
*  (nota:  a Lista de Mensagens é uma lista dinâmica duplamente encadeada circular com Head Node)
*  A chave de busca é o msgIDack para um determinado nodo.  Retorna posição do item buscado, se encontrado;
*  caso contrário, retorna NULL.  Função que chama deve testar isso.
*/
struct nodeMsgQueue *searchInNodeMsgQueueAck(int n_node, int msgIDack) {
	struct nodeMsgQueue *p;
	
	p=tarvosModel.node[n_node].nodeMsgQueue->next;
	tarvosModel.node[n_node].nodeMsgQueue->msgID=msgIDack;  //coloca item buscado no Head Node
	while (p->msgID != msgIDack) {
		p=p->next; //se item não existir, busca termina no HEAD NODE (lista circular)
	}
	if (p==tarvosModel.node[n_node].nodeMsgQueue) //se p for o Head Node (guardado na estrutura 'node'), item buscado não foi encontrado
		return NULL;
	else
		return p; //item foi encontrado; retorne sua posição
}

/* REMOVE ITEM DA FILA DE MENSAGENS DE CONTROLE DO NODO USANDO LSPid
*  (nota:  a Fila de Mensagens é uma lista dinâmica duplamente encadeada circular com Head Node)
*  A chave de busca é o LSPid para um determinado nodo.
*/
void removeFromNodeMsgQueueLSPid(int n_node, int LSPid) {
	struct nodeMsgQueue *p; //variável tipo apontador auxiliar
	char traceString[255];

	p=searchInNodeMsgQueueLSPid(n_node, LSPid);
	if (p!=NULL) { //se for NULL, item não foi encontrado
		p->next->previous=p->previous;
		p->previous->next=p->next;
		//Debug
		sprintf(traceString, "Ctrl Msg REMOVED - msgID: %d msgIDack: %d LSPid: %d src: %d dst: %d iIface: %d oIface: %d  %s\n", p->msgID, p->msgIDack, p->LSPid, p->src,
			p->dst, p->iIface, p->oIface, p->msgType);
		mainTrace(traceString);
		free(p); //libera espaço ocupado por nó em p
	}
}

/* REMOVE ITEM DA FILA DE MENSAGENS DE CONTROLE DO NODO USANDO msgID
*  (nota:  a Fila de Mensagens é uma lista dinâmica duplamente encadeada circular com Head Node)
*  A chave de busca é o LSPid para um determinado nodo.
*/
void removeFromNodeMsgQueueAck(int n_node, int msgIDack) {
	struct nodeMsgQueue *p; //variável tipo apontador auxiliar
	char traceString[255];

	p=searchInNodeMsgQueueAck(n_node, msgIDack);
	if (p!=NULL) { //se for NULL, item não foi encontrado
		p->next->previous=p->previous;
		p->previous->next=p->next;
		//Debug
		sprintf(traceString, "Ctrl Msg REMOVED - msgID: %d msgIDack: %d LSPid: %d src: %d dst: %d iIface: %d oIface: %d  %s\n", p->msgID, p->msgIDack, p->LSPid, p->src,
			p->dst, p->iIface, p->oIface, p->msgType);
		mainTrace(traceString);
		free(p); //libera espaço ocupado por nó em p
	}
}

/* RECEPÇÃO DE UM PACOTE POR UM NODO
*
*  Esta função recebe um pacote pelo nodo e aciona os procedimentos necessários dependendo do tipo de pacote recebido.
*  Basicamente checa se o pacote é de dados, se é mensagem de controle, se é com destino final ao nodo em questão e se a rota percorrida pelo
*  pacote deve ser gravada no objeto RecordRoute.
*
*  Esta função atualiza as estatísticas convenientes na estrutura de dados do nodo
*  e também descarta o pacote, caso o nodo seja o destino final do pacote.
*
*  A função retorna 0 se o nodo atual não for o nodo destino do pacote; o programa do
*  usuário deve então fazer o escalonamento do pacote para a rotina de tratamento de
*  novo encaminhamento por um link.
*  A função retorna 1 se o nodo for o destino final do pacote.  O pacote já terá sido
*  descartado e as estatísticas atualizadas.  O programa do usuário pode tomar alguma
*  providência específica neste caso, se desejar.
*  A função retorna 2 se o pacote foi descartado (tipicamente por causa do estouro do TTL).
*
*  Atenção deve ser dada para chamar a função nodeUpdateStats quando as estatísticas do nodo necessitarem ser atualizadas (basicamente, sempre antes de
*  um return sem descarte).
*/
int nodeReceivePacket(struct Packet *pkt) {
	if (pkt->er.recordThisRoute==1) //se a flag estiver ativada, grave a rota no objeto RecordRoute
		recordRoute(pkt);

	/*Se o link atual do pacote for menor que 1, isto significa que o pacote NÃO está trafegando por um
	link físico, mas vindo diretamente de um gerador de tráfego.*/
	if (pkt->outgoingLink > 0)
		removePktFromTransitQueue(pkt->outgoingLink, pkt->id); //só faz a remoção se o link de fato existir
	
	if (pkt->ttl <= 0) { //TTL abaixo do limite; descartar o pacote
		//verificar se aqui as estatísticas do nodo devem ser reparadas (bytesReceived, PacketsReceived, etc.), pois o pacote está sendo descartado
		nodeDropPacket(pkt, "TTL limit reached");
		return 2;
	}
	//Testar se o pacote contém uma mensagem de controle; caso positivo, chame rotina de processamento de controle
	if (pkt->lblHdr.msgID!=0) { //se msgID=0, então pacote contém dados, e não mensagem de controle
		if (nodeReceiveCtrlMsg(pkt)==0) { //mensagem resultou em falha; descarte-a
			//verificar se aqui as estatísticas do nodo devem ser reparadas (bytesReceived, PacketsReceived, etc.), pois o pacote está sendo descartado
			nodeDropPacket(pkt, "RSVP-TE control message error");
			return 2;
		}
	}
	
	if (pkt->currentNode == pkt->dst) {
        /* Se for verdade significa que o destino do pacote é para o nodo atual.  Deve-se liberar o pacote da memória (freePkt(pkt)) */
		nodeUpdateStats(pkt); //atualiza estatísticas (menos encaminhamento)
		freePkt(pkt); //libera o espaço de memória ocupado pelo packet
		return 1;
	} else {
		/* Caso seja falso, significa que o pacote está em um roteador que não é o seu destino e por conseguinte deverá ser
		encaminhado para um link, através da rotina de encaminhamento apropriada.
		Isto deve ser feito no programa principal do usuário.  O TTL é decrementado aqui, mas não se o pacote foi "gerado" neste nodo.*/

		/*Se o link atual do pacote for menor que 1, isto significa que o pacote NÃO está trafegando por um
		link físico, mas vindo diretamente de um gerador de tráfego.  Então, neste caso, o TTL NÃO deve ser decrementado*/
		if (pkt->outgoingLink > 0)
			pkt->ttl--; //TTL acima do limite; decremente (só se o pacote não houver sido originado neste mesmo nodo)
		nodeUpdateStats(pkt); //atualiza estatísticas globais (menos encaminhamento)
		nodeUpdateForwardStats(pkt); //atualiza estatísticas de encaminhamento
		return 0;
	}
}

/* ATUALIZA ESTATÍSTICAS FORWARD DO NODO
*
*  As estatísticas atualizadas são:
*  packetsForwarded:  quantidade de pacotes encaminhados por este nodo
*  bytesForwarded:  quantidade de bytes encaminhados por este nodo
*  packetsForwardedAppl:  quantidade de pacotes encaminhados por este nodo somente para aplicação (ou seja, pacotes que não sejam de controle)
*  bytesForwardedAppl:  quantidade de bytes encaminhados por este nodo somente para aplicação (ou seja, pacotes que não sejam de controle)
*
*  O nodo corrente é obtido do campo currentNode do pacote.
*/
static void nodeUpdateForwardStats(struct Packet *pkt) {
	tarvosModel.node[pkt->currentNode].packetsForwarded++;
	tarvosModel.node[pkt->currentNode].bytesForwarded+=pkt->length;
	if (pkt->lblHdr.msgID==0) {//pacote é de aplicação; atualize as estatísticas específicas para aplicação (ou seja, pacote que não é de controle)
		tarvosModel.node[pkt->currentNode].packetsForwardedAppl++;
		tarvosModel.node[pkt->currentNode].bytesForwardedAppl+=pkt->length;
	}
}

/* ATUALIZA ESTATÍSTICAS DO NODO
*
*  As estatísticas atualizadas são:
*  bytesReceived:  quantidade de bytes recebidos pelo nodo
*  packetsReceived:  quantidade de pacotes recebidos pelo nodo
*  delay:  atraso registrado para o pacote
*  delaySum:  somatório de todos os delays registrados no nodo
*  meanDelay:  atraso médio (meanDelay = delaySum/packetsReceived)
*  jitter:  jitter deste pacote (jitter = atraso deste pacote - atraso do pacote anterior)
*  jitterSum:  somatório dos jitters
*  meanJitter:  jitter médio (meanJitter = jitterSum / (packetsReceived-1)). Notice that meanJitter = jitterSum / #Jitter Samples. And #Jitter Samples = packetsReceived - 1 (every two packets received yield one jitter sample).
*
*  O número packetsForwarded NÃO é atualizado aqui!  Cuidar disso na função nodeUpdateForwardStats.
*
*  O nodo corrente ou atual é obtido do campo currentNode do pacote.
*/
static void nodeUpdateStats(struct Packet *pkt) {
	double previousDelay; //atraso anterior do pacote (anterior) recebido por este nodo, para cálculo do jitter
	double stime;
	
	/*Questão:  os pacotes gerados no próprio nodo passam por esta função, antes de serem transmitidos.  Ainda assim as estatísticas devem
	ser atualizadas para estes pacotes?  (O delay de um pacote deste tipo será zero!)*/
	if (pkt->currentNode==pkt->src) //se verdadeiro, pacote foi gerado no próprio nodo; não computar estas estatísticas
		return;
	
	stime=simtime(); //registra tempo corrente uma única vez (para evitar repetidas chamadas à função simtime)
	//estatísticas globais
	previousDelay=tarvosModel.node[pkt->currentNode].delay;
	tarvosModel.node[pkt->currentNode].packetsReceived++; //atualiza estatísticas do nodo para packetsReceived
	tarvosModel.node[pkt->currentNode].bytesReceived+=pkt->length; //atualiza estatísticas do nodo para bytesReceived
	tarvosModel.node[pkt->currentNode].delay=stime-pkt->generationTime; //atualiza delay deste pacote
	tarvosModel.node[pkt->currentNode].delaySum+=tarvosModel.node[pkt->currentNode].delay; //atualiza somatório dos delays
	tarvosModel.node[pkt->currentNode].meanDelay=tarvosModel.node[pkt->currentNode].delaySum/tarvosModel.node[pkt->currentNode].packetsReceived; //atualiza delay médio
	if (tarvosModel.node[pkt->currentNode].packetsReceived>1) //só calcula jitter se houver pacote anterior recebido (não calcula para o primeiro pacote)
		tarvosModel.node[pkt->currentNode].jitter=tarvosModel.node[pkt->currentNode].delay-previousDelay; //calcula jitter para este pacote
	tarvosModel.node[pkt->currentNode].jitterSum+=tarvosModel.node[pkt->currentNode].jitter; //calcula somatório dos jitters
	if (tarvosModel.node[pkt->currentNode].packetsReceived>1) //só calcula o jitter médio se houver pacote já recebido
		tarvosModel.node[pkt->currentNode].meanJitter=tarvosModel.node[pkt->currentNode].jitterSum/(tarvosModel.node[pkt->currentNode].packetsReceived-1); //calcula jitter médio (a quantidade de jitters guardados é igual à de pacotes recebidos -1)
	//Desabilitar a chamada abaixo para distribuição
	jitterDelayTrace(pkt->currentNode, stime, tarvosModel.node[pkt->currentNode].jitter, tarvosModel.node[pkt->currentNode].delay);

	//estatísticas para pacotes exclusivamente de aplicação
	if (pkt->lblHdr.msgID==0) {
		previousDelay=tarvosModel.node[pkt->currentNode].delayAppl;
		tarvosModel.node[pkt->currentNode].packetsReceivedAppl++; //atualiza estatísticas do nodo para packetsReceived
		tarvosModel.node[pkt->currentNode].bytesReceivedAppl+=pkt->length; //atualiza estatísticas do nodo para bytesReceived
		tarvosModel.node[pkt->currentNode].delayAppl=stime-pkt->generationTime; //atualiza delay deste pacote
		tarvosModel.node[pkt->currentNode].delaySumAppl+=tarvosModel.node[pkt->currentNode].delayAppl; //atualiza somatório dos delays
		tarvosModel.node[pkt->currentNode].meanDelayAppl=tarvosModel.node[pkt->currentNode].delaySumAppl/tarvosModel.node[pkt->currentNode].packetsReceivedAppl; //atualiza delay médio
		if (tarvosModel.node[pkt->currentNode].packetsReceivedAppl>1) //só calcula jitter se houver pacote anterior recebido (não calcula para o primeiro pacote)
			tarvosModel.node[pkt->currentNode].jitterAppl=tarvosModel.node[pkt->currentNode].delayAppl-previousDelay; //calcula jitter para este pacote
		tarvosModel.node[pkt->currentNode].jitterSumAppl+=tarvosModel.node[pkt->currentNode].jitterAppl; //calcula somatório dos jitters
		if (tarvosModel.node[pkt->currentNode].packetsReceivedAppl>1) //só calcula o jitter médio se houver pacote já recebido
			tarvosModel.node[pkt->currentNode].meanJitterAppl=tarvosModel.node[pkt->currentNode].jitterSumAppl/(tarvosModel.node[pkt->currentNode].packetsReceivedAppl-1); //calcula jitter médio (a quantidade de jitters guardados é igual à de pacotes recebidos -1)
		//Desabilitar a chamada abaixo para distribuição
		jitterDelayApplTrace(pkt->currentNode, stime, tarvosModel.node[pkt->currentNode].jitterAppl, tarvosModel.node[pkt->currentNode].delayAppl);
	}
}	

/* REGISTRA A PERDA DE UM PACOTE POR UM NODO, NA TRANSMISSÃO OU PROPAGAÇÃO
*
*  Incrementa o contador de pacotes perdidos ou descartados no nodo.  Os pacotes sempre
*  devem estar partindo do nodo, no servidor de transmissão ou na propagação pelo link.
*  Os pacotes perdidos durante a propagação constarão da contagem de pacotes perdidos no
*  nodo de origem destes, e não no nodo de destino (que, para todos os efeitos, jamais soube
*  que os pacotes descartados existiram).
*  Esta função deve ser chamada idealmente por uma função que traz um link para o estado down,
*  chamando a função interna do kernel do simulador simm.  O nodo atual do pacote ou correspondente
*  à facility tem de ser obtido antes do descarte, caso contrário este dado será perdido junto com
*  a estrutura.
*  Importante notar que o pacote em si não é descartado, pois supostamente este já o foi em
*  alguma outra função (como no kernel do simulador simm, que descarta tokens ao ter uma facility
*  passada para status down).
*  11.Jan.2006 Marcos Portnoi
*/
void nodeIncDroppedPacketsNumber(int nodeNumber) {
	tarvosModel.node[nodeNumber].packetsDropped++;
}

/* DESCARTA UM PACOTE NO NODO
*
*  Se uma facility de transmissão de um link recusar um pacote por estar não-operacional (down),
*  então o pacote deve ser descartado.  Esta função cuida disto, eliminando a estrutura e
*  incrementando o contador de pacotes descartados do nodo.
*  11.Jan.2006 Marcos Portnoi
*/
void nodeDropPacket(struct Packet *pkt, char *dropReason) {
	char dropTraceEntry[255];

	nodeIncDroppedPacketsNumber(pkt->currentNode); //incrementa contador de packets dropped
	sprintf(dropTraceEntry, "simtime: %f  node: %d  ID: %d  msgID: %d label: %d src: %d  dst: %d outgoingLink: %d  reason: %s\n", simtime(), pkt->currentNode, pkt->id, pkt->lblHdr.msgID, pkt->lblHdr.label, pkt->src, pkt->dst, pkt->outgoingLink, dropReason);
	dropPktTrace(dropTraceEntry);
	freePkt(pkt); //descarta o pacote da memória
}

/* RECEBE E PROCESSA UM PACOTE COM MENSAGEM DE CONTROLE DE PROTOCOLO
*
*  Esta função é tipicamente chamada pela função nodeReceivePacket.  Aqui deve-se checar que tipo de mensagem contém o pacote e processá-la
*  de acordo.
*  A função retorna 0 se houve alguma falha no processamento da mensagem, e retorna 1 se houve o processamento completo bem sucedido.
*/
static int nodeReceiveCtrlMsg(struct Packet *pkt) {
	char mainTraceString[255];
	
	//trace para DEBUG
	sprintf(mainTraceString, "Ctrl Msg Received - node:  %d  msgID:  %d  msgIDack:  %d  LSPid:  %d  src:  %d  dst:  %d label:  %d  %s\n", pkt->currentNode,
		pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->src, pkt->dst, pkt->lblHdr.label, pkt->lblHdr.msgType);
	mainTrace(mainTraceString);
	
	//PATH_LABEL_REQUEST
	if (strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST")==0) {
		if (nodeProcessPathLabelRequest(pkt)==1) //verifique se o processamento da PATH foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a função chamante
	}

	//RESV_LABEL_MAPPING
	if (strcmp(pkt->lblHdr.msgType, "RESV_LABEL_MAPPING")==0) {
		if (nodeProcessResvLabelMapping(pkt)==1)
			return 1; //retorne sucesso para processamento RESV_LABEL_MAPPING
		else
			return 0; //retorne FALHA
	}

	//PATH_REFRESH
	if (strcmp(pkt->lblHdr.msgType, "PATH_REFRESH")==0) {
		if (nodeProcessPathRefresh(pkt)==1) //verifique se o processamento da PATH_REFRESH foi bem sucedido
			return 1; //PATH_REFRESH foi bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da PATH_REFRESH; indique isto para função chamante
	}

	//RESV_REFRESH
	if (strcmp(pkt->lblHdr.msgType, "RESV_REFRESH")==0) {
		if (nodeProcessResvRefresh(pkt)==1) //verifique se o processamento foi bem sucedido
			return 1; //retorne SUCESSO
		else
			return 0; //retorne com FALHA
	}

	//HELLO
	if (strcmp(pkt->lblHdr.msgType, "HELLO")==0) {
		if (nodeProcessHello(pkt)==1)
			return 1; //retorne SUCESSO como processamento da mensagem HELLO
		else
			return 0; //retorne com FALHA
	}

	//HELLO_ACK
	if (strcmp(pkt->lblHdr.msgType, "HELLO_ACK")==0) {
		if (nodeProcessHelloAck(pkt)==1)
			return 1; //retorne SUCESSO como processamento da mensagem HELLO_ACK
		else
			return 0; //retorne FALHA
	}
	
	//PATH_DETOUR
	if (strcmp(pkt->lblHdr.msgType, "PATH_DETOUR")==0) {
		if (nodeProcessPathDetour(pkt)==1) //verifique se o processamento da PATH foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a função chamante
	}

	//RESV_DETOUR_MAPPING
	if (strcmp(pkt->lblHdr.msgType, "RESV_DETOUR_MAPPING")==0) {
		if (nodeProcessResvDetourMapping(pkt)==1)
			return 1; //retorne sucesso para processamento RESV_DETOUR_MAPPING
		else
			return 0; //retorne FALHA
	}
	
	//PATH_ERR
	if (strcmp(pkt->lblHdr.msgType, "PATH_ERR")==0) {
		if (nodeProcessPathErr(pkt)==1) //verifique se o processamento da PATH foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a função chamante
	}

	//RESV_ERR
	if (strcmp(pkt->lblHdr.msgType, "RESV_ERR")==0) {
		if (nodeProcessResvErr(pkt)==1) //verifique se o processamento da RESV foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da RESV; indique isto para a função chamante
	}
	
	//PATH_LABEL_REQUEST_PREEMPT
	if (strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST_PREEMPT")==0) {
		if (nodeProcessPathPreempt(pkt)==1) //verifique se o processamento da PATH foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a função chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a função chamante
	}

	//RESV_LABEL_MAPPING_PREEMPT
	if (strcmp(pkt->lblHdr.msgType, "RESV_LABEL_MAPPING_PREEMPT")==0) {
		if (nodeProcessResvPreempt(pkt)==1)
			return 1; //retorne sucesso para processamento RESV_LABEL_MAPPING_PREEMPT
		else
			return 0; //retorne FALHA
	}

	return 0; //se chegar até aqui, houve algum tipo de falha no processamento da mensagem de controle
}

/* CRIA E FORNECE UM RÓTULO A PARTIR DO POOL DE RÓTULOS DISPONÍVEIS PARA O NODO
*
*  Fornece um número de rótulo disponível para a interface do nodo em questão.  Este rótulo tipicamente será usado em mapeamento pelo RSVP-TE numa mensagem RESV.
*  Os rótulos são únicos por interface no nodo (sendo que as interfaces recebem o número dos links a que estão ligadas).
*  Interface zero (ou índice zero) indica que o pacote está sendo gerado neste próprio nodo.
*/
static int nodeCreateLabel(int n_node, int iFace) {
	int label;
	label=(*(tarvosModel.node[n_node].nextLabel))[iFace];
	(*(tarvosModel.node[n_node].nextLabel))[iFace]++; //incrementa novo rótulo disponível; os números de rótulo são únicos por interface
	return label;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Define o valor de timeout para controlar a morte de mensagens de controle do protocolo RSVP-TE para o nodo.
*  Uma rotina específica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
void setNodeCtrlMsgTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].ctrlMsgTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Devolve o valor de timeout para controlar a morte de mensagens de controle do protocolo RSVP-TE para o nodo.
*  Uma rotina específica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
double getNodeCtrlMsgTimeout(int n_node) {
	return tarvosModel.node[n_node].ctrlMsgTimeout;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Define o valor de timeout para controlar a morte de mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Uma rotina específica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
void setNodeHelloMsgTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].helloMsgTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Devolve o valor de timeout para controlar a morte de mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Uma rotina específica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
double getNodeHelloMsgTimeout(int n_node) {
	return tarvosModel.node[n_node].helloMsgTimeout;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA RECEBIMENTO DE MENSAGEM DE CONTROLE HELLO PARA O NODO
*
*  Define o valor de timeout para controlar o limite de tempo pelo qual um nodo deve esperar a chegada de uma mensagem HELLO ou HELLO_ACK para
*  considerar um nodo alcançável (reachable).
*/
void setNodeHelloTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].helloTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA RECEBIMENTO DE MENSAGEM DE CONTROLE HELLO PARA O NODO
*
*  Devolve o valor de timeout para controlar o limite de tempo pelo qual um nodo deve esperar a chegada de uma mensagem HELLO ou HELLO_ACK para
*  considerar um nodo alcançável (reachable).
*/
double getNodeHelloTimeout(int n_node) {
	return tarvosModel.node[n_node].helloTimeout;
}

/* SETA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Define o valor do evento para tratar mensagens de controle do protocolo RSVP-TE para o nodo.
*  Tipicamente é um evento de envio para as mensagens de controle criadas pelo nodo.
*/
void setNodeCtrlMsgHandlEv(int n_node, int ev) {
	tarvosModel.node[n_node].ctrlMsgHandlEv=ev;
}

/* RECUPERA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Devolve o valor do evento para tratar mensagens de controle do protocolo RSVP-TE para o nodo.
*  Tipicamente é um evento de envio para as mensagens de controle criadas pelo nodo.
*/
int getNodeCtrlMsgHandlEv(int n_node) {
	return tarvosModel.node[n_node].ctrlMsgHandlEv;
}

/* SETA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Define o valor do evento para tratar mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Tipicamente é um evento de envio para as mensagens de controle criadas pelo nodo.
*/
void setNodeHelloMsgGenEv(int n_node, int ev) {
	tarvosModel.node[n_node].helloMsgGenEv=ev;
}

/* RECUPERA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE HELLO RSVP-TE PARA O NODO
*
*  Devolve o valor do evento para tratar mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Tipicamente é um evento de envio para as mensagens de controle criadas pelo nodo.
*/
int getNodeHelloMsgGenEv(int n_node) {
	return tarvosModel.node[n_node].helloMsgGenEv;
}

/* SETA VALOR DO TIMEOUT (RELATIVO) PARA LSPs QUE PARTEM DO NODO
*
*  Define o valor do timeout relativo, em segundos, para a extinção de LSPs que partem deste nodo.  Este valor deve ser recuperado por uma
*  função específica de controle de timeout a fim de calcular o tempo de relógio absoluto e introduzi-lo na LIB.
*/
void setNodeLSPTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].LSPtimeout=timeout;
}

/* RECUPERA VALOR DO TIMEOUT (RELATIVO) PARA LSPs QUE PARTEM DO NODO
*
*  Recupera o valor do timeout relativo, em segundos, para a extinção de LSPs que partem deste nodo.  Este valor deve ser recuperado por uma
*  função específica de controle de timeout a fim de calcular o tempo de relógio absoluto e introduzi-lo na LIB.
*/
double getNodeLSPTimeout(int n_node) {
	return tarvosModel.node[n_node].LSPtimeout;
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_LABEL_REQUEST
*
*  A mensagem PATH_LABEL_REQUEST pede mapeamento de rótulo e faz pré-reserva de recursos para um LSP Tunnel.
*  Os casos a considerar são:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), então este é o último destino da mensagem PATH.  Uma mensagem RESV_LABEL_MAPPING deve ser agora
*  criada, mas o mapeamento de rótulo é feito pela função de processamento da mensagem RESV_LABEL_MAPPING.  Assim, os dados da mensagem PATH_LABEL_REQUEST
*  são introduzidos na fila de mensagens de controle do nodo.  Criar rota explícita inversa e colocar na mensagem RESV.  O oLabel para a RESV será ZERO e
*  o oIface introduzido na fila de mensagens também será ZERO.  Nenhuma reserva de recursos precisa ser feita.
*
*  A mensagem PATH_LABEL_REQUEST sempre é introduzida na fila de mensagens de controle do nodo, desde o LER de ingresso até o LER de egresso.
*
*  No segundo caso (recebido por um LSR genérico), as informações pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve então seguir adiante.  Observar que, nesta implementação do TARVOS, uma mesma
*  mensagem PATH_LABEL_REQUEST percorre todo o caminho indicado pela rota explícita, desde o LER de ingresso até o LER de egresso.  Na vida real, provavelmente
*  cada nodo criaria sua própria mensagem de controle, com um msgID próprio.  A implementação presente simplifica o processo, aproveitando as funções já
*  implementadas de tratamento de um pacote de dados comum.  Neste caso, as reservas de recursos devem ser feitas, portanto a função específica deve ser chamada.
*  Se os recursos foram reservados, a função deve retornar sucesso; caso contrário, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.
*
*  A função retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (ou não havia recursos para reservar ou rota não existe;
*  mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathLabelRequest(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	
	/*Caso 1:  recebido no LER de egresso; insira na fila do nodo e crie mensagem RESV_LABEL_MAPPING para a rota inversa.
			   Não é preciso fazer reserva de recursos aqui, pois em tese a mensagem só chegou aqui porque todos os links anteriores
			   aceitaram a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o próximo item do vetor ER (que começa de 0); portanto indica de fato o tamanho
																		 da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		
		/* O mapeamento de rótulo e inserção na LIB foram retirados daqui; somente a função nodeProcessResvLabelMapping é que tratará disso
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser criada agora
		iLabel=nodeCreateLabel(pkt->currentNode, pkt->outgoingLink);
		insertInLIB(pkt->currentNode, pkt->outgoingLink, iLabel, 0, 0, pkt->lblHdr.LSPid, "up", timeout, 0); //coloca ZERO no timeoutStamp
		*/

		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface é ZERO, pois trata-se do LER de egresso
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
		createResvMapControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, 0); //cria a mensagem RESV e também escalona evento de tratamento (contido no pacote PATH); o rótulo é ZERO, pois é o LER de egresso
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH está sendo recebido num nodo LSR genérico; recolha as informações pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas só faça isso se houver recursos disponíveis no link de saída para a LSP.*/
	} else {
		/*se currentNode == próximo nodo da lista ER, então entenda que o pacote está no nodo de origem
		* e que o usuário começou sua lista explícita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a próxima.
		* (Não faz nenhum sentido a origem e destino serem iguais.  Este teste permitirá ao usuário construir sua lista
		* de roteamento explícito por nodos, começando pelo nodo onde o pacote é gerado, ou pelo nodo imediatamente após,
		* e ambas opções serão aceitas.)
		* Se isto for resultado de alguma inconsistência na lista ER, o funcionamento daqui para a frente é imprevisível.
		* (Rotina copiada da função decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avança o número nextIndex para a próxima posição (próximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o próximo nodo a percorrer, indicado por erNextIndex
		if (link!=0) {//se link==0, rota não foi encontrada; não insira nada na fila; o pacote deverá ser descartado em outra função
			if (reserveResouces(pkt->lblHdr.LSPid, link)==1) { //recursos foram reservados; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, 1);
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a geração de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		}
		return 0; //FALHA:  houve problema na reserva de recurso ou rota (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_LABEL_MAPPING (working LSP)
*
*  A mensagem RESV_LABEL_MAPPING distribui o mapeamento de rótulo e efetiva a reserva de recursos para o LSP Tunnel.
*  Os mapeamentos e reserva são feitos hop-by-hop, mas o LSP Tunnel só estará efetivado e válido quando a mensagem RESV_LABEL_MAPPING atingir o LER
*  de ingresso.  Caso isto não aconteça, os recursos e rótulos nos hops já efetivados entrarão em timeout, pois não haverá REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive LER de egresso), e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do rótulo,\
*  buscando no pool de rótulos disponíveis na interface de entrada um valor válido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adição de apagar da memória (com free) o
*  espaço ocupado pelo vetor de rota explícita invertida, criado juntamente com a mensagem RESV, e marcar a entrada para o LSP tunnel, na LSP Table, como
*  completado.  (A fim de se fazer uma backup LSP, o working LSP precisa estar completado.)
*
*  Somente esta função tem a prerrogativa de fazer mapeamento de rótulo e inserir os dados pertinentes na LIB para a working LSP.
*/
static int nodeProcessResvLabelMapping(struct Packet *pkt) {
	int iLabel;
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR genérico; fazer o mapeamento de rótulo, inserir dados na LIB e LSP Table; rótulo inicial deve ser recuperado
	através de função específica.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem PATH correspondente; nada faça, neste caso.
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface);
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 0, timeout, 0); //coloca zero no timeoutStamp, zero para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (working) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o rótulo agora criado na mensagem, para que o próximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguirá adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//LSP foi criada; usuário deve usar a função getWorkingLSPLabel para recuperar o rótulo inicial
	//Caso 2:  recebida pelo LER de ingresso:  faça tudo acima, também remova a rota explícita inversa da memória e marque o LSP tunnel como completo
		if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (último destino), então libere o espaço da variável rota explícita inversa
			setLSPtunnelDone(pkt->lblHdr.LSPid);  //marque o LSP tunnel como totalmente completado
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
		}
		return 1; //retorne com SUCESSO
	} else
		return 0; //retorne com FALHA; nenhuma mensagem PATH correspondente encontrada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_REFRESH SEM MERGING (RFC 4090)
*
*  Para fins de simulação, a mensagem PATH_REFRESH precisa conter a LSPid, msgID e ter o objeto recordRoute ativado, para que a rota seguida
*  seja gravada no pacote da mensagem de controle.  Nesta implementação, a mensagem PATH_REFRESH segue comutada por rótulo, colocado na mensagem em sua
*  criação.
*  Os casos a considerar para processamento são:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), então este é o último destino da mensagem PATH.  Uma mensagem RESV_REFRESH deve ser agora
*  criada, aqui mesmo na função.  A rota a ser percorrida pela mensagem RESV_REFRESH deve ser recolhida do objeto recordRoute, apropriadamente invertida
*  (para que seja seguida no caminho inverso).  A mensagem é incluída na fila de mensagens de controle do nodo, para uso da função de processamento da
*  RESV_REFRESH.
*
*  No segundo caso (recebido por um LSR genérico), as informações pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve então seguir adiante.  Observar que, nesta implementação do TARVOS, uma mesma
*  mensagem PATH_REFRESH percorre todo o caminho indicado comutado por rótulo, desde o LER de ingresso até o LER de egresso.
*  Os recursos só devem ser efetivamente renovados pela mensagem RESV_REFRESH.
*  No momento, não foram implementadas as mensagems PATH_ERR ou RESV_ERR.
*  A função retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (rota não encontrada para o próximo LSR);
*  mensagem deve ser descartada ou gerar erro).
*
*  Esta função não trata merging de mensagens PATH_REFRESH.  Ou seja, ao chegar num Merge Poing, a mensagem PATH_REFRESH segue para seu destino, mesmo que
*  outra mensagem PATH_REFRESH, para a mesma LSPid (LSP tunnel), já tenha prosseguido para o mesmo destino e path, dentro do tempo mínimo.
*  Para tratar merging, usar a função nodeProcessPathRefresh (RFC 4090, seção 7.1.2.1).
*/
static int nodeProcessPathRefreshNoMerge(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; criar mensagem RESV_REFRESH para a rota inversa.
			   Se a mensagem chegou até aqui, então o PATH seguido por ela está válido; emita RESV_REFRESH para renovar a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o próximo item do vetor recordRoute (que começa de 0); portanto indica de fato o tamanho
																			 da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface é ZERO, pois trata-se do LER de egresso
		
		createResvRefreshControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid); //cria a mensagem RESV e também escalona evento de tratamento (parâmetros da simulação)
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH está sendo recebido num nodo LSR genérico; recolha as informações pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Os recursos reservados só serão renovados pela mensagem RESV_REFRESH.*/
	} else {
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada é o conteúdo de pkt->outgoingLink
		//se a tupla não for encontrada, significa rota inexistente; retornar com erro e o pacote será descartado na função de encaminhamento
		
		if (p!=NULL) {//se p==NULL, rota não foi encontrada; não insira nada na fila; o pacote deverá ser descartado em outra função
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, p->oIface, p->iLabel, p->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (link) (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
	}
	//se chegou até aqui, houve algum problema no processamento da PATH_REFRESH
	return 0; //FALHA:  houve problema na rota (label não encontrada); a mensagem PATH_REFRESH deverá ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_REFRESH SEM MERGING (RFC 4090)
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR, e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada
*  procurar a entrada correspondente à chave nodo-iIface-iLabel-status "up" (estes dados são recuperados da mensagem armazenada na fila de mensagens de controle)
*  na tabela LIB e recalcular o timeout.  Se a entrada não for encontrada, nada faça.
*  Na realidade, na atual implementação, a mensagem guardada na fila de mensagens de controle não é usada para nada (a não ser para fazer a correspondência para a RESV_REFRESH);
*  todos os dados necessários são recuperados do pacote RESV_REFRESH e de outras tabelas de controle.
*
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adição de apagar da memória (com free) o
*  espaço ocupado pelo vetor de rota explícita invertida, criado juntamente com a mensagem RESV.
*
*  Esta função não trata merging de mensagens RESV_REFRESH.  Ou seja, ao chegar num Merge Poing, a mensagem RESV_REFRESH nunca cria outras mensagens RESV_REFRESH para backup LSPs
*  da mesma working LSP.  Para tratar merging, usar a função nodeProcessResvRefresh (RFC 4090, seção 7.1.2.1).
*/
static int nodeProcessResvRefreshNoMerge(struct Packet *pkt) {
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	struct LIBEntry *p;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR genérico; renove os recursos reservados para o pedaço de LSP que compete a este nodo (como origem) e encaminhe
	a mensagem à frente.  A renovação é basicamente o recálculo do timeout na tabela LIB.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem PATH correspondente; nada faça, neste caso.
		//p = searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up");
		//busca entrada na LIB, usando como chave primária a mensagem PATH correspondente anteriormente armazenada; tanto working LSP como backup LSPs funcionam aqui
		p = searchInLIBStatus(pkt->currentNode, msg->iIface, msg->iLabel, "up");
		if (p!=NULL) { //se p==NULL, entrada na LIB não foi encontrada; nada a renovar
			timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser renovada agora
			p->timeout = timeout; //coloque (atualize) o novo timeout da LSP no campo apropriado
			sprintf(mainTraceString, "Resources for LSP successfully refreshed.  LSPid:  %d  at node %d\n", msg->LSPid, pkt->currentNode);
			mainTrace(mainTraceString);
			removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
			//recursos foram renovados neste nodo

	/*Caso 2:  recebida pelo LER de ingresso:  faça tudo acima e também remova a rota explícita inversa da memória.*/
			if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (último destino), então libere o espaço da variável rota explícita inversa
				free(pkt->er.explicitRoute);
				pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
			}
			return 1;  //retorne com SUCESSO
		}
	}
	return 0; //se chegou até aqui, retorne com FALHA; PATH correspondente ou entrada na LIB não encontrados
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_REFRESH
*
*  Para fins de simulação, a mensagem PATH_REFRESH precisa conter a LSPid, msgID e ter o objeto recordRoute ativado, para que a rota seguida
*  seja gravada no pacote da mensagem de controle.  Nesta implementação, a mensagem PATH_REFRESH segue comutada por rótulo, colocado na mensagem em sua
*  criação.
*  Os casos a considerar para processamento são:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), então este é o último destino da mensagem PATH.  Uma mensagem RESV_REFRESH deve ser agora
*  criada, aqui mesmo na função.  A rota a ser percorrida pela mensagem RESV_REFRESH deve ser recolhida do objeto recordRoute, apropriadamente invertida
*  (para que seja seguida no caminho inverso).  A mensagem é incluída na fila de mensagens de controle do nodo, para uso da função de processamento da
*  RESV_REFRESH.
*
*  No segundo caso (recebido por um LSR genérico), as informações pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve então seguir adiante.  Observar que, nesta implementação do TARVOS, uma mesma
*  mensagem PATH_REFRESH percorre todo o caminho indicado comutado por rótulo, desde o LER de ingresso até o LER de egresso.
*  Os recursos só devem ser efetivamente renovados pela mensagem RESV_REFRESH.
*  A função retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (rota não encontrada para o próximo LSR);
*  mensagem deve ser descartada ou gerar erro).
*
*  No momento, não foram implementadas as mensagems PATH_ERR ou RESV_ERR, tampouco o MERGING (RFC 4090, seção 7.1.2.1).
*/
static int nodeProcessPathRefresh(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; criar mensagem RESV_REFRESH para a rota inversa.
			   Se a mensagem chegou até aqui, então o PATH seguido por ela está válido; emita RESV_REFRESH para renovar a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o próximo item do vetor recordRoute (que começa de 0); portanto indica de fato o tamanho
																			 da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface é ZERO, pois trata-se do LER de egresso
		
		createResvRefreshControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid); //cria a mensagem RESV e também escalona evento de tratamento (parâmetros da simulação)
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH está sendo recebido num nodo LSR genérico; recolha as informações pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Os recursos reservados só serão renovados pela mensagem RESV_REFRESH.*/
	} else {
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada é o conteúdo de pkt->outgoingLink
		//se a tupla não for encontrada, significa rota inexistente; retornar com erro e o pacote será descartado na função de encaminhamento
		
		if (p!=NULL) {//se p==NULL, rota não foi encontrada; não insira nada na fila; o pacote deverá ser descartado em outra função
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, p->oIface, p->iLabel, p->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (link) (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
	}
	//se chegou até aqui, houve algum problema no processamento da PATH_REFRESH
	return 0; //FALHA:  houve problema na rota (label não encontrada); a mensagem PATH_REFRESH deverá ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_REFRESH
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR, e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada
*  procurar a entrada correspondente à chave nodo-iIface-iLabel-status "up" (estes dados são recuperados da mensagem armazenada na fila de mensagens de controle)
*  na tabela LIB e recalcular o timeout.  Se a entrada não for encontrada, nada faça.
*  todos os dados necessários são recuperados do pacote RESV_REFRESH e de outras tabelas de controle.
*
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adição de apagar da memória (com free) o
*  espaço ocupado pelo vetor de rota explícita invertida, criado juntamente com a mensagem RESV.
*
*  Nesta implementação, ainda não foi implementado o MERGING (RFC 4090, seção 7.1.2.1).
*/
static int nodeProcessResvRefresh(struct Packet *pkt) {
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	struct LIBEntry *p;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR genérico; renove os recursos reservados para o pedaço de LSP que compete a este nodo (como origem) e encaminhe
	a mensagem à frente.  A renovação é basicamente o recálculo do timeout na tabela LIB.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem PATH correspondente; nada faça, neste caso.
		//p = searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up");
		//busca entrada na LIB, usando como chave primária a mensagem PATH correspondente anteriormente armazenada; tanto working LSP como backup LSPs funcionam aqui
		p = searchInLIBStatus(pkt->currentNode, msg->iIface, msg->iLabel, "up");
		if (p!=NULL) { //se p==NULL, entrada na LIB não foi encontrada; nada a renovar
			timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser renovada agora
			p->timeout = timeout; //coloque (atualize) o novo timeout da LSP no campo apropriado
			sprintf(mainTraceString, "Resources for LSP successfully refreshed.  LSPid:  %d  at node %d\n", msg->LSPid, pkt->currentNode);
			mainTrace(mainTraceString);
			removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
			//recursos foram renovados neste nodo

	/*Caso 2:  recebida pelo LER de ingresso:  faça tudo acima e também remova a rota explícita inversa da memória.*/
			if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (último destino), então libere o espaço da variável rota explícita inversa
				free(pkt->er.explicitRoute);
				pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
			}
			return 1;  //retorne com SUCESSO
		}
	}
	return 0; //se chegou até aqui, retorne com FALHA; PATH correspondente ou entrada na LIB não encontrados
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO HELLO
*
*  As mensagens de controle do RSVP-TE do tipo HELLO são processadas aqui.
*  Tecnicamente, as mensagens HELLO são enviadas somente para os nodos vizinhos, portanto a uma distância máxima de 1 hop.  Assim, ela seria recebida
*  somente pelo destino e não passaria por nenhum LSR intermediário.  Devido à filosofia de implementação do simulador, e também prevendo uma mensagem
*  HELLO que trafegaria por mais de um LSR, programou-se aqui dois casos de processamento.
*  No CASO 1, a mensagem é recebida por um LSR intermediário, não destino.  A mensagem é gravada na fila de mensagens de controle do nodo e encaminhada
*  adiante (comutada por rótulo).
*  No CASO 2, a mensagem é recebida pelo LSR destino.  O processo para o caso 1 é repetido e dispara-se imediatamente a mensagem
*  HELLO_ACK com destino à origem daquela mensagem HELLO recebida, usando o objeto RecordRoute do pacote HELLO original.
*/
static int nodeProcessHello(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para cálculo do tempo absoluto de timeout de uma mensagem HELLO
	
	/*Caso 1:  recebido no LSR de destino; LSP é alcançável:  criar mensagem HELLO_ACK para a rota inversa.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o próximo item do vetor recordRoute (que começa de 0); portanto indica de fato o tamanho
																			 da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		timeout=simtime() + getNodeHelloMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem HELLO
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface é ZERO, pois é irrelevante aqui
		createHelloAckControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID); //cria a mensagem RESV e também escalona evento de tratamento (parâmetros da simulação)
		free(pkt->er.explicitRoute); //libera rota explícita criada temporariamente
		pkt->er.explicitRoute=NULL; //evita erros em novas operações com free neste campo (em outras funções)
		return 1; //SUCESSO:  mensagem foi processada completamente

	/*Caso 2:  HELLO recebido num LSR intermediário; recolha as informações pertinentes e insira na fila do nodo, e deixe a mensagem ser encaminhada adiante.*/
	} else {	
		timeout=simtime() + getNodeHelloMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem HELLO
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //oIface é irrelevante
		//pkt->outgoingLink contém o link ou interface pela qual o pacote está chegando; se for ZERO, significa que o pacote foi gerado no nodo
		return 1; //SUCESSO:  mensagem foi processada completamente
	}
	return 0; //FALHA:  houve algum problema; a mensagem HELLO deverá ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO HELLO_ACK
**  
*  Ao receber uma HELLO_ACK, o nodo verifica se existe uma HELLO com mesmo msgIDack na fila de mensagens de controle do nodo.  Se houver, então o valor
*  do helloTimeLimit no tarvosModel.node é atualizado para mais um intervalo de detecção de falha (3,5 * helloInterval, por default).  Caso uma mensagem
*  HELLO com mesmo msgIDack não seja encontrada, então a HELLO_ACK é descartada e o helloTimeLimit não é atualizado.
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida pelo LSR destino, e recebida por um LSR intermediário.
*  No caso 1 (recebida pelo LSR destino), procurar a mensagem HELLO correspondente na fila de mensagens de controle do nodo, retirá-la se achada, e renovar o helloTimeLimit no
*  nodo origem.  Se a mensagem HELLO não for achada na fila, descartar a HELLO_ACK e nada mais fazer (verificar esse comportamento na RFC 3209).
*
*  No caso 2 (recebida por um LSR intermediário), deve-se buscar a mensagem HELLO correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada,
*  retirá-la da fila.  Se a entrada não for encontrada, nada faça.  Em ambas situações, encaminhar a mensagem adiante para seu destino.
*  Na realidade, na atual implementação, a mensagem guardada na fila de mensagens de controle não é usada para nada, a não ser para fazer o casamento com sua ACK;
*  todos os dados necessários são recuperados do pacote HELLO_ACK e de outras tabelas de controle.
*
*  O espaço ocupado pelo objeto rota explícita deve ser apagado com free, pois este foi criado juntamente com a mensagem HELLO_ACK.
*/
static int nodeProcessHelloAck(struct Packet *pkt) {
	double helloTimeLimit; //para cálculo do tempo absoluto de helloTimeLimit (para considerar nodo alcançável)
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	//procure a mensagem HELLO correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem HELLO correspondente; nada faça, neste caso.
		
		/*Caso 1:  recebida pelo LSR de destino:  recalcule o helloTimeLimit e também remova a rota explícita inversa da memória.*/
		if (pkt->currentNode == pkt->dst) {
			helloTimeLimit=simtime()+getNodeHelloTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração de uma conexão válida entre dois nodos
			(*(tarvosModel.node[pkt->currentNode].helloTimeLimit))[findLink(msg->src, msg->dst)] = helloTimeLimit; //coloque (atualize) o novo timeout da conexão entre os nodos src e dst
			sprintf(mainTraceString, "Node found reachable by HELLO.  src:  %d  dst:  %d  at node %d.  Next timeout:  %f\n", msg->src, msg->dst, pkt->currentNode, helloTimeLimit);
			mainTrace(mainTraceString);
			free(pkt->er.explicitRoute);  //elimine rota explícita, criada temporariamente juntamente com a HELLO_ACK
			pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
		}
		/*Caso 2:  recebido no LSR intermediário; retire a HELLO na fila de mensagens e encaminhe o HELLO_ACK adiante.*/
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		return 1;  //retorne com SUCESSO
	}
	return 0; //retorne com FALHA, pois mensagem correspondente HELLO não foi encontrada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_DETOUR
*
*  A mensagem PATH_DETOUR implanta uma backup LSP a partir de um Merge Point de ingresso até um Merge Point de egresso.
*  Os Merge Points (MPs) encontram a working LSP, que já deve existir previamente.
*  Os casos a considerar são:  PATH recebido pelo MP de egresso e recebido por um LSR qualquer.
*
*  No primeiro caso (recebido por um MP de egresso), então este é o último destino da mensagem PATH.  A partir daqui, o tunnel deverá unir-se ao working LSP tunnel.
*  Para isto, a função buscará o oLabel e oIface existentes para o working LSP e copiará estes valores para o backup LSP.
*  Uma mensagem RESV_DETOUR_MAPPING deve ser agora criada, mas o mapeamento de rótulo é feito pela função de processamento da mensagem RESV_DETOUR_MAPPING.
*  Assim, os dados da mensagem PATH_DETOUR são introduzidos na fila de mensagens de controle do nodo.
*  Criar rota explícita inversa e colocar na mensagem RESV.  O oLabel e oIface para a RESV serão copiados da entrada já existente para working LSP e
*  introduzidos na fila de mensagens.  Nenhuma reserva de recursos precisa ser feita, pois a reserva será compartilhada com a working LSP.
*  Se uma working LSP não for encontrada, de forma a causar o merging com o MP de egresso, então a função deve retornar com erro (e assim a PATH deverá
*  ser descartada, ou então uma PATH_ERR gerada).
*
*  A mensagem PATH_DETOUR sempre é introduzida na fila de mensagens de controle do nodo, desde o MP de ingresso até o MP de egresso.
*
*  No segundo caso (recebido por um LSR qualquer), as informações pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve então seguir adiante.  Observar que, nesta implementação do TARVOS, uma mesma
*  mensagem PATH_DETOUR percorre todo o caminho indicado pela rota explícita, desde o MP de ingresso até o MP de egresso.  Na vida real, provavelmente
*  cada nodo criaria sua própria mensagem de controle, com um msgID próprio.  A implementação presente simplifica o processo, aproveitando as funções já
*  implementadas de tratamento de um pacote de dados comum.
*  Uma busca é feita na LIB a fim de verificar se a working LSP, para o nodo em questão, tem o mesmo link de saída (oIface) que a backup LSP.  Se o link
*  de saída for o mesmo, então uma nova reserva de recursos não será feita; os recursos serão compartilhados pela working e backup LSP (sharing).  Nesta
*  implementação do TARVOS, ainda não se contempla recursos não compartilhados para um mesmo link de saída ou ainda modificação da reserva de recursos.
*  Caso a working e backup LSP não tenham o mesmo link (oIface) de saída, então as reservas de recursos devem ser feitas, portanto a função específica deve ser chamada.
*  Se os recursos foram reservados, a função deve retornar sucesso; caso contrário, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.
*
*  A função retorna 1 para SUCESSO (pacote foi processado completamente), e retorna 0 para FALHA (ou não havia recursos para reservar ou rota não existe ou
*  não há working LSP para merging; mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathDetour(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *plib;
	
	/*Caso 1:  recebido no MP de egresso; insira na fila do nodo e crie mensagem RESV_DETOUR_MAPPING para a rota inversa.
			   É preciso fazer uma busca para a entrada da working LSP na LIB, a fim de copiar o oIface e oLabel.  Não é preciso fazer
			   reserva de recursos aqui, pois haverá o sharing com os recursos já reservados para a working LSP.  Caso a working LSP não
			   seja encontrada, retornar com FALHA, pois compreende-se aqui que a backup LSP necessariamente termina num Merge Point.*/
	if (pkt->currentNode == pkt->dst) {
		plib=searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up"); //busca pela entrada na LIB correspondente à working LSP
		if (plib==NULL) //nenhuma entrada para working LSP foi encontrada; retorne com FALHA.  É preciso haver um Merge Point no final da backup LSP
			return 0; //FALHA

		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o próximo item do vetor ER (que começa de 0); portanto indica de fato o tamanho
																			   da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, plib->oIface, plib->iLabel, plib->oLabel, 0); //a oIface e oLabel são os mesmos para a working LSP
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
		createResvDetourControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, plib->oLabel); /*cria a mensagem RESV (a função também escalona evento de tratamento)
																													oLabel é copiado da entrada na LIB para a working LSP.*/
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH está sendo recebido num nodo LSR genérico; recolha as informações pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas só faça isso se houver recursos disponíveis no link de saída para a LSP.  Se o link de saída (oIface) for
	o mesmo da working LSP, então não faça nova reserva de recursos, pois estes serão compartilhados pela backup e working LSP.*/
	} else {
		/*se currentNode == próximo nodo da lista ER, então entenda que o pacote está no nodo de origem
		* e que o usuário começou sua lista explícita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a próxima.
		* (Não faz nenhum sentido a origem e destino serem iguais.  Este teste permitirá ao usuário construir sua lista
		* de roteamento explícito por nodos, começando pelo nodo onde o pacote é gerado, ou pelo nodo imediatamente após,
		* e ambas opções serão aceitas.)
		* Se isto for resultado de alguma inconsistência na lista ER, o funcionamento daqui para a frente é imprevisível.
		* (Rotina copiada da função decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avança o número nextIndex para a próxima posição (próximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o próximo nodo a percorrer, indicado por erNextIndex
		if (link==0) //não achou rota; não insira nada na fila; o pacote deverá ser descartado em outra função
			return 0; //FALHA:  rota não encontrada
	
		plib=searchInLIBnodLSPstatoIface(pkt->currentNode, pkt->lblHdr.LSPid, "up", link); //busca pela entrada na LIB correspondente à working LSP e para o mesmo link (oIface) de saída
		if (plib==NULL) { //não achou a entrada; então, tente a reserva de recursos, pois não há compartilhamento a fazer
			//tentar reserva de recursos
			if (reserveResouces(pkt->lblHdr.LSPid, link)==1) { //recursos foram reservados; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, 1);
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a geração de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		} else { //entrada na LIB para uma working LSP com mesmo link (oIface) de saída foi encontrada; não faça nova reserva de recurso -> RESOURCE SHARING
			//resource sharing:  não reservar recursos, encaminhe a mensagem adiante
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, plib->iLabel, plib->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
		return 0; //FALHA:  houve problema na reserva de recurso (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_DETOUR_MAPPING (backup LSP)
*
*  A mensagem RESV_DETOUR_MAPPING distribui o mapeamento de rótulo para backup LSP e efetiva a reserva de recursos para o LSP Tunnel (onde não houver
*  compartilhamento).
*  Os mapeamentos e reserva são feitos hop-by-hop, mas o LSP Tunnel só estará efetivado e válido quando a mensagem RESV_DETOUR_MAPPING atingir o Merge Point (MP)
*  de ingresso.  Caso isto não aconteça, os recursos e rótulos nos hops já efetivados entrarão em timeout, pois não haverá REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive MP de egresso), e recebida por um MP de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do rótulo,\
*  buscando no pool de rótulos disponíveis na interface de entrada um valor válido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  Quando recebida pelo MP de egresso, dados para a entrada na LIB são coletados do pacote e da mensagem armazenada na fila de mensagens de controle.
*  O oIface e oLabel são copiados da working LSP existente.  O iLabel também deve ser gerado a partir do pool de rótulos disponíveis.
*  No caso 2 (recebida por um MP de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adição de apagar da memória (com free) o
*  espaço ocupado pelo vetor de rota explícita invertida, criado juntamente com a mensagem RESV.  Um iLabel também é criado neste caso.  Este iLabel será
*  perdido, quando for substituído pelo iLabel da working LSP no momento do Rapid Recovery.
*
*  Somente esta função tem a prerrogativa de fazer mapeamento de rótulo e inserir os dados pertinentes na LIB para a backup LSP.
*/
static int nodeProcessResvDetourMapping(struct Packet *pkt) {
	int iLabel;
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR genérico; fazer o mapeamento de rótulo, inserir dados na LIB e LSP Table; rótulo inicial deve ser recuperado
	através de função específica.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem PATH correspondente; nada faça, neste caso.
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface); //cria iLabel a partir do pool de rótulos disponíveis
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 1, timeout, 0); //coloca zero no timeoutStamp, 1 para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (backup) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o rótulo agora criado na mensagem, para que o próximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguirá adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//backup LSP foi criada; o rótulo inicial é o mesmo que a working LSP de mesmo LSPid
	//Caso 2:  recebida pelo MP de ingresso:  faça tudo acima e também remova a rota explícita inversa da memória
		if (pkt->currentNode == pkt->dst) { //se este for o MP de ingresso (último destino), então libere o espaço da variável rota explícita inversa
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
		}
		return 1; //retorne com SUCESSO
	} else
		return 0; //retorne com FALHA; nenhuma mensagem PATH correspondente encontrada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_ERR
*
*  
*/
static int nodeProcessPathErr(struct Packet *pkt) {
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de ingresso; tomar atitude desejada.*/
	if (pkt->currentNode == pkt->dst) {
		//processamentos desejados aqui
		free(pkt->er.explicitRoute); //remover rota explícita
		pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  RESV está sendo recebido num nodo LSR genérico; deixe a mensagem ser encaminhada adiante.*/
	} else {
		p=searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up"); //busca a entrada na LIB para o nodo atual (working LSP)
		//se a tupla não for encontrada, significa rota inexistente; retornar com erro e o pacote será descartado na função de encaminhamento
		if (p!=NULL && p->iIface!=0) {
			if (pkt->er.explicitRoute==NULL) {//criará espaço para rota explícita, se ainda não foi criado
				pkt->er.explicitRoute = (int*)malloc(sizeof (*(pkt->er.explicitRoute))); //cria espaço para um int
				if (pkt->er.explicitRoute == NULL) {
					printf("\nError - nodeProcessPathErr - insufficient memory to allocate for explicit route object");
					exit (1);
				}
			}
			/*coloca o nodo destino na rota explícita; isto terá de ser removido com free antes da remoção do pacote;
			observar que o pacote trafega upstream; portanto, o nodo destino deverá ser o nodo src do link = iIface*/
			*pkt->er.explicitRoute = tarvosModel.lnk[p->iIface].src;
			pkt->er.erNextIndex = 0; //assegura que índice aponta para início da rota explícita
			pkt->lblHdr.label = 0; //assegura que não há label válido
			return 1; //SUCESSO:  mensagem foi processada completamente
		} else
			return 0; //FALHA:  rota não encontrada ou problemas na rota upstream
	}
	//se chegou até aqui, houve algum problema no processamento da RESV_ERR
	return 0; //FALHA:  a mensagem RESV_ERR deverá ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_ERR
*
*  A mensagem RESV_ERR segue no sentido downstream, para o LER de egresso.  Dois casos são válidos para processamento.
*
*  Caso 1:  Recebido pelo LER de egresso.  Neste caso, as ações desejadas pelo usuário devem ser tomadas (retorno de recursos reservados, etc.).
*
*  Caso 2:  Recebido por um LSR genérico.  Tomar alguma ação desejada e encaminhá-la adiante (seguirá comutada por rótulo.
*/
static int nodeProcessResvErr(struct Packet *pkt) {
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; tomar atitude desejada.*/
	if (pkt->currentNode == pkt->dst) {
		//processamentos desejados aqui
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  RESV está sendo recebido num nodo LSR genérico; deixe a mensagem ser encaminhada adiante.*/
	} else {
		//ações desejadas aqui
		
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada é o conteúdo de pkt->outgoingLink
		//se a tupla não for encontrada, significa rota inexistente; retornar com erro e o pacote será descartado na função de encaminhamento
		if (p!=NULL)
			return 1; //SUCESSO:  mensagem foi processada completamente
		else
			return 0; //FALHA:  rota não encontrada
	}
	//se chegou até aqui, houve algum problema no processamento da RESV_ERR
	return 0; //FALHA:  a mensagem RESV_ERR deverá ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_LABEL_REQUEST_PREEMPT
*
*  A mensagem PATH_LABEL_REQUEST pede mapeamento de rótulo e faz pré-reserva de recursos para um LSP Tunnel, caso haja recursos disponíveis.
*  Se não estiverem disponíveis, outras LSPs existentes no link de saída podem sofrer preempção para liberar os recursos necessários.
*  Os casos a considerar são:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), então este é o último destino da mensagem PATH.  Uma mensagem RESV_LABEL_MAPPING_PREEMPT deve ser agora
*  criada, mas o mapeamento de rótulo é feito pela função de processamento da mensagem RESV_LABEL_MAPPING_PREEMPT.  Assim, os dados da mensagem PATH_LABEL_REQUEST_PREEMPT
*  são introduzidos na fila de mensagens de controle do nodo.  Criar rota explícita inversa e colocar na mensagem RESV.  O oLabel para a RESV será ZERO e
*  o oIface introduzido na fila de mensagens também será ZERO.  Nenhuma reserva de recursos precisa ser feita para o LER de egresso.
*
*  A mensagem PATH_LABEL_REQUEST sempre é introduzida na fila de mensagens de controle do nodo, desde o LER de ingresso até o LER de egresso.
*
*  No segundo caso (recebido por um LSR genérico), os recursos só serão pré-reservados se estiverem disponíveis.  Se não estiverem, far-se-á uma varredura na LIB
*  para o nodo em questão, buscando LSPs estabelecidas no nodo para o link de saída.  Somente serão consideradas as LSPs com Holding Priority menor que a Setup Priority
*  da LSP corrente (0 é a maior prioridade, 7 a menor).  Para cada uma encontrada, os recursos serão somados para um acumulador.  Se o valor do acumulador for suficiente
*  para os requerimentos da LSP corrente, então a varredura termina e a mensagem segue adiante.  Não é feita nenhuma pré-reserva.  Caso a varredura termine e os recursos
*  continuarem insuficientes, mesmo com preempção, então a mensagem não segue adiante.
*  Observar que, nesta implementação do TARVOS, uma mesma mensagem PATH_LABEL_REQUEST_PREEMPT percorre todo o caminho indicado pela rota explícita, desde o LER de ingresso
*  até o LER de egresso.
*  Se os recursos foram reservados e a mensagem deve seguir adiante, o campo específico na fila de mensagens de controle é marcado e a função retorna 1.
*  Se os recursos não foram reservados, mas a mensagem deve seguir adiante (a preempção pode ser bem sucedida), a função deve retornar sucesso (1).
*  Caso contrário, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.  A preempção só é efetivamente realizada no processamento da mensagem
*  RESV_LABEL_MAPPING_PREEMPT, que pode encontrar uma outra realidade de recursos disponíveis no nodo.
*
*  A função retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (ou não havia recursos para reservar ou rota não existe;
*  mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathPreempt(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	int preempt=0; //flag que indica se preempção será possível
	int resv=0; //retorno da função reserveResource
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP

	/*Caso 1:  recebido no LER de egresso; insira na fila do nodo e crie mensagem RESV_LABEL_MAPPING_PREEMPT para a rota inversa.
			   Não é preciso fazer reserva de recursos aqui, pois em tese a mensagem só chegou aqui porque todos os links anteriores
			   aceitaram a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o próximo item do vetor ER (que começa de 0); portanto indica de fato o tamanho
																		 da rota explícita (todos os nodos até aqui percorridos) até o nodo atual.  Free() esta rota após uso!*/
		
		/* O mapeamento de rótulo e inserção na LIB foram retirados daqui; somente a função nodeProcessResvLabelMapping é que tratará disso
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser criada agora
		iLabel=nodeCreateLabel(pkt->currentNode, pkt->outgoingLink);
		insertInLIB(pkt->currentNode, pkt->outgoingLink, iLabel, 0, 0, pkt->lblHdr.LSPid, "up", timeout, 0); //coloca ZERO no timeoutStamp
		*/

		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface é ZERO, pois trata-se do LER de egresso
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
		createResvPreemptControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, 0); //cria a mensagem RESV e também escalona evento de tratamento (contido no pacote PATH); o rótulo é ZERO, pois é o LER de egresso
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH está sendo recebido num nodo LSR genérico; recolha as informações pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas só faça isso se houver recursos disponíveis no link de saída para a LSP, com ou sem preempção.
	Se estiverem imediatamente disponíveis, faça a pré-reserva.*/
	} else {
		/*se currentNode == próximo nodo da lista ER, então entenda que o pacote está no nodo de origem
		* e que o usuário começou sua lista explícita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a próxima.
		* (Não faz nenhum sentido a origem e destino serem iguais.  Este teste permitirá ao usuário construir sua lista
		* de roteamento explícito por nodos, começando pelo nodo onde o pacote é gerado, ou pelo nodo imediatamente após,
		* e ambas opções serão aceitas.)
		* Se isto for resultado de alguma inconsistência na lista ER, o funcionamento daqui para a frente é imprevisível.
		* (Rotina copiada da função decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avança o número nextIndex para a próxima posição (próximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o próximo nodo a percorrer, indicado por erNextIndex
		if (link!=0) {//se link==0, rota não foi encontrada; não insira nada na fila; o pacote deverá ser descartado em outra função
			resv=reserveResouces(pkt->lblHdr.LSPid, link);
			if (resv==0) { //recursos não estão imediatamente disponíveis; faça varredura p/ verificar se com preempção, estarão disponíveis; mas não reserve nada agora!
				preempt=testResources(pkt->lblHdr.LSPid, pkt->currentNode, link);
			}

			if (resv==1 || preempt==1) { //recursos foram reservados ou podem ser reservados com preempção; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expiração da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, resv); //resv aqui indicará se recursos foram pré-reservados, ou não
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote está chegando no nodo para onde foi gerado (o primeiro do Path), então este número será zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a geração de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		}
		return 0; //FALHA:  houve problema na reserva de recurso ou rota (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_LABEL_MAPPING_PREEMPT (working LSP)
*
*  A mensagem RESV_LABEL_MAPPING_PREEMPT distribui o mapeamento de rótulo e efetiva a reserva de recursos para o LSP Tunnel, usando também de preempção
*  de LSPs existentes caso necessário.
*  Os mapeamentos e reserva são feitos hop-by-hop, mas o LSP Tunnel só estará efetivado e válido quando a mensagem RESV_LABEL_MAPPING_PREEMPT atingir o LER
*  de ingresso.  Caso isto não aconteça, os recursos e rótulos nos hops já efetivados entrarão em timeout, pois não haverá REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive LER de egresso), e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do rótulo,\
*  buscando no pool de rótulos disponíveis na interface de entrada um valor válido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  O processamento verificará, também, se o campo resourcesReserved está marcado, o que indica que recursos já foram pré-reservados pela PATH_PREEMPT.
*  Se o foram, nada mais é feito.  Se não o foram, verificar-se-á se existem recursos suficientes para a reserva.  Se houver, faz a reserva e a mensagem
*  segue adiante (a reserva pode não ter sido feita pela PATH pois, no momento, os recursos não eram suficientes).
*  Se não os recursos não forem imediatamente suficientes, então far-se-á uma busca na LIB para LSPs pré-existentes no nodo-oIface.
*  Uma lista auxiliar (LSPList) será criada, por ordem decrescente de Holding Priority, somente com aquelas LSPs de Holding Priority menor que a Setup Priority
*  da LSP corrente (0 é a maior prioridade, 7 a menor).
*  Para cada LSP de menor prioridade, os recursos desta serão acumulados, até que os acumuladores acusem valores suficientes para a LSP corrente.  Se isto acontecer,
*  então a lista auxiliar é percorrida, e cada LSP nesta lista é colocada em estado "preempted" e seus recursos, devolvidos ao link oIface.  Isso se repete até
*  que os recursos acumulados sejam suficientes para a LSP corrente.  Então, uma nova reserva para a LSP corrente é feita e a mensagem segue adiante.
*  Se, mesmo com preempção, não houver recursos necessários para a LSP corrente, a mensagem deverá ser descartada.
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adição de apagar da memória (com free) o
*  espaço ocupado pelo vetor de rota explícita invertida, criado juntamente com a mensagem RESV, e marcar a entrada para o LSP tunnel, na LSP Table, como
*  completado.  (A fim de se fazer uma backup LSP, o working LSP precisa estar completado.)
*
*  Somente esta função tem a prerrogativa de fazer mapeamento de rótulo e inserir os dados pertinentes na LIB (popular a LIB) para a working LSP, com preempção.
*
*  A LSPList é uma lista duplamente encadeada, circular, com Head Node.
*  Na manipulação da lista auxiliar LSPList, está é construída inserindo cada LSP encontrada para o nodo-oIface em ordem decrescente de prioridade, mas somente para
*  LSPs com Holding Priority menor que a Setup Priority da LSP corrente e até que os recursos sejam suficientes para os requerimentos, ou até que a varredura da LIB
*  termine.
*  Na efetivação da preempção, esta lista é percorrida da menor prioridade para a maior, a LSP indicada é marcada como "preempted", seus recursos, devolvidos, e a
*  entrada correspondente na LSPList, apagada com free.  Ao final, também o Head Node é apagado.
*/
static int nodeProcessResvPreempt(struct Packet *pkt) {
	int iLabel;
	double timeout; //para cálculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR genérico; fazer o mapeamento de rótulo, inserir dados na LIB e LSP Table; rótulo inicial deve ser recuperado
	através de função específica.  Fazer preempção se necessário.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, não foi encontrada nenhuma mensagem PATH correspondente; nada faça, neste caso.
		if (msg->resourcesReserved==0 && msg->oIface!=0) { //se recursos não foram previamente reservados, tente reservá-los agora (mas não o faça para nodos de egresso! msg->oIface==0)
			if (preemptResouces(msg->LSPid, pkt->currentNode, msg->oIface)==0)
				return 0; //reserva não foi conseguida mesmo com preempção; acuse falha e saia
		}
		//recursos foram pré-reservados ou conseguidos agora com sucesso; prossiga
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface);
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expiração da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 0, timeout, 0); //coloca zero no timeoutStamp, zero para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (working) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o rótulo agora criado na mensagem, para que o próximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguirá adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//LSP foi criada; usuário deve usar a função getWorkingLSPLabel para recuperar o rótulo inicial
	//Caso 2:  recebida pelo LER de ingresso:  faça tudo acima, também remova a rota explícita inversa da memória e marque o LSP tunnel como completo
		if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (último destino), então libere o espaço da variável rota explícita inversa
			setLSPtunnelDone(pkt->lblHdr.LSPid);  //marque o LSP tunnel como totalmente completado
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execução em uma nova eliminação desta rota explícita em outra função
		}
		return 1; //retorne com SUCESSO
	} else
		return 0; //retorne com FALHA; nenhuma mensagem PATH correspondente encontrada
}
