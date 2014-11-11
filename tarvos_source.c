/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_source
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

/* Funcoes que serao usadas pelo programa de roteamento do nucleo para: . criacao . parametrizacao
. inicializacao . operacao das fontes */

/* CRIA AS FONTES - SOURCES
*
* Se o gerador user 1 gera um pacote o pacote  deve ter uma informacao do destino
* final e do proximo roteador para onde ele devera ir.
*
* Com a informacao do destino final, cada roteador tera que ter uma tabela de poss�veis caminhos
*
* As fontes serao criadas uma a uma onde isto pode ser feito de forma individualizada ou
* generalizada (por exemplo varias fontes com a mesma especificacao a criacao pode ser feita dentro de um for)
*
*/
void createTrafficSource(int n_src) {
	tarvosModel.src[n_src].packetsGenerated = 0;  //Inicializa n�mero de pacotes gerados pela fonte
	tarvosModel.src[n_src].expooAbsoluteTurnOffTime = 0;  //Inicializa o rel�gio de cada fonte, para uso dos geradores Expoo
}

/* Geracao para o Nucleo FONTES - SOURCES */

/* Especifica a geracao e escalonamento de pacotes gerados por diversas fontes
sejam baseadas em algum algoritmo ou a partir de arquivos de tracing */

/* O processamento no nucleo comeca no momento em que o pacote chega ao roteador
atraves do qual ele inicia o seu percurso no nucleo. Este roteador estara
identificado no respectivo pacote como src. E terminara no roteador que
estara vinculado ao sorvedouro deste pacote. Este roteador estara definido como dst na struct deste pacote */

/* Gerador de Tr�fego Exponencial (chegadas de Poisson)
*
*  gera um �nico pacote com m�dia de tempo tau, distribu�da exponencialmente, chamando
*  diretamente o escalonador (schedulep)
* 
*  a fun��o deve receber o tipo do evento (ev), o n�mero da fonte geradora de tr�fego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte est� ligada (src),
*  o nodo destino do tr�fego ou pacote (dst), e a m�dia de tempo de interchegada (tau) (tipicamente em seg)
*/
void expTrafficGenerator(int ev, int n_src, int length, int source, int dst, double tau, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte est� ligada
	   dst = destino do pacote
	   tau = m�dia do tempo de interchegada (tipicamente em segundos)
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	struct Packet *pkt;
	double nextArrival; //conter� o intervalo de tempo para a pr�xima chegada

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS
	pkt->lblHdr.priority=prio; //coloca a prioridade no pacote
	nextArrival=expntl(tau);
	pkt->generationTime=nextArrival+simtime(); //marca o tempo em que o pacote foi gerado
	
	//o escalonamento � feito diretamente aqui
	schedulep(ev, nextArrival, pkt->id, pkt);
	
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}

/* Gerador de Tr�fego CBR (Constant Bit Rate)
*
*  gera pacotes continuamente, com intervalo de tempo calculado segundo a taxa de bits por seg informada
*  o escalonador � chamado diretamente aqui (schedulep)
* 
*  a fun��o deve receber o tipo do evento (ev), o n�mero da fonte geradora de tr�fego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte est� ligada (src),
*  o nodo destino do tr�fego ou pacote (dst), e a taxa de gera��o em bits por segundo (rate)
*  o tempo de interevento � calculado segundo a f�rmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  � importante manter as unidades coerentes na constru��o do modelo, ou ent�o ajustar as f�rmulas
*/
void cbrTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte est� ligada
	   dst = destino do pacote
	   rate = taxa de gera��o em bits per second
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	double ie_t;
	struct Packet *pkt;

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	
	/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
	deste aqui que foi gerado agora */
	//O tempo interevento ser� o tamanho do pacote em bytes * 8 / taxa em bits por segundo
	ie_t=length*8.0/rate;
	pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(ev, ie_t, pkt->id, pkt);
	
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}

/* Gerador de Tr�fego Exponential On/Off
*
*  gera pacotes continuamente, com intervalo de tempo (interchegada) calculado segundo a taxa de bits por seg informada
*  durante o tempo ON, distribu�do exponencialmente com m�dia ton
*  permanece idle durante o tempo OFF, distribu�do exponencialmente com m�dia toff
*  o escalonador � chamado diretamente aqui (schedulep)
* 
*  a fun��o deve receber o tipo do evento (ev), o n�mero da fonte geradora de tr�fego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte est� ligada (src),
*  o nodo destino do tr�fego ou pacote (dst), o tempo ON (ton), o tempo OFF (toff),
*  e a taxa de gera��o em bits por segundo (rate)
*  o tempo de interevento para o per�odo ON � calculado segundo a f�rmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  � importante manter as unidades coerentes na constru��o do modelo, ou ent�o ajustar as f�rmulas
*/
void expooTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte est� ligada
	   dst = destino do pacote
	   rate = taxa de gera��o em bits por segundo enquanto ON
	   ton = m�dia de tempo ON (exponencial)
	   toff = m�dia de tempo OFF (exponencial)
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */

	double ie_t;
	double expooRelativeTurnOnTime; //tempo relativo em que o gerador deve ser ligado
	struct Packet *pkt;
	char traceString[255]; //string que conter� a linha de trace gerada

	/*
	sprintf(traceString, "expoo gen #%d - ton (mean=%f), toff (mean=%f)\n", n_src, ton, toff);
	expooSourceTrace(traceString);
	*/

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte est� vinculada; isto permite que v�rias fontes gerem para o mesmo roteador (fontes com tipos de gera��o diferentes)
	pkt->dst = dst;  //Nodo ao qual o sorvedouro est� vinculado		
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	if (simtime() < tarvosModel.src[n_src].expooAbsoluteTurnOffTime) { //Se simtime() n�o houver ainda atingido o expooAbsoluteTurnOffTime, est� no per�odo BURST ou ON; gerar uma chegada
        		/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		deste aqui que foi gerado agora */
		//O tempo interevento ser� o tamanho do pacote em bytes * 8 / taxa em bits por segundo
		ie_t=length*8.0/rate;
		pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
		schedulep(ev, ie_t, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
		
		//linhas para gera��o de tracing
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+ie_t, pkt->id, length, source, source, dst); //debug
		sourceTrace(traceString);
	} else {
		//aqui, o simtime() superou o expooAbsoluteTurnOffTime; ent�o, entrar no per�odo IDLE ou OFF; gerar uma nova chegada
		//em expooRelativeTurnOnTime e um novo expooAbsoluteTurnOffTime
		expooRelativeTurnOnTime=expntl(toff); //a nova chegada ser� gerada ap�s um tempo toff;
		//observar que aqui pode haver um ligeiro acr�scimo de tempo entre expooAbsoluteTurnOffTime e o novo expooRelativeTurnOnTime
		tarvosModel.src[n_src].expooAbsoluteTurnOffTime=expntl(ton)+expooRelativeTurnOnTime+simtime(); //aqui o simtime() deve ser acrescido, pois expooAbsoluteTurnOffTime deve representar o tempo absoluto
		pkt->generationTime=expooRelativeTurnOnTime+simtime(); //marca o tempo em que o pacote foi gerado
		/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		deste aqui que foi gerado agora */
		
		//linhas para gera��o de tracing
		sprintf(traceString, "-----expoo gen #%d - simtime: %f expooRelativeTurnOnTime: %f sim+turn_on: %f expooAbsoluteTurnOffTime: %f\n", n_src, simtime(), expooRelativeTurnOnTime, simtime()+expooRelativeTurnOnTime, tarvosModel.src[n_src].expooAbsoluteTurnOffTime); //debug
		sourceTrace(traceString);
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+expooRelativeTurnOnTime, pkt->id, length, source, source, dst);
		sourceTrace(traceString);
		
		schedulep(ev, expooRelativeTurnOnTime, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source

		//Gera arquivo para validacao do gerador exponencial on/off
		sprintf(traceString, "expoo gen #%d - %f, %f\n", n_src, tarvosModel.src[n_src].expooAbsoluteTurnOffTime-(expooRelativeTurnOnTime+simtime()), expooRelativeTurnOnTime);
		expooSourceTrace(traceString);
	}
}

/* Gerador de Tr�fego Exponential On/Off com R�tulo (label) para MLPS
*
*  gera pacotes continuamente, com intervalo de tempo (interchegada) calculado segundo a taxa de bits por seg informada
*  durante o tempo ON, distribu�do exponencialmente com m�dia ton
*  permanece idle durante o tempo OFF, distribu�do exponencialmente com m�dia toff
*  o escalonador � chamado diretamente aqui (schedulep)
*
*  A estrat�gia de implementa��o � a seguinte:
*  A fonte exponencial on/off gera pacotes como uma fonte CBR durante um per�odo de tempo ON ou BURST, e permanece desligada
*  durante um per�odo de tempo OFF ou IDLE.  Estes dois per�odos de tempo s�o distribu�dos segundo a Exponencial.
*  A rotina abaixo usa duas vari�veis para controlar o liga/desliga do gerador exponencial on/off:
*
*  expooRelativeTurnOnTime especifica o per�odo de tempo idle ou off, durante o qual a fonte dever� permanecer desligada e ao
*  final do qual, a fonte ser� ligada, iniciando a gera��o de pacotes.  TurnOnTime �, portanto, o "tempo para ligar".  O valor
*  � relativo, ou seja, � um intervalo de tempo que ser� passado diretamente para a fun��o escalonadora (schedulep).  O tempo
*  � obtido atrav�s da fun��o expntl com a m�dia toff.
*
*  tarvosModel.src[n_src].expooAbsoluteTurnOffTime, que � parte da estrutura de dados da fonte (src), armazena o tempo absoluto em que a 
*  fonte deve ser desligada.  Este tempo � obtido gerando-se um intervalo com a fun��o expntl com a m�dia ton e somando-se com
*  o rel�gio atual (simtime()) e tamb�m o intervalo ON gerado anteriormente.  Desta forma, a fonte ser� desligada no tempo
*  absoluto que � a soma do tempo ON + rel�gio atual (simtime()) + tempo OFF.  O tempo OFF � somado aqui porque tanto o tempo
*  ON quanto o tempo OFF s�o calculados no mesmo momento.  Desta forma, a fonte permanecer� desligada durante OFF e ligar�
*  exatamente na hora OFF + ON + rel�gio atual.  Este �ltimo valor � armazenado na estrutura de dados da fonte (src).
*
*  A fun��o testa se a hora atual (simtime()) � menor que o valor armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime;
*  se for, � porque a hora de desligar a fonte ainda n�o foi atingida.  Est�-se ent�o no per�odo ON e deve-se gerar pacotes
*  segundo a raz�o rate.  A fun��o calcula o tempo de interchegada e escalona uma nova chegada.
*  Se a hora atual (simtime()) for maior que o valor armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime, � porque a hora de
*  desligar o gerador chegou ou foi ultrapassada.  A fun��o ent�o calcular� o intervalo OFF e escalonar� uma chegada para o
*  final deste intervalo (durante este intervalo, logicamente n�o haver� chegadas desta fonte).  Calcular� tamb�m o intervalo
*  ON, durante o qual a fonte dever� permanecer gerando pacotes ao ser ligada.  O intervalo OFF calculado, somado com este
*  intervalo ON e com o rel�gio atual (simtime()) resultar� na hora absoluta em que a fonte deve ser novamente desligada.  Este
*  novo valor � portanto armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime.
*
*  O valor inicial de tarvosModel.src[n_src].expooAbsoluteTurnOffTime, quando da cria��o da fonte, � zero.  Em tese, o simtime() nunca ser�
*  menor que zero, portanto o gerador necessariamente entrar� primeiro na rotina de cria��o dos intervalos de tempo ON e OFF; mesmo
*  que hipoteticamente o simtime() comece menor que zero, eventualmente ao ser atualizado, simtime() superar� o valor inicial
*  de tarvosModel.src[n_src].expooAbsoluteTurnOffTime (que � zero) e a fun��o entrar� na rotina de cria��o dos per�odos ON e OFF.  Isso
*  garante que o gerador exponencial on/off n�o fique ligado infinitamente.
*
*
*  A fun��o deve receber o tipo do evento (ev), o n�mero da fonte geradora de tr�fego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte est� ligada (src),
*  o nodo destino do tr�fego ou pacote (dst), o tempo ON (ton), o tempo OFF (toff),
*  a taxa de gera��o em bits por segundo (rate) e o r�tulo (label) inicial designado pelo LER de ingresso
*  o tempo de interevento para o per�odo ON � calculado segundo a f�rmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  � importante manter as unidades coerentes na constru��o do modelo, ou ent�o ajustar as f�rmulas
*/
void expooTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int label, int LSPid, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte est� ligada
	   dst = destino do pacote
	   rate = taxa de gera��o em bits por segundo enquanto ON
	   ton = m�dia de tempo ON (exponencial)
	   toff = m�dia de tempo OFF (exponencial)
	   label = r�tulo inicial, que seria designado pelo LER de ingresso
	   LSPid = n�mero globalmente �nico do t�nel LSP
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	double ie_t;
	double expooRelativeTurnOnTime; //tempo (relativo) em que o gerador de trafego deve ser ligado
	struct Packet *pkt;
	char traceString[255];

	/*
	sprintf(traceString, "expoo gen #%d - ton (mean=%f), toff (mean=%f)\n", n_src, ton, toff);
	expooSourceTrace(traceString);
	*/

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length; 
	pkt->src = source;  //Nodo ao qual a fonte est� vinculada; isto permite que v�rias fontes gerem para o mesmo roteador (fontes com tipos de gera��o diferentes)
	pkt->dst = dst; // Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS
	pkt->lblHdr.label=label; //configura o r�tulo inicial
	pkt->lblHdr.LSPid = LSPid;
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	if (simtime() < tarvosModel.src[n_src].expooAbsoluteTurnOffTime) { //Se simtime() n�o houver ainda atingido o expooAbsoluteTurnOffTime, est� no per�odo BURST ou ON; gerar uma chegada
       	//O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		//deste aqui que foi gerado agora
		//O tempo interevento ser� o tamanho do pacote em bytes * 8 / taxa em bits por segundo
		ie_t=length*8.0/rate;
		pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
		schedulep(ev, ie_t, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source

		//linhas para gera��o de tracing
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+ie_t, pkt->id, length, source, source, dst); //debug
		sourceTrace(traceString);
	} else {
		//aqui, o simtime() superou o expooAbsoluteTurnOffTime; ent�o, entrar no per�odo IDLE ou OFF; gerar uma nova chegada
		//em expooRelativeTurnOnTime e um novo expooAbsoluteTurnOffTime
		expooRelativeTurnOnTime=expntl(toff); //a nova chegada ser� gerada ap�s um tempo toff;
		//observar que aqui pode haver um ligeiro acr�scimo de tempo entre expooAbsoluteTurnOffTime e o novo expooRelativeTurnOnTime
		tarvosModel.src[n_src].expooAbsoluteTurnOffTime=expntl(ton)+expooRelativeTurnOnTime+simtime(); //aqui o simtime() deve ser acrescido, pois expooAbsoluteTurnOffTime deve representar o tempo absoluto
		pkt->generationTime=expooRelativeTurnOnTime+simtime(); //marca o tempo em que o pacote foi gerado

		//linhas para gera��o de tracing
		sprintf(traceString, "-----expoo gen #%d - simtime: %f expooRelativeTurnOnTime: %f sim+turn_on: %f expooAbsoluteTurnOffTime: %f\n", n_src, simtime(), expooRelativeTurnOnTime, simtime()+expooRelativeTurnOnTime, tarvosModel.src[n_src].expooAbsoluteTurnOffTime); //debug
		sourceTrace(traceString);
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+expooRelativeTurnOnTime, pkt->id, length, source, source, dst);
		sourceTrace(traceString);
		
		schedulep(ev, expooRelativeTurnOnTime, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source

		//Gera arquivo para validacao do gerador exponencial on/off
		sprintf(traceString, "expoo gen #%d - %f, %f\n", n_src, tarvosModel.src[n_src].expooAbsoluteTurnOffTime-(expooRelativeTurnOnTime+simtime()), expooRelativeTurnOnTime);
		expooSourceTrace(traceString);
	}
}

/* Gerador de Tr�fego CBR (Constant Bit Rate) com Label (para MPLS)
*
*  gera pacotes continuamente, com intervalo de tempo calculado segundo a taxa de bits por seg informada
*  o escalonador � chamado diretamente aqui (schedulep)
* 
*  a fun��o deve receber o tipo do evento (ev), o n�mero da fonte geradora de tr�fego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte est� ligada (src),
*  o nodo destino do tr�fego ou pacote (dst), a taxa de gera��o em bits por segundo (rate) e o label inicial
*  o tempo de interevento � calculado segundo a f�rmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  � importante manter as unidades coerentes na constru��o do modelo, ou ent�o ajustar as f�rmulas
*/
void cbrTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, int label, int LSPid, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte est� ligada
	   dst = destino do pacote
	   rate = taxa de gera��o em bits per second
	   label = label inicial
	   LSPid = n�mero globalmente �nico do t�nel LSP
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	double ie_t;
	struct Packet *pkt;

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->lblHdr.label=label; //configura o r�tulo inicial
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	pkt->lblHdr.LSPid = LSPid;
	
	/* O escalonamento tem que ser aqui pois o pacote atual que est� sendo processado no main � diferente deste aqui que foi gerado agora */
	//O tempo interevento ser� o tamanho do pacote em bytes * 8 / taxa em bits por segundo
	ie_t=length*8.0/rate;
	pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado

	schedulep(ev, ie_t, pkt->id, pkt);
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}