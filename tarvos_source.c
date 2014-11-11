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
* Com a informacao do destino final, cada roteador tera que ter uma tabela de possíveis caminhos
*
* As fontes serao criadas uma a uma onde isto pode ser feito de forma individualizada ou
* generalizada (por exemplo varias fontes com a mesma especificacao a criacao pode ser feita dentro de um for)
*
*/
void createTrafficSource(int n_src) {
	tarvosModel.src[n_src].packetsGenerated = 0;  //Inicializa número de pacotes gerados pela fonte
	tarvosModel.src[n_src].expooAbsoluteTurnOffTime = 0;  //Inicializa o relógio de cada fonte, para uso dos geradores Expoo
}

/* Geracao para o Nucleo FONTES - SOURCES */

/* Especifica a geracao e escalonamento de pacotes gerados por diversas fontes
sejam baseadas em algum algoritmo ou a partir de arquivos de tracing */

/* O processamento no nucleo comeca no momento em que o pacote chega ao roteador
atraves do qual ele inicia o seu percurso no nucleo. Este roteador estara
identificado no respectivo pacote como src. E terminara no roteador que
estara vinculado ao sorvedouro deste pacote. Este roteador estara definido como dst na struct deste pacote */

/* Gerador de Tráfego Exponencial (chegadas de Poisson)
*
*  gera um único pacote com média de tempo tau, distribuída exponencialmente, chamando
*  diretamente o escalonador (schedulep)
* 
*  a função deve receber o tipo do evento (ev), o número da fonte geradora de tráfego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte está ligada (src),
*  o nodo destino do tráfego ou pacote (dst), e a média de tempo de interchegada (tau) (tipicamente em seg)
*/
void expTrafficGenerator(int ev, int n_src, int length, int source, int dst, double tau, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte está ligada
	   dst = destino do pacote
	   tau = média do tempo de interchegada (tipicamente em segundos)
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	struct Packet *pkt;
	double nextArrival; //conterá o intervalo de tempo para a próxima chegada

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS
	pkt->lblHdr.priority=prio; //coloca a prioridade no pacote
	nextArrival=expntl(tau);
	pkt->generationTime=nextArrival+simtime(); //marca o tempo em que o pacote foi gerado
	
	//o escalonamento é feito diretamente aqui
	schedulep(ev, nextArrival, pkt->id, pkt);
	
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}

/* Gerador de Tráfego CBR (Constant Bit Rate)
*
*  gera pacotes continuamente, com intervalo de tempo calculado segundo a taxa de bits por seg informada
*  o escalonador é chamado diretamente aqui (schedulep)
* 
*  a função deve receber o tipo do evento (ev), o número da fonte geradora de tráfego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte está ligada (src),
*  o nodo destino do tráfego ou pacote (dst), e a taxa de geração em bits por segundo (rate)
*  o tempo de interevento é calculado segundo a fórmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  É importante manter as unidades coerentes na construção do modelo, ou então ajustar as fórmulas
*/
void cbrTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte está ligada
	   dst = destino do pacote
	   rate = taxa de geração em bits per second
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	double ie_t;
	struct Packet *pkt;

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	
	/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
	deste aqui que foi gerado agora */
	//O tempo interevento será o tamanho do pacote em bytes * 8 / taxa em bits por segundo
	ie_t=length*8.0/rate;
	pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(ev, ie_t, pkt->id, pkt);
	
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}

/* Gerador de Tráfego Exponential On/Off
*
*  gera pacotes continuamente, com intervalo de tempo (interchegada) calculado segundo a taxa de bits por seg informada
*  durante o tempo ON, distribuído exponencialmente com média ton
*  permanece idle durante o tempo OFF, distribuído exponencialmente com média toff
*  o escalonador é chamado diretamente aqui (schedulep)
* 
*  a função deve receber o tipo do evento (ev), o número da fonte geradora de tráfego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte está ligada (src),
*  o nodo destino do tráfego ou pacote (dst), o tempo ON (ton), o tempo OFF (toff),
*  e a taxa de geração em bits por segundo (rate)
*  o tempo de interevento para o período ON é calculado segundo a fórmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  É importante manter as unidades coerentes na construção do modelo, ou então ajustar as fórmulas
*/
void expooTrafficGenerator(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte está ligada
	   dst = destino do pacote
	   rate = taxa de geração em bits por segundo enquanto ON
	   ton = média de tempo ON (exponencial)
	   toff = média de tempo OFF (exponencial)
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */

	double ie_t;
	double expooRelativeTurnOnTime; //tempo relativo em que o gerador deve ser ligado
	struct Packet *pkt;
	char traceString[255]; //string que conterá a linha de trace gerada

	/*
	sprintf(traceString, "expoo gen #%d - ton (mean=%f), toff (mean=%f)\n", n_src, ton, toff);
	expooSourceTrace(traceString);
	*/

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte está vinculada; isto permite que várias fontes gerem para o mesmo roteador (fontes com tipos de geração diferentes)
	pkt->dst = dst;  //Nodo ao qual o sorvedouro está vinculado		
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	if (simtime() < tarvosModel.src[n_src].expooAbsoluteTurnOffTime) { //Se simtime() não houver ainda atingido o expooAbsoluteTurnOffTime, está no período BURST ou ON; gerar uma chegada
        		/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		deste aqui que foi gerado agora */
		//O tempo interevento será o tamanho do pacote em bytes * 8 / taxa em bits por segundo
		ie_t=length*8.0/rate;
		pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
		schedulep(ev, ie_t, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
		
		//linhas para geração de tracing
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+ie_t, pkt->id, length, source, source, dst); //debug
		sourceTrace(traceString);
	} else {
		//aqui, o simtime() superou o expooAbsoluteTurnOffTime; então, entrar no período IDLE ou OFF; gerar uma nova chegada
		//em expooRelativeTurnOnTime e um novo expooAbsoluteTurnOffTime
		expooRelativeTurnOnTime=expntl(toff); //a nova chegada será gerada após um tempo toff;
		//observar que aqui pode haver um ligeiro acréscimo de tempo entre expooAbsoluteTurnOffTime e o novo expooRelativeTurnOnTime
		tarvosModel.src[n_src].expooAbsoluteTurnOffTime=expntl(ton)+expooRelativeTurnOnTime+simtime(); //aqui o simtime() deve ser acrescido, pois expooAbsoluteTurnOffTime deve representar o tempo absoluto
		pkt->generationTime=expooRelativeTurnOnTime+simtime(); //marca o tempo em que o pacote foi gerado
		/* O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		deste aqui que foi gerado agora */
		
		//linhas para geração de tracing
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

/* Gerador de Tráfego Exponential On/Off com Rótulo (label) para MLPS
*
*  gera pacotes continuamente, com intervalo de tempo (interchegada) calculado segundo a taxa de bits por seg informada
*  durante o tempo ON, distribuído exponencialmente com média ton
*  permanece idle durante o tempo OFF, distribuído exponencialmente com média toff
*  o escalonador é chamado diretamente aqui (schedulep)
*
*  A estratégia de implementação é a seguinte:
*  A fonte exponencial on/off gera pacotes como uma fonte CBR durante um período de tempo ON ou BURST, e permanece desligada
*  durante um período de tempo OFF ou IDLE.  Estes dois períodos de tempo são distribuídos segundo a Exponencial.
*  A rotina abaixo usa duas variáveis para controlar o liga/desliga do gerador exponencial on/off:
*
*  expooRelativeTurnOnTime especifica o período de tempo idle ou off, durante o qual a fonte deverá permanecer desligada e ao
*  final do qual, a fonte será ligada, iniciando a geração de pacotes.  TurnOnTime é, portanto, o "tempo para ligar".  O valor
*  é relativo, ou seja, é um intervalo de tempo que será passado diretamente para a função escalonadora (schedulep).  O tempo
*  é obtido através da função expntl com a média toff.
*
*  tarvosModel.src[n_src].expooAbsoluteTurnOffTime, que é parte da estrutura de dados da fonte (src), armazena o tempo absoluto em que a 
*  fonte deve ser desligada.  Este tempo é obtido gerando-se um intervalo com a função expntl com a média ton e somando-se com
*  o relógio atual (simtime()) e também o intervalo ON gerado anteriormente.  Desta forma, a fonte será desligada no tempo
*  absoluto que é a soma do tempo ON + relógio atual (simtime()) + tempo OFF.  O tempo OFF é somado aqui porque tanto o tempo
*  ON quanto o tempo OFF são calculados no mesmo momento.  Desta forma, a fonte permanecerá desligada durante OFF e ligará
*  exatamente na hora OFF + ON + relógio atual.  Este último valor é armazenado na estrutura de dados da fonte (src).
*
*  A função testa se a hora atual (simtime()) é menor que o valor armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime;
*  se for, é porque a hora de desligar a fonte ainda não foi atingida.  Está-se então no período ON e deve-se gerar pacotes
*  segundo a razão rate.  A função calcula o tempo de interchegada e escalona uma nova chegada.
*  Se a hora atual (simtime()) for maior que o valor armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime, é porque a hora de
*  desligar o gerador chegou ou foi ultrapassada.  A função então calculará o intervalo OFF e escalonará uma chegada para o
*  final deste intervalo (durante este intervalo, logicamente não haverá chegadas desta fonte).  Calculará também o intervalo
*  ON, durante o qual a fonte deverá permanecer gerando pacotes ao ser ligada.  O intervalo OFF calculado, somado com este
*  intervalo ON e com o relógio atual (simtime()) resultará na hora absoluta em que a fonte deve ser novamente desligada.  Este
*  novo valor é portanto armazenado em tarvosModel.src[n_src].expooAbsoluteTurnOffTime.
*
*  O valor inicial de tarvosModel.src[n_src].expooAbsoluteTurnOffTime, quando da criação da fonte, é zero.  Em tese, o simtime() nunca será
*  menor que zero, portanto o gerador necessariamente entrará primeiro na rotina de criação dos intervalos de tempo ON e OFF; mesmo
*  que hipoteticamente o simtime() comece menor que zero, eventualmente ao ser atualizado, simtime() superará o valor inicial
*  de tarvosModel.src[n_src].expooAbsoluteTurnOffTime (que é zero) e a função entrará na rotina de criação dos períodos ON e OFF.  Isso
*  garante que o gerador exponencial on/off não fique ligado infinitamente.
*
*
*  A função deve receber o tipo do evento (ev), o número da fonte geradora de tráfego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte está ligada (src),
*  o nodo destino do tráfego ou pacote (dst), o tempo ON (ton), o tempo OFF (toff),
*  a taxa de geração em bits por segundo (rate) e o rótulo (label) inicial designado pelo LER de ingresso
*  o tempo de interevento para o período ON é calculado segundo a fórmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  É importante manter as unidades coerentes na construção do modelo, ou então ajustar as fórmulas
*/
void expooTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, double ton, double toff, int label, int LSPid, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte está ligada
	   dst = destino do pacote
	   rate = taxa de geração em bits por segundo enquanto ON
	   ton = média de tempo ON (exponencial)
	   toff = média de tempo OFF (exponencial)
	   label = rótulo inicial, que seria designado pelo LER de ingresso
	   LSPid = número globalmente único do túnel LSP
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
	pkt->src = source;  //Nodo ao qual a fonte está vinculada; isto permite que várias fontes gerem para o mesmo roteador (fontes com tipos de geração diferentes)
	pkt->dst = dst; // Nodo ao qual o sorvedouro esta vinculado
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS
	pkt->lblHdr.label=label; //configura o rótulo inicial
	pkt->lblHdr.LSPid = LSPid;
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	if (simtime() < tarvosModel.src[n_src].expooAbsoluteTurnOffTime) { //Se simtime() não houver ainda atingido o expooAbsoluteTurnOffTime, está no período BURST ou ON; gerar uma chegada
       	//O escalonamento tem que ser aqui pois o pacote atual que esta sendo processado no main eh diferente
		//deste aqui que foi gerado agora
		//O tempo interevento será o tamanho do pacote em bytes * 8 / taxa em bits por segundo
		ie_t=length*8.0/rate;
		pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado
		schedulep(ev, ie_t, pkt->id, pkt);
		tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source

		//linhas para geração de tracing
		sprintf(traceString, "expoo gen #%d - simtime: %f pktID: %d pktLENGTH: %d pktACNODE: %d pktSRC: %d pktSINK: %d\n", n_src, simtime()+ie_t, pkt->id, length, source, source, dst); //debug
		sourceTrace(traceString);
	} else {
		//aqui, o simtime() superou o expooAbsoluteTurnOffTime; então, entrar no período IDLE ou OFF; gerar uma nova chegada
		//em expooRelativeTurnOnTime e um novo expooAbsoluteTurnOffTime
		expooRelativeTurnOnTime=expntl(toff); //a nova chegada será gerada após um tempo toff;
		//observar que aqui pode haver um ligeiro acréscimo de tempo entre expooAbsoluteTurnOffTime e o novo expooRelativeTurnOnTime
		tarvosModel.src[n_src].expooAbsoluteTurnOffTime=expntl(ton)+expooRelativeTurnOnTime+simtime(); //aqui o simtime() deve ser acrescido, pois expooAbsoluteTurnOffTime deve representar o tempo absoluto
		pkt->generationTime=expooRelativeTurnOnTime+simtime(); //marca o tempo em que o pacote foi gerado

		//linhas para geração de tracing
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

/* Gerador de Tráfego CBR (Constant Bit Rate) com Label (para MPLS)
*
*  gera pacotes continuamente, com intervalo de tempo calculado segundo a taxa de bits por seg informada
*  o escalonador é chamado diretamente aqui (schedulep)
* 
*  a função deve receber o tipo do evento (ev), o número da fonte geradora de tráfego (n_src),
*  o tamanho do pacote (length) (tipicamente em bytes), o nodo ao qual a fonte está ligada (src),
*  o nodo destino do tráfego ou pacote (dst), a taxa de geração em bits por segundo (rate) e o label inicial
*  o tempo de interevento é calculado segundo a fórmula
*    ie_t=length*8/rate
* 
*  sendo 'length' em bytes e 'rate' em bits/seg (bps)
*  É importante manter as unidades coerentes na construção do modelo, ou então ajustar as fórmulas
*/
void cbrTrafficGeneratorLabel(int ev, int n_src, int length, int source, int dst, double rate, int label, int LSPid, int prio) {
    /* ev = tipo de evento
	   n_src = numero da fonte
	   length = tamanho do pacote em bytes
	   source = nodo ao qual a fonte está ligada
	   dst = destino do pacote
	   rate = taxa de geração em bits per second
	   label = label inicial
	   LSPid = número globalmente único do túnel LSP
	   prio = prioridade para o fluxo (exatamente a mesma prioridade usada no requestp)
    */
	double ie_t;
	struct Packet *pkt;

	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte esta vinculada, isto permite que varias fontes estejam gerando para o mesmo roteador, fontes com tipos de geracao diferentes
	pkt->dst = dst; //Nodo ao qual o sorvedouro esta vinculado
	pkt->lblHdr.label=label; //configura o rótulo inicial
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS
	pkt->lblHdr.priority = prio; //coloca prioridade no pacote (dentro do header para Label)
	pkt->lblHdr.LSPid = LSPid;
	
	/* O escalonamento tem que ser aqui pois o pacote atual que está sendo processado no main é diferente deste aqui que foi gerado agora */
	//O tempo interevento será o tamanho do pacote em bytes * 8 / taxa em bits por segundo
	ie_t=length*8.0/rate;
	pkt->generationTime=ie_t+simtime(); //marca o tempo em que o pacote foi gerado

	schedulep(ev, ie_t, pkt->id, pkt);
	tarvosModel.src[n_src].packetsGenerated++;  //incrementa contador de pacotes gerados por este Source
}