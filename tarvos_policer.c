/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_policer
*
* Rotinas para reserva e manutenção de reservas de recursos do protocolo RSVP, como o Token Bucket
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
#include "simm_globals.h" //alguma função local usa funções do kernel do simulador, como simtime()

//Prototypes das funções static locais
static void tokenBucket(struct LSPTableEntry *lsp);

/* TOKEN BUCKET
*
*  Esta função implementa o algoritmo de Token Bucket.  Os parâmetros a tratar são:
*    _cir:  Committed Information Rate, taxa em bytes por segundo de enchimento do Bucket
*    _cbs:  Committed Bucket Size, tamanho máximo do Bucket em bytes
*    _pir:  Peak Information Rate, taxa máxima em bytes por segundo (atualmente não utilizada)
*    _currentTime:  Tempo atual de simulação
*    _lastTime:  Último tempo usado pelo Bucket
*    _cBucket:  Committed Bucket size, tamanho atual do Bucket em bytes
*
*  O Token Bucket permite que o número máximo de bytes seja transmitido, de acordo com a equação abaixo:
*    max(cir * (currentTime - lastTime) + cBucket; cbs) (O tamanho é o teto entre o cálculo indicado e o tamanho cbs)
*  A função retorna o tamanho atualizado do Bucket.  A função que chama deverá testar se este número é maior ou igual ao número de bytes
*  a transmitir:  se for positivo, então o pacote é dito conforme; caso contrário, é considerado não conforme e deve-se tomar a medida
*  apropriada (descartar o pacote, por exemplo).
*  Se for conforme, o Bucket deverá ser reduzido do tamanho do pacote.  Se for não conforme, o Bucket não é alterado.
*/
static void tokenBucket(struct LSPTableEntry *lsp) {
	double now, maxConformSize; //tamanho máximo de bytes a ser considerado "conforme"

	now=simtime();
	maxConformSize = lsp->cir * (now - lsp->arrivalTime) + lsp->cBucket;
	if (maxConformSize > lsp->cbs) //se o novo tamanho superar cbs, então considere cbs como o novo valor (bucket não deve superar cbs)
		maxConformSize=lsp->cbs;
	lsp->cBucket = (maxConformSize < 0)? (0):(maxConformSize); //atualiza tamanho corrente do cBucket, sendo ZERO o valor mínimo
	lsp->arrivalTime=now; //atualiza última marcação do relógio
}

/* APLICA A REGRA DO POLICER
*
*  O Policer aplica o algoritmo Token Bucket para o pacote passado, usando os parâmetros da LSP indicada no próprio pacote para o cálculo do
*  token bucket.
*  Se o pacote estiver conforme, a função retorna 1; se estiver não-conforme, o pacote é descartado e a função retorna 0.  A função que chama
*  deve testar o retorno e tomar o cuidado de não requisitar a transmissão, se o pacote houver sido descartado por não-conformidade.
*  Se qualquer um dos parâmetros CBS ou CIR forem zero, então o Bucket nunca encherá.  Neste caso, considerar que o policer não deve ser aplicado.
*/
int applyPolicer(struct Packet *pkt) {
	struct LSPTableEntry *lsp;
	int length;


	if (pkt->lblHdr.msgID!=0) //pacote é de controle (msgID!=0); não aplique nenhum policer, pois este pacote não deve ser descartado
		return 1;

	lsp=searchInLSPTable(pkt->lblHdr.LSPid);
	if (lsp==NULL)
		return 1; //se não houver LSPid válido para o pacote, considere-o "conforme" e retorne
	if (lsp->cbs == 0 || lsp->cir == 0)
		return 1; //se o CBS ou CIR forem zero, então o Bucket nunca encherá (qualquer pacote seria não-conforme); não aplique o policer, neste caso
	if (pkt->length <= lsp->maxPktSize || lsp->maxPktSize <= 0) { //tamanho do pacote não supera o tamanho máximo de conformidade, ou o tamanho máximo é <= 0; aplique o policer
		tokenBucket(lsp); //aplica o algoritmo Token Bucket, atualizando o valor cBucket da LSP
		length = (pkt->length < lsp->minPolUnit)? (lsp->minPolUnit):(pkt->length); //se tamanho do pacote for menor que minPolUnit, use o valor mínimo ao invés do tamanho real do pacote
		if (length <= lsp->cBucket) { //pacote está conforme; atualize o cBucket e retorne
			lsp->cBucket -= length;
			return 1;
		}
	}
	//pacote não-conforme; descarte-o
	nodeDropPacket(pkt, "Non-conformant packet (RSVP)");
	return 0;
}
