/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_pktHandler.c
*
* Fun��es pertinentes � manipula��o dos pacotes de rede
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

/* CRIA��O DO PACOTE E ALOCA��O DE �REA DE MEM�RIA
* Aloca espaco para a estrutura de um novo pacote e retorna o apontador para esta area
*/
struct Packet *createPacket() {
	struct Packet *pkt;
	static int packetNumber = 0;  /*Inicializando o numero de pacotes serial (packetNumber � vari�vel global). O modelo TARVOS foi constru�do
								baseado na assun��o de que este n�mero � �NICO e nunca se repete; se assim n�o o for, resultados inesperados
								podem acontecer.*/

	pkt = (Packet*)malloc(sizeof *pkt);
	if (pkt == NULL) {
		printf("\nError - createPacket - insufficient memory to allocate for new packet");
		exit(1);
	}
	packetNumber++;  //Atualiza a variavel global que tem o numero de serie dos pacotes. Esta atualizacao so ocorre nesta subrotina; o primeiro packetNumber � 1
	pkt->id = packetNumber; //identifica o pacote, este identificador que sera o tkn de toda a simula��o
	pkt->ttl=tarvosParam.ttl; //define o TTL inicial (default) coletado na estrutura de par�metros (sugest�o:  se necess�rio outro, modificar no gerador de tr�fego)

	//Inicializa campos do Packet com valores nulos para evitar aleatoriedades.  As fun��es de manipula��o dos Packets
	//devem colocar os valores apropriados nestes campos.
	pkt->currentNode=0;
	pkt->er.erNextIndex=0;
	pkt->er.explicitRoute=NULL; //rota expl�cita a ser seguida pelo pacote
	pkt->er.recordRoute=NULL; //rota expl�cita seguida pelo pacote e gravada, nodo por nodo
	pkt->er.rrNextIndex=0;
	pkt->er.recordThisRoute=0; //flag indicativa se a rota deve ser gravada no objeto recordRoute
	pkt->generationTime=0;

	pkt->lblHdr.label=0;
	pkt->lblHdr.LSPid=0;
	pkt->lblHdr.msgID=0; //msgID=0 significa que o pacote n�o cont�m mensagem de controle
	pkt->lblHdr.priority=0; //prioridade default ZERO, a menor poss�vel
	strcpy(pkt->lblHdr.msgType, "");
	strcpy(pkt->lblHdr.errorCode, "");
	strcpy(pkt->lblHdr.errorValue, "");

	pkt->length=0;
	pkt->next=NULL;
	pkt->outgoingLink=0; //muito importante; pacotes gerados para um nodo sempre devem ter este campo ZERO, caso contr�rio v�rias fun��es funcionar�o de maneira imprevista
	pkt->previous=NULL;
	pkt->dst=0;
	pkt->src=0;
	return (pkt);
}

/* REMOVE PACOTE DA MEM�RIA
*
*  Ap�s o pacote ter descartado ou ter chegado a seu destino, � preciso elimin�-lo da mem�ria com uma instru��o free.
*  Esta rotina recebe o ponteiro do pacote e faz a remo��o.
*/
void freePkt(struct Packet *pkt) {
	free(pkt->er.recordRoute); //descarta a rota gravada (mesmo que n�o tenha sido, caso em que recordRoute ser� NULL)
	if (pkt->lblHdr.msgID !=0 ) //se msgID == 0, ent�o n�o � pacote de controle (pacotes de dados cont�m rota expl�cita const)
		 //s� elimine a rota expl�cita se a mensagem n�o for PATH_LABEL_REQUEST ou PATH_DETOUR ou PATH_LABEL_REQUEST_PREEMPT, pois estas recebem uma rota expl�cita const
		 if (strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST")!=0 && strcmp(pkt->lblHdr.msgType, "PATH_DETOUR")!=0 && strcmp(pkt->lblHdr.msgType, "PATH_LABEL_REQUEST_PREEMPT")!=0)
			free(pkt->er.explicitRoute); //descarta a rota expl�cita (s� pode ser feito se a rota expl�cita for din�mica, caso contr�rio haver� um erro de execu��o!)
	free(pkt); //descarta o pacote da mem�ria
}