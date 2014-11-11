/*
* TARVOS Computer Networks Simulator
* File tarvos_about.c
*
* Print copyright message and other data
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

#include <stdio.h>

/* PRINT COPYRIGHT MESSAGE AND OTHER DATA
*
*  This function receives a pointer to an already opened file for writing (or stdout) and prints Copyright information and other data.
*/
void about(FILE *fp) {
	
	fprintf(fp,"TARVOS Computer Networks Simulator - version 0.71\n");
	fprintf(fp,"-------------------------------------------------\n");
	fprintf(fp,"Copyright (C) 2005, 2006, 2007 Marcos Portnoi\n");
	fprintf(fp,"TARVOS Computer Networks Simulator is free software: you can redistribute it\n");
	fprintf(fp,"and/or modify it under the terms of the GNU General Public License as published\n");
	fprintf(fp,"by the Free Software Foundation, either version 3 of the License, or\n");
	fprintf(fp,"(at your option) any later version.\n\n");
	fprintf(fp,"TARVOS Computer Networks Simulator is distributed in the hope that it will be\n");
	fprintf(fp," useful, but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	fprintf(fp,"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	fprintf(fp,"GNU General Public License for more details.\n\n");
	fprintf(fp,"You should have received a copy of the GNU General Public License\n");
	fprintf(fp,"along with TARVOS Computer Networks Simulator.  If not,\n");
	fprintf(fp,"see <http://www.gnu.org/licenses/>.");
	fprintf(fp,"\n\n");
}