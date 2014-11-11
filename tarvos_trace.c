/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_trace.c
*
* Fun��es pertinentes � gera��o de traces e debug
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

/* TRACE PRINCIPAL
*
*  colocar aqui as informa��es desejadas para o trace principal da simula��o
*/
void mainTrace(char *entry) {
	static FILE *traceDump=NULL;
	
	if (!tarvosParam.traceMain) //verifica se a gera��o de trace est� ativa
		return;
	if (traceDump==NULL)
		traceDump=fopen(tarvosParam.traceDump, "w");

	fprintf(traceDump, "%s", entry);
}

/* TRACE DOS PACOTES PERDIDOS
*
*  colocar aqui as informa��es desejadas para os pacotes descartados durante a simula��o
*/
void dropPktTrace(char *entry) {
	static FILE *dropDump=NULL;
	
	if (!tarvosParam.traceDrop) //verifica se a gera��o de trace est� ativa
		return;
	if (dropDump==NULL)
		dropDump=fopen(tarvosParam.dropPktTrace, "w");
	
	fprintf(dropDump, "%s", entry);
}

/* TRACE DOS GERADORES DE TR�FEGO (FONTES)
*
*  colocar aqui as informa��es desejadas para tracing dos geradores de tr�fego
*/
void sourceTrace(char *entry) {
	static FILE *srcDump=NULL;
	
	if (!tarvosParam.traceSource) //verifica se a gera��o de trace est� ativa
		return;
	if (srcDump==NULL)
		srcDump=fopen(tarvosParam.sourceTrace, "w");

	fprintf(srcDump, "%s", entry);
}

/* TRACE DOS GERADORES DE TR�FEGO EXPONENCIAL ON/OFF (FONTES)
*
*  colocar aqui as informa��es desejadas para tracing dos geradores de tr�fego expoo
*/
void expooSourceTrace(char *entry) {
	static FILE *expooSrcDump=NULL;
	
	if (!tarvosParam.traceExpoo) //verifica se a gera��o de trace est� ativa
		return;
	if (expooSrcDump==NULL)
		expooSrcDump=fopen(tarvosParam.expooTrace, "w");

	fprintf(expooSrcDump, "%s", entry);
}

/* REGISTRA AS ESTAT�STICAS DELAY E JITTER DO NODO EM ARQUIVO
*
*  Para o nodo atual, esta fun��o registra em arquivos definidos na estrutura de par�metros do TARVOS (tipo xls) o valor do delay e jitter, correntes.
*  As estat�sticas s� s�o atualizadas se o pacote n�o houver sido descartado e n�o est� sendo gerado no nodo corrente (isso � garantido pela fun��o
*  que chama esta).
*  Da mesma forma, as estat�sticas s� devem ser registradas em arquivo se de fato tiverem sido atualizadas, a fim de evitar registros repetidos
*  ou com delay igual a zero (que acontece quando a fun��o � chamada mesmo quando as estat�stica n�o foram atualizadas, caso em que o registro ser�
*  feito com as estat�sticas anteriormente armazenadas no nodo, para o pacote anterior).
*
*  Esta � uma fun��o desenhada especificamente para testes na Disserta��o de Mestrado de Marcos Portnoi.  Na distribui��o final, esta fun��o poder�
*  ser deixada no c�digo, mas n�o ser� chamada por nenhuma outra neste arquivo.
*/
void jitterDelayTrace(int node, double stime, double jitter, double delay) {
	static FILE **delay_out;
	static FILE **jitter_out; //arquivos de sa�da de jitter e delay
	char filename[255];
	int i, nodes;
	static int isOpen=0;

	if (!tarvosParam.traceJitterDelayGlobal) 
		return;
	//executa se a flag de gera��o de trace para Jitter e Delay estiver habilitada
	if (!isOpen) { //cria todos os arquivos de sa�da para jitter e delay; s� � chamado 1 vez, na primeira vez que a fun��o � chamada
		isOpen=1;
		nodes=(sizeof tarvosModel.node / sizeof *(tarvosModel.node)); //tamanho do vetor nodes (que � o n�mero real de nodos + 1)
		delay_out = (FILE**)malloc(nodes * sizeof **delay_out); //cria vetor com [n�mero de nodes] posi��es
		jitter_out = (FILE**)malloc(nodes * sizeof **jitter_out); //cria vetor com [n�mero de nodes] posi��es
		for (i=1; i<=nodes; i++) {
			sprintf(filename, tarvosParam.delayNodes, i);
			delay_out[i] = fopen(filename,"w");
			fprintf(delay_out[i], "simtime\tdelay\n");
			sprintf(filename, tarvosParam.jitterNodes, i);
			jitter_out[i] = fopen(filename,"w");
			fprintf(jitter_out[i], "simtime\tjitter\n");
		}
	}
	fprintf(delay_out[node], "%.20f\t%.20f\n", stime, delay);
	fprintf(jitter_out[node], "%.20f\t%.20f\n", stime, jitter);
}

/* REGISTRA AS ESTAT�STICAS DELAY E JITTER DE APLICA��O DO NODO EM ARQUIVO
*
*  Para o nodo atual, esta fun��o registra em arquivos definidos na estrutura de par�metros do TARVOS (tipo xls) o valor do delay e jitter, correntes.
*  As estat�sticas s� s�o atualizadas se o pacote n�o houver sido descartado e n�o est� sendo gerado no nodo corrente (isso � garantido pela fun��o
*  que chama esta).
*  Da mesma forma, as estat�sticas s� devem ser registradas em arquivo se de fato tiverem sido atualizadas, a fim de evitar registros repetidos
*  ou com delay igual a zero (que acontece quando a fun��o � chamada mesmo quando as estat�stica n�o foram atualizadas, caso em que o registro ser�
*  feito com as estat�sticas anteriormente armazenadas no nodo, para o pacote anterior).
*
*  Esta � uma fun��o desenhada especificamente para testes na Disserta��o de Mestrado de Marcos Portnoi.  Na distribui��o final, esta fun��o poder�
*  ser deixada no c�digo, mas n�o ser� chamada por nenhuma outra neste arquivo.
*/
void jitterDelayApplTrace(int node, double stime, double jitter, double delay) {
	static FILE **delay_out;
	static FILE **jitter_out; //arquivos de sa�da de jitter e delay
	char filename[50];
	int i, nodes;
	static int isOpen=0;
	
	if (!tarvosParam.traceJitterDelayAppl) 
		return;
	if (!isOpen) { //cria todos os arquivos de sa�da para jitter e delay Aplica��es; s� � chamado 1 vez, na primeira vez que a fun��o � chamada
		isOpen=1;
		nodes=(sizeof tarvosModel.node / sizeof *(tarvosModel.node)); //tamanho do vetor nodes (que � o n�mero real de nodos + 1)
		delay_out = (FILE**)malloc(nodes * sizeof **delay_out); //cria vetor com [n�mero de nodes] posi��es
		jitter_out = (FILE**)malloc(nodes * sizeof **jitter_out); //cria vetor com [n�mero de nodes] posi��es
		for (i=1; i<=nodes; i++) {
			sprintf(filename, tarvosParam.applDelayNodes, i);
			delay_out[i] = fopen(filename,"w");
			fprintf(delay_out[i], "simtime\tAppldelay\n");
			sprintf(filename, tarvosParam.applJitterNodes, i);
			jitter_out[i] = fopen(filename,"w");
			fprintf(jitter_out[i], "simtime\tAppljitter\n");
		}
	}
	fprintf(delay_out[node], "%.20f\t%.20f\n", stime, delay);
	fprintf(jitter_out[node], "%.20f\t%.20f\n", stime, jitter);
}
