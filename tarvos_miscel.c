/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_miscel.c
*
* Funções extras que não pertencem a outro arquivo específico
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
#include "simm_globals.h"

/* MASTER RESET ou RESET DOS ACUMULADORES ESTATÍSTICOS
*
*  Inicializa os acumuladores estatísticos pertinentes tanto do shell TARVOS, quanto do kernel SimM.
*/
void statReset() {
	//colocar aqui as rotinas de inicialização dos contadores estatísticos do TARVOS
	reset();
}