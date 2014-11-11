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

/* RELACIONA UMA ROTA EXPLÍCITA PREVIAMENTE CRIADA COM UM PACOTE
*
* O ponteiro para o pacote e para o array da rota explícita já criada devem ser passados como parâmetro.
* Esta função deve ser chamada logo no recebimento do pacote por um gerador de tráfego, antes de ser encaminhado
* para qualquer link.
*/
void attachExplicitRoute(struct Packet *pkt, int er[]) {
	pkt->er.explicitRoute = er;
	pkt->er.erNextIndex=0; //assegura que o índice aponta para o início
}

/* INVERTE ROTA EXPLÍCITA
*
*  Esta função cria um vetor de rota explícita de nodos (com inteiros) contendo o caminho inverso da rota explícita dada como parâmetro.
*  O uso típico é para uma mensagem de controle RESV, que precisa percorrer o caminho inverso de uma mensagem PATH.
*  Deve ser passado um ponteiro para o vetor de inteiros com a rota de nodos a inverter e também o tamanho, em número de itens,
*  do vetor.
*  Retorna um ponteiro do tipo inteiro contendo o vetor invertido.  Este vetor invertido deve ser descartado com free após ser usado.
*/
int *invertExplicitRoute(int route[], int size) {
		int i;
		int *p; //conterá o vetor invertido

		p = (int*)malloc(size*sizeof *p);
		if (p==NULL) {
			printf("\nError - invertExplicitRoute - insufficient memory to allocate for explicit route object");
			exit(1);
		}
		for (i=0; i<size; i++) {
			*(p+i)=*(route+size-i-1); //transfere os conteúdos, começando do início para o vetor destino, e do final para o vetor origem
		}
		return p;
}

/* GRAVA UMA ROTA PERCORRIDA PELO PACOTE
*
*  Esta função grava o nodo atual no objeto RecordRoute do próprio pacote.  Se este objeto for nulo, a função cria-o e grava o nodo atual.
*  Em adição, o índice para gravação do próximo nodo é incrementado (o objeto RecordRoute é um ponteiro para um array de inteiros).
*/
void recordRoute(struct Packet *pkt) {
	int size;
	if (pkt->er.recordRoute==NULL) {  //objeto RecordRoute ainda não existe; crie-o
		size=sizeof(tarvosModel.node)/sizeof(*(tarvosModel.node)); //o vetor de nodos tem uma posição a mais, de modo que a primeira (zero) seja descartada; porém, o recordRoute usará a posição zero
		pkt->er.recordRoute = (int*)malloc(size * sizeof (*(pkt->er.recordRoute))); /*calcula o tamanho do vetor tarvosModel.node e multiplica pelo tamanho (int), de modo
														 que o objeto recordRoute tenha um máximo alocado igual ao número de nodos na topologia*/
		if (pkt->er.recordRoute==NULL) {
			printf("\nErro - recordRoute - insufficient memory to allocate for explicit route object");
			exit(1);
		}
		pkt->er.rrNextIndex=0;
	}
	*(pkt->er.recordRoute + pkt->er.rrNextIndex) = pkt->currentNode; //grava o nodo atual no objeto RecordRoute
	pkt->er.rrNextIndex++;  //incrementa índice para a próxima inserção de rota
}