/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_policer
*
* Rotinas para reserva e manuten��o de reservas de recursos do protocolo RSVP, como o Token Bucket
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
#include "simm_globals.h" //alguma fun��o local usa fun��es do kernel do simulador, como simtime()

//Prototypes das fun��es static locais
static void tokenBucket(struct LSPTableEntry *lsp);

/* TOKEN BUCKET
*
*  Esta fun��o implementa o algoritmo de Token Bucket.  Os par�metros a tratar s�o:
*    _cir:  Committed Information Rate, taxa em bytes por segundo de enchimento do Bucket
*    _cbs:  Committed Bucket Size, tamanho m�ximo do Bucket em bytes
*    _pir:  Peak Information Rate, taxa m�xima em bytes por segundo (atualmente n�o utilizada)
*    _currentTime:  Tempo atual de simula��o
*    _lastTime:  �ltimo tempo usado pelo Bucket
*    _cBucket:  Committed Bucket size, tamanho atual do Bucket em bytes
*
*  O Token Bucket permite que o n�mero m�ximo de bytes seja transmitido, de acordo com a equa��o abaixo:
*    max(cir * (currentTime - lastTime) + cBucket; cbs) (O tamanho � o teto entre o c�lculo indicado e o tamanho cbs)
*  A fun��o retorna o tamanho atualizado do Bucket.  A fun��o que chama dever� testar se este n�mero � maior ou igual ao n�mero de bytes
*  a transmitir:  se for positivo, ent�o o pacote � dito conforme; caso contr�rio, � considerado n�o conforme e deve-se tomar a medida
*  apropriada (descartar o pacote, por exemplo).
*  Se for conforme, o Bucket dever� ser reduzido do tamanho do pacote.  Se for n�o conforme, o Bucket n�o � alterado.
*/
static void tokenBucket(struct LSPTableEntry *lsp) {
	double now, maxConformSize; //tamanho m�ximo de bytes a ser considerado "conforme"

	now=simtime();
	maxConformSize = lsp->cir * (now - lsp->arrivalTime) + lsp->cBucket;
	if (maxConformSize > lsp->cbs) //se o novo tamanho superar cbs, ent�o considere cbs como o novo valor (bucket n�o deve superar cbs)
		maxConformSize=lsp->cbs;
	lsp->cBucket = (maxConformSize < 0)? (0):(maxConformSize); //atualiza tamanho corrente do cBucket, sendo ZERO o valor m�nimo
	lsp->arrivalTime=now; //atualiza �ltima marca��o do rel�gio
}

/* APLICA A REGRA DO POLICER
*
*  O Policer aplica o algoritmo Token Bucket para o pacote passado, usando os par�metros da LSP indicada no pr�prio pacote para o c�lculo do
*  token bucket.
*  Se o pacote estiver conforme, a fun��o retorna 1; se estiver n�o-conforme, o pacote � descartado e a fun��o retorna 0.  A fun��o que chama
*  deve testar o retorno e tomar o cuidado de n�o requisitar a transmiss�o, se o pacote houver sido descartado por n�o-conformidade.
*  Se qualquer um dos par�metros CBS ou CIR forem zero, ent�o o Bucket nunca encher�.  Neste caso, considerar que o policer n�o deve ser aplicado.
*/
int applyPolicer(struct Packet *pkt) {
	struct LSPTableEntry *lsp;
	int length;


	if (pkt->lblHdr.msgID!=0) //pacote � de controle (msgID!=0); n�o aplique nenhum policer, pois este pacote n�o deve ser descartado
		return 1;

	lsp=searchInLSPTable(pkt->lblHdr.LSPid);
	if (lsp==NULL)
		return 1; //se n�o houver LSPid v�lido para o pacote, considere-o "conforme" e retorne
	if (lsp->cbs == 0 || lsp->cir == 0)
		return 1; //se o CBS ou CIR forem zero, ent�o o Bucket nunca encher� (qualquer pacote seria n�o-conforme); n�o aplique o policer, neste caso
	if (pkt->length <= lsp->maxPktSize || lsp->maxPktSize <= 0) { //tamanho do pacote n�o supera o tamanho m�ximo de conformidade, ou o tamanho m�ximo � <= 0; aplique o policer
		tokenBucket(lsp); //aplica o algoritmo Token Bucket, atualizando o valor cBucket da LSP
		length = (pkt->length < lsp->minPolUnit)? (lsp->minPolUnit):(pkt->length); //se tamanho do pacote for menor que minPolUnit, use o valor m�nimo ao inv�s do tamanho real do pacote
		if (length <= lsp->cBucket) { //pacote est� conforme; atualize o cBucket e retorne
			lsp->cBucket -= length;
			return 1;
		}
	}
	//pacote n�o-conforme; descarte-o
	nodeDropPacket(pkt, "Non-conformant packet (RSVP)");
	return 0;
}
