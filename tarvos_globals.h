/*
* TARVOS Computer Networks Simulator
* File tarvos_globals
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

/* Este arquivo contém a especificação de constantes que serão usadas na simulação */

/* Neste arquivo definem-se variáveis globais que são usadas por toda a simulação: quando a variável é usada no main, ela
é declarada normalmente; caso seja usada em outros arquivos, ela tem que ser declarada como extern */

#pragma once


#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <conio.h>

#define TRUE 1  //boolean true
#define FALSE 0  //boolean true
#define Giga *1e9  //define o fator multiplicativo para Giga
#define Mega *1e6  //define o fator multiplicativo para Mega
#define Kilo *1e3  //define o fator multiplicativo para Kilo

#define MAX_EXPLICIT_LINKS 50 //número máximo de links para Explicit Routing
#define GENERATORS 6	// num de fontes que serao usadas nesta simulação
#define NODES 10	 // num de nodos (roteadores) que serao usados nesta simulação
#define LINKS 26 // num de links da simulação em questao. A conexão é simplex e representa um link entre dois roteadores
#define SEED 20  //define a semente inicial para o gerador de números aleatórios
#define MAX_TIME 200 //tempo máximo de simulação
#define MAX_PKTS 100000  //número máximo de packets ou clientes a gerar
#define LINK_DOWN 10.029 //tempo da ocorrência de queda de um link específico
#define LINK_UP 15 //tempo de reabilitação do link
#define START_TRAFFIC 5 //tempo de início da geração de tráfego
#define RESET_TIME 10 //tempo para reset das estatísticas (eliminar transitórios)
#define BKP_LSP 3 //tempo para iniciar construção de backup LSPs (deve-se aguardar para as working LSPs estarem totalmente completadas)
#define SET_LSPs 10 //tempo para construir algumas LSPs

/* Tipo do evento que será processado no simulador */
enum EventType {EXPOO_1_ARRIVAL=1, EXPOO_2_ARRIVAL, CBR_1_ARRIVAL, CBR_2_ARRIVAL, CBR_3_ARRIVAL, CBR_4_ARRIVAL, LINK_TRANSMIT_REQUEST,
	LINK_PROPAGATE, ARRIVAL_NODE, CTRL_MSG_ARRIVAL, REFRESH_LSP, HELLO_GEN, TIMEOUT, BRING_LINK_UP, BRING_LINK_DOWN, TRAFGEN_ON,
	END_SIMULATION, RESET, SET_BKP_LSP, SET_LSP};

//parâmetros dos geradores de tráfego
#define expoo1_nscr 1
#define expoo1_length 512
#define expoo1_src 1
#define expoo1_dst 6
#define expoo1_rate 64 Kilo
#define expoo1_ton 1.2
#define expoo1_toff 0.8
#define expoo1_label 1
#define expoo1_prio 0

#define expoo2_nscr 2
#define expoo2_length 128
#define expoo2_src 1
#define expoo2_dst 5
#define expoo2_rate 64 Kilo
#define expoo2_ton 1.2
#define expoo2_toff 0.8
#define expoo2_label 2
#define expoo2_prio 0

#define cbr1_nsrc 3
#define cbr1_length 32
#define cbr1_src 1
#define cbr1_dst 5
#define cbr1_rate 100 Kilo
#define cbr1_label 3
#define cbr1_resvbw 100 Kilo
#define cbr1_prio 0

#define cbr2_nsrc 4
#define cbr2_length 40
#define cbr2_src 1
#define cbr2_dst 6
#define cbr2_rate 700 Kilo
#define cbr2_label 4
#define cbr2_resvbw 400 Kilo
#define cbr2_prio 0

#define cbr3_nsrc 5
#define cbr3_length 512
#define cbr3_src 1
#define cbr3_dst 6
#define cbr3_rate 600 Kilo
#define cbr3_label 4
#define cbr3_resvbw 600 Kilo
#define cbr3_prio 0

#define cbr4_nsrc 6
#define cbr4_length 512
#define cbr4_src 1
#define cbr4_dst 5
#define cbr4_rate 300 Kilo
#define cbr4_label 4
#define cbr4_resvbw 100 Kilo
#define cbr4_prio 0

#include "tarvos_types.h"

/* Colocar as seguintes linhas no arquivo que contém a função main() no programa:
*
*	#define MAIN_MODULE //define as variáveis e estruturas, e não apenas as declara como extern
*	#include "simm_globals.h"
*	#include "tarvos_globals.h"
*
*	Via de regra, em outros arquivos do usuário que não contenham a função main(), adicionar as seguintes linhas:
*
*	#include "simm_globals.h" //em caso de serem usadas funções do Kernel do Tarvos
*	#include "tarvos_globals.h" //em caso de serem usadas funções do Shell 1 e/ou Shell 2 do Tarvos
*
*/

#ifdef MAIN_MODULE //se estiver definido, as estruturas e variáveis serão definidas, e não apenas declaradas como extern; somente o arquivo com a função main() deve conter uma linha tipo #define MAIN_MODULE

struct TarvosModel tarvosModel = {0};

/*struct TarvosParam {
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
*/

struct TarvosParam tarvosParam = {120,	//tamanho em bytes de uma mensagem de controle PATH
	120,								//tamanho em bytes de uma mensagem de controle RESV
	20,									//tamanho em bytes de uma mensagem de controle HELLO
	10,									//prioridade de uma mensagem de controle (PATH, RESV, HELLO, etc.); usada nas filas PQ do simulador (requestp)
	255,								//TTL (Time-To-Live inicial dos pacotes gerados)
	30,									//tempo default para timeout das mensagens de controle do RSVP-TE para um nodo (para que sejam descartadas da fila de mensagens do nodo)
	90,									//tempo default para timeout para as LSPs que partem de um nodo
	90,									//tempo default para timeout de recursos reservados para uma LSP
	30,									//tempo default para timeout de uma mensagem HELLO
	0.0175,								//tempo default limite para recebimento de um HELLO_ACK (o estouro deste indica falha na comunicação com o nodo)
	0.005,								//tempo entre verificações de timeout de mensagens de controle e LSPs para todos os nodos; tempo entre escalonamentos da rotina timeoutWatchdog
	CTRL_MSG_ARRIVAL,					//número do evento default para tratamento das mensagens de controle para um nodo
	HELLO_GEN,							//número do evento default especificamente para tratamento das mensagems HELLO para um nodo
	0.005,								//intervalo de tempo para geração das mensagens HELLO a partir de um LSR
	30,									//intervalo de tempo para geração de mensagens PATH refresh para as LSPs
	30,									//intervalo de tempo para geração de mensagens RESV refresh para as LSPs
	0,									//flag traceMain; 0 para OFF, 1 para ON
	1,									//flag traceDrop;
	0,									//flag traceSource;
	0,									//flag traceExpoo;
	1,									//flag traceJitterDelayGlobal
	1,									//flag traceJitterDelayAppl
	"lib.txt",							//nome do arquivo contendo a LIB para MPLS (para leitura do simulador)
	"sourcetrace.txt",					//nome do arquivo que conterá o dump de certas fontes (para debug)
	"expootrace.txt",					//nome do arquivo que conterá o dump das fontes exponenciais on/off (para debug)
	"traceall.txt",						//nome do arquivo que conterá o trace principal
	"libdump.txt",						//nome do arquivo que conterá o conteúdo da LIB, impresso pelo programa
	"lsptabledump.txt",					//nome do arquivo que conterá o conteúdo da LSP Table
	"linksdump.txt",					//nome do arquivo que conterá a impressão dos parâmetros dos links
	"dropPktTrace.txt",					//nome do arquivo que conterá o trace dos pacotes descartos
	"stats\\delay_node%02d.xls",		//nome dos arquivos base que conterão as medições Delay de cada nodo
	"stats\\jitter_node%02d.xls",		//nome dos arquivos base que conterão as medições Jitter de cada nodo
	"stats\\Appldelay_node%02d.xls",	//nome dos arquivos base que conterão as medições Delay de cada nodo para Aplicações
	"stats\\Appljitter_node%02d.xls"};	//nome dos arquivos base que conterão as medições Jitter de cada nodo para Aplicações

#else //arquivo não contém a função main(): declarar estruturas como extern

extern struct TarvosParam tarvosParam; //parâmetros para o TARVOS
extern struct TarvosModel tarvosModel;

#endif

/* --------- Prototypes de funções usadas no Computer Networks Simulator -----------------*/
struct Packet *createPacket();
void freePkt(struct Packet *pkt);
void createTrafficSource (int n_src);
void expTrafficGenerator(int ev, int n_src, int length, int source, int dst, double tau, int prio);
int nodeReceivePacket(struct Packet *pkt);
void nodeIncDroppedPacketsNumber(int nodeNumber);
void nodeDropPacket(struct Packet *pkt, char *dropReason);
void createSimplexLink(int linkNumber, char *name, double bandwidth, double delay, double delayOther1, double delayOther2, int source, int dst, double maxCbs);
void createDuplexLink(int linkNumberSrcDst, int linkNumberDstSrc, char *nameSrcDst, char *nameDstSrc, double bandwidth, double delay, double delayOther1, double delayOther2, int source, int dst, double maxCbs);
void dumpLinks (char *outfile);
int linkBeginTransmitPacket(int ev, struct Packet *pkt);
int linkBeginTransmitPacketPreempt(int ev, struct Packet *pkt);
void linkPropagatePacket(int ev, struct Packet *pkt);
void linkEndTransmitPacket (int facility, int currentPacket);
void setSimplexLinkDown(int linkNumber);
void setSimplexLinkUp(int linkNumber);
void setDuplexLinkDown(int linkNumber);
void setDuplexLinkUp(int linkNumber);
char *getLinkStatus(int linkNumber);
int getPktInTransitQueueSize(int linkNumber);
void removePktFromTransitQueue(int linkNumber, int pktId);
void createNode(int n_node);
void insertInNodeMsgQueue(int n_node, char *msgType, int msgID, int msgIDack, int LSPid, int er[], int erIndex, int source, int dst, double timeout, int ev, int iIface, int oIface, int iLabel, int oLabel, int resourcesReserved);
struct nodeMsgQueue *searchInNodeMsgQueueLSPid(int n_node, int LSPid);
struct nodeMsgQueue *searchInNodeMsgQueueAck(int n_node, int msgIDack);
void removeFromNodeMsgQueueLSPid(int n_node, int LSPid);
void removeFromNodeMsgQueueAck(int n_node, int msgIDack);
void setNodeCtrlMsgTimeout(int n_node, double timeout);
double getNodeCtrlMsgTimeout(int n_node);
void setNodeCtrlMsgHandlEv(int n_node, int ev);
int getNodeCtrlMsgHandlEv(int n_node);
void setNodeLSPTimeout(int n_node, double timeout);
double getNodeLSPTimeout(int n_node);
void decidePathStaticRoute(struct Packet *pkt);
void cbrTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, int prio);
void expooTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int prio);
void buildLIBTableFromFile(char *libFile);
void buildLIBTable();
void insertInLIB(int node, int iIface, int iLabel, int oIface, int oLabel, int LSPid, char *status, int bak, double timeout, double timeoutStamp);
struct Packet *createPathLabelControlMsg(int source, int dst, int er[], int LSPid);
struct Packet *createResvMapControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label);
int getWorkingLSPLabel(int n_node, int LSPid);
int decidePathMpls(struct Packet *pkt);
int decidePathER(struct Packet *pkt);
int findLink(int source, int dst);
void dumpLIB(char *outfile);
int getLIBSize();
struct LIBEntry *searchInLIB(int node, int iIface, int iLabel);
struct LIBEntry *searchInLIBnodLSPstat(int node, int LSPid, char *status);
int getLSPTableSize();
struct LSPTableEntry *searchInLSPTable(int LSPid);
void dumpLSPTable(char *outfile);
struct Packet *setLSP(int source, int dst, int er[], double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio, int preempt);
void expooTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int label, int LSPid, int prio);
void cbrTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, int label, int LSPid, int prio);
void attachExplicitRoute(struct Packet *pkt, int er[]);
int *invertExplicitRoute(int route[], int size);
void mainTrace(char *entry);
void dropPktTrace(char *entry);
void sourceTrace(char *entry);
void expooSourceTrace(char *entry);
int applyPolicer(struct Packet *pkt);
int reserveResouces(int LSPid, int link);
double getLSPcbs(int LSPid);
double getLSPcir(int LSPid);
double getLSPpir(int LSPid);
int getLSPminPolUnit(int LSPid);
int getLSPmaxPktSize(int LSPid);
double getLSPcBucket(int LSPid);
void timeoutWatchdog();
void recordRoute(struct Packet *pkt);
void refreshLSP();
struct Packet *createPathRefreshControlMsg(int source, int dst, int LSPid, int iLabel);
struct Packet *createResvRefreshControlMsg(int source, int dst, int er[], int msgIDack, int LSPid);
void startTimers(int timeoutEv, int refreshEv, int helloEv);
void generateHello();
void setNodeHelloMsgTimeout(int n_node, double timeout);
double getNodeHelloMsgTimeout(int n_node);
void setNodeHelloMsgGenEv(int n_node, int ev);
int getNodeHelloMsgGenEv(int n_node);
struct Packet *createHelloControlMsg(int source, int dst);
struct Packet *createHelloAckControlMsg(int source, int dst, int er[], int msgIDack);
void setNodeHelloTimeout(int n_node, double timeout);
double getNodeHelloTimeout(int n_node);
struct LIBEntry *searchInLIBStatus(int node, int iIface, int iLabel, char *status);
struct Packet *setBackupLSP(int LSPid, int sourceMP, int dstMP, int er[]);
void statReset();
struct Packet *createPathDetourControlMsg(int sourceMP, int dstMP, int er[], int LSPid);
struct Packet *createResvDetourControlMsg(int sourceMP, int dstMP, int er[], int msgIDack, int LSPid, int oLabel);
struct LIBEntry *searchInLIBnodLSPstatoIface(int node, int LSPid, char *status, int oIface);
int getLSPtunnelStatus(int LSPid);
int setLSPtunnelDone(int LSPid);
int setLSPtunnelNotDone(int LSPid);
struct LIBEntry *searchInLIBnodLSPstatBak(int node, int LSPid, char *status);
void jitterDelayTrace(int node, double stime, double jitter, double delay);
void jitterDelayApplTrace(int node, double stime, double jitter, double delay);
struct Packet *createPathErrControlMsg(int source, int dst, int LSPid, char *errorCode, char *errorValue);
struct Packet *createResvErrControlMsg(int source, int dst, int LSPid, int iLabel, char *errorCode, char *errorValue);
struct Packet *createPathPreemptControlMsg(int source, int dst, int er[], int LSPid);
struct Packet *createResvPreemptControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label);
int preemptResouces(int LSPid, int node, int link);
int testResources(int LSPid, int node, int link);
void about(FILE *fp);