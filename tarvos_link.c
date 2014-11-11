/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_link
* 
* Fun��es de cria��o, parametriza��o, inicializa��o e opera��o dos links.
*
* Nota:    Algumas fun��es reescritas em julho/2005
*		   Marcos Portnoi
* 
*		   Modelagem do link consertada.  N�o pode ser modelado segundo um servidor com tempo
*		   de servi�o fun��o do tamanho do pacote/largura de banda + atraso de propaga��o.
*		   Desta forma, o link permaneceria ocupado at� que um pacote chegasse a seu destino,
*		   n�o permitindo mais de um pacote trafegando por vez.  A modelagem deve ser um servidor
*		   de transmiss�o, com tempo de servi�o fun��o do tamanho do pacote/largura de banda,
*		   e um centro de atraso para modelar o atraso de propaga��o.  A id�ia � implementar
*		   duas fun��es:  linkBeginTransmitPacket, que ser� o servidor de transmiss�o e que efetivamente
*		   permanece ocupado durante a transmiss�o (e tem as filas), e uma fun��o
*		   linkPropagatePacket, que ser� uma simples chamada schedulep, atrasando a chegada do
*		   pacote at� seu destino segundo o atraso de propaga��o do link.
*		   O programa do usu�rio deve, para implementar o modelo, separar a transmiss�o e a
*		   propaga��o em dois eventos distintos.  O primeiro evento tratar� de chamar a fun��o
*		   linkBeginTransmitPacket.  Esta fun��o retornar� o controle para o outro evento, que
*		   chamar� a fun��o linkPropagatePacket.  S� ap�s, o evento "chegada de pacote no nodo de
*		   destino" dever� ocorrer.  29/12/2005 Marcos Portnoi
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

#include "simm_globals.h"
#include "tarvos_globals.h"

//Prototypes das fun��es locais static
static struct PacketsInTransitQueue *createPktInTransitQueue(int linkNumber);
static void insertInPktInTransitQueue(int linkNumber, int pktId);
static struct PacketsInTransitCell *searchInPktInTransitQueue(int linkNumber, int pktId);
static int dropPktsInTransit(int linkNumber);

/* CRIA��O DOS LINKS DE REDE TIPO SIMPLEX
*
* Esta rotina cria links simplex (um sentido somente) entre dois nodos de rede.
* O link entre dois roteadores � bem modelado por uma facility com tipicamente um servidor e um centro de atraso.  Os pacotes s�o
* atendidos com um tempo de servi�o basicamente uniforme, correspondente � transmiss�o do pacote para o link f�sico, e um atraso
* constante, correspondente ao tempo de propaga��o pelo meio f�sico.  O tempo de servi�o de transmiss�o � baseado na largura de
* banda do link (um par�metro recebido). Neste modelo de rede, os links s�o efetivamente formados por um servidor de transmiss�o
* e um centro de atraso.  O programa do usu�rio deve escalonar a transmiss�o e a propaga��o individualmente para cada pacote, uma
* ap�s a outra (e a propaga��o somente ap�s a transmiss�o ter sido bem sucedida).  Poder� haver fila na transmiss�o, mas n�o na
* propaga��o (pois o meio f�sico, em tese, tem capacidade de servi�o infinita).
* Usa-se sempre 1 server para cada facility.
* Os links est�o associados a estruturas de dados com seus par�metros.
* Os par�metros aceitos s�o:
*  linkNumber:		n�mero do link
*  name:			nome para a facility de transmiss�o associada ao link
*  bandwidth:		largura de banda do link (em bits por segundo)
*  delay:			atraso de propaga��o (segundos)
*  delayOther1/2	atrasos extras para modelagem de atraso de processamento, etc.
*  source:			n�mero do nodo de origem
*  dst:				n�mero do nodo de destino
*  maxCbs:			tamanho m�ximo do Bucket para RSVP
*
* Os demais par�metros que controlam os n�meros dispon�veis para Token Bucket do RSVP s�o setados em modo default para a pr�pria largura
* de banda do link, que s�o os par�metros availCbr e availPir.  Estes valores s�o atualizados sempre que uma nova LSP � criada no link (os valores
* diminuem) ou uma LSP � deletada (os valores aumentam).
*/
void createSimplexLink(int linkNumber, char *name, double bandwidth, double delay, double delayOther1, double delayOther2, 
					   int source, int dst, double maxCbs) {
    tarvosModel.lnk[linkNumber].linkNumber=linkNumber; //n�mero do link; o modelo de simulador correntemente n�o usa este campo
	tarvosModel.lnk[linkNumber].facility = facility(name, 1); //Criacao da facility do link linkNumber
	strcpy(tarvosModel.lnk[linkNumber].name, name); //Insere o nome do link para tratamento das facility
	tarvosModel.lnk[linkNumber].bandwidth = bandwidth;  //Largura de banda do link (bps)
	tarvosModel.lnk[linkNumber].delay = delay;  //Atraso de propaga��o do link (seg)
	tarvosModel.lnk[linkNumber].delayOther1 = delayOther1;  //atrasos extras
	tarvosModel.lnk[linkNumber].delayOther2 = delayOther2;
	tarvosModel.lnk[linkNumber].src = source;  //Nodo origem do link
	tarvosModel.lnk[linkNumber].dst = dst;  //Nodo destino do link
	strcpy(tarvosModel.lnk[linkNumber].status, "up"); //link est� UP por default
	tarvosModel.lnk[linkNumber].availCbs = maxCbs;
	tarvosModel.lnk[linkNumber].availCir = bandwidth / 8; //CIR � indicado em bytes por segundo
	tarvosModel.lnk[linkNumber].availPir = bandwidth / 8; //PIR � indicado em bytes por segundo
	createPktInTransitQueue(linkNumber); //cria lista de pacotes em tr�nsito
	return;
}

/* CRIA��O DOS LINKS DE REDE TIPO DUPLEX
*
* Esta rotina cria links duplex (dois sentidos de comunica��o) entre dois nodos de rede.
* Basicamente trata-se de dois links simplex, um para cada sentido, com n�meros e nomes de facility independentes, mas com mesma
* largura de banda e atraso de propaga��o.  Usa-se sempre 1 server para cada facility.
* O link entre dois roteadores � bem modelado por uma facility com tipicamente um servidor e um centro de atraso.  Os pacotes s�o
* atendidos com um tempo de servi�o basicamente uniforme, correspondente � transmiss�o do pacote para o link f�sico, e um atraso
* constante, correspondente ao tempo de propaga��o pelo meio f�sico.  O tempo de servi�o de transmiss�o � baseado na largura de
* banda do link (um par�metro recebido). Neste modelo de rede, os links s�o efetivamente formados por um servidor de transmiss�o
* e um centro de atraso.  O programa do usu�rio deve escalonar a transmiss�o e a propaga��o individualmente para cada pacote, uma
* ap�s a outra (e a propaga��o somente ap�s a transmiss�o ter sido bem sucedida).  Poder� haver fila na transmiss�o, mas n�o na
* propaga��o (pois o meio f�sico, em tese, tem capacidade de servi�o infinita).
* Os links est�o associados a estruturas de dados com seus par�metros.
* Os par�metros aceitos s�o:
*  linkNumberSrcDst:	n�mero do link correspondente ao sentido src->dst
*  linkNumberDstSrc:	n�mero do link correspondente ao sentido dst->src
*  nameSrcDst:			nome para a facility de transmiss�o associada ao link sentido src->dst
*  nameDstSrc:			nome para a facility de transmiss�o associada ao link sentido dst->src
*  bandwidth:			largura de banda do link (em bits por segundo)
*  delay:				atraso de propaga��o (segundos)
*  delayOther1/2		atrasos extras para modelagem de atraso de processamento e etc. (os mesmos para ambos links)
*  source:				n�mero do nodo de origem
*  dst:					n�mero do nodo de destino
*  maxCbs:				tamanho m�ximo do Bucket para RSVP
*
* Os demais par�metros que controlam os n�meros dispon�veis para Token Bucket do RSVP s�o setados em modo default para a pr�pria largura
* de banda do link, que s�o os par�metros availCbr e availPir.  Estes valores s�o atualizados sempre que uma nova LSP � criada no link (os valores
* diminuem) ou uma LSP � deletada (os valores aumentam).
*/
void createDuplexLink(int linkNumberSrcDst, int linkNumberDstSrc, char *nameSrcDst, char *nameDstSrc, double bandwidth, double delay,
					  double delayOther1, double delayOther2, int source, int dst, double maxCbs) {
    //link sentido src->dst
	tarvosModel.lnk[linkNumberSrcDst].linkNumber=linkNumberSrcDst; //n�mero do link; o modelo de simulador correntemente n�o usa este campo
	tarvosModel.lnk[linkNumberSrcDst].facility = facility(nameSrcDst, 1); //Criacao da facility do link linkNumberSrcDst
	strcpy(tarvosModel.lnk[linkNumberSrcDst].name, nameSrcDst); //Insere o nome do link para tratamento das facility
	tarvosModel.lnk[linkNumberSrcDst].bandwidth = bandwidth;  //Largura de banda do link (bps)
	tarvosModel.lnk[linkNumberSrcDst].delay = delay;  //Atraso de propaga��o do link (seg)
	tarvosModel.lnk[linkNumberSrcDst].delayOther1 = delayOther1;  //atrasos extras
	tarvosModel.lnk[linkNumberSrcDst].delayOther2 = delayOther2;
	tarvosModel.lnk[linkNumberSrcDst].src = source;  //Nodo origem do link
	tarvosModel.lnk[linkNumberSrcDst].dst = dst;  //Nodo destino do link
	strcpy(tarvosModel.lnk[linkNumberSrcDst].status, "up"); //link est� UP por default
	tarvosModel.lnk[linkNumberSrcDst].availCbs = maxCbs;
	tarvosModel.lnk[linkNumberSrcDst].availCir = bandwidth / 8; //CIR � indicado em bytes por segundo
	tarvosModel.lnk[linkNumberSrcDst].availPir = bandwidth / 8; //PIR � indicado em bytes por segundo
	createPktInTransitQueue(linkNumberSrcDst); //cria lista de pacotes em tr�nsito
	
	//link sentido dst->src
	tarvosModel.lnk[linkNumberDstSrc].linkNumber=linkNumberDstSrc; //n�mero do link; o modelo de simulador correntemente n�o usa este campo
	tarvosModel.lnk[linkNumberDstSrc].facility = facility(nameDstSrc, 1); //Criacao da facility do link linkNumberDstSrc
	strcpy(tarvosModel.lnk[linkNumberDstSrc].name, nameDstSrc); //Insere o nome do link para tratamento das facility
	tarvosModel.lnk[linkNumberDstSrc].bandwidth = bandwidth;  //Largura de banda do link (bps)
	tarvosModel.lnk[linkNumberDstSrc].delay = delay;  //Atraso de propaga��o do link (seg)
	tarvosModel.lnk[linkNumberDstSrc].delayOther1 = delayOther1;  //atrasos extras
	tarvosModel.lnk[linkNumberDstSrc].delayOther2 = delayOther2;
	tarvosModel.lnk[linkNumberDstSrc].src = dst;  //Nodo origem do link; o inverso do primeiro sentido acima
	tarvosModel.lnk[linkNumberDstSrc].dst = source;  //Nodo destino do link; o inverso do primeiro sentido acima
	strcpy(tarvosModel.lnk[linkNumberDstSrc].status, "up"); //link est� UP por default
	tarvosModel.lnk[linkNumberDstSrc].availCbs = maxCbs;
	tarvosModel.lnk[linkNumberDstSrc].availCir = bandwidth / 8; //CIR � indicado em bytes por segundo
	tarvosModel.lnk[linkNumberDstSrc].availPir = bandwidth / 8; //PIR � indicado em bytes por segundo
	createPktInTransitQueue(linkNumberDstSrc); //cria lista de pacotes em tr�nsito
}

/* INICIAR TRANSMISS�O DE PACOTE PELO LINK
*
*  Faz a transmiss�o do pacote para o link, segundo a largura de banda definida para o link.
*  O link de rede � modelado como um servidor com fila, com tempo de servi�o correspondente � largura de banda
*  do link (na vida real, isto � representado pelo switch ou roteador) e por um centro de atraso logo ap�s, com 
*  atraso fixo determinado pelo atraso de propaga��o do link e capacidade de processamento infinita (na vida real,
*  � o meio f�sico do enlace).  As filas ocorrem quando o recebimento de pacotes pelo link supera a largura de banda
*  (a capacidade de processamento do switch ou roteador).  Uma vez transmitidos, os pacotes trafegam pelo link com atraso
*  fixo, um ap�s o outro.
*
*  � importante frisar que, para a perfeita modelagem do link, o programa principal deve reservar dois eventos diferentes
*  para o link:  um evento far� a chamada da fun��o linkBeginTransmitPacket.  O outro evento far� o release do servidor-link
*  (observar que o pacote *ainda* n�o chegou no nodo de destino) e introduzir� o atraso de propaga��o do link, na forma
*  de uma simples chamada schedulep para o pr�ximo evento (que dever� ser a chegada do pacote no nodo de destino).
*  Assim, por exemplo, tenha-se um evento 5 do tipo Transmiss�o e um evento 6 do tipo propaga��o.  O evento 5 dever�
*  chamar linkBeginTransmitPacket, que escalonar� um servi�o de transmiss�o, que terminar� num escalonamento de evento 6.
*  O evento 6 dever� introduzir o atraso de propaga��o (o pacote ainda n�o foi recebido pelo nodo de destino) e escalonar
*  o pr�ximo evento, que ser� finalmente o recebimento do pacote pelo nodo de destino (e onde as estat�sticas pertinentes)
*  dever�o ser atualizadas).
*
*  Os par�metros passados s�o o tipo do evento a ser escalonado para este pacote, a prioridade e o
*  ponteiro para o pacote a ser encaminhado.
*/
int linkBeginTransmitPacket(int ev, struct Packet *pkt) {
    int currentPacket, r; //currentPacket:  id do pacote sendo processado no momento; 
	double ie_t;
	currentPacket = pkt->id;
	// obtem o numero do pacote atual que no caso e a tkn para o escalonamento do evento na cadeia simm
	
	//Implementa��o correta da transmiss�o:  calcula o tempo de transmiss�o para o link, que depende do tamanho do pacote e
	//da largura de banda do link
	// ie_t - inter event time: tempo de ocorrencia entre eventos
	ie_t = pkt->length*8.0/tarvosModel.lnk[pkt->outgoingLink].bandwidth;

	// Testa se o servidor link de saida esta livre; se sim, ja escalona o  termino da transmiss�o:  a propaga��o deve vir logo ap�s
	r=requestp(tarvosModel.lnk[pkt->outgoingLink].facility, currentPacket, pkt->lblHdr.priority, ev, ie_t, pkt);
	if (r == 0) //servidor livre; escalone fim de transmiss�o
		schedulep(ev, ie_t, currentPacket, pkt);
	//se r==1, pacote foi enfileirado; nada fa�a, ele ser� transmitido ao ser retirado da fila (o kernel faz isso automaticamente)
	if (r == 2) //servidor down; descarte pacote
		nodeDropPacket(pkt, "server down - not transmitted");

	return r; //retorne 0 para transmiss�o bem sucedida; 1 para pacote em fila; 2 para pacote descartado

}

/* IMPRIMIR LISTA DE LINKS CONFIGURADOS NA SIMULA��O
*
*  Imprime, no arquivo recebido como par�metro, lista de links configurados com seus par�metros.
*
*/
void dumpLinks (char *outfile) {
	int i;
	FILE *fp;
	
	fp=fopen(outfile, "w");
	fprintf(fp, "Links Parameters - Tarvos Simulator\n\n");
	fprintf(fp, "Entry LinkNumber Name                               Bandwidth     Delay   DelayOther1 DelayOther2  Src   Dst Facility# Status          availCBS      availCIR      availPIR  PktInTransitQueue Pointer\n");
	for (i=1; i<=sizeof(tarvosModel.lnk)/sizeof(*(tarvosModel.lnk))-1; i++) {
		fprintf(fp, "%3d     %3d      %-30s  %13.1f   %8.4f   %8.4f   %8.4f   %4d  %4d   %4d     %s         %12.1f  %12.1f  %12.1f         %p\n",
			i, tarvosModel.lnk[i].linkNumber, tarvosModel.lnk[i].name, tarvosModel.lnk[i].bandwidth, tarvosModel.lnk[i].delay, tarvosModel.lnk[i].delayOther1,
			tarvosModel.lnk[i].delayOther2, tarvosModel.lnk[i].src, tarvosModel.lnk[i].dst, tarvosModel.lnk[i].facility, tarvosModel.lnk[i].status,
			tarvosModel.lnk[i].availCbs, tarvosModel.lnk[i].availCir, tarvosModel.lnk[i].availPir, tarvosModel.lnk[i].packetsInTransitQueue);
	}
	fclose(fp);
}

/* INICIAR TRANSMISS�O DE PACOTE PELO LINK COM PREEMP��O
*
*  Faz a transmiss�o do pacote para o link, segundo a largura de banda definida para o link.
*  O link de rede � modelado como um servidor com fila, com tempo de servi�o correspondente � largura de banda
*  do link (na vida double, isto � representado pelo switch ou roteador) e por um centro de atraso logo ap�s, com 
*  atraso fixo determinado pelo atraso de propaga��o do link e capacidade de processamento infinita (na vida double,
*  � o meio f�sico do enlace).  As filas ocorrem quando o recebimento de pacotes pelo link supera a largura de banda
*  (a capacidade de processamento do switch ou roteador).  Uma vez transmitidos, os pacotes trafegam pelo link com atraso
*  fixo, um ap�s o outro.
*  Esta fun��o usa a preemp��o para desalocar algum pacote sendo transmitido por outro com prioridade maior.  A preemp��o
*  � feita pelo n�cleo do simulador simm.
*
*  � importante frisar que, para a perfeita modelagem do link, o programa principal deve reservar dois eventos diferentes
*  para o link:  um evento far� a chamada da fun��o linkBeginTransmitPacket.  O outro evento far� o release do servidor-link
*  (observar que o pacote *ainda* n�o chegou no nodo de destino) e introduzir� o atraso de propaga��o do link, na forma
*  de uma simples chamada schedulep para o pr�ximo evento (que dever� ser a chegada do pacote no nodo de destino).
*  Assim, por exemplo, tenha-se um evento 5 do tipo Transmiss�o e um evento 6 do tipo propaga��o.  O evento 5 dever�
*  chamar linkBeginTransmitPacket, que escalonar� um servi�o de transmiss�o, que terminar� num escalonamento de evento 6.
*  O evento 6 dever� introduzir o atraso de propaga��o (o pacote ainda n�o foi recebido pelo nodo de destino) e escalonar
*  o pr�ximo evento, que ser� finalmente o recebimento do pacote pelo nodo de destino (e onde as estat�sticas pertinentes)
*  dever�o ser atualizadas).
*
*  Os par�metros passados s�o o tipo do evento a ser escalonado para este pacote e ponteiro para o pacote a ser encaminhado.
*/
int linkBeginTransmitPacketPreempt(int ev, struct Packet *pkt) {
    int currentPacket, r; //id do pacote sendo processado neste momento
	double ie_t;
	currentPacket = pkt->id;
	// obtem o numero do pacote atual que no caso e a tkn para o escalonamento do evento na cadeia simm
	
	//Implementa��o correta da transmiss�o:  calcula o tempo de transmiss�o para o link, que depende do tamanho do pacote e
	//da largura de banda do link
	// ie_t - inter event time: tempo de ocorrencia entre eventos
	ie_t = pkt->length*8.0/tarvosModel.lnk[pkt->outgoingLink].bandwidth;

	// Testa se o servidor link de saida esta livre; se sim, ja escalona o  termino da transmiss�o:  a propaga��o deve vir logo ap�s
	r=preemptp(tarvosModel.lnk[pkt->outgoingLink].facility, currentPacket, pkt->lblHdr.priority, ev, ie_t, pkt);
	if (r == 0) //servidor livre; escalone fim de transmiss�o
		schedulep(ev, ie_t, currentPacket, pkt);
	//se r==1, pacote foi enfileirado; nada fa�a
	if (r == 2) //servidor down; descarte pacote
		nodeDropPacket(pkt, "server down - not transmitted");

	return r; //retorne 0 para transmiss�o bem sucedida; 1 para pacote em fila; 2 para pacote descartado
}

/*  PROPAGAR PACOTE PELO MEIO F�SICO DO LINK
*
*   A modelagem aqui � um simples centro de atraso.
*   A atualiza��o do currentNode na estrutura de dados do pkt deve ser feito ap�s chamar
*   esta rotina.
*   Esta fun��o tamb�m atualiza a estrutura pkt de modo que o campo currentNode indique o nodo
*   no qual o pacote estar� ap�s a propaga��o.
*
*   Note! When a link is made down and it currently has a PDU being transmitted, that PDU will not
*   be dropped. The result is that the PDU will finish transmission and will be propagated here.
*   This is an unwanted behavior (or bug). A workaround is to test, in this function, whether the
*   facility is down; if it is, do not propagate the PDU. (August 19, 2013)
*
*/
void linkPropagatePacket (int ev, struct Packet *pkt) {
	schedulep(ev, tarvosModel.lnk[pkt->outgoingLink].delay, pkt->id, pkt);
	insertInPktInTransitQueue(pkt->outgoingLink, pkt->id); //insere pacote na lista de pacotes em tr�nsito
	pkt->currentNode = tarvosModel.lnk[pkt->outgoingLink].dst; //atualiza o pacote para o nodo em que ele estara apos ser encaminhado pelo link
}

/*  TERMINAR TRANSMISS�O DE PACOTE PELO LINK (SERVIDOR DE TRANSMISS�O)
*
*   Esta fun��o deve ser chamada para sinalizar o fim da transmiss�o do pacote.  A facility correspondente
*   � transmiss�o do link ser� liberada.  O pr�ximo passo dever� ser a propaga��o do pacote pelo
*   link f�sico.
*   Os par�metros s�o o n�mero da facility e o n�mero do pacote (token) correspondentes ao releasep.
*
*/
void linkEndTransmitPacket (int facility, int currentPacket) {
	releasep(facility, currentPacket);  //Libera a transmiss�o
}

/*  COLOCA O DUPLEX LINK EM ESTADO DOWN (N�O-OPERACIONAL)
*
*   Marca o link como down, descarta pacotes em fila no servidor associado ao link e
*   atualiza estat�sticas.  Faz isso tanto para o link passado como par�metro, como para o link reverso que liga os mesmos nodos
*   (no sentido inverso).
*/
void setDuplexLinkDown(int linkNumber) {
	int reverseLink;
	
	reverseLink=findLink(tarvosModel.lnk[linkNumber].dst, tarvosModel.lnk[linkNumber].src); //encontra link reverso relacionado ao linkNumber
	setSimplexLinkDown(linkNumber);
	setSimplexLinkDown(reverseLink);
}
	
/*  COLOCA O LINK EM ESTADO DOWN (N�O-OPERACIONAL)
*
*   Marca o link como down, descarta pacotes em fila no servidor associado ao link e
*   atualiza estat�sticas.
*/
void setSimplexLinkDown(int linkNumber) {
	int i, packetsDropped;
	char dropTraceEntry[255];

	strcpy(tarvosModel.lnk[linkNumber].status, "down");
	//coloca o servidor de transmiss�o em down;
	//descarta os pacotes em fila e incrementa apropriadamente o contador de pacotes perdidos no nodo de origem do link linkNumber
	packetsDropped=setFacDown(tarvosModel.lnk[linkNumber].facility); //retorna o n�mero de tokens (pacotes) descartados da fila
	packetsDropped+=dropPktsInTransit(linkNumber); //adiciona pacotes em propaga��o descartados
	for (i=0;i<packetsDropped;i++) {
		nodeIncDroppedPacketsNumber(tarvosModel.lnk[linkNumber].src);
	}
	sprintf(dropTraceEntry, "LINK %d DOWN at simtime: %f  Total Packets Dropped:  %d\n", linkNumber, simtime(),packetsDropped);
	dropPktTrace(dropTraceEntry);
	mainTrace(dropTraceEntry);
}

/*  COLOCA O LINK EM ESTADO UP (OPERACIONAL)
*/
void setSimplexLinkUp(int linkNumber) {
	char mainTraceEntry[255];

	strcpy(tarvosModel.lnk[linkNumber].status, "up");
	setFacUp(tarvosModel.lnk[linkNumber].facility);
	sprintf(mainTraceEntry, "LINK %d UP at simtime: %f\n", linkNumber, simtime());
	mainTrace(mainTraceEntry);
}

/*  COLOCA O DUPLEX LINK EM ESTADO UP (OPERACIONAL)
*/
void setDuplexLinkUp(int linkNumber) {
	int reverseLink;
	
	reverseLink=findLink(tarvosModel.lnk[linkNumber].dst, tarvosModel.lnk[linkNumber].src); //encontra link reverso relacionado ao linkNumber
	setSimplexLinkUp(linkNumber);
	setSimplexLinkUp(reverseLink);
}

/*  RETORNA STATUS DO LINK (DOWN OU UP)
*
*   retorna string caracterizador do status do link ("up" ou "down")
*/
char *getLinkStatus(int linkNumber) {
	return tarvosModel.lnk[linkNumber].status;
}

/* CRIA LISTA DE PACOTES EM TR�NSITO EM UM LINK
*
*  A estrutura relacionada � uma lista din�mica simplesmente encadeada com um Head Node.
*
*/
static struct PacketsInTransitQueue *createPktInTransitQueue(int linkNumber) {
	//as instru��es abaixo criam os apontadores para a lista de pacotes em tr�nsito no link
	tarvosModel.lnk[linkNumber].packetsInTransitQueue = (PacketsInTransitQueue*)malloc(sizeof *(tarvosModel.lnk[linkNumber].packetsInTransitQueue));
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->head = (PacketsInTransitCell*)malloc(sizeof *(tarvosModel.lnk[linkNumber].packetsInTransitQueue->head)); //cria Head Node
	if (tarvosModel.lnk[linkNumber].packetsInTransitQueue==NULL || tarvosModel.lnk[linkNumber].packetsInTransitQueue->head==NULL) {
		printf("\nError - creatPktInTransitQueue - insufficient memory to allocate for Packets in Transit Queue");
		exit (1);
	}
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->head->next=NULL; //lista est� vazia
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->last=tarvosModel.lnk[linkNumber].packetsInTransitQueue->head;
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->NumberInTransit=0; //nenhum pacote em tr�nsito
	return tarvosModel.lnk[linkNumber].packetsInTransitQueue;
}

/* INSERE NOVO ITEM NA LISTA DE PACOTES EM TR�NSITO EM UM LINK
*/
static void insertInPktInTransitQueue(int linkNumber, int pktId) {
	//cria novo espa�o para o item
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->next = (PacketsInTransitCell*)malloc(sizeof *(tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->next));
	if (tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->next==NULL) {
		printf("\nError - insertInPktInTransitQueue - insufficient memory to allocate for Packets in Transit Queue");
		exit (1);
	}
	//atualiza apontador para o �ltimo item da lista
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->last=tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->next;
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->id=pktId; //guarda o ID do pacote
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->last->next=NULL; //garantir terminador da lista
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->NumberInTransit++; //mais um pacote em tr�nsito
}

/* RETORNA A QUANTIDADE DE PACOTES EM TR�NSITO (PROPAGA��O) EM UM LINK
*/
int getPktInTransitQueueSize(int linkNumber) {
	return(tarvosModel.lnk[linkNumber].packetsInTransitQueue->NumberInTransit);
}

/* FAZ UMA BUSCA NA LISTA DE PACOTES EM TR�NSITO EM UM LINK
*
*  Os par�metros s�o o ID do pacote e o n�mero do link; retorna o ponteiro para a posi��o (item) imediatamente anterior
*  ou NULL para n�o achado.
*  A t�cnica de busca usa HEAD NODE e TAIL NODE (ou SENTINEL), que conter� o registro procurado.
*/
static struct PacketsInTransitCell *searchInPktInTransitQueue(int linkNumber, int pktId) {
	struct PacketsInTransitCell *found, *pos, *tail;
	struct PacketsInTransitQueue *list;

	list=tarvosModel.lnk[linkNumber].packetsInTransitQueue;
	if (list->NumberInTransit==0) return NULL; //retorne imediatamente se lista estiver vazia
	pos=list->head;
	//cria TAIL node
	tail = (PacketsInTransitCell*)malloc(sizeof *tail);
	if (tail==NULL) {
		printf("\nError - searchInPktInTransitQueue - insufficient memory to allocate for Packets in Transit Queue");
		exit(1);
	}
	list->last->next=tail;
	tail->next=NULL;
	tail->id=pktId; //Tail node conter� o ID a ser procurado; se a busca terminar nele, significa que n�o foi achado
	while (pos->next->id != pktId) {
		pos=pos->next;
	}
	//Se *pos* indicar o item imediatamente anterior ao Tail node, � porque o Id n�o foi achado
	if (pos->next == tail) found=NULL;
	else found=pos;
	free(tail);
	list->last->next=NULL;
	return (found);
}

/* REMOVE UM PACOTE DA LISTA DE PACOTES EM TR�NSITO EM UM LINK
*
*  Os par�metros s�o o ID do pacote e o n�mero do link.  Se pacote n�o for encontrado, mostra
*  mensagem de erro e encerra a simula��o.
*/
void removePktFromTransitQueue(int linkNumber, int pktId) {
	struct PacketsInTransitCell *pos, *aux;
	pos=searchInPktInTransitQueue(linkNumber, pktId);
	if (pos==NULL) {
		printf("\nError - removePktFromTransitQueue - packet not found in Packets in Transit Queue");
		exit(1);
	}
	//se pacote a ser removido for o �ltimo da lista, atualize o apontador 'last'
	if (pos->next==tarvosModel.lnk[linkNumber].packetsInTransitQueue->last)
		tarvosModel.lnk[linkNumber].packetsInTransitQueue->last=pos; //'pos' � a posi��o anterior � remo��o
	//remover pacote da lista de pacotes em tr�nsito; *pos* � a posi��o imediatamente anterior
	aux=pos->next;
	pos->next=pos->next->next; //atualiza apontador para pr�ximo item do item anterior � remo��o
	free(aux); //descarta pacote da lista somente (o pacote original n�o � removido da mem�ria!)
	tarvosModel.lnk[linkNumber].packetsInTransitQueue->NumberInTransit--; //decrementa contador de pacotes em tr�nsito
}

/* DESCARTA TODOS OS PACOTES EM TR�NSITO (PROPAGA��O) EM UM LINK
*
*  Descarta os pacotes na lista de pacotes em tr�nsito de um link, incluindo os eventos relacionados (chegadas)
*  na cadeia de eventos do kernel do SimM.  Tamb�m elimina da mem�ria os elementos cancelados da cadeia de eventos.
*  Recebe como par�metro o n�mero do link, e devolve o n�mero de pacotes descartados.
*/
static int dropPktsInTransit(int linkNumber) {
	struct PacketsInTransitCell *p, *pNext;
	struct PacketsInTransitQueue *list;
	struct Packet *pkt;
	struct evchain *ev;
	int i=0;

	list=tarvosModel.lnk[linkNumber].packetsInTransitQueue; //ponteiro para a lista de pacotes em tr�nsito
	pNext=list->head->next;
	while (pNext != NULL) {
		p=pNext; //esta � a posi��o a remover
		pNext=pNext->next; //pNext deve apontar para o elemento adiante � remo��o
		ev=cancelp_tkn(p->id); //busca evento correspondente ao pacote que ser� descartado
		if (ev == NULL) {
			printf("\nError - dropPktsInTransit - inconsistency:  packet to be dropped from Packets in Transit Queue does not have associated event in Event Chain");
			exit(1);
		}
		pkt=ev->ev_tkn_p; //remove a chegada do pacote da cadeia de eventos
		freePkt(pkt); //remova o pacote da mem�ria
		free(p); //remova o elemento apontado por p
		free(ev); //remove tamb�m o evento que foi cancelado (n�o remov�-lo causa um memory leak)
		i++; //mais um elemento removido
	}
	list->head->next=NULL; //garantir termina��o correta da lista
	list->last=list->head; //lista agora est� vazia
	list->NumberInTransit=0; //certificar que os ponteiros e contadores est�o exatos para lista vazia
	return i; //retorne n�mero de pacotes descartados
}