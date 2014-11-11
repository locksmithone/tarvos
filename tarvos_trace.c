/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_trace.c
*
* Funções pertinentes à geração de traces e debug
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
*  colocar aqui as informações desejadas para o trace principal da simulação
*/
void mainTrace(char *entry) {
	static FILE *traceDump=NULL;
	
	if (!tarvosParam.traceMain) //verifica se a geração de trace está ativa
		return;
	if (traceDump==NULL)
		traceDump=fopen(tarvosParam.traceDump, "w");

	fprintf(traceDump, "%s", entry);
}

/* TRACE DOS PACOTES PERDIDOS
*
*  colocar aqui as informações desejadas para os pacotes descartados durante a simulação
*/
void dropPktTrace(char *entry) {
	static FILE *dropDump=NULL;
	
	if (!tarvosParam.traceDrop) //verifica se a geração de trace está ativa
		return;
	if (dropDump==NULL)
		dropDump=fopen(tarvosParam.dropPktTrace, "w");
	
	fprintf(dropDump, "%s", entry);
}

/* TRACE DOS GERADORES DE TRÁFEGO (FONTES)
*
*  colocar aqui as informações desejadas para tracing dos geradores de tráfego
*/
void sourceTrace(char *entry) {
	static FILE *srcDump=NULL;
	
	if (!tarvosParam.traceSource) //verifica se a geração de trace está ativa
		return;
	if (srcDump==NULL)
		srcDump=fopen(tarvosParam.sourceTrace, "w");

	fprintf(srcDump, "%s", entry);
}

/* TRACE DOS GERADORES DE TRÁFEGO EXPONENCIAL ON/OFF (FONTES)
*
*  colocar aqui as informações desejadas para tracing dos geradores de tráfego expoo
*/
void expooSourceTrace(char *entry) {
	static FILE *expooSrcDump=NULL;
	
	if (!tarvosParam.traceExpoo) //verifica se a geração de trace está ativa
		return;
	if (expooSrcDump==NULL)
		expooSrcDump=fopen(tarvosParam.expooTrace, "w");

	fprintf(expooSrcDump, "%s", entry);
}

/* REGISTRA AS ESTATÍSTICAS DELAY E JITTER DO NODO EM ARQUIVO
*
*  Para o nodo atual, esta função registra em arquivos definidos na estrutura de parâmetros do TARVOS (tipo xls) o valor do delay e jitter, correntes.
*  As estatísticas só são atualizadas se o pacote não houver sido descartado e não está sendo gerado no nodo corrente (isso é garantido pela função
*  que chama esta).
*  Da mesma forma, as estatísticas só devem ser registradas em arquivo se de fato tiverem sido atualizadas, a fim de evitar registros repetidos
*  ou com delay igual a zero (que acontece quando a função é chamada mesmo quando as estatística não foram atualizadas, caso em que o registro será
*  feito com as estatísticas anteriormente armazenadas no nodo, para o pacote anterior).
*
*  Esta é uma função desenhada especificamente para testes na Dissertação de Mestrado de Marcos Portnoi.  Na distribuição final, esta função poderá
*  ser deixada no código, mas não será chamada por nenhuma outra neste arquivo.
*/
void jitterDelayTrace(int node, double stime, double jitter, double delay) {
	static FILE **delay_out;
	static FILE **jitter_out; //arquivos de saída de jitter e delay
	char filename[255];
	int i, nodes;
	static int isOpen=0;

	if (!tarvosParam.traceJitterDelayGlobal) 
		return;
	//executa se a flag de geração de trace para Jitter e Delay estiver habilitada
	if (!isOpen) { //cria todos os arquivos de saída para jitter e delay; só é chamado 1 vez, na primeira vez que a função é chamada
		isOpen=1;
		nodes=(sizeof tarvosModel.node / sizeof *(tarvosModel.node)); //tamanho do vetor nodes (que é o número real de nodos + 1)
		delay_out = (FILE**)malloc(nodes * sizeof **delay_out); //cria vetor com [número de nodes] posições
		jitter_out = (FILE**)malloc(nodes * sizeof **jitter_out); //cria vetor com [número de nodes] posições
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

/* REGISTRA AS ESTATÍSTICAS DELAY E JITTER DE APLICAÇÃO DO NODO EM ARQUIVO
*
*  Para o nodo atual, esta função registra em arquivos definidos na estrutura de parâmetros do TARVOS (tipo xls) o valor do delay e jitter, correntes.
*  As estatísticas só são atualizadas se o pacote não houver sido descartado e não está sendo gerado no nodo corrente (isso é garantido pela função
*  que chama esta).
*  Da mesma forma, as estatísticas só devem ser registradas em arquivo se de fato tiverem sido atualizadas, a fim de evitar registros repetidos
*  ou com delay igual a zero (que acontece quando a função é chamada mesmo quando as estatística não foram atualizadas, caso em que o registro será
*  feito com as estatísticas anteriormente armazenadas no nodo, para o pacote anterior).
*
*  Esta é uma função desenhada especificamente para testes na Dissertação de Mestrado de Marcos Portnoi.  Na distribuição final, esta função poderá
*  ser deixada no código, mas não será chamada por nenhuma outra neste arquivo.
*/
void jitterDelayApplTrace(int node, double stime, double jitter, double delay) {
	static FILE **delay_out;
	static FILE **jitter_out; //arquivos de saída de jitter e delay
	char filename[50];
	int i, nodes;
	static int isOpen=0;
	
	if (!tarvosParam.traceJitterDelayAppl) 
		return;
	if (!isOpen) { //cria todos os arquivos de saída para jitter e delay Aplicações; só é chamado 1 vez, na primeira vez que a função é chamada
		isOpen=1;
		nodes=(sizeof tarvosModel.node / sizeof *(tarvosModel.node)); //tamanho do vetor nodes (que é o número real de nodos + 1)
		delay_out = (FILE**)malloc(nodes * sizeof **delay_out); //cria vetor com [número de nodes] posições
		jitter_out = (FILE**)malloc(nodes * sizeof **jitter_out); //cria vetor com [número de nodes] posições
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
