/**********************************************************************/
/*                                                                    */
/*                           File "simm_globals.h"                      */
/*  Includes, Defines, & Extern Declarations for Simulation Programs  */
/*                                                                    */
/**********************************************************************/

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

#pragma once

#include "simm_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#define MAX_SIMULATIONS 20			/* Aqui e definido o numero maximo de simulacoes permitido para o estudo do modelo */

#ifdef MAIN_MODULE

struct simul sim[MAX_SIMULATIONS];	/* Poderemos ter varias simulacoes para o mesmo modelo */

#else

extern struct simul sim[MAX_SIMULATIONS];
#endif

/* Funcoes do arquivo simm_kernel */
void simm(int m, char *s);
void reset();
//static void resetf();
int facility(char *s, int n);
int requestp (int f, int tkn, int pri, int ev, double te, TOKEN *tkp);
int preemptp (int f, int tkn, int pri, int ev, double te, TOKEN *tkp);
//static void enqueuep (int f, int tkn, TOKEN *tkp, int pri, double te, int ev);
//static void enqueuep_preempt (int f, int tkn, TOKEN *tkp, int pri, double te, int ev);
struct evchain *cancelp_tkn(int tkn);
void releasep (int f, int tkn);
void schedulep(int ev, double te, int tkn, TOKEN *tkp);
TOKEN *causep(int *ev, int *tkn);
int evChainIsEmpty();
void cause(int *ev, int *tkn);
double simtime();
int status(int f);
int inq(int f);
double U(int f);
double B(int f);
double Lq(int f);
void report();
void reportf();
//static struct facilit *rept_page(struct facilit *f);
int lns(int i);
void endpage();
void newpage();
FILE *sendto(FILE *dest);
void error(int n, char *s);
char *fname(int f);
char *mname();
int getFacMaxQueueSize(int f);
int getFacDropTokenCount(int f);
void setFacUp(int f);
int setFacDown(int f);
int getFacUpStatus(int f);
struct evchain *cancelp_ev(int ev);
//static int purgeFacQueue(int f);
void dbg_init();
void dbg_cab();
void dbg_cause(double tempo, int pkt, int ev);
void dbg_evc();
void dbg_fct();
void dbg_fct_srv();
void dbg_fct_queue();
void dbg_tp_1();
void dbg_tp_2();
void dbg_tp_3();

/* Funcoes do arquivo simm_rand */
double ranf();
int stream (int n);
long seed(long Ik, int n);
double uniform(double a, double b);
int irandom(int i,int n);
double expntl(double x);
double erlang(double x, double s);
double hyperx(double x, double s);
double normal(double x, double s);
double pareto(double a, double k);
double randJain(int seed);

/* Funcoes do arquivo simm_stat */
double Z(double p);
double T(double p, double ndf);

