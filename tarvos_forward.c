/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_forward
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

/* Funcoes que serao usadas pelo programa de roteamento do nucleo Roteamento propriamente dito. Definicao do encaminhamento
dos pacotes Aqui define-se o proximo nodo ao pelo qual o pacote vai passar */

/* Neste arquivo e efetivamente definida estrutura de encaminhamento dos
pacotes pela sub-rede indicando as ligacoes entre os nodos e as possibilidades de encaminhamento. */

/* A partir daqui estrategia de encaminhamento de pacotes devem ser implementadas, RED/RIO, Tabela de
Roteamento, MPLS, etc... */

/* Decis�o de ROTEAMENTO Est�tica
*
* O roteamento aqui � est�tico:  a decis�o � feita de forma igual para qualquer pacote, a depender do nodo de origem
* e destino.  As rotas devem ser programadas diretamente na estrutura SWITCH-CASE.
* (talvez mudar para uma tabela de rotas constru�da no in�cio do programa?)
*/

void decidePathStaticRoute(struct Packet *pkt) {
    switch (pkt->currentNode) {
		case 1:
			/* Significa que o pacote foi gerado e foi lancado no roteador 1 e sera tomada a decisao de para qual link devera ser
			encaminhado esta decisao podera ser simples onde uma decisao estatistica podera ser tomada ou complexa
			associando uma rota, label, tabela de roteamento */
			pkt->outgoingLink = 1;
			/* Link configurado para 1 por default */
			if (irandom(1, 100) > 50)
				pkt->outgoingLink = 0;
			/* Com 50% de probabilidade a configura��o anterior pode ser mudada para o Link 2 */
			break;

		case 2:
			/* Roteador 2 */
			pkt->outgoingLink = 0;
			/* Configura-se o link para 3 por default */
			break;
	}
	return;
}

/* DECIS�O DE ROTEAMENTO BASEADO NA LIB (MPLS)
*
* A tabela Label Information Base dever� ser consultada para determinar o path a ser percorrido pelo pacote
* Aqui est�o embutidas as fun��es de push, pop e swap das labels.
* A fun��o retorna 0 se a rota foi encontrada, e 1 se a rota n�o existe e o pacote foi automaticamente descartado.
*/
int decidePathMpls(struct Packet *pkt) {
    struct LIBEntry *p;
	p=searchInLIBStatus(pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label, "up"); //busca a entrada na LIB para o nodo atual; a interface de entrada � o conte�do de pkt->outgoingLink
	//se a rota n�o for encontrada, descartar o pacote ou imprimir erro?
	if (p==NULL) {
		nodeDropPacket(pkt, "(mpls) label not found or LSP down");  //se rota n�o encontrada, descarte o pacote
		return 1;
		/*printf("Erro:  Entrada na LIB nao encontrada.  currentNode:  %d; iIface:  %d; iLabel:  %d\n", pkt->currentNode, pkt->outgoingLink, pkt->lblHdr.label);
		exit(1);*/
	}
	pkt->outgoingLink=p->oIface; //configura a outgoing interface, que � o n�mero do link de sa�da
	pkt->lblHdr.label=p->oLabel; //configura o novo r�tulo (LABEL SWAP)
	pkt->lblHdr.LSPid=p->LSPid; //coloca tamb�m o LSPid no pacote, para o caso de n�o ainda cont�-lo
	return 0;
}

/* DECIS�O DE ROTEAMENTO BASEADO EM ROTEAMENTO EXPL�CITO (EXPLICIT ROUTING)
* O roteamento estar� contido no pr�prio pacote, expl�cito, na forma de um ponteiro para uma estrutura de dados que conter�
* a lista de nodos a percorrer.
* A fun��o retorna 0 se a rota foi encontrada, e 1 se a rota n�o existe e o pacote foi automaticamente descartado.
*/
int decidePathER(struct Packet *pkt) {
	int link;
	/*se currentNode == pr�ximo nodo da lista ER, ent�o entenda que o pacote est� no nodo de origem
	* e que o usu�rio come�ou sua lista expl�cita ER com este mesmo nodo de origem.  Assim, desconsidere esta
	* primeira entrada na lista ER e pegue a pr�xima.
	* (N�o faz nenhum sentido a origem e destino serem iguais.  Este teste permitir� ao usu�rio construir sua lista
	* de roteamento expl�cito por nodos, come�ando pelo nodo onde o pacote � gerado, ou pelo nodo imediatamente ap�s,
	* e ambas op��es ser�o aceitas.)
	* Se isto for resultado de alguma inconsist�ncia na lista ER, o funcionamento daqui para a frente � imprevis�vel.
	*/
	if(pkt->currentNode==*(pkt->er.explicitRoute + pkt->er.erNextIndex))
		pkt->er.erNextIndex++; //avan�a o n�mero nextIndex para a pr�xima posi��o (pr�ximo nodo a ser atingido)
	
	link=findLink(pkt->currentNode, *(pkt->er.explicitRoute + pkt->er.erNextIndex)); //encontra o link que conecta currentNode e o pr�ximo nodo a percorrer, indicado por erNextIndex
	//se a rota n�o for encontrada, imprimir erro ou descartar o pacote?
	if (link==0) {
		nodeDropPacket(pkt, "(er) route not found");  //se rota n�o encontrada, descarte o pacote
		return 1;
		/*printf("\nErro:  funcao decidePathER - link que conecta nodo %d ao nodo %d nao encontrado", pkt->currentNode, *(pkt->er.explicitRoute+pkt->er.erNextIndex));
		exit(1);*/
	}
	pkt->outgoingLink=link; //coloca o link retornado pela fun��o findLink em outgoingLink
	pkt->er.erNextIndex++; //avan�a o n�mero nextIndex para a pr�xima posi��o (pr�ximo nodo a ser atingido)
	return 0;
}

/* TRADUTOR NODO PARA LINK
* Esta rotina recebe n�meros de nodo origem e destino e retorna o n�mero do link que conecta estes nodos.
* Retorna 0 (zero) se nenhum link conectando os nodos for encontrado.
* source = nodo origem
* dst = nodo destino (a ordem importa)
*/
int findLink(int source, int dst) {
	int i;
	for (i=1; i<=sizeof(tarvosModel.lnk)/sizeof(*(tarvosModel.lnk))-1; i++) {
		if (tarvosModel.lnk[i].src==source && tarvosModel.lnk[i].dst==dst)
			return i; //link encontrado; retorne seu n�mero
	}
	return 0; //link n�o encontrado
}