/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_pktHandler.c
*
* Funções pertinentes à manipulação dos pacotes de rede
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

/* CRIAÇÃO DO PACOTE E ALOCAÇÃO DE ÁREA DE MEMÓRIA
* Aloca espaco para a estrutura de um novo pacote e retorna o apontador para esta area
*/
struct Packet *createPacket() {
	struct Packet *pkt;
	static int packetNumber = 0;  /*Inicializando o numero de pacotes serial (packetNumber é variável global). O modelo TARVOS foi construído
								baseado na assunção de que este número é ÚNICO e nunca se repete; se assim não o for, resultados inesperados
								podem acontecer.*/

	pkt = (Packet*)malloc(sizeof *pkt);
	if (pkt == NULL) {
		printf("\nError - createPacket - insufficient memory to allocate for new packet");
		exit(1);
	}
	packetNumber++;  //Atualiza a variavel global que tem o numero de serie dos pacotes. Esta atualizacao so ocorre nesta subrotina; o primeiro packetNumber é 1
	pkt->id = packetNumber; //identifica o pacote, este identificador que sera o tkn de toda a simulação
	pkt->ttl=tarvosParam.ttl; //define o TTL inicial (default) coletado na estrutura de parâmetros (sugestão:  se necessário outro, modificar no gerador de tráfego)

	//Inicializa campos do Packet com valores nulos para evitar aleatoriedades.  As funções de manipulação dos Packets
	//devem colocar os valores apropriados nestes campos.
	pkt->currentNode=0;
	pkt->er.erNextIndex=0;
	pkt->er.explicitRoute=NULL; //rota explícita a ser seguida pelo pacote
	pkt->er.recordRoute=NULL; //rota explícita seguida pelo pacote e gravada, nodo por nodo
	pkt->er.rrNextIndex=0;
	pkt->er.recordThisRoute=0; //flag indicativa se a rota deve ser gravada no objeto recordRoute
	pkt->generationTime=0;

	pkt->lblHdr.label=0;
	pkt->lblHdr.LSPid=0;
	pkt->lblHdr.msgID=0; //msgID=0 significa que o pacote não contém mensagem de controle
	pkt->lblHdr.priority=0; //prioridade default ZERO, a menor possível
	strcpy(pkt->lblHdr.msgType, "");
	strcpy(pkt->lblHdr.errorCode, "");
	strcpy(pkt->lblHdr.errorValue, "");

	pkt->length=0;
	pkt->next=NULL;
	pkt->outgoingLink=0; //muito importante; pacotes gerados para um nodo sempre devem ter este campo ZERO, caso contrário várias funções funcionarão de maneira imprevista
	pkt->previous=NULL;
	pkt->dst=0;
	pkt->src=0;
	return (pkt);
}

/* REMOVE PACOTE DA MEMÓRIA
*
*  Após o pacote ter descartado ou ter chegado a seu destino, é preciso eliminá-lo da memória com uma instrução free.
*  Esta rotina recebe o ponteiro do pacote e faz a remoção.
*/
void freePkt(struct Packet *pkt) {
	free(pkt->er.recordRoute); //descarta a rota gravada (mesmo que não tenha sido, caso em que recordRoute será NULL)
	if (pkt->lblHdr.msgID !=0 ) //se msgID == 0, então não é pacote de controle (pacotes de dados contém rota explícita const)
		 //só elimine a rota explícita se a mensagem não for PATH_LABEL_REQUEST ou PATH_DETOUR ou PATH_LABEL_REQUEST_PREEMPT, pois estas recebem uma rota explícita const
		 if (strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST")!=0 && strcmp(pkt->lblHdr.msgType, "PATH_DETOUR")!=0 && strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST_PREEMPT")!=0)
			free(pkt->er.explicitRoute); //descarta a rota explícita (só pode ser feito se a rota explícita for dinâmica, caso contrário haverá um erro de execução!)
	free(pkt); //descarta o pacote da memória
}