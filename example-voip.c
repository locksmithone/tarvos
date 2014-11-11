/*
* TARVOS Computer Networks Simulator
* File example-voip.c
*
* Topology for investigation and validation of Tarvos Simulator
* For details on this simulation, see <http://www.geocities.com/locksmithone>.
*
* Copyright (C) 2004, 2005, 2006, 2007 Marcos Portnoi
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

#define MAIN_MODULE //define variables and structs, and not only declare them as extern
#include "simm_globals.h"
#include "tarvos_globals.h"

int main() {
    enum EventType eventType; //number of events to be treated
	int currentPacket = 0; //specifies current packet taken from event chain; this value is in fact the token number (from Tarvos Kernel), utilized throughout the simulation.

	int i, pktCounter=1, simEnd=0, label, LSPid[10]={0}; //simEnd (flag) forces simulation to end
	struct Packet *pkt, *aux;
	int expRoute1[]={1,2,3,4,5,6}; //explicit route (node numbers)
	int expRoute2[]={2,7,8,9,4,5,6};
	int expRoute3[]={3,8,9,5,6};
	int expRoute4[]={3,10,5,6};

	char mainTraceString[255]; //string that will contain the generated trace (output) line
	FILE *fp;

	fp=fopen("screenOutput.txt", "w");

	about(stdout); //prints copyright information to the screen and to file
	about(fp);
	
	simm(1, "tarvos_v0.71");  //update the title of this simulation here

	stream(SEED); //Seeds the random number generator

	system("mkdir stats"); //create directory 'stats' to save the statistics output files for this model; remove this line if creation of this directory is not necessary
	
	//create all traffic sources for the simulation
	for (i=1; i<= GENERATORS; i++) {
		createTrafficSource(i);
	}

	//create links
	createDuplexLink(1, 2, "lnk01:1-2", "lnk02:2-1", 10.0 Mega, .01, 0, 0, 1, 2, 10 Mega);
	createDuplexLink(3, 4, "lnk03:2-3", "lnk04:3-2", 10.0 Mega, .01, 0, 0, 2, 3, 10 Mega);
	createDuplexLink(5, 6, "lnk05:3-4", "lnk06:4-3", 10.0 Mega, .01, 0, 0, 3, 4, 10 Mega);
	createDuplexLink(7, 8, "lnk07:4-5", "lnk08:5-4", 10.0 Mega, .01, 0, 0, 4, 5, 10 Mega);
	createDuplexLink(9, 10, "lnk09:5-6", "lnk10:6-5", 10.0 Mega, .01, 0, 0, 5, 6, 10 Mega);
	createDuplexLink(11, 12, "lnk11:2-7", "lnk12:7-2", 10.0 Mega, .01, 0, 0, 2, 7, 10 Mega);
	createDuplexLink(13, 14, "lnk13:7-8", "lnk14:8-7", 10.0 Mega, .01, 0, 0, 7, 8, 10 Mega);
	createDuplexLink(15, 16, "lnk15:8-9", "lnk16:9-8", 10.0 Mega, .01, 0, 0, 8, 9, 10 Mega);
	createDuplexLink(17, 18, "lnk17:9-4", "lnk18:4-9", 10.0 Mega, .01, 0, 0, 9, 4, 10 Mega);
	createDuplexLink(19, 20, "lnk19:9-5", "lnk20:5-9", 10.0 Mega, .01, 0, 0, 9, 5, 10 Mega);
	createDuplexLink(21, 22, "lnk21:3-10", "lnk22:10-3", 10.0 Mega, .01, 0, 0, 3, 10, 10 Mega);
	createDuplexLink(23, 24, "lnk23:10-5", "lnk24:5-10", 10.0 Mega, .01, 0, 0, 10, 5, 10 Mega);
	createDuplexLink(25, 26, "lnk25:3-8", "lnk26:8-3", 10.0 Mega, .01, 0, 0, 3, 8, 10 Mega);

	//create nodes
	for(i=1;i<=NODES;i++) {
		createNode(i);
	}

	//set event for control message at nodes
	//Note:  already done by default when nodes are created
	/*for(i=1;i<=NODES;i++) {
		setNodeCtrlMsgHandlEv(i, CTRL_MSG_ARRIVAL);
	}*/
	  
	//----- SCHEDULING EVENTS OF END OF SIMULATION AND OTHER TIMERS -----
	schedulep(END_SIMULATION, MAX_TIME, -1, NULL); //schedule END of simulation
	startTimers(TIMEOUT, REFRESH_LSP, HELLO_GEN); //start timers:  timeout and refresh
	schedulep(SET_BKP_LSP, BKP_LSP, -1, NULL); //build backup LSPs
	//schedulep(SET_LSP, SET_LSPs, -1, NULL); //build some LPSs with preemption
	schedulep(TRAFGEN_ON, START_TRAFFIC, -1, NULL); //restart traffic generation
	schedulep(BRING_LINK_DOWN, LINK_DOWN, -1, NULL); //schedule link failure event
	schedulep(BRING_LINK_UP, LINK_UP, -1, NULL); //schedule link re-establishment event
	//schedulep(RESET, RESET_TIME, -1, NULL); //schedule event to reset statistics
	//----- END OF SCHEDULING AND TIMERS -----

	//pkt=setLSP(1, 6, expRoute1, 1 Mega/8, 10000, 1 Mega/8, 0, 5000, 0, 0, 0);
	pkt=setLSP(1, 6, expRoute1, 100000, 1 Mega, 100000, 0, 5000, 0, 0, 0);
	LSPid[3]=pkt->lblHdr.LSPid; //obtain LSPid that were created for the generator
	/*setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 2,2, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 3,3, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 4,4, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	setLSP(1,6,expRoute1, 100000, 1 Mega, 100000, 0, 5000, 5,5, 0);
	*/

	while (!simEnd && !evChainIsEmpty()) {
		pkt = causep((int*)&eventType, &currentPacket);
		if ((int)simtime()%10==0)
			printf("simtime:  %lf\n", simtime());
		/* causep pops the next event from the event chain and returns the related/attached packet (that might contain a control message for internal simulation control) */

		switch (eventType) {
		case TRAFGEN_ON:
			//initiate traffic generation
			//initialize traffic sources (traffic generators) in the model
			//each source must generate an initial arrival, such that this initial arrival, or initial event, triggers new arrivals
			//
			//Parameters:
			//expooTrafficGeneratorLabel:  int ev, int n_src, int length, int src, int dst, double rate, double ton, double toff, int label
			//cbrTrafficGeneratorLabel:  int ev, int n_src, int length, int src, int dst, double rate, int label

			expooTrafficGeneratorLabel(EXPOO_1_ARRIVAL, expoo1_nscr, expoo1_length, expoo1_src, expoo1_dst, expoo1_rate, expoo1_ton, expoo1_toff, expoo1_label, 0, expoo1_prio);
			//expooTrafficGeneratorLabel(EXPOO_2_ARRIVAL, expoo2_nscr, expoo2_length, expoo2_src, expoo2_dst, expoo2_rate, expoo2_ton, expoo2_toff, expoo2_label, 0, expoo2_prio);
			//cbrTrafficGeneratorLabel(CBR_1_ARRIVAL, cbr1_nsrc, cbr1_length, cbr1_src, cbr1_dst, cbr1_rate, cbr1_label, LSPid[1], cbr1_prio);
			//cbrTrafficGeneratorLabel(CBR_2_ARRIVAL, cbr2_nsrc, cbr2_length, cbr2_src, cbr2_dst, cbr2_rate, cbr2_label, LSPid[2], cbr2_prio);
			//cbrTrafficGeneratorLabel(CBR_3_ARRIVAL, cbr3_nsrc, cbr3_length, cbr3_src, cbr3_dst, cbr3_rate, cbr3_label, LSPid[3], cbr3_prio);
			//cbrTrafficGeneratorLabel(CBR_4_ARRIVAL, cbr4_nsrc, cbr4_length, cbr4_src, cbr4_dst, cbr4_rate, cbr4_label, LSPid[4], cbr4_prio);
			break;

		case EXPOO_1_ARRIVAL:
			/* process the type of packet coming from the source; this event signals when a packet efectivelly leaves the source and arrives at the node pointed by currentNode. */
			
			//attachExplicitRoute(pkt, &expRoute1); //attaches the explicit route to this packet

			pkt->lblHdr.label=getWorkingLSPLabel(pkt->currentNode, LSPid[3]); //puts the initial label in the packet
			
			//If the policer is desired, remove the following 2 lines and reativate (uncomment) the policer below
			//-------------
			//schedules event (packet) to be processed by queue/server of the facility; the facility, here, is the outgoing link
			schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);
			nodeReceivePacket(pkt); //updates node statistics
			//-------------
			
			/*if (applyPolicer(pkt)) { /* if ==1, packet conforms; schedule transmission.
										  if ==0, packet does not conform and was already discarded.  No need to treat it anymore. 
				//schedules event (packet) to be processed by queue/server of the facility; the facility, here, is the outgoing link
				schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);
				nodeReceivePacket(pkt); //updates node statistics
			}*/
			
			/* generates new packet for source 1 and schedules it (puts event in event chain) */
			//int ev, int n_src, int length, int src, int dst, double rate, double ton, double toff, int label
			expooTrafficGeneratorLabel(EXPOO_1_ARRIVAL, expoo1_nscr, expoo1_length, expoo1_src, expoo1_dst, expoo1_rate, expoo1_ton, expoo1_toff, expoo1_label, 0, expoo1_prio);
			nodeReceivePacket(pkt); //updates node statistics
			break;

		case EXPOO_2_ARRIVAL:
			/* Processamento do tipo pacote saindo da fonte, ou seja ele indica quando um pacote efetivamente saira da fonte e ira
			para um nodo indicado pelo currentNode */
			
			//attachExplicitRoute(pkt, &expRoute1); //liga a rota explícita a este pacote

			/* Escalona o evento (pacote i) para ser processado pela fila/servidor da facility neste caso o link de saída */
			schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);
			
			/* gera um novo pacote para a fonte 2 e escalona na cadeia de eventos do simm */
			//int ev, int n_src, int length, int src, int dst, double rate, double ton, double toff, int label
			expooTrafficGeneratorLabel(EXPOO_2_ARRIVAL, expoo2_nscr, expoo2_length, expoo2_src, expoo2_dst, expoo2_rate, expoo2_ton, expoo2_toff, expoo2_label, 0, expoo2_prio);
			nodeReceivePacket(pkt); //atualiza estatísticas do nodo
			break;

		case CBR_1_ARRIVAL:
			/* Processamento do tipo pacote saindo da fonte, ou seja ele indica quando um pacote efetivamente saira da fonte e ira
			para um nodo indicado pelo currentNode */
			
			attachExplicitRoute(pkt, expRoute1); //liga a rota explícita a este pacote

			/* Escalona o evento (pacote i) para ser processado pela fila/servidor da facility neste caso o link de saída */
			schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);

			/* gera um novo pacote para a fonte 3 e escalona na cadeia de eventos do simm */
			//int ev, int n_src, int length, int src, int dst, double rate, int label
			if (pktCounter != MAX_PKTS) {
				cbrTrafficGeneratorLabel(CBR_1_ARRIVAL, cbr1_nsrc, cbr1_length, cbr1_src, cbr1_dst, cbr1_rate, cbr1_label, LSPid[1], cbr1_prio);
				pktCounter++;
			}
			nodeReceivePacket(pkt); //atualiza estatísticas do nodo
			break;

		case CBR_2_ARRIVAL:
			/* Processamento do tipo pacote saindo da fonte, ou seja ele indica quando um pacote efetivamente saira da fonte e ira
			para um nodo indicado pelo currentNode */
			
			//attachExplicitRoute(pkt, &expRoute1); //liga a rota explícita a este pacote

			/* Escalona o evento (pacote i) para ser processado pela fila/servidor da facility neste caso o link de saída */
			schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);

			/* gera um novo pacote para a fonte 4 e escalona na cadeia de eventos do simm */
			//int ev, int n_src, int length, int src, int dst, double rate, int label
			cbrTrafficGeneratorLabel(CBR_2_ARRIVAL, cbr2_nsrc, cbr2_length, cbr2_src, cbr2_dst, cbr2_rate, cbr2_label, LSPid[2], cbr2_prio);
			nodeReceivePacket(pkt); //atualiza estatísticas do nodo
			break;

		case CBR_3_ARRIVAL:
			/* Processamento do tipo pacote saindo da fonte, ou seja ele indica quando um pacote efetivamente saira da fonte e ira
			para um nodo indicado pelo currentNode */
			
			//attachExplicitRoute(pkt, &expRoute1); //liga a rota explícita a este pacote
			pkt->lblHdr.label=getWorkingLSPLabel(pkt->currentNode, LSPid[3]); //coloca o label inicial no pacote
			
			if (applyPolicer(pkt)) { //se ==1, pacote está conforme; escalone transmissão.  se ==0, pacote está não-conforme e foi descartado
				/* Escalona o evento (pacote i) para ser processado pela fila/servidor da facility neste caso o link de saída */
				schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);
				/* gera um novo pacote para a fonte 4 e escalona na cadeia de eventos do simm */
				nodeReceivePacket(pkt); //atualiza estatísticas do nodo
			}
			//a nova geração SEMPRE tem de ocorrer, senão a fonte será interrompida
			cbrTrafficGeneratorLabel(CBR_3_ARRIVAL, cbr3_nsrc, cbr3_length, cbr3_src, cbr3_dst, cbr3_rate, label, LSPid[3], cbr3_prio);
			break;

		case CBR_4_ARRIVAL:
			/* Processamento do tipo pacote saindo da fonte, ou seja ele indica quando um pacote efetivamente saira da fonte e ira
			para um nodo indicado pelo currentNode */
			
			//attachExplicitRoute(pkt, &expRoute1); //liga a rota explícita a este pacote
			label=getWorkingLSPLabel(pkt->currentNode, LSPid[4]);
			if (applyPolicer(pkt)) { //se ==1, pacote está conforme; escalone transmissão
				/* Escalona o evento (pacote i) para ser processado pela fila/servidor da facility neste caso o link de saída */
				schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);

				/* gera um novo pacote para a fonte 4 e escalona na cadeia de eventos do simm */
				//cbrTrafficGeneratorLabel(CBR_4_ARRIVAL, cbr4_nsrc, cbr4_length, cbr4_src, cbr4_dst, cbr4_rate, cbr4_label);
				nodeReceivePacket(pkt); //atualiza estatísticas do nodo
			}
			//a nova geração SEMPRE tem de ocorrer, senão a fonte será interrompida
			cbrTrafficGeneratorLabel(CBR_4_ARRIVAL, cbr4_nsrc, cbr4_length, cbr4_src, cbr4_dst, cbr4_rate, label, LSPid[4], cbr4_prio);
			break;

		case LINK_TRANSMIT_REQUEST:
			/* Situacao de envio de pacote por um link a ser determinado. Tecnicamente colocacao em servico de um
			pacote em uma facility link*/
		
			//define para que link de saída o pacote neste nodo deverá ser encaminhado. Esta funcao gera o pkt->outgoingLink
			if (pkt->er.explicitRoute==NULL) { //não há rota explícita:  use a LIB
				if (decidePathMpls(pkt)==0) //decide com base no MPLS; se ==1, rota não existe, portanto não transmita
					linkBeginTransmitPacket(LINK_PROPAGATE, pkt);
			} else { //explicitRoute != NULL; então, há rota explícita.  Use-a.
				if (decidePathER(pkt)==0) //decide com base na rota explícita contida no próprio pacote; se ==1, rota não existe, portanto não transmita
					linkBeginTransmitPacket(LINK_PROPAGATE, pkt);
			}
			/* Encaminha o pacote para o link de saida, ou seja, escalona-o na cadeia de eventos do simm com o tempo que
			terminara o servico ou entao o coloca na fila de espera da facility */
			break;

		case LINK_PROPAGATE:
			/* Liberação do serviço de transmissão do pacote.  Este evento deve necessariamente encadear para o evento
			   linkPropagate relacionado.  Observar que o pacote ainda não chegou no próximo nodo, mas apenas foi
			   transmitido para o link e agora deve ser propagado.
			*/
			//Aqui a expressão tarvosModel.lnk[pkt->outgoingLink].facility estava sendo resolvida para ZERO no Borland C++BuilderX.
			//O problema estava acontecendo devido à definição das variáveis globais do tipo struct quando da própria declaração da
			//struct, no próprio arquivo .h (struct something {...} some[10];).
			//Como o arquivo .h era incluído em todos os módulos, havia múltipla definição das variáveis globais, causando
			//confusão para o linker.  O problema foi resolvido separando as declarações das structs, que permaneceram nos
			//arquivos .h incluídos em todos os módulos, das definições das variáveis globais do tipo struct, que foram incluídas
			//somente no módulo principal (através de diretivas de pré-processamento) e colocadas como extern nos outros módulos
			linkEndTransmitPacket(tarvosModel.lnk[pkt->outgoingLink].facility, currentPacket); //Finaliza a transmissão
			linkPropagatePacket(ARRIVAL_NODE, pkt);  //introduz o atraso de propagação (um simples schedulep)
			
			//Isto abaixo foi incluso na função linkPropagatePacket
			//pkt->currentNode = tarvosModel.lnk[pkt->outgoingLink].dst; //atualiza o pacote para o nodo em que ele estara apos ser encaminhado pelo link
			break;
    	
		case ARRIVAL_NODE:
			/* Chegada de um pacote em um nodo.  Deve ser testado se o nodo é o destino do pacote, ou
			   se é um nodo intermediário.  Atualizar também aqui estatísticas pertinentes.
			*/

			/*aux é criada aqui para que as estatísticas sejam impressas no arquivo;
			*como a função nodeReceivePacket é chamada antes da impressão, e esta função
			*descarta o pacote (freePkt(pkt)), a rotina de impressão não funcionaria pois o pacote
			*não mais existe.  Após a impressão, aux é eliminada com free.
			*/
			//aux=malloc(sizeof *aux);
			//*aux=*pkt;
			if (nodeReceivePacket(pkt)==0) { //se retorno ==0, o pacote deve ser encaminhado para transmissão
				//Se retorno ==1, significa que o destino do pacote é o nodo atual.
				//Se retorno ==2, significa que o pacote foi descartado pelo nodo (tipicamente, estouro do TTL).
				//sprintf(mainTraceString, "Simtime:  %f  Nodo:  %d  pktID:  %d  Size:  %d  TTL:  %d  Pacotes que chegaram:  %d    Atraso:  %f\n", simtime(), aux->currentNode, aux->id, aux->length, aux->ttl, tarvosModel.node[aux->dst].packetsReceived, simtime()-aux->generationTime);
				//mainTrace(mainTraceString);
				schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt); //Escalona o evento (pacote i) para ser processado pela fila/servidor da facility (ou seja, transmitido para o próximo nodo)
			}
			//free(aux);
			break;

		case CTRL_MSG_ARRIVAL:
			/* Processamento de mensagens de controle do protocolo RSVP-TE */
			//aux=malloc(sizeof *aux);
			//*aux=*pkt;
			if (nodeReceivePacket(pkt)==0)
				schedulep(LINK_TRANSMIT_REQUEST, 0, currentPacket, pkt);
			//jitterDelayTrace(aux->currentNode, simtime(), tarvosModel.node[aux->currentNode].jitter, tarvosModel.node[aux->currentNode].delay);
			//free(aux);
			break;

		case BRING_LINK_UP:
			setDuplexLinkUp(3);
			break;

		case BRING_LINK_DOWN:
			sprintf(mainTraceString, "Packets in Transit Queue at this moment simtime(): %f  packets: %d\n", simtime(), inq(tarvosModel.lnk[3].facility));
			mainTrace(mainTraceString);
			setDuplexLinkDown(3);
			break;

		case END_SIMULATION:
			simEnd=1;  //encerre agora a simulação
			break;

		case TIMEOUT:
			timeoutWatchdog();
			schedulep(TIMEOUT, tarvosParam.timeoutWatchdog, -1, NULL);
			break;

		case REFRESH_LSP:
			refreshLSP();
			schedulep(REFRESH_LSP, tarvosParam.LSPrefreshInterval, -1, NULL);
			break;

		case HELLO_GEN:
			generateHello();
			schedulep(HELLO_GEN, tarvosParam.helloInterval, -1, NULL);
			break;

		case RESET:
			statReset();
			break;

		case SET_BKP_LSP:
			setBackupLSP(LSPid[3], 2, 6, expRoute2);
			setBackupLSP(LSPid[3], 3, 6, expRoute3);
			setBackupLSP(LSPid[3], 3, 6, expRoute4);
			break;

		case SET_LSP:
			setLSP(1,6,expRoute1, 100000, 3 Mega, 100000, 0, 5000, 2,2, 1);
			setLSP(2,6,expRoute2, 100000, 2.1 Mega, 100000, 0, 5000, 2,2, 1);
			break;

		} //end switch-case
	} //end while
	report();
	fprintf(fp,"Modelo: %s\n", mname());
	fprintf(fp,"SEED utilizada: %d\n", stream(0));
	for (i=1; i<=LINKS; i++) {
		fprintf(fp,"Maxqueue facility %d: %d\n", i, getFacMaxQueueSize(i));
	}
	for (i=1; i<=GENERATORS; i++) {
		fprintf(fp,"Pacotes que foram gerados fonte %d:  %d\n", i, tarvosModel.src[i].packetsGenerated);
	}
	for (i=1; i<=NODES; i++) {
		fprintf(fp,"Pacotes received no nodo %d:  %d\n", i, tarvosModel.node[i].packetsReceived);
		fprintf(fp,"..Pacotes forwarded no nodo %d:  %d\n", i, tarvosModel.node[i].packetsForwarded);
		fprintf(fp,"..Bytes forwarded no nodo %d:  %f\n", i, tarvosModel.node[i].bytesForwarded);
		fprintf(fp,"..Pacotes perdidos no nodo %d:  %d\n", i, tarvosModel.node[i].packetsDropped);
		fprintf(fp,"..Bytes recebidos no nodo %d:  %f\n", i, tarvosModel.node[i].bytesReceived);
		fprintf(fp,"..Throughput no nodo %d:  %f (bytes/seg)\n", i, tarvosModel.node[i].bytesReceived/simtime());
		fprintf(fp,"..Ultimo delay no nodo %d:  %f (s)\n", i, tarvosModel.node[i].delay);
		fprintf(fp,"..mean Delay nodo %d:  %f (s)\n", i, tarvosModel.node[i].meanDelay);
		fprintf(fp,"..Ultimo jitter nodo %d:  %f (s)\n", i, tarvosModel.node[i].jitter);
		fprintf(fp,"..mean Jitter nodo %d:  %f (s)\n", i, tarvosModel.node[i].meanJitter);
	}
	for (i=1;i<=LINKS;i++) {
		fprintf(fp,"Pacotes perdidos na facility %d:  %d\n", i, getFacDropTokenCount(tarvosModel.lnk[i].facility));
	}
	for (i=1; i<=LINKS; i++) {
		fprintf(fp,"Pacotes em Transito link %d: %d\n", i, getPktInTransitQueueSize(i));
	}
	dumpLIB(tarvosParam.libDump);
	dumpLSPTable(tarvosParam.lspTableDump);
	dumpLinks(tarvosParam.linksDump);
	system("PAUSE");
}


