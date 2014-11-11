/*
*          Sistema Simulation Machine - Simulacao de Filas
*
*          Versao modificada do smpl usando a filosofia de
*         construcao de modelos de forma reestruturada para 
*       utilizar mais adequadamente os recursos computacionais 
* 
*                  Arquivo <simm_types> 
*/

/* Copyright (C) 2002, 2005, 2006, 2007 Marcos Portnoi, Sergio F. Brito
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

/* Este arquivo contem a especificacao das structs utilizadas para o simulador
   implementar a sua execucao

   Aqui sao declaradas as duas estruturas principais do simulador simm (simulation
   machine):  a estrutura de facilities e a estrutura da cadeia de eventos

   Uma modificacao realizada na cadeia de eventos e a possibilidade de se 
   introduzir nesta cadeia um campo do tipo apontador que permitira
	 alem da designacao de um numero para a token tambem um apontador associado
	 a esta token  permitindo assim que cada evento tenha uma estrutura particular
	 para o seu processamento

   Dentro da cláusula #ifdef USE_TKN, abaixo, deve-se declarar a estrutura do token como extern e usar
   uma cláusula typedef para definir a estrutura como TOKEN.  Por exemplo:
   extern struct Packet;
   typedef struct Packet TOKEN;

   A estrutura do token deve então ser definida apropriadamente posteriormente, por exemplo, 
   num arquivo .h do modelo.
*/

#pragma once

#define USE_TKN 1	/* Este define indica 1 se se deseja utilizar a estrutura de apontador
	                   de token ou 0 se nao se deseja. Caso nao se deseje utilizar a estrutura
					   apontador de token, deve-se em todas as chamadas de funcoes que a definam,
					   colocar NULL no campo que seria preenchido por esta */


#ifdef USE_TKN

extern struct Packet; //a estrutura deve ser definida posteriormente, no modelo do usuário
typedef struct Packet TOKEN;	//declaracao do typedef associando a estrutura da token

#else //definir então uma estrutura de TOKEN padrão.  (isso é realmente necessário?)

struct Packet
{
	int f;
};
typedef struct Packet TOKEN;

#endif

/* Armazena informacoes de simulacao do modelo atual
*  Poderemos ter varias simulacoes para o mesmo modelo
*/
struct simul {
	char name[50];						/* nome a ser usado para denominar o modelo simulado */
	struct evchain *evc_begin;			/* Apontador para o inicio da cadeia de eventos (tokens) */
	struct evchain *evc_end;			/* Apontador para o final da cadeia de eventos (tokens) */
	struct facilit *fct_begin;			/* Apontador para o inicio da lista de facilities */
	int fct_number;						/* Especifica o ultimo numero de facility especificada */

};

/* estrutura da facility tipicamente associada a servidores */
struct facilit {
	char f_name[50];			/* nome da facility */	
	int f_number;				/* numero da facility */
	int f_n_serv;				/* num de servidores associados a facility */
	int f_n_busy_serv;			/* num de servidores ocupados */
	int f_n_length_q;			/* tamanho fila; cada facility tem só uma fila associada nesta implementação*/
	int f_max_queue;			/* tamanho máximo da fila */
	int f_exit_count_q;			/* numero de dequeues durante o período simulado */
	double f_last_ch_time_q;	/* momento da ultima mudanca na fila */
	double f_busy_time;			/* tempo que a facility fica ocupada - somatorio de fs_busy_time de todos os seus servidores */
	int f_preempt_count;		/* numero de preempções efetivamente realizadas */
	double length_time_prod_sum;	/* utilizado para obter o tamanho medio da fila */
	int f_release_count;		/* total de tokens que foram servidas pela facility. The preempt function *does not* increment this variable! Only the release function is incrementing this variable, in contrast to SMPL approach.*/
	struct fserv *f_serv;		/* apontador para a lista de servidores */
	struct fqueue *f_queue;		/* apontador para a fila de tokens de espera */
	struct facilit *fct_next;	/* apontador para a proxima facility */
	int f_up;					// status da facility:  1 para operacional (up), 0 para não-operacional (down) (25.Dec.2005 Marcos Portnoi)
	int f_tkn_dropped;			/* número de tokens descartadas pela facility (ao entrar em estado down, a fila é descartada e este contador é atualizado.
								Se uma token for escalonada para uma facility (requestp ou preemptp) e esta estiver down, este contador também será incrementado.*/
};

/* estrutura de cada servidor associado a uma facility */
struct fserv {
	int fs_number;				/* numero do servidor */
	int fs_tkn;					/* numero da token em processamento neste servidor */
	int fs_p_tkn;				/* prioridade da token */
	int fs_release_count;   	/* numero de tokens que foram servidas por este servidor. The preempt function *does not* increment this variable! Only the release function is incrementing this variable, in contrast to SMPL approach. */
	double fs_start;			/* momento de inicio do servico */
	double fs_busy_time;		/* somatorio do tempo que o servidor ficou ocupado na simulacao */
	struct fserv *fs_next;		/* apontador para o proximo servidor */
};						

struct fqueue {					/* estrutura da fila que é associada a facility */
	int fq_tkn;					/* numero da token */
	int fq_ev;					/* numero do evento ao qual a token esta associada */
	double fq_time;				/* tempo de execucao do processamento da token, podera ser 
	                               tambem o restante do tempo para processamento da token
	                               se esta tiver sido retirada de servico por uma token 
	                               preempted; para uma token bloqueada, este fq_time será zero */
	int  fq_pri;				/* prioridade da token */
	struct fqueue *fq_next;		/* apontador para a proxima token */
	TOKEN *fq_tkp;				/* apontador para struct com informacoes da token */ 
};						

/* A estrutura da cadeia a seguir tem algumas caracteristicas de interesse
   que sao:
	 1. Ela e uma lista duplamente encadeada e que estara ordenada no tempo,
	    ou seja o tempo mais proximo sera o primeiro da lista. Assim, o
			cabecalho da lista estara apontando sempre para o proximo evento a 
			ser processado.
	 2. Esta lista tera um apontador inicio da lista que sera usado para obter
			o proximo evento a ser processado e um apontador de final da lista que
			sera utilizado para permitir que os novos eventos a serem incluidos
			comecem pesquisando pelo final da lista. Ele pesquisara a partir do final
			da lista e encontrara o local onde devera ser inserido o novo evento.
			Esta estrategia provavelmente diminuira sensivelmente o tempo de busca
			do local de insercao na cadeia de eventos.
			Os apontadores de inicio e final sao globais:
			ev_begin -> apontador para o inicio da cadeia de eventos
			ev_end -> apontador para o final da cadeia de eventos
	 3. A estrutura permitira que seja incluido um campo do tipo apontador que 
			pode variar de modelo para modelo para descrever a estrutura da token que
			esta sendo usada para simulacao. 
*/

struct evchain {					//declaracao da estrutura dos elementos da cadeia de eventos
	double ev_time;					//tempo do proximo processamento desta token
	int ev_tkn;						//numero da token
	int ev_type;					//tipo do evento associado a esta token para ser processada
	struct evchain *ev_previous;	//apontador para a token anterior (alterado de 'prior', (2005) Marcos Portnoi)
	struct evchain *ev_next;		//apontador para a proxima token

									//Inicio da declaracao da estrutura da token
	TOKEN *ev_tkn_p;				//Apontador do tipo da struct associada a token - packet
									//Final da declaracao da estrutura da token
};