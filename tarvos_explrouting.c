/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_explrouting
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

#include "tarvos_globals.h"

/* RELACIONA UMA ROTA EXPL�CITA PREVIAMENTE CRIADA COM UM PACOTE
*
* O ponteiro para o pacote e para o array da rota expl�cita j� criada devem ser passados como par�metro.
* Esta fun��o deve ser chamada logo no recebimento do pacote por um gerador de tr�fego, antes de ser encaminhado
* para qualquer link.
*/
void attachExplicitRoute(struct Packet *pkt, int er[]) {
	pkt->er.explicitRoute = er;
	pkt->er.erNextIndex=0; //assegura que o �ndice aponta para o in�cio
}

/* INVERTE ROTA EXPL�CITA
*
*  Esta fun��o cria um vetor de rota expl�cita de nodos (com inteiros) contendo o caminho inverso da rota expl�cita dada como par�metro.
*  O uso t�pico � para uma mensagem de controle RESV, que precisa percorrer o caminho inverso de uma mensagem PATH.
*  Deve ser passado um ponteiro para o vetor de inteiros com a rota de nodos a inverter e tamb�m o tamanho, em n�mero de itens,
*  do vetor.
*  Retorna um ponteiro do tipo inteiro contendo o vetor invertido.  Este vetor invertido deve ser descartado com free ap�s ser usado.
*/
int *invertExplicitRoute(int route[], int size) {
		int i;
		int *p; //conter� o vetor invertido

		p = (int*)malloc(size*sizeof *p);
		if (p==NULL) {
			printf("\nError - invertExplicitRoute - insufficient memory to allocate for explicit route object");
			exit(1);
		}
		for (i=0; i<size; i++) {
			*(p+i)=*(route+size-i-1); //transfere os conte�dos, come�ando do in�cio para o vetor destino, e do final para o vetor origem
		}
		return p;
}

/* GRAVA UMA ROTA PERCORRIDA PELO PACOTE
*
*  Esta fun��o grava o nodo atual no objeto RecordRoute do pr�prio pacote.  Se este objeto for nulo, a fun��o cria-o e grava o nodo atual.
*  Em adi��o, o �ndice para grava��o do pr�ximo nodo � incrementado (o objeto RecordRoute � um ponteiro para um array de inteiros).
*/
void recordRoute(struct Packet *pkt) {
	int size;
	if (pkt->er.recordRoute==NULL) {  //objeto RecordRoute ainda n�o existe; crie-o
		size=sizeof(tarvosModel.node)/sizeof(*(tarvosModel.node)); //o vetor de nodos tem uma posi��o a mais, de modo que a primeira (zero) seja descartada; por�m, o recordRoute usar� a posi��o zero
		pkt->er.recordRoute = (int*)malloc(size * sizeof (*(pkt->er.recordRoute))); /*calcula o tamanho do vetor tarvosModel.node e multiplica pelo tamanho (int), de modo
														 que o objeto recordRoute tenha um m�ximo alocado igual ao n�mero de nodos na topologia*/
		if (pkt->er.recordRoute==NULL) {
			printf("\nErro - recordRoute - insufficient memory to allocate for explicit route object");
			exit(1);
		}
		pkt->er.rrNextIndex=0;
	}
	*(pkt->er.recordRoute + pkt->er.rrNextIndex) = pkt->currentNode; //grava o nodo atual no objeto RecordRoute
	pkt->er.rrNextIndex++;  //incrementa �ndice para a pr�xima inser��o de rota
}