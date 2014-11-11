/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_node
*
* Fun��es que ser�o usadas pelo programa de roteamento do n�cleo para cria��o, parametriza��o,
* inicializa��o, opera��o dos nodos.
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

#include "simm_globals.h" //para uso da fun��o nodeDropPacket, que faz uma chamada a simtime().  Se esta chamada for dispensada, pode-se apagar esta linha
#include "tarvos_globals.h"

//Prototypes das fun��es locais (static)
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

/* CRIA��O DOS NODOS
*
*  Cria a Estrutura dos Nodos, que conter� campos para armazenagem de estat�sticas e outras coisas.
*  Os par�metros a passar s�o o n�mero do nodo a criar (que � o �ndice do vetor de estruturas de nodos) e o n�mero de interfaces que o nodo
*  conter�.  Cada n�mero de interface, neste simulador, coincidir� necessariamente com o n�mero �nico do link a que a interface est� conectada.
*  O vetor de interfaces ser� criado dinamicamente.
*/
void createNode(int n_node) {
    //Inicializa estat�sticas dos nodos
	int i, links;
	
	tarvosModel.node[n_node].packetsReceived = 0; //N�mero de pacotes que chegaram a este nodo
	tarvosModel.node[n_node].packetsForwarded = 0; //N�mero de pacotes que foram encaminhados a partir deste nodo
	tarvosModel.node[n_node].packetsDropped = 0; //N�mero de pacotes descartados ou perdidos no nodo (sempre partindo do nodo)(transmiss�o + propaga��o)
	tarvosModel.node[n_node].bytesReceived = 0; //Quantidade de bytes recebidos pelo nodo (admite-se pacotes de tamanho diferente)
	tarvosModel.node[n_node].bytesForwarded = 0; //quantidade de bytes encaminhados a partir deste nodo
	tarvosModel.node[n_node].delay=0; //atraso medido para o �ltimo pacote recebido pelo nodo
	tarvosModel.node[n_node].delaySum=0; //somat�rio dos atrasos medidos para pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanDelay=0; //atraso m�dio medido para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].jitter=0; //�ltimo jitter medido para o �ltimo pacote recebido por este nodo
	tarvosModel.node[n_node].jitterSum=0; //somat�rio dos jitters medidos para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanJitter=0; //jitter m�dio medido para os pacotes recebidos por este nodo
	//abaixo, estat�sticas para pacotes n�o-controle
	tarvosModel.node[n_node].packetsReceivedAppl = 0; //N�mero de pacotes que chegaram a este nodo
	tarvosModel.node[n_node].packetsForwardedAppl = 0; //N�mero de pacotes que foram encaminhados a partir deste nodo
	tarvosModel.node[n_node].bytesReceivedAppl = 0; //Quantidade de bytes recebidos pelo nodo (admite-se pacotes de tamanho diferente)
	tarvosModel.node[n_node].bytesForwardedAppl = 0; //quantidade de bytes encaminhados a partir deste nodo
	tarvosModel.node[n_node].delayAppl=0; //atraso medido para o �ltimo pacote recebido pelo nodo
	tarvosModel.node[n_node].delaySumAppl=0; //somat�rio dos atrasos medidos para pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanDelayAppl=0; //atraso m�dio medido para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].jitterAppl=0; //�ltimo jitter medido para o �ltimo pacote recebido por este nodo
	tarvosModel.node[n_node].jitterSumAppl=0; //somat�rio dos jitters medidos para os pacotes recebidos por este nodo
	tarvosModel.node[n_node].meanJitterAppl=0; //jitter m�dio medido para os pacotes recebidos por este nodo
	links=(sizeof tarvosModel.lnk / sizeof *(tarvosModel.lnk)); //tamanho do vetor de links (que � o n�mero real de links + 1)
	tarvosModel.node[n_node].nextLabel=(int(*)[])malloc(links * sizeof(**(tarvosModel.node[n_node].nextLabel))); //cria vetor com [n�mero de links] posi��es
	if (tarvosModel.node[n_node].nextLabel==NULL) {
		printf("\nError - createNode - insufficient memory to allocate for label pool");
		exit(1);
	}
	(*(tarvosModel.node[n_node].nextLabel))[0]=1; //a posi��o zero do vetor serve para pacotes gerados no pr�prio nodo; use r�tulo inicial 1
	for(i=1; i<links; i++) { /*a primeira posi��o do vetor, zero, � usada para indicar pacote sendo gerado no pr�prio nodo;
							   as interfaces come�am de 1 (para seguir o padr�o dos links, fontes, nodos, etc.*/
		(*(tarvosModel.node[n_node].nextLabel))[i]=i*100; /*pr�ximo r�tulo dispon�vel para uso em constru��o de uma LSP para MPLS; ao usar este r�tulo, este campo deve ser
														 incrementado a fim de assegurar r�tulos �nicos por interface.
														 R�tulo zero significa, no m�dulo TARVOS, que o pacote n�o deve ser encaminhado por r�tulo (est� saindo de um dom�nio MPLS).*/
	}
	tarvosModel.node[n_node].helloTimeLimit=(double(*)[])malloc(links * sizeof(**(tarvosModel.node[n_node].helloTimeLimit))); //cria vetor para limites de tempo para recebimento de HELLO-ACKS (um time stamp para cada link)
	if (tarvosModel.node[n_node].helloTimeLimit==NULL) {
		printf("\nError - createNode - insufficient memory to allocate for helloTimeLimit");
		exit(1);
	}
	for(i=0; i<links; i++) {
		(*(tarvosModel.node[n_node].helloTimeLimit))[i]=0; //inicializa todos os limites (inclusive o �ndice zero, n�o usado) para zero
	}
	tarvosModel.node[n_node].ctrlMsgHandlEv = tarvosParam.ctrlMsgHandlEv; //seta evento default (evento que conter� o tratamento de mensagens de controle)
	tarvosModel.node[n_node].helloMsgGenEv = tarvosParam.helloMsgGenEv; //seta evento default para tratar mensagens HELLO
	tarvosModel.node[n_node].ctrlMsgTimeout = tarvosParam.ctrlMsgTimeout; //define timeout default para mensagens de controle do RSVP-TE (tipicamente PATH)
	tarvosModel.node[n_node].LSPtimeout = tarvosParam.LSPtimeout; //define timeout default para as LSPs que partem deste nodo
	tarvosModel.node[n_node].helloMsgTimeout = tarvosParam.helloMsgTimeout; //define timeout default para as mensagens HELLO geradas a partir deste nodo
    tarvosModel.node[n_node].helloTimeout = tarvosParam.helloTimeout; //tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunica��o com o nodo)
	createNodeMsgQueue(n_node);
}

/* CRIA��O DA LISTA DE MENSAGENS DE CONTROLE DO NODO
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
	tarvosModel.node[n_node].nodeMsgQueue->next = tarvosModel.node[n_node].nodeMsgQueue; //perfaz a caracter�stica circular da lista
	return tarvosModel.node[n_node].nodeMsgQueue;
}

/* INSERE NOVO ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO
*  (nota:  a Lista de Mensagens � uma lista din�mica duplamente encadeada circular com Head Node)
*  Passar todo o conte�do de uma linha da lista como par�metros
*/
void insertInNodeMsgQueue(int n_node, char *msgType, int msgID, int msgIDack, int LSPid, int er[], int erIndex, int source, int dst, double timeout,
						  int ev, int iIface, int oIface, int iLabel, int oLabel, int resourcesReserved) {
	struct nodeMsgQueue *p; //vari�vel tipo apontador auxiliar
	char traceString[255];

	p=tarvosModel.node[n_node].nodeMsgQueue;
	p->previous->next = (nodeMsgQueue*)malloc(sizeof *(p->previous->next)); //cria mais um n� ao final da lista (lista duplamente encadeada)
	if (p->previous->next==NULL) {
		printf("\nError - insertInNodeMsgQueue - insufficient memory to allocate for Node Control Message Queue");
		exit(1);
	}
	p->previous->next->previous=p->previous; //atualiza ponteiro previous do novo n� da lista
	p->previous->next->next=p; //atualiza ponteiro next do novo n� da lista
	p->previous=p->previous->next; //atualiza ponteiro previous do Head Node
	p->previous->previous->next=p->previous; //atualiza ponteiro next do pen�ltimo n�
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
	p->previous->ev=ev; /*insere o evento de tratamento informado pelo usu�rio (pode ser, por exemplo, um evento de transmiss�o do pacote;
						um LER de destino recebe uma mensagem PATH e gera automaticamente uma mensagem RESV para o caminho inverso; deve ent�o
						escalonar este evento para tratar a mensagem RESV rec�m-criada*/
	sprintf(traceString, "CtrlMsg inserted in node Queue: node:  %d  msgID:  %d  LSPid:  %d  src:  %d  dst:  %d  iLabel:  %d\n", n_node, msgID, LSPid, source, dst, iLabel);
	mainTrace(traceString);
}

/* BUSCA ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO POR LSP_ID
*  (nota:  a Lista de Mensagens � uma lista din�mica duplamente encadeada circular com Head Node)
*  A chave de busca � o LSPid para um determinado nodo.  Retorna posi��o do item buscado, se encontrado;
*  caso contr�rio, retorna NULL.  Fun��o que chama deve testar isso.
*/
struct nodeMsgQueue *searchInNodeMsgQueueLSPid(int n_node, int LSPid) {
	struct nodeMsgQueue *p;
	
	p=tarvosModel.node[n_node].nodeMsgQueue->next;
	tarvosModel.node[n_node].nodeMsgQueue->LSPid=LSPid;  //coloca item buscado no Head Node
	while (p->LSPid != LSPid) {
		p=p->next; //se item n�o existir, busca termina no HEAD NODE (lista circular)
	}
	if (p==tarvosModel.node[n_node].nodeMsgQueue) //se p for o Head Node (guardado na estrutura 'node'), item buscado n�o foi encontrado
		return NULL;
	else
		return p; //item foi encontrado; retorne sua posi��o
}

/* BUSCA ITEM NA LISTA DE MENSAGENS DE CONTROLE DO NODO POR MSG_ID_ACK
*  (nota:  a Lista de Mensagens � uma lista din�mica duplamente encadeada circular com Head Node)
*  A chave de busca � o msgIDack para um determinado nodo.  Retorna posi��o do item buscado, se encontrado;
*  caso contr�rio, retorna NULL.  Fun��o que chama deve testar isso.
*/
struct nodeMsgQueue *searchInNodeMsgQueueAck(int n_node, int msgIDack) {
	struct nodeMsgQueue *p;
	
	p=tarvosModel.node[n_node].nodeMsgQueue->next;
	tarvosModel.node[n_node].nodeMsgQueue->msgID=msgIDack;  //coloca item buscado no Head Node
	while (p->msgID != msgIDack) {
		p=p->next; //se item n�o existir, busca termina no HEAD NODE (lista circular)
	}
	if (p==tarvosModel.node[n_node].nodeMsgQueue) //se p for o Head Node (guardado na estrutura 'node'), item buscado n�o foi encontrado
		return NULL;
	else
		return p; //item foi encontrado; retorne sua posi��o
}

/* REMOVE ITEM DA FILA DE MENSAGENS DE CONTROLE DO NODO USANDO LSPid
*  (nota:  a Fila de Mensagens � uma lista din�mica duplamente encadeada circular com Head Node)
*  A chave de busca � o LSPid para um determinado nodo.
*/
void removeFromNodeMsgQueueLSPid(int n_node, int LSPid) {
	struct nodeMsgQueue *p; //vari�vel tipo apontador auxiliar
	char traceString[255];

	p=searchInNodeMsgQueueLSPid(n_node, LSPid);
	if (p!=NULL) { //se for NULL, item n�o foi encontrado
		p->next->previous=p->previous;
		p->previous->next=p->next;
		//Debug
		sprintf(traceString, "Ctrl Msg REMOVED - msgID: %d msgIDack: %d LSPid: %d src: %d dst: %d iIface: %d oIface: %d  %s\n", p->msgID, p->msgIDack, p->LSPid, p->src,
			p->dst, p->iIface, p->oIface, p->msgType);
		mainTrace(traceString);
		free(p); //libera espa�o ocupado por n� em p
	}
}

/* REMOVE ITEM DA FILA DE MENSAGENS DE CONTROLE DO NODO USANDO msgID
*  (nota:  a Fila de Mensagens � uma lista din�mica duplamente encadeada circular com Head Node)
*  A chave de busca � o LSPid para um determinado nodo.
*/
void removeFromNodeMsgQueueAck(int n_node, int msgIDack) {
	struct nodeMsgQueue *p; //vari�vel tipo apontador auxiliar
	char traceString[255];

	p=searchInNodeMsgQueueAck(n_node, msgIDack);
	if (p!=NULL) { //se for NULL, item n�o foi encontrado
		p->next->previous=p->previous;
		p->previous->next=p->next;
		//Debug
		sprintf(traceString, "Ctrl Msg REMOVED - msgID: %d msgIDack: %d LSPid: %d src: %d dst: %d iIface: %d oIface: %d  %s\n", p->msgID, p->msgIDack, p->LSPid, p->src,
			p->dst, p->iIface, p->oIface, p->msgType);
		mainTrace(traceString);
		free(p); //libera espa�o ocupado por n� em p
	}
}

/* RECEP��O DE UM PACOTE POR UM NODO
*
*  Esta fun��o recebe um pacote pelo nodo e aciona os procedimentos necess�rios dependendo do tipo de pacote recebido.
*  Basicamente checa se o pacote � de dados, se � mensagem de controle, se � com destino final ao nodo em quest�o e se a rota percorrida pelo
*  pacote deve ser gravada no objeto RecordRoute.
*
*  Esta fun��o atualiza as estat�sticas convenientes na estrutura de dados do nodo
*  e tamb�m descarta o pacote, caso o nodo seja o destino final do pacote.
*
*  A fun��o retorna 0 se o nodo atual n�o for o nodo destino do pacote; o programa do
*  usu�rio deve ent�o fazer o escalonamento do pacote para a rotina de tratamento de
*  novo encaminhamento por um link.
*  A fun��o retorna 1 se o nodo for o destino final do pacote.  O pacote j� ter� sido
*  descartado e as estat�sticas atualizadas.  O programa do usu�rio pode tomar alguma
*  provid�ncia espec�fica neste caso, se desejar.
*  A fun��o retorna 2 se o pacote foi descartado (tipicamente por causa do estouro do TTL).
*
*  Aten��o deve ser dada para chamar a fun��o nodeUpdateStats quando as estat�sticas do nodo necessitarem ser atualizadas (basicamente, sempre antes de
*  um return sem descarte).
*/
int nodeReceivePacket(struct Packet *pkt) {
	if (pkt->er.recordThisRoute==1) //se a flag estiver ativada, grave a rota no objeto RecordRoute
		recordRoute(pkt);

	/*Se o link atual do pacote for menor que 1, isto significa que o pacote N�O est� trafegando por um
	link f�sico, mas vindo diretamente de um gerador de tr�fego.*/
	if (pkt->outgoingLink > 0)
		removePktFromTransitQueue(pkt->outgoingLink, pkt->id); //s� faz a remo��o se o link de fato existir
	
	if (pkt->ttl <= 0) { //TTL abaixo do limite; descartar o pacote
		//verificar se aqui as estat�sticas do nodo devem ser reparadas (bytesReceived, PacketsReceived, etc.), pois o pacote est� sendo descartado
		nodeDropPacket(pkt, "TTL limit reached");
		return 2;
	}
	//Testar se o pacote cont�m uma mensagem de controle; caso positivo, chame rotina de processamento de controle
	if (pkt->lblHdr.msgID!=0) { //se msgID=0, ent�o pacote cont�m dados, e n�o mensagem de controle
		if (nodeReceiveCtrlMsg(pkt)==0) { //mensagem resultou em falha; descarte-a
			//verificar se aqui as estat�sticas do nodo devem ser reparadas (bytesReceived, PacketsReceived, etc.), pois o pacote est� sendo descartado
			nodeDropPacket(pkt, "RSVP-TE control message error");
			return 2;
		}
	}
	
	if (pkt->currentNode == pkt->dst) {
        /* Se for verdade significa que o destino do pacote � para o nodo atual.  Deve-se liberar o pacote da mem�ria (freePkt(pkt)) */
		nodeUpdateStats(pkt); //atualiza estat�sticas (menos encaminhamento)
		freePkt(pkt); //libera o espa�o de mem�ria ocupado pelo packet
		return 1;
	} else {
		/* Caso seja falso, significa que o pacote est� em um roteador que n�o � o seu destino e por conseguinte dever� ser
		encaminhado para um link, atrav�s da rotina de encaminhamento apropriada.
		Isto deve ser feito no programa principal do usu�rio.  O TTL � decrementado aqui, mas n�o se o pacote foi "gerado" neste nodo.*/

		/*Se o link atual do pacote for menor que 1, isto significa que o pacote N�O est� trafegando por um
		link f�sico, mas vindo diretamente de um gerador de tr�fego.  Ent�o, neste caso, o TTL N�O deve ser decrementado*/
		if (pkt->outgoingLink > 0)
			pkt->ttl--; //TTL acima do limite; decremente (s� se o pacote n�o houver sido originado neste mesmo nodo)
		nodeUpdateStats(pkt); //atualiza estat�sticas globais (menos encaminhamento)
		nodeUpdateForwardStats(pkt); //atualiza estat�sticas de encaminhamento
		return 0;
	}
}

/* ATUALIZA ESTAT�STICAS FORWARD DO NODO
*
*  As estat�sticas atualizadas s�o:
*  packetsForwarded:  quantidade de pacotes encaminhados por este nodo
*  bytesForwarded:  quantidade de bytes encaminhados por este nodo
*  packetsForwardedAppl:  quantidade de pacotes encaminhados por este nodo somente para aplica��o (ou seja, pacotes que n�o sejam de controle)
*  bytesForwardedAppl:  quantidade de bytes encaminhados por este nodo somente para aplica��o (ou seja, pacotes que n�o sejam de controle)
*
*  O nodo corrente � obtido do campo currentNode do pacote.
*/
static void nodeUpdateForwardStats(struct Packet *pkt) {
	tarvosModel.node[pkt->currentNode].packetsForwarded++;
	tarvosModel.node[pkt->currentNode].bytesForwarded+=pkt->length;
	if (pkt->lblHdr.msgID==0) {//pacote � de aplica��o; atualize as estat�sticas espec�ficas para aplica��o (ou seja, pacote que n�o � de controle)
		tarvosModel.node[pkt->currentNode].packetsForwardedAppl++;
		tarvosModel.node[pkt->currentNode].bytesForwardedAppl+=pkt->length;
	}
}

/* ATUALIZA ESTAT�STICAS DO NODO
*
*  As estat�sticas atualizadas s�o:
*  bytesReceived:  quantidade de bytes recebidos pelo nodo
*  packetsReceived:  quantidade de pacotes recebidos pelo nodo
*  delay:  atraso registrado para o pacote
*  delaySum:  somat�rio de todos os delays registrados no nodo
*  meanDelay:  atraso m�dio (meanDelay = delaySum/packetsReceived)
*  jitter:  jitter deste pacote (jitter = atraso deste pacote - atraso do pacote anterior)
*  jitterSum:  somat�rio dos jitters
*  meanJitter:  jitter m�dio (meanJitter = jitterSum / (packetsReceived-1)). Notice that meanJitter = jitterSum / #Jitter Samples. And #Jitter Samples = packetsReceived - 1 (every two packets received yield one jitter sample).
*
*  O n�mero packetsForwarded N�O � atualizado aqui!  Cuidar disso na fun��o nodeUpdateForwardStats.
*
*  O nodo corrente ou atual � obtido do campo currentNode do pacote.
*/
static void nodeUpdateStats(struct Packet *pkt) {
	double previousDelay; //atraso anterior do pacote (anterior) recebido por este nodo, para c�lculo do jitter
	double stime;
	
	/*Quest�o:  os pacotes gerados no pr�prio nodo passam por esta fun��o, antes de serem transmitidos.  Ainda assim as estat�sticas devem
	ser atualizadas para estes pacotes?  (O delay de um pacote deste tipo ser� zero!)*/
	if (pkt->currentNode==pkt->src) //se verdadeiro, pacote foi gerado no pr�prio nodo; n�o computar estas estat�sticas
		return;
	
	stime=simtime(); //registra tempo corrente uma �nica vez (para evitar repetidas chamadas � fun��o simtime)
	//estat�sticas globais
	previousDelay=tarvosModel.node[pkt->currentNode].delay;
	tarvosModel.node[pkt->currentNode].packetsReceived++; //atualiza estat�sticas do nodo para packetsReceived
	tarvosModel.node[pkt->currentNode].bytesReceived+=pkt->length; //atualiza estat�sticas do nodo para bytesReceived
	tarvosModel.node[pkt->currentNode].delay=stime-pkt->generationTime; //atualiza delay deste pacote
	tarvosModel.node[pkt->currentNode].delaySum+=tarvosModel.node[pkt->currentNode].delay; //atualiza somat�rio dos delays
	tarvosModel.node[pkt->currentNode].meanDelay=tarvosModel.node[pkt->currentNode].delaySum/tarvosModel.node[pkt->currentNode].packetsReceived; //atualiza delay m�dio
	if (tarvosModel.node[pkt->currentNode].packetsReceived>1) //s� calcula jitter se houver pacote anterior recebido (n�o calcula para o primeiro pacote)
		tarvosModel.node[pkt->currentNode].jitter=tarvosModel.node[pkt->currentNode].delay-previousDelay; //calcula jitter para este pacote
	tarvosModel.node[pkt->currentNode].jitterSum+=tarvosModel.node[pkt->currentNode].jitter; //calcula somat�rio dos jitters
	if (tarvosModel.node[pkt->currentNode].packetsReceived>1) //s� calcula o jitter m�dio se houver pacote j� recebido
		tarvosModel.node[pkt->currentNode].meanJitter=tarvosModel.node[pkt->currentNode].jitterSum/(tarvosModel.node[pkt->currentNode].packetsReceived-1); //calcula jitter m�dio (a quantidade de jitters guardados � igual � de pacotes recebidos -1)
	//Desabilitar a chamada abaixo para distribui��o
	jitterDelayTrace(pkt->currentNode, stime, tarvosModel.node[pkt->currentNode].jitter, tarvosModel.node[pkt->currentNode].delay);

	//estat�sticas para pacotes exclusivamente de aplica��o
	if (pkt->lblHdr.msgID==0) {
		previousDelay=tarvosModel.node[pkt->currentNode].delayAppl;
		tarvosModel.node[pkt->currentNode].packetsReceivedAppl++; //atualiza estat�sticas do nodo para packetsReceived
		tarvosModel.node[pkt->currentNode].bytesReceivedAppl+=pkt->length; //atualiza estat�sticas do nodo para bytesReceived
		tarvosModel.node[pkt->currentNode].delayAppl=stime-pkt->generationTime; //atualiza delay deste pacote
		tarvosModel.node[pkt->currentNode].delaySumAppl+=tarvosModel.node[pkt->currentNode].delayAppl; //atualiza somat�rio dos delays
		tarvosModel.node[pkt->currentNode].meanDelayAppl=tarvosModel.node[pkt->currentNode].delaySumAppl/tarvosModel.node[pkt->currentNode].packetsReceivedAppl; //atualiza delay m�dio
		if (tarvosModel.node[pkt->currentNode].packetsReceivedAppl>1) //s� calcula jitter se houver pacote anterior recebido (n�o calcula para o primeiro pacote)
			tarvosModel.node[pkt->currentNode].jitterAppl=tarvosModel.node[pkt->currentNode].delayAppl-previousDelay; //calcula jitter para este pacote
		tarvosModel.node[pkt->currentNode].jitterSumAppl+=tarvosModel.node[pkt->currentNode].jitterAppl; //calcula somat�rio dos jitters
		if (tarvosModel.node[pkt->currentNode].packetsReceivedAppl>1) //s� calcula o jitter m�dio se houver pacote j� recebido
			tarvosModel.node[pkt->currentNode].meanJitterAppl=tarvosModel.node[pkt->currentNode].jitterSumAppl/(tarvosModel.node[pkt->currentNode].packetsReceivedAppl-1); //calcula jitter m�dio (a quantidade de jitters guardados � igual � de pacotes recebidos -1)
		//Desabilitar a chamada abaixo para distribui��o
		jitterDelayApplTrace(pkt->currentNode, stime, tarvosModel.node[pkt->currentNode].jitterAppl, tarvosModel.node[pkt->currentNode].delayAppl);
	}
}	

/* REGISTRA A PERDA DE UM PACOTE POR UM NODO, NA TRANSMISS�O OU PROPAGA��O
*
*  Incrementa o contador de pacotes perdidos ou descartados no nodo.  Os pacotes sempre
*  devem estar partindo do nodo, no servidor de transmiss�o ou na propaga��o pelo link.
*  Os pacotes perdidos durante a propaga��o constar�o da contagem de pacotes perdidos no
*  nodo de origem destes, e n�o no nodo de destino (que, para todos os efeitos, jamais soube
*  que os pacotes descartados existiram).
*  Esta fun��o deve ser chamada idealmente por uma fun��o que traz um link para o estado down,
*  chamando a fun��o interna do kernel do simulador simm.  O nodo atual do pacote ou correspondente
*  � facility tem de ser obtido antes do descarte, caso contr�rio este dado ser� perdido junto com
*  a estrutura.
*  Importante notar que o pacote em si n�o � descartado, pois supostamente este j� o foi em
*  alguma outra fun��o (como no kernel do simulador simm, que descarta tokens ao ter uma facility
*  passada para status down).
*  11.Jan.2006 Marcos Portnoi
*/
void nodeIncDroppedPacketsNumber(int nodeNumber) {
	tarvosModel.node[nodeNumber].packetsDropped++;
}

/* DESCARTA UM PACOTE NO NODO
*
*  Se uma facility de transmiss�o de um link recusar um pacote por estar n�o-operacional (down),
*  ent�o o pacote deve ser descartado.  Esta fun��o cuida disto, eliminando a estrutura e
*  incrementando o contador de pacotes descartados do nodo.
*  11.Jan.2006 Marcos Portnoi
*/
void nodeDropPacket(struct Packet *pkt, char *dropReason) {
	char dropTraceEntry[255];

	nodeIncDroppedPacketsNumber(pkt->currentNode); //incrementa contador de packets dropped
	sprintf(dropTraceEntry, "simtime: %f  node: %d  ID: %d  msgID: %d label: %d src: %d  dst: %d outgoingLink: %d  reason: %s\n", simtime(), pkt->currentNode, pkt->id, pkt->lblHdr.msgID, pkt->lblHdr.label, pkt->src, pkt->dst, pkt->outgoingLink, dropReason);
	dropPktTrace(dropTraceEntry);
	freePkt(pkt); //descarta o pacote da mem�ria
}

/* RECEBE E PROCESSA UM PACOTE COM MENSAGEM DE CONTROLE DE PROTOCOLO
*
*  Esta fun��o � tipicamente chamada pela fun��o nodeReceivePacket.  Aqui deve-se checar que tipo de mensagem cont�m o pacote e process�-la
*  de acordo.
*  A fun��o retorna 0 se houve alguma falha no processamento da mensagem, e retorna 1 se houve o processamento completo bem sucedido.
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
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a fun��o chamante
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
			return 1; //PATH_REFRESH foi bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da PATH_REFRESH; indique isto para fun��o chamante
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
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a fun��o chamante
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
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a fun��o chamante
	}

	//RESV_ERR
	if (strcmp(pkt->lblHdr.msgType, "RESV_ERR")==0) {
		if (nodeProcessResvErr(pkt)==1) //verifique se o processamento da RESV foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da RESV; indique isto para a fun��o chamante
	}
	
	//PATH_LABEL_REQUEST_PREEMPT
	if (strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST_PREEMPT")==0) {
		if (nodeProcessPathPreempt(pkt)==1) //verifique se o processamento da PATH foi bem sucedido
			return 1; //PATH foi processada de forma bem sucedida; reporte isso para a fun��o chamante
		else
			return 0; //houve falha no processamento da PATH; indique isto para a fun��o chamante
	}

	//RESV_LABEL_MAPPING_PREEMPT
	if (strcmp(pkt->lblHdr.msgType, "RESV_LABEL_MAPPING_PREEMPT")==0) {
		if (nodeProcessResvPreempt(pkt)==1)
			return 1; //retorne sucesso para processamento RESV_LABEL_MAPPING_PREEMPT
		else
			return 0; //retorne FALHA
	}

	return 0; //se chegar at� aqui, houve algum tipo de falha no processamento da mensagem de controle
}

/* CRIA E FORNECE UM R�TULO A PARTIR DO POOL DE R�TULOS DISPON�VEIS PARA O NODO
*
*  Fornece um n�mero de r�tulo dispon�vel para a interface do nodo em quest�o.  Este r�tulo tipicamente ser� usado em mapeamento pelo RSVP-TE numa mensagem RESV.
*  Os r�tulos s�o �nicos por interface no nodo (sendo que as interfaces recebem o n�mero dos links a que est�o ligadas).
*  Interface zero (ou �ndice zero) indica que o pacote est� sendo gerado neste pr�prio nodo.
*/
static int nodeCreateLabel(int n_node, int iFace) {
	int label;
	label=(*(tarvosModel.node[n_node].nextLabel))[iFace];
	(*(tarvosModel.node[n_node].nextLabel))[iFace]++; //incrementa novo r�tulo dispon�vel; os n�meros de r�tulo s�o �nicos por interface
	return label;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Define o valor de timeout para controlar a morte de mensagens de controle do protocolo RSVP-TE para o nodo.
*  Uma rotina espec�fica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
void setNodeCtrlMsgTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].ctrlMsgTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Devolve o valor de timeout para controlar a morte de mensagens de controle do protocolo RSVP-TE para o nodo.
*  Uma rotina espec�fica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
double getNodeCtrlMsgTimeout(int n_node) {
	return tarvosModel.node[n_node].ctrlMsgTimeout;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Define o valor de timeout para controlar a morte de mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Uma rotina espec�fica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
void setNodeHelloMsgTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].helloMsgTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Devolve o valor de timeout para controlar a morte de mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Uma rotina espec�fica de timeout deve ser chamada periodicamente para eliminar as mensagens expiradas.
*/
double getNodeHelloMsgTimeout(int n_node) {
	return tarvosModel.node[n_node].helloMsgTimeout;
}

/* SETA VALOR DE TIMEOUT (RELATIVO) PARA RECEBIMENTO DE MENSAGEM DE CONTROLE HELLO PARA O NODO
*
*  Define o valor de timeout para controlar o limite de tempo pelo qual um nodo deve esperar a chegada de uma mensagem HELLO ou HELLO_ACK para
*  considerar um nodo alcan��vel (reachable).
*/
void setNodeHelloTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].helloTimeout=timeout;
}

/* RECUPERA VALOR DE TIMEOUT (RELATIVO) PARA RECEBIMENTO DE MENSAGEM DE CONTROLE HELLO PARA O NODO
*
*  Devolve o valor de timeout para controlar o limite de tempo pelo qual um nodo deve esperar a chegada de uma mensagem HELLO ou HELLO_ACK para
*  considerar um nodo alcan��vel (reachable).
*/
double getNodeHelloTimeout(int n_node) {
	return tarvosModel.node[n_node].helloTimeout;
}

/* SETA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Define o valor do evento para tratar mensagens de controle do protocolo RSVP-TE para o nodo.
*  Tipicamente � um evento de envio para as mensagens de controle criadas pelo nodo.
*/
void setNodeCtrlMsgHandlEv(int n_node, int ev) {
	tarvosModel.node[n_node].ctrlMsgHandlEv=ev;
}

/* RECUPERA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE RSVP-TE PARA O NODO
*
*  Devolve o valor do evento para tratar mensagens de controle do protocolo RSVP-TE para o nodo.
*  Tipicamente � um evento de envio para as mensagens de controle criadas pelo nodo.
*/
int getNodeCtrlMsgHandlEv(int n_node) {
	return tarvosModel.node[n_node].ctrlMsgHandlEv;
}

/* SETA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE HELLO PARA O NODO
*
*  Define o valor do evento para tratar mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Tipicamente � um evento de envio para as mensagens de controle criadas pelo nodo.
*/
void setNodeHelloMsgGenEv(int n_node, int ev) {
	tarvosModel.node[n_node].helloMsgGenEv=ev;
}

/* RECUPERA VALOR DO EVENTO PARA TRATAMENTO DE MENSAGENS DE CONTROLE HELLO RSVP-TE PARA O NODO
*
*  Devolve o valor do evento para tratar mensagens de controle HELLO do protocolo RSVP-TE para o nodo.
*  Tipicamente � um evento de envio para as mensagens de controle criadas pelo nodo.
*/
int getNodeHelloMsgGenEv(int n_node) {
	return tarvosModel.node[n_node].helloMsgGenEv;
}

/* SETA VALOR DO TIMEOUT (RELATIVO) PARA LSPs QUE PARTEM DO NODO
*
*  Define o valor do timeout relativo, em segundos, para a extin��o de LSPs que partem deste nodo.  Este valor deve ser recuperado por uma
*  fun��o espec�fica de controle de timeout a fim de calcular o tempo de rel�gio absoluto e introduzi-lo na LIB.
*/
void setNodeLSPTimeout(int n_node, double timeout) {
	tarvosModel.node[n_node].LSPtimeout=timeout;
}

/* RECUPERA VALOR DO TIMEOUT (RELATIVO) PARA LSPs QUE PARTEM DO NODO
*
*  Recupera o valor do timeout relativo, em segundos, para a extin��o de LSPs que partem deste nodo.  Este valor deve ser recuperado por uma
*  fun��o espec�fica de controle de timeout a fim de calcular o tempo de rel�gio absoluto e introduzi-lo na LIB.
*/
double getNodeLSPTimeout(int n_node) {
	return tarvosModel.node[n_node].LSPtimeout;
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_LABEL_REQUEST
*
*  A mensagem PATH_LABEL_REQUEST pede mapeamento de r�tulo e faz pr�-reserva de recursos para um LSP Tunnel.
*  Os casos a considerar s�o:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), ent�o este � o �ltimo destino da mensagem PATH.  Uma mensagem RESV_LABEL_MAPPING deve ser agora
*  criada, mas o mapeamento de r�tulo � feito pela fun��o de processamento da mensagem RESV_LABEL_MAPPING.  Assim, os dados da mensagem PATH_LABEL_REQUEST
*  s�o introduzidos na fila de mensagens de controle do nodo.  Criar rota expl�cita inversa e colocar na mensagem RESV.  O oLabel para a RESV ser� ZERO e
*  o oIface introduzido na fila de mensagens tamb�m ser� ZERO.  Nenhuma reserva de recursos precisa ser feita.
*
*  A mensagem PATH_LABEL_REQUEST sempre � introduzida na fila de mensagens de controle do nodo, desde o LER de ingresso at� o LER de egresso.
*
*  No segundo caso (recebido por um LSR gen�rico), as informa��es pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve ent�o seguir adiante.  Observar que, nesta implementa��o do TARVOS, uma mesma
*  mensagem PATH_LABEL_REQUEST percorre todo o caminho indicado pela rota expl�cita, desde o LER de ingresso at� o LER de egresso.  Na vida real, provavelmente
*  cada nodo criaria sua pr�pria mensagem de controle, com um msgID pr�prio.  A implementa��o presente simplifica o processo, aproveitando as fun��es j�
*  implementadas de tratamento de um pacote de dados comum.  Neste caso, as reservas de recursos devem ser feitas, portanto a fun��o espec�fica deve ser chamada.
*  Se os recursos foram reservados, a fun��o deve retornar sucesso; caso contr�rio, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.
*
*  A fun��o retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (ou n�o havia recursos para reservar ou rota n�o existe;
*  mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathLabelRequest(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	
	/*Caso 1:  recebido no LER de egresso; insira na fila do nodo e crie mensagem RESV_LABEL_MAPPING para a rota inversa.
			   N�o � preciso fazer reserva de recursos aqui, pois em tese a mensagem s� chegou aqui porque todos os links anteriores
			   aceitaram a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o pr�ximo item do vetor ER (que come�a de 0); portanto indica de fato o tamanho
																		 da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		
		/* O mapeamento de r�tulo e inser��o na LIB foram retirados daqui; somente a fun��o nodeProcessResvLabelMapping � que tratar� disso
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser criada agora
		iLabel=nodeCreateLabel(pkt->currentNode, pkt->outgoingLink);
		insertInLIB(pkt->currentNode, pkt->outgoingLink, iLabel, 0, 0, pkt->lblHdr.LSPid, "up", timeout, 0); //coloca ZERO no timeoutStamp
		*/

		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface � ZERO, pois trata-se do LER de egresso
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
		createResvMapControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, 0); //cria a mensagem RESV e tamb�m escalona evento de tratamento (contido no pacote PATH); o r�tulo � ZERO, pois � o LER de egresso
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH est� sendo recebido num nodo LSR gen�rico; recolha as informa��es pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas s� fa�a isso se houver recursos dispon�veis no link de sa�da para a LSP.*/
	} else {
		/*se currentNode == pr�ximo nodo da lista ER, ent�o entenda que o pacote est� no nodo de origem
		* e que o usu�rio come�ou sua lista expl�cita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a pr�xima.
		* (N�o faz nenhum sentido a origem e destino serem iguais.  Este teste permitir� ao usu�rio construir sua lista
		* de roteamento expl�cito por nodos, come�ando pelo nodo onde o pacote � gerado, ou pelo nodo imediatamente ap�s,
		* e ambas op��es ser�o aceitas.)
		* Se isto for resultado de alguma inconsist�ncia na lista ER, o funcionamento daqui para a frente � imprevis�vel.
		* (Rotina copiada da fun��o decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avan�a o n�mero nextIndex para a pr�xima posi��o (pr�ximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o pr�ximo nodo a percorrer, indicado por erNextIndex
		if (link!=0) {//se link==0, rota n�o foi encontrada; n�o insira nada na fila; o pacote dever� ser descartado em outra fun��o
			if (reserveResouces(pkt->lblHdr.LSPid, link)==1) { //recursos foram reservados; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, 1);
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a gera��o de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		}
		return 0; //FALHA:  houve problema na reserva de recurso ou rota (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_LABEL_MAPPING (working LSP)
*
*  A mensagem RESV_LABEL_MAPPING distribui o mapeamento de r�tulo e efetiva a reserva de recursos para o LSP Tunnel.
*  Os mapeamentos e reserva s�o feitos hop-by-hop, mas o LSP Tunnel s� estar� efetivado e v�lido quando a mensagem RESV_LABEL_MAPPING atingir o LER
*  de ingresso.  Caso isto n�o aconte�a, os recursos e r�tulos nos hops j� efetivados entrar�o em timeout, pois n�o haver� REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive LER de egresso), e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do r�tulo,\
*  buscando no pool de r�tulos dispon�veis na interface de entrada um valor v�lido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adi��o de apagar da mem�ria (com free) o
*  espa�o ocupado pelo vetor de rota expl�cita invertida, criado juntamente com a mensagem RESV, e marcar a entrada para o LSP tunnel, na LSP Table, como
*  completado.  (A fim de se fazer uma backup LSP, o working LSP precisa estar completado.)
*
*  Somente esta fun��o tem a prerrogativa de fazer mapeamento de r�tulo e inserir os dados pertinentes na LIB para a working LSP.
*/
static int nodeProcessResvLabelMapping(struct Packet *pkt) {
	int iLabel;
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR gen�rico; fazer o mapeamento de r�tulo, inserir dados na LIB e LSP Table; r�tulo inicial deve ser recuperado
	atrav�s de fun��o espec�fica.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem PATH correspondente; nada fa�a, neste caso.
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface);
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 0, timeout, 0); //coloca zero no timeoutStamp, zero para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (working) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o r�tulo agora criado na mensagem, para que o pr�ximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguir� adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//LSP foi criada; usu�rio deve usar a fun��o getWorkingLSPLabel para recuperar o r�tulo inicial
	//Caso 2:  recebida pelo LER de ingresso:  fa�a tudo acima, tamb�m remova a rota expl�cita inversa da mem�ria e marque o LSP tunnel como completo
		if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (�ltimo destino), ent�o libere o espa�o da vari�vel rota expl�cita inversa
			setLSPtunnelDone(pkt->lblHdr.LSPid);  //marque o LSP tunnel como totalmente completado
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
		}
		return 1; //retorne com SUCESSO
	} else
		return 0; //retorne com FALHA; nenhuma mensagem PATH correspondente encontrada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_REFRESH SEM MERGING (RFC 4090)
*
*  Para fins de simula��o, a mensagem PATH_REFRESH precisa conter a LSPid, msgID e ter o objeto recordRoute ativado, para que a rota seguida
*  seja gravada no pacote da mensagem de controle.  Nesta implementa��o, a mensagem PATH_REFRESH segue comutada por r�tulo, colocado na mensagem em sua
*  cria��o.
*  Os casos a considerar para processamento s�o:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), ent�o este � o �ltimo destino da mensagem PATH.  Uma mensagem RESV_REFRESH deve ser agora
*  criada, aqui mesmo na fun��o.  A rota a ser percorrida pela mensagem RESV_REFRESH deve ser recolhida do objeto recordRoute, apropriadamente invertida
*  (para que seja seguida no caminho inverso).  A mensagem � inclu�da na fila de mensagens de controle do nodo, para uso da fun��o de processamento da
*  RESV_REFRESH.
*
*  No segundo caso (recebido por um LSR gen�rico), as informa��es pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve ent�o seguir adiante.  Observar que, nesta implementa��o do TARVOS, uma mesma
*  mensagem PATH_REFRESH percorre todo o caminho indicado comutado por r�tulo, desde o LER de ingresso at� o LER de egresso.
*  Os recursos s� devem ser efetivamente renovados pela mensagem RESV_REFRESH.
*  No momento, n�o foram implementadas as mensagems PATH_ERR ou RESV_ERR.
*  A fun��o retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (rota n�o encontrada para o pr�ximo LSR);
*  mensagem deve ser descartada ou gerar erro).
*
*  Esta fun��o n�o trata merging de mensagens PATH_REFRESH.  Ou seja, ao chegar num Merge Poing, a mensagem PATH_REFRESH segue para seu destino, mesmo que
*  outra mensagem PATH_REFRESH, para a mesma LSPid (LSP tunnel), j� tenha prosseguido para o mesmo destino e path, dentro do tempo m�nimo.
*  Para tratar merging, usar a fun��o nodeProcessPathRefresh (RFC 4090, se��o 7.1.2.1).
*/
static int nodeProcessPathRefreshNoMerge(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; criar mensagem RESV_REFRESH para a rota inversa.
			   Se a mensagem chegou at� aqui, ent�o o PATH seguido por ela est� v�lido; emita RESV_REFRESH para renovar a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o pr�ximo item do vetor recordRoute (que come�a de 0); portanto indica de fato o tamanho
																			 da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface � ZERO, pois trata-se do LER de egresso
		
		createResvRefreshControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid); //cria a mensagem RESV e tamb�m escalona evento de tratamento (par�metros da simula��o)
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH est� sendo recebido num nodo LSR gen�rico; recolha as informa��es pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Os recursos reservados s� ser�o renovados pela mensagem RESV_REFRESH.*/
	} else {
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada � o conte�do de pkt->outgoingLink
		//se a tupla n�o for encontrada, significa rota inexistente; retornar com erro e o pacote ser� descartado na fun��o de encaminhamento
		
		if (p!=NULL) {//se p==NULL, rota n�o foi encontrada; n�o insira nada na fila; o pacote dever� ser descartado em outra fun��o
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, p->oIface, p->iLabel, p->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (link) (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
	}
	//se chegou at� aqui, houve algum problema no processamento da PATH_REFRESH
	return 0; //FALHA:  houve problema na rota (label n�o encontrada); a mensagem PATH_REFRESH dever� ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_REFRESH SEM MERGING (RFC 4090)
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR, e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada
*  procurar a entrada correspondente � chave nodo-iIface-iLabel-status "up" (estes dados s�o recuperados da mensagem armazenada na fila de mensagens de controle)
*  na tabela LIB e recalcular o timeout.  Se a entrada n�o for encontrada, nada fa�a.
*  Na realidade, na atual implementa��o, a mensagem guardada na fila de mensagens de controle n�o � usada para nada (a n�o ser para fazer a correspond�ncia para a RESV_REFRESH);
*  todos os dados necess�rios s�o recuperados do pacote RESV_REFRESH e de outras tabelas de controle.
*
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adi��o de apagar da mem�ria (com free) o
*  espa�o ocupado pelo vetor de rota expl�cita invertida, criado juntamente com a mensagem RESV.
*
*  Esta fun��o n�o trata merging de mensagens RESV_REFRESH.  Ou seja, ao chegar num Merge Poing, a mensagem RESV_REFRESH nunca cria outras mensagens RESV_REFRESH para backup LSPs
*  da mesma working LSP.  Para tratar merging, usar a fun��o nodeProcessResvRefresh (RFC 4090, se��o 7.1.2.1).
*/
static int nodeProcessResvRefreshNoMerge(struct Packet *pkt) {
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	struct LIBEntry *p;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR gen�rico; renove os recursos reservados para o peda�o de LSP que compete a este nodo (como origem) e encaminhe
	a mensagem � frente.  A renova��o � basicamente o rec�lculo do timeout na tabela LIB.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem PATH correspondente; nada fa�a, neste caso.
		//p = searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up");
		//busca entrada na LIB, usando como chave prim�ria a mensagem PATH correspondente anteriormente armazenada; tanto working LSP como backup LSPs funcionam aqui
		p = searchInLIBStatus(pkt->currentNode, msg->iIface, msg->iLabel, "up");
		if (p!=NULL) { //se p==NULL, entrada na LIB n�o foi encontrada; nada a renovar
			timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser renovada agora
			p->timeout = timeout; //coloque (atualize) o novo timeout da LSP no campo apropriado
			sprintf(mainTraceString, "Resources for LSP successfully refreshed.  LSPid:  %d  at node %d\n", msg->LSPid, pkt->currentNode);
			mainTrace(mainTraceString);
			removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
			//recursos foram renovados neste nodo

	/*Caso 2:  recebida pelo LER de ingresso:  fa�a tudo acima e tamb�m remova a rota expl�cita inversa da mem�ria.*/
			if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (�ltimo destino), ent�o libere o espa�o da vari�vel rota expl�cita inversa
				free(pkt->er.explicitRoute);
				pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
			}
			return 1;  //retorne com SUCESSO
		}
	}
	return 0; //se chegou at� aqui, retorne com FALHA; PATH correspondente ou entrada na LIB n�o encontrados
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_REFRESH
*
*  Para fins de simula��o, a mensagem PATH_REFRESH precisa conter a LSPid, msgID e ter o objeto recordRoute ativado, para que a rota seguida
*  seja gravada no pacote da mensagem de controle.  Nesta implementa��o, a mensagem PATH_REFRESH segue comutada por r�tulo, colocado na mensagem em sua
*  cria��o.
*  Os casos a considerar para processamento s�o:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), ent�o este � o �ltimo destino da mensagem PATH.  Uma mensagem RESV_REFRESH deve ser agora
*  criada, aqui mesmo na fun��o.  A rota a ser percorrida pela mensagem RESV_REFRESH deve ser recolhida do objeto recordRoute, apropriadamente invertida
*  (para que seja seguida no caminho inverso).  A mensagem � inclu�da na fila de mensagens de controle do nodo, para uso da fun��o de processamento da
*  RESV_REFRESH.
*
*  No segundo caso (recebido por um LSR gen�rico), as informa��es pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve ent�o seguir adiante.  Observar que, nesta implementa��o do TARVOS, uma mesma
*  mensagem PATH_REFRESH percorre todo o caminho indicado comutado por r�tulo, desde o LER de ingresso at� o LER de egresso.
*  Os recursos s� devem ser efetivamente renovados pela mensagem RESV_REFRESH.
*  A fun��o retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (rota n�o encontrada para o pr�ximo LSR);
*  mensagem deve ser descartada ou gerar erro).
*
*  No momento, n�o foram implementadas as mensagems PATH_ERR ou RESV_ERR, tampouco o MERGING (RFC 4090, se��o 7.1.2.1).
*/
static int nodeProcessPathRefresh(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; criar mensagem RESV_REFRESH para a rota inversa.
			   Se a mensagem chegou at� aqui, ent�o o PATH seguido por ela est� v�lido; emita RESV_REFRESH para renovar a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o pr�ximo item do vetor recordRoute (que come�a de 0); portanto indica de fato o tamanho
																			 da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface � ZERO, pois trata-se do LER de egresso
		
		createResvRefreshControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid); //cria a mensagem RESV e tamb�m escalona evento de tratamento (par�metros da simula��o)
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH est� sendo recebido num nodo LSR gen�rico; recolha as informa��es pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Os recursos reservados s� ser�o renovados pela mensagem RESV_REFRESH.*/
	} else {
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada � o conte�do de pkt->outgoingLink
		//se a tupla n�o for encontrada, significa rota inexistente; retornar com erro e o pacote ser� descartado na fun��o de encaminhamento
		
		if (p!=NULL) {//se p==NULL, rota n�o foi encontrada; n�o insira nada na fila; o pacote dever� ser descartado em outra fun��o
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, p->oIface, p->iLabel, p->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (link) (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
	}
	//se chegou at� aqui, houve algum problema no processamento da PATH_REFRESH
	return 0; //FALHA:  houve problema na rota (label n�o encontrada); a mensagem PATH_REFRESH dever� ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_REFRESH
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR, e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada
*  procurar a entrada correspondente � chave nodo-iIface-iLabel-status "up" (estes dados s�o recuperados da mensagem armazenada na fila de mensagens de controle)
*  na tabela LIB e recalcular o timeout.  Se a entrada n�o for encontrada, nada fa�a.
*  todos os dados necess�rios s�o recuperados do pacote RESV_REFRESH e de outras tabelas de controle.
*
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adi��o de apagar da mem�ria (com free) o
*  espa�o ocupado pelo vetor de rota expl�cita invertida, criado juntamente com a mensagem RESV.
*
*  Nesta implementa��o, ainda n�o foi implementado o MERGING (RFC 4090, se��o 7.1.2.1).
*/
static int nodeProcessResvRefresh(struct Packet *pkt) {
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	struct LIBEntry *p;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR gen�rico; renove os recursos reservados para o peda�o de LSP que compete a este nodo (como origem) e encaminhe
	a mensagem � frente.  A renova��o � basicamente o rec�lculo do timeout na tabela LIB.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem PATH correspondente; nada fa�a, neste caso.
		//p = searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up");
		//busca entrada na LIB, usando como chave prim�ria a mensagem PATH correspondente anteriormente armazenada; tanto working LSP como backup LSPs funcionam aqui
		p = searchInLIBStatus(pkt->currentNode, msg->iIface, msg->iLabel, "up");
		if (p!=NULL) { //se p==NULL, entrada na LIB n�o foi encontrada; nada a renovar
			timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser renovada agora
			p->timeout = timeout; //coloque (atualize) o novo timeout da LSP no campo apropriado
			sprintf(mainTraceString, "Resources for LSP successfully refreshed.  LSPid:  %d  at node %d\n", msg->LSPid, pkt->currentNode);
			mainTrace(mainTraceString);
			removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
			//recursos foram renovados neste nodo

	/*Caso 2:  recebida pelo LER de ingresso:  fa�a tudo acima e tamb�m remova a rota expl�cita inversa da mem�ria.*/
			if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (�ltimo destino), ent�o libere o espa�o da vari�vel rota expl�cita inversa
				free(pkt->er.explicitRoute);
				pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
			}
			return 1;  //retorne com SUCESSO
		}
	}
	return 0; //se chegou at� aqui, retorne com FALHA; PATH correspondente ou entrada na LIB n�o encontrados
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO HELLO
*
*  As mensagens de controle do RSVP-TE do tipo HELLO s�o processadas aqui.
*  Tecnicamente, as mensagens HELLO s�o enviadas somente para os nodos vizinhos, portanto a uma dist�ncia m�xima de 1 hop.  Assim, ela seria recebida
*  somente pelo destino e n�o passaria por nenhum LSR intermedi�rio.  Devido � filosofia de implementa��o do simulador, e tamb�m prevendo uma mensagem
*  HELLO que trafegaria por mais de um LSR, programou-se aqui dois casos de processamento.
*  No CASO 1, a mensagem � recebida por um LSR intermedi�rio, n�o destino.  A mensagem � gravada na fila de mensagens de controle do nodo e encaminhada
*  adiante (comutada por r�tulo).
*  No CASO 2, a mensagem � recebida pelo LSR destino.  O processo para o caso 1 � repetido e dispara-se imediatamente a mensagem
*  HELLO_ACK com destino � origem daquela mensagem HELLO recebida, usando o objeto RecordRoute do pacote HELLO original.
*/
static int nodeProcessHello(struct Packet *pkt) {
	int *revEr;  //revEr:  reverse explicit route
	double timeout; //para c�lculo do tempo absoluto de timeout de uma mensagem HELLO
	
	/*Caso 1:  recebido no LSR de destino; LSP � alcan��vel:  criar mensagem HELLO_ACK para a rota inversa.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.recordRoute, pkt->er.rrNextIndex); /*rrNextIndex indica o pr�ximo item do vetor recordRoute (que come�a de 0); portanto indica de fato o tamanho
																			 da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		timeout=simtime() + getNodeHelloMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem HELLO
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface � ZERO, pois � irrelevante aqui
		createHelloAckControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID); //cria a mensagem RESV e tamb�m escalona evento de tratamento (par�metros da simula��o)
		free(pkt->er.explicitRoute); //libera rota expl�cita criada temporariamente
		pkt->er.explicitRoute=NULL; //evita erros em novas opera��es com free neste campo (em outras fun��es)
		return 1; //SUCESSO:  mensagem foi processada completamente

	/*Caso 2:  HELLO recebido num LSR intermedi�rio; recolha as informa��es pertinentes e insira na fila do nodo, e deixe a mensagem ser encaminhada adiante.*/
	} else {	
		timeout=simtime() + getNodeHelloMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem HELLO
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //oIface � irrelevante
		//pkt->outgoingLink cont�m o link ou interface pela qual o pacote est� chegando; se for ZERO, significa que o pacote foi gerado no nodo
		return 1; //SUCESSO:  mensagem foi processada completamente
	}
	return 0; //FALHA:  houve algum problema; a mensagem HELLO dever� ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO HELLO_ACK
**  
*  Ao receber uma HELLO_ACK, o nodo verifica se existe uma HELLO com mesmo msgIDack na fila de mensagens de controle do nodo.  Se houver, ent�o o valor
*  do helloTimeLimit no tarvosModel.node � atualizado para mais um intervalo de detec��o de falha (3,5 * helloInterval, por default).  Caso uma mensagem
*  HELLO com mesmo msgIDack n�o seja encontrada, ent�o a HELLO_ACK � descartada e o helloTimeLimit n�o � atualizado.
*
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida pelo LSR destino, e recebida por um LSR intermedi�rio.
*  No caso 1 (recebida pelo LSR destino), procurar a mensagem HELLO correspondente na fila de mensagens de controle do nodo, retir�-la se achada, e renovar o helloTimeLimit no
*  nodo origem.  Se a mensagem HELLO n�o for achada na fila, descartar a HELLO_ACK e nada mais fazer (verificar esse comportamento na RFC 3209).
*
*  No caso 2 (recebida por um LSR intermedi�rio), deve-se buscar a mensagem HELLO correspondente na fila de mensagens de controle do nodo.  Se esta mensagem for encontrada,
*  retir�-la da fila.  Se a entrada n�o for encontrada, nada fa�a.  Em ambas situa��es, encaminhar a mensagem adiante para seu destino.
*  Na realidade, na atual implementa��o, a mensagem guardada na fila de mensagens de controle n�o � usada para nada, a n�o ser para fazer o casamento com sua ACK;
*  todos os dados necess�rios s�o recuperados do pacote HELLO_ACK e de outras tabelas de controle.
*
*  O espa�o ocupado pelo objeto rota expl�cita deve ser apagado com free, pois este foi criado juntamente com a mensagem HELLO_ACK.
*/
static int nodeProcessHelloAck(struct Packet *pkt) {
	double helloTimeLimit; //para c�lculo do tempo absoluto de helloTimeLimit (para considerar nodo alcan��vel)
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	//procure a mensagem HELLO correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem HELLO correspondente; nada fa�a, neste caso.
		
		/*Caso 1:  recebida pelo LSR de destino:  recalcule o helloTimeLimit e tamb�m remova a rota expl�cita inversa da mem�ria.*/
		if (pkt->currentNode == pkt->dst) {
			helloTimeLimit=simtime()+getNodeHelloTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o de uma conex�o v�lida entre dois nodos
			(*(tarvosModel.node[pkt->currentNode].helloTimeLimit))[findLink(msg->src, msg->dst)] = helloTimeLimit; //coloque (atualize) o novo timeout da conex�o entre os nodos src e dst
			sprintf(mainTraceString, "Node found reachable by HELLO.  src:  %d  dst:  %d  at node %d.  Next timeout:  %f\n", msg->src, msg->dst, pkt->currentNode, helloTimeLimit);
			mainTrace(mainTraceString);
			free(pkt->er.explicitRoute);  //elimine rota expl�cita, criada temporariamente juntamente com a HELLO_ACK
			pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
		}
		/*Caso 2:  recebido no LSR intermedi�rio; retire a HELLO na fila de mensagens e encaminhe o HELLO_ACK adiante.*/
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		return 1;  //retorne com SUCESSO
	}
	return 0; //retorne com FALHA, pois mensagem correspondente HELLO n�o foi encontrada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_DETOUR
*
*  A mensagem PATH_DETOUR implanta uma backup LSP a partir de um Merge Point de ingresso at� um Merge Point de egresso.
*  Os Merge Points (MPs) encontram a working LSP, que j� deve existir previamente.
*  Os casos a considerar s�o:  PATH recebido pelo MP de egresso e recebido por um LSR qualquer.
*
*  No primeiro caso (recebido por um MP de egresso), ent�o este � o �ltimo destino da mensagem PATH.  A partir daqui, o tunnel dever� unir-se ao working LSP tunnel.
*  Para isto, a fun��o buscar� o oLabel e oIface existentes para o working LSP e copiar� estes valores para o backup LSP.
*  Uma mensagem RESV_DETOUR_MAPPING deve ser agora criada, mas o mapeamento de r�tulo � feito pela fun��o de processamento da mensagem RESV_DETOUR_MAPPING.
*  Assim, os dados da mensagem PATH_DETOUR s�o introduzidos na fila de mensagens de controle do nodo.
*  Criar rota expl�cita inversa e colocar na mensagem RESV.  O oLabel e oIface para a RESV ser�o copiados da entrada j� existente para working LSP e
*  introduzidos na fila de mensagens.  Nenhuma reserva de recursos precisa ser feita, pois a reserva ser� compartilhada com a working LSP.
*  Se uma working LSP n�o for encontrada, de forma a causar o merging com o MP de egresso, ent�o a fun��o deve retornar com erro (e assim a PATH dever�
*  ser descartada, ou ent�o uma PATH_ERR gerada).
*
*  A mensagem PATH_DETOUR sempre � introduzida na fila de mensagens de controle do nodo, desde o MP de ingresso at� o MP de egresso.
*
*  No segundo caso (recebido por um LSR qualquer), as informa��es pertinentes da mensagem devem ser recolhidas e inseridas na fila de mensagens de controle
*  do nodo (como interface de entrada, msgID, LSPid, etc.) e a mensagem deve ent�o seguir adiante.  Observar que, nesta implementa��o do TARVOS, uma mesma
*  mensagem PATH_DETOUR percorre todo o caminho indicado pela rota expl�cita, desde o MP de ingresso at� o MP de egresso.  Na vida real, provavelmente
*  cada nodo criaria sua pr�pria mensagem de controle, com um msgID pr�prio.  A implementa��o presente simplifica o processo, aproveitando as fun��es j�
*  implementadas de tratamento de um pacote de dados comum.
*  Uma busca � feita na LIB a fim de verificar se a working LSP, para o nodo em quest�o, tem o mesmo link de sa�da (oIface) que a backup LSP.  Se o link
*  de sa�da for o mesmo, ent�o uma nova reserva de recursos n�o ser� feita; os recursos ser�o compartilhados pela working e backup LSP (sharing).  Nesta
*  implementa��o do TARVOS, ainda n�o se contempla recursos n�o compartilhados para um mesmo link de sa�da ou ainda modifica��o da reserva de recursos.
*  Caso a working e backup LSP n�o tenham o mesmo link (oIface) de sa�da, ent�o as reservas de recursos devem ser feitas, portanto a fun��o espec�fica deve ser chamada.
*  Se os recursos foram reservados, a fun��o deve retornar sucesso; caso contr�rio, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.
*
*  A fun��o retorna 1 para SUCESSO (pacote foi processado completamente), e retorna 0 para FALHA (ou n�o havia recursos para reservar ou rota n�o existe ou
*  n�o h� working LSP para merging; mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathDetour(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct LIBEntry *plib;
	
	/*Caso 1:  recebido no MP de egresso; insira na fila do nodo e crie mensagem RESV_DETOUR_MAPPING para a rota inversa.
			   � preciso fazer uma busca para a entrada da working LSP na LIB, a fim de copiar o oIface e oLabel.  N�o � preciso fazer
			   reserva de recursos aqui, pois haver� o sharing com os recursos j� reservados para a working LSP.  Caso a working LSP n�o
			   seja encontrada, retornar com FALHA, pois compreende-se aqui que a backup LSP necessariamente termina num Merge Point.*/
	if (pkt->currentNode == pkt->dst) {
		plib=searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up"); //busca pela entrada na LIB correspondente � working LSP
		if (plib==NULL) //nenhuma entrada para working LSP foi encontrada; retorne com FALHA.  � preciso haver um Merge Point no final da backup LSP
			return 0; //FALHA

		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o pr�ximo item do vetor ER (que come�a de 0); portanto indica de fato o tamanho
																			   da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, plib->oIface, plib->iLabel, plib->oLabel, 0); //a oIface e oLabel s�o os mesmos para a working LSP
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
		createResvDetourControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, plib->oLabel); /*cria a mensagem RESV (a fun��o tamb�m escalona evento de tratamento)
																													oLabel � copiado da entrada na LIB para a working LSP.*/
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH est� sendo recebido num nodo LSR gen�rico; recolha as informa��es pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas s� fa�a isso se houver recursos dispon�veis no link de sa�da para a LSP.  Se o link de sa�da (oIface) for
	o mesmo da working LSP, ent�o n�o fa�a nova reserva de recursos, pois estes ser�o compartilhados pela backup e working LSP.*/
	} else {
		/*se currentNode == pr�ximo nodo da lista ER, ent�o entenda que o pacote est� no nodo de origem
		* e que o usu�rio come�ou sua lista expl�cita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a pr�xima.
		* (N�o faz nenhum sentido a origem e destino serem iguais.  Este teste permitir� ao usu�rio construir sua lista
		* de roteamento expl�cito por nodos, come�ando pelo nodo onde o pacote � gerado, ou pelo nodo imediatamente ap�s,
		* e ambas op��es ser�o aceitas.)
		* Se isto for resultado de alguma inconsist�ncia na lista ER, o funcionamento daqui para a frente � imprevis�vel.
		* (Rotina copiada da fun��o decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avan�a o n�mero nextIndex para a pr�xima posi��o (pr�ximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o pr�ximo nodo a percorrer, indicado por erNextIndex
		if (link==0) //n�o achou rota; n�o insira nada na fila; o pacote dever� ser descartado em outra fun��o
			return 0; //FALHA:  rota n�o encontrada
	
		plib=searchInLIBnodLSPstatoIface(pkt->currentNode, pkt->lblHdr.LSPid, "up", link); //busca pela entrada na LIB correspondente � working LSP e para o mesmo link (oIface) de sa�da
		if (plib==NULL) { //n�o achou a entrada; ent�o, tente a reserva de recursos, pois n�o h� compartilhamento a fazer
			//tentar reserva de recursos
			if (reserveResouces(pkt->lblHdr.LSPid, link)==1) { //recursos foram reservados; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, 1);
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a gera��o de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		} else { //entrada na LIB para uma working LSP com mesmo link (oIface) de sa�da foi encontrada; n�o fa�a nova reserva de recurso -> RESOURCE SHARING
			//resource sharing:  n�o reservar recursos, encaminhe a mensagem adiante
			timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
			insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
				pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, plib->iLabel, plib->oLabel, 0);
			//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
			return 1; //SUCESSO:  mensagem foi processada completamente
		}
		return 0; //FALHA:  houve problema na reserva de recurso (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_DETOUR_MAPPING (backup LSP)
*
*  A mensagem RESV_DETOUR_MAPPING distribui o mapeamento de r�tulo para backup LSP e efetiva a reserva de recursos para o LSP Tunnel (onde n�o houver
*  compartilhamento).
*  Os mapeamentos e reserva s�o feitos hop-by-hop, mas o LSP Tunnel s� estar� efetivado e v�lido quando a mensagem RESV_DETOUR_MAPPING atingir o Merge Point (MP)
*  de ingresso.  Caso isto n�o aconte�a, os recursos e r�tulos nos hops j� efetivados entrar�o em timeout, pois n�o haver� REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive MP de egresso), e recebida por um MP de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do r�tulo,\
*  buscando no pool de r�tulos dispon�veis na interface de entrada um valor v�lido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  Quando recebida pelo MP de egresso, dados para a entrada na LIB s�o coletados do pacote e da mensagem armazenada na fila de mensagens de controle.
*  O oIface e oLabel s�o copiados da working LSP existente.  O iLabel tamb�m deve ser gerado a partir do pool de r�tulos dispon�veis.
*  No caso 2 (recebida por um MP de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adi��o de apagar da mem�ria (com free) o
*  espa�o ocupado pelo vetor de rota expl�cita invertida, criado juntamente com a mensagem RESV.  Um iLabel tamb�m � criado neste caso.  Este iLabel ser�
*  perdido, quando for substitu�do pelo iLabel da working LSP no momento do Rapid Recovery.
*
*  Somente esta fun��o tem a prerrogativa de fazer mapeamento de r�tulo e inserir os dados pertinentes na LIB para a backup LSP.
*/
static int nodeProcessResvDetourMapping(struct Packet *pkt) {
	int iLabel;
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR gen�rico; fazer o mapeamento de r�tulo, inserir dados na LIB e LSP Table; r�tulo inicial deve ser recuperado
	atrav�s de fun��o espec�fica.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem PATH correspondente; nada fa�a, neste caso.
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface); //cria iLabel a partir do pool de r�tulos dispon�veis
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 1, timeout, 0); //coloca zero no timeoutStamp, 1 para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (backup) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o r�tulo agora criado na mensagem, para que o pr�ximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguir� adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//backup LSP foi criada; o r�tulo inicial � o mesmo que a working LSP de mesmo LSPid
	//Caso 2:  recebida pelo MP de ingresso:  fa�a tudo acima e tamb�m remova a rota expl�cita inversa da mem�ria
		if (pkt->currentNode == pkt->dst) { //se este for o MP de ingresso (�ltimo destino), ent�o libere o espa�o da vari�vel rota expl�cita inversa
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
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
		free(pkt->er.explicitRoute); //remover rota expl�cita
		pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  RESV est� sendo recebido num nodo LSR gen�rico; deixe a mensagem ser encaminhada adiante.*/
	} else {
		p=searchInLIBnodLSPstat(pkt->currentNode, pkt->lblHdr.LSPid, "up"); //busca a entrada na LIB para o nodo atual (working LSP)
		//se a tupla n�o for encontrada, significa rota inexistente; retornar com erro e o pacote ser� descartado na fun��o de encaminhamento
		if (p!=NULL && p->iIface!=0) {
			if (pkt->er.explicitRoute==NULL) {//criar� espa�o para rota expl�cita, se ainda n�o foi criado
				pkt->er.explicitRoute = (int*)malloc(sizeof (*(pkt->er.explicitRoute))); //cria espa�o para um int
				if (pkt->er.explicitRoute == NULL) {
					printf("\nError - nodeProcessPathErr - insufficient memory to allocate for explicit route object");
					exit (1);
				}
			}
			/*coloca o nodo destino na rota expl�cita; isto ter� de ser removido com free antes da remo��o do pacote;
			observar que o pacote trafega upstream; portanto, o nodo destino dever� ser o nodo src do link = iIface*/
			*pkt->er.explicitRoute = tarvosModel.lnk[p->iIface].src;
			pkt->er.erNextIndex = 0; //assegura que �ndice aponta para in�cio da rota expl�cita
			pkt->lblHdr.label = 0; //assegura que n�o h� label v�lido
			return 1; //SUCESSO:  mensagem foi processada completamente
		} else
			return 0; //FALHA:  rota n�o encontrada ou problemas na rota upstream
	}
	//se chegou at� aqui, houve algum problema no processamento da RESV_ERR
	return 0; //FALHA:  a mensagem RESV_ERR dever� ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_ERR
*
*  A mensagem RESV_ERR segue no sentido downstream, para o LER de egresso.  Dois casos s�o v�lidos para processamento.
*
*  Caso 1:  Recebido pelo LER de egresso.  Neste caso, as a��es desejadas pelo usu�rio devem ser tomadas (retorno de recursos reservados, etc.).
*
*  Caso 2:  Recebido por um LSR gen�rico.  Tomar alguma a��o desejada e encaminh�-la adiante (seguir� comutada por r�tulo.
*/
static int nodeProcessResvErr(struct Packet *pkt) {
	struct LIBEntry *p;
	
	/*Caso 1:  recebido no LER de egresso; tomar atitude desejada.*/
	if (pkt->currentNode == pkt->dst) {
		//processamentos desejados aqui
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  RESV est� sendo recebido num nodo LSR gen�rico; deixe a mensagem ser encaminhada adiante.*/
	} else {
		//a��es desejadas aqui
		
		p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada � o conte�do de pkt->outgoingLink
		//se a tupla n�o for encontrada, significa rota inexistente; retornar com erro e o pacote ser� descartado na fun��o de encaminhamento
		if (p!=NULL)
			return 1; //SUCESSO:  mensagem foi processada completamente
		else
			return 0; //FALHA:  rota n�o encontrada
	}
	//se chegou at� aqui, houve algum problema no processamento da RESV_ERR
	return 0; //FALHA:  a mensagem RESV_ERR dever� ser descartada
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO PATH_LABEL_REQUEST_PREEMPT
*
*  A mensagem PATH_LABEL_REQUEST pede mapeamento de r�tulo e faz pr�-reserva de recursos para um LSP Tunnel, caso haja recursos dispon�veis.
*  Se n�o estiverem dispon�veis, outras LSPs existentes no link de sa�da podem sofrer preemp��o para liberar os recursos necess�rios.
*  Os casos a considerar s�o:  PATH recebido por um LER de egresso e recebido por um LSR.
*  No primeiro caso (recebido por um LER de egresso), ent�o este � o �ltimo destino da mensagem PATH.  Uma mensagem RESV_LABEL_MAPPING_PREEMPT deve ser agora
*  criada, mas o mapeamento de r�tulo � feito pela fun��o de processamento da mensagem RESV_LABEL_MAPPING_PREEMPT.  Assim, os dados da mensagem PATH_LABEL_REQUEST_PREEMPT
*  s�o introduzidos na fila de mensagens de controle do nodo.  Criar rota expl�cita inversa e colocar na mensagem RESV.  O oLabel para a RESV ser� ZERO e
*  o oIface introduzido na fila de mensagens tamb�m ser� ZERO.  Nenhuma reserva de recursos precisa ser feita para o LER de egresso.
*
*  A mensagem PATH_LABEL_REQUEST sempre � introduzida na fila de mensagens de controle do nodo, desde o LER de ingresso at� o LER de egresso.
*
*  No segundo caso (recebido por um LSR gen�rico), os recursos s� ser�o pr�-reservados se estiverem dispon�veis.  Se n�o estiverem, far-se-� uma varredura na LIB
*  para o nodo em quest�o, buscando LSPs estabelecidas no nodo para o link de sa�da.  Somente ser�o consideradas as LSPs com Holding Priority menor que a Setup Priority
*  da LSP corrente (0 � a maior prioridade, 7 a menor).  Para cada uma encontrada, os recursos ser�o somados para um acumulador.  Se o valor do acumulador for suficiente
*  para os requerimentos da LSP corrente, ent�o a varredura termina e a mensagem segue adiante.  N�o � feita nenhuma pr�-reserva.  Caso a varredura termine e os recursos
*  continuarem insuficientes, mesmo com preemp��o, ent�o a mensagem n�o segue adiante.
*  Observar que, nesta implementa��o do TARVOS, uma mesma mensagem PATH_LABEL_REQUEST_PREEMPT percorre todo o caminho indicado pela rota expl�cita, desde o LER de ingresso
*  at� o LER de egresso.
*  Se os recursos foram reservados e a mensagem deve seguir adiante, o campo espec�fico na fila de mensagens de controle � marcado e a fun��o retorna 1.
*  Se os recursos n�o foram reservados, mas a mensagem deve seguir adiante (a preemp��o pode ser bem sucedida), a fun��o deve retornar sucesso (1).
*  Caso contr�rio, deve retornar falha para sinalizar que a mensagem PATH deve ser descartada.  A preemp��o s� � efetivamente realizada no processamento da mensagem
*  RESV_LABEL_MAPPING_PREEMPT, que pode encontrar uma outra realidade de recursos dispon�veis no nodo.
*
*  A fun��o retorna 1 para sucesso (pacote foi processado completamente), e retorna 0 para falha (ou n�o havia recursos para reservar ou rota n�o existe;
*  mensagem deve ser descartada ou gerar erro).
*/
static int nodeProcessPathPreempt(struct Packet *pkt) {
	int *revEr, link, erNextIndex;  //revEr:  reverse explicit route
	int preempt=0; //flag que indica se preemp��o ser� poss�vel
	int resv=0; //retorno da fun��o reserveResource
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP

	/*Caso 1:  recebido no LER de egresso; insira na fila do nodo e crie mensagem RESV_LABEL_MAPPING_PREEMPT para a rota inversa.
			   N�o � preciso fazer reserva de recursos aqui, pois em tese a mensagem s� chegou aqui porque todos os links anteriores
			   aceitaram a reserva de recursos.*/
	if (pkt->currentNode == pkt->dst) {
		revEr=invertExplicitRoute(pkt->er.explicitRoute, pkt->er.erNextIndex); /*erNextIndex indica o pr�ximo item do vetor ER (que come�a de 0); portanto indica de fato o tamanho
																		 da rota expl�cita (todos os nodos at� aqui percorridos) at� o nodo atual.  Free() esta rota ap�s uso!*/
		
		/* O mapeamento de r�tulo e inser��o na LIB foram retirados daqui; somente a fun��o nodeProcessResvLabelMapping � que tratar� disso
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser criada agora
		iLabel=nodeCreateLabel(pkt->currentNode, pkt->outgoingLink);
		insertInLIB(pkt->currentNode, pkt->outgoingLink, iLabel, 0, 0, pkt->lblHdr.LSPid, "up", timeout, 0); //coloca ZERO no timeoutStamp
		*/

		timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
		insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
			pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, 0, pkt->lblHdr.label, 0, 0); //a oIface � ZERO, pois trata-se do LER de egresso
		//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
		createResvPreemptControlMsg(pkt->dst, pkt->src, revEr, pkt->lblHdr.msgID, pkt->lblHdr.LSPid, 0); //cria a mensagem RESV e tamb�m escalona evento de tratamento (contido no pacote PATH); o r�tulo � ZERO, pois � o LER de egresso
		return 1; //SUCESSO:  mensagem foi processada completamente
	
	/*Caso 2:  PATH est� sendo recebido num nodo LSR gen�rico; recolha as informa��es pertinentes e insira na fila do nodo, e deixe
	a mensagem ser encaminhada adiante.  Mas s� fa�a isso se houver recursos dispon�veis no link de sa�da para a LSP, com ou sem preemp��o.
	Se estiverem imediatamente dispon�veis, fa�a a pr�-reserva.*/
	} else {
		/*se currentNode == pr�ximo nodo da lista ER, ent�o entenda que o pacote est� no nodo de origem
		* e que o usu�rio come�ou sua lista expl�cita ER com este mesmo nodo de origem.  Assim, desconsidere esta
		* primeira entrada na lista ER e pegue a pr�xima.
		* (N�o faz nenhum sentido a origem e destino serem iguais.  Este teste permitir� ao usu�rio construir sua lista
		* de roteamento expl�cito por nodos, come�ando pelo nodo onde o pacote � gerado, ou pelo nodo imediatamente ap�s,
		* e ambas op��es ser�o aceitas.)
		* Se isto for resultado de alguma inconsist�ncia na lista ER, o funcionamento daqui para a frente � imprevis�vel.
		* (Rotina copiada da fun��o decidePathER)
		*/
		erNextIndex=pkt->er.erNextIndex;
		if(pkt->currentNode==*(pkt->er.explicitRoute + erNextIndex))
			erNextIndex++; //avan�a o n�mero nextIndex para a pr�xima posi��o (pr�ximo nodo a ser atingido)
		
		link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + erNextIndex)); //encontra o link que conecta currentNode e o pr�ximo nodo a percorrer, indicado por erNextIndex
		if (link!=0) {//se link==0, rota n�o foi encontrada; n�o insira nada na fila; o pacote dever� ser descartado em outra fun��o
			resv=reserveResouces(pkt->lblHdr.LSPid, link);
			if (resv==0) { //recursos n�o est�o imediatamente dispon�veis; fa�a varredura p/ verificar se com preemp��o, estar�o dispon�veis; mas n�o reserve nada agora!
				preempt=testResources(pkt->lblHdr.LSPid, pkt->currentNode, link);
			}

			if (resv==1 || preempt==1) { //recursos foram reservados ou podem ser reservados com preemp��o; prossiga
				timeout=simtime() + getNodeCtrlMsgTimeout(pkt->currentNode); //calcular tempo absoluto de expira��o da mensagem PATH
				insertInNodeMsgQueue(pkt->currentNode, pkt->lblHdr.msgType, pkt->lblHdr.msgID, pkt->lblHdr.msgIDack, pkt->lblHdr.LSPid, pkt->er.explicitRoute,
					pkt->er.erNextIndex, pkt->src, pkt->dst, timeout, getNodeCtrlMsgHandlEv(pkt->currentNode), pkt->outgoingLink, link, pkt->lblHdr.label, 0, resv); //resv aqui indicar� se recursos foram pr�-reservados, ou n�o
				//Obs.:  pkt->outgoingLink indica o iIface (pacote acabou de chegar); se o pacote est� chegando no nodo para onde foi gerado (o primeiro do Path), ent�o este n�mero ser� zero
				return 1; //SUCESSO:  mensagem foi processada completamente
			} //else:  a gera��o de uma mensagem PATH_ERR (PathErr) deve ser ativada aqui para falha de reserva de recurso
		}
		return 0; //FALHA:  houve problema na reserva de recurso ou rota (uma PATH_ERR deve ser gerada aqui para uma falha)
	}
}

/* PROCESSA MENSAGEM DE CONTROLE DO TIPO RESV_LABEL_MAPPING_PREEMPT (working LSP)
*
*  A mensagem RESV_LABEL_MAPPING_PREEMPT distribui o mapeamento de r�tulo e efetiva a reserva de recursos para o LSP Tunnel, usando tamb�m de preemp��o
*  de LSPs existentes caso necess�rio.
*  Os mapeamentos e reserva s�o feitos hop-by-hop, mas o LSP Tunnel s� estar� efetivado e v�lido quando a mensagem RESV_LABEL_MAPPING_PREEMPT atingir o LER
*  de ingresso.  Caso isto n�o aconte�a, os recursos e r�tulos nos hops j� efetivados entrar�o em timeout, pois n�o haver� REFRESH.
*  Dois casos precisam ser tratados no recebimento desta mensagem:  recebida por um LSR (inclusive LER de egresso), e recebida por um LER de ingresso.
*  No caso 1 (recebida por um LSR), deve-se buscar a mensagem PATH correspondente na fila de mensagens de controle do nodo, fazer o mapeamento do r�tulo,\
*  buscando no pool de r�tulos dispon�veis na interface de entrada um valor v�lido.  Insere-se a entrada correspondente na LIB, remove-se a mensagem
*  da fila de mensagens de controle do nodo.
*  O processamento verificar�, tamb�m, se o campo resourcesReserved est� marcado, o que indica que recursos j� foram pr�-reservados pela PATH_PREEMPT.
*  Se o foram, nada mais � feito.  Se n�o o foram, verificar-se-� se existem recursos suficientes para a reserva.  Se houver, faz a reserva e a mensagem
*  segue adiante (a reserva pode n�o ter sido feita pela PATH pois, no momento, os recursos n�o eram suficientes).
*  Se n�o os recursos n�o forem imediatamente suficientes, ent�o far-se-� uma busca na LIB para LSPs pr�-existentes no nodo-oIface.
*  Uma lista auxiliar (LSPList) ser� criada, por ordem decrescente de Holding Priority, somente com aquelas LSPs de Holding Priority menor que a Setup Priority
*  da LSP corrente (0 � a maior prioridade, 7 a menor).
*  Para cada LSP de menor prioridade, os recursos desta ser�o acumulados, at� que os acumuladores acusem valores suficientes para a LSP corrente.  Se isto acontecer,
*  ent�o a lista auxiliar � percorrida, e cada LSP nesta lista � colocada em estado "preempted" e seus recursos, devolvidos ao link oIface.  Isso se repete at�
*  que os recursos acumulados sejam suficientes para a LSP corrente.  Ent�o, uma nova reserva para a LSP corrente � feita e a mensagem segue adiante.
*  Se, mesmo com preemp��o, n�o houver recursos necess�rios para a LSP corrente, a mensagem dever� ser descartada.
*  No caso 2 (recebida por um LER de ingresso), exatamente o mesmo procedimento acima deve ser tomado, com a adi��o de apagar da mem�ria (com free) o
*  espa�o ocupado pelo vetor de rota expl�cita invertida, criado juntamente com a mensagem RESV, e marcar a entrada para o LSP tunnel, na LSP Table, como
*  completado.  (A fim de se fazer uma backup LSP, o working LSP precisa estar completado.)
*
*  Somente esta fun��o tem a prerrogativa de fazer mapeamento de r�tulo e inserir os dados pertinentes na LIB (popular a LIB) para a working LSP, com preemp��o.
*
*  A LSPList � uma lista duplamente encadeada, circular, com Head Node.
*  Na manipula��o da lista auxiliar LSPList, est� � constru�da inserindo cada LSP encontrada para o nodo-oIface em ordem decrescente de prioridade, mas somente para
*  LSPs com Holding Priority menor que a Setup Priority da LSP corrente e at� que os recursos sejam suficientes para os requerimentos, ou at� que a varredura da LIB
*  termine.
*  Na efetiva��o da preemp��o, esta lista � percorrida da menor prioridade para a maior, a LSP indicada � marcada como "preempted", seus recursos, devolvidos, e a
*  entrada correspondente na LSPList, apagada com free.  Ao final, tamb�m o Head Node � apagado.
*/
static int nodeProcessResvPreempt(struct Packet *pkt) {
	int iLabel;
	double timeout; //para c�lculo do tempo absoluto de timeout de uma LSP
	struct nodeMsgQueue *msg;
	char mainTraceString[255];
	
	/*Caso 1:  recebido no LSR gen�rico; fazer o mapeamento de r�tulo, inserir dados na LIB e LSP Table; r�tulo inicial deve ser recuperado
	atrav�s de fun��o espec�fica.  Fazer preemp��o se necess�rio.*/
	//procure a mensagem PATH correspondente na fila do nodo, pelo msgIDack
	msg=searchInNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
	if (msg!=NULL) { //se msg == NULL, n�o foi encontrada nenhuma mensagem PATH correspondente; nada fa�a, neste caso.
		if (msg->resourcesReserved==0 && msg->oIface!=0) { //se recursos n�o foram previamente reservados, tente reserv�-los agora (mas n�o o fa�a para nodos de egresso! msg->oIface==0)
			if (preemptResouces(msg->LSPid, pkt->currentNode, msg->oIface)==0)
				return 0; //reserva n�o foi conseguida mesmo com preemp��o; acuse falha e saia
		}
		//recursos foram pr�-reservados ou conseguidos agora com sucesso; prossiga
		iLabel=nodeCreateLabel(pkt->currentNode, msg->iIface);
		timeout=simtime()+getNodeLSPTimeout(pkt->currentNode);  //calcular tempo absoluto de expira��o da LSP a ser criada agora
		insertInLIB(pkt->currentNode, msg->iIface, iLabel, msg->oIface, pkt->lblHdr.label, msg->LSPid, "up", 0, timeout, 0); //coloca zero no timeoutStamp, zero para marcar o campo Backup
		
		sprintf(mainTraceString, "LSP (working) successfully created.  LSPid:  %d  iLabel:  %d oLabel: %d at node %d\n", msg->LSPid, iLabel, pkt->lblHdr.label, pkt->currentNode);
		mainTrace(mainTraceString);
		
		pkt->lblHdr.label=iLabel; //coloca o r�tulo agora criado na mensagem, para que o pr�ximo LSR use como mapeamento oLabel; a mesma mensagem RESV seguir� adiante
		removeFromNodeMsgQueueAck(pkt->currentNode, pkt->lblHdr.msgIDack);
		//LSP foi criada; usu�rio deve usar a fun��o getWorkingLSPLabel para recuperar o r�tulo inicial
	//Caso 2:  recebida pelo LER de ingresso:  fa�a tudo acima, tamb�m remova a rota expl�cita inversa da mem�ria e marque o LSP tunnel como completo
		if (pkt->currentNode == pkt->dst) { //se este for o LER de ingresso (�ltimo destino), ent�o libere o espa�o da vari�vel rota expl�cita inversa
			setLSPtunnelDone(pkt->lblHdr.LSPid);  //marque o LSP tunnel como totalmente completado
			free(pkt->er.explicitRoute);
			pkt->er.explicitRoute = NULL; //evita erro de execu��o em uma nova elimina��o desta rota expl�cita em outra fun��o
		}
		return 1; //retorne com SUCESSO
	} else
		return 0; //retorne com FALHA; nenhuma mensagem PATH correspondente encontrada
}
