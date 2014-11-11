/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_rsvp-te
*
* Rotinas para constru��o e manuten��o da Label Information Base (LIB), de uso do MPLS
* de tabelas para controle de Constraint Routing
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
#include "simm_globals.h" //alguma fun��o local usa fun��es do kernel do simulador, como schedulep

//Prototypes das fun��es locais static
static void buildLSPTable();
static int getNewLSPid();
static void insertInLSPTable(int LSPid, int source, int dst, double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio);
static void LSPtimeoutCheck(double now);
static void returnResources(int LSPid, int link);
static void nodeCtrlMsgTimeoutCheck(double now);
static void helloFailureCheck(double now);
static int rapidRecovery(int node, int LSPid, int iIface, int iLabel);
static struct LspList *initLspList();
static struct LspListItem *searchInLspList(struct LspList *lspList, int holdPrio);
static void insertInLspList(struct LspList *lspList, int LSPid, int setPrio, int holdPrio, struct LIBEntry *libEntry);

//Vari�veis locais (static)
static struct LIB lib ={0}; //defina uma var�avel tipo estrutura LIB para uso exclusivo deste m�dulo e inicialize tudo com NULL ou zeros
static struct LSPTable lspTable ={0}; //defina uma var�avel tipo estrutura LSPTable para uso exclusivo deste m�dulo

/* LEITOR DO ARQUIVO LIB.TXT, PARA CRIA��O DA TABELA DE ROTEAMENTO (LIB) MPLS
*
*  Esta fun��o deve ser chamada pelo programa principal (main) no in�cio da simula��o,
*  antes da gera��o de qualquer evento, a fim de que o forwarding opere corretamente.
*
*  A LIB � �nica para a simula��o, contendo a informa��o de todos os nodos.
*  Deve ser fornecida pelo usu�rio no arquivo LIB.TXT, contendo n�meros inteiros.
*  O seguinte formato deve ser obedecido, para cada linha:
*
*  Node Incoming_Interface Incoming_Label Outgoing_Interface Outgoing_Label
*
*  Node:  nodo atual.  Tecnicamente, cada nodo (roteador) deveria ter sua pr�pria tabela de roteamento,
*		  mas aqui todas elas s�o concentradas em uma s�.  Portanto, � necess�rio ter um �ndice para os
*		  nodos.
*  Incoming_Interface:  Interface de entrada do nodo.  Aqui, as interfaces s�o os pr�prios n�meros dos links,
*		               que s�o �nicos
*  Incoming_Label:  R�tulo de entrada
*  Outgoing_Interface:  Interface de Sa�da.  Aqui, as interfaces s�o os pr�prios n�meros dos links,
*					    que s�o �nicos
*  Outgoing_Label:  R�tulo de sa�da, que substituir� o r�tulo de entrada (label swap)
*  Status:  estado da entrada da LIB; up (funcional) ou down (n�o-funcional)
*  LSPid:  �ndice para a tabela de LSPid, que conter� par�metros relativos ao LSP.  Um LSPid � �nico por todo o dom�nio MPLS
*						e � composto de v�rios "subLSPs" entre cada par de nodos.  O "subLSP" � unicamente identificado pela chave
*						nodo-iIface-iLabel.  Este LSPid ter� de ser criado e coordenado pelo LER de ingresso, tipicamente como um
*						n�mero inteiro �nico durante toda a simula��o.
*
*  Todos os n�meros devem ser inteiros.
*  
*  A chave prim�ria � composta de node, in_interface, iLabel.  Deve ser �NICA.
*  
*  Os n�meros para in_interface, iLabel, out_interface e oLabel para os LERs de ingresso e egresso
*  podem ser, a princ�pio, qualquer valor, j� que n�o ser�o processados (a n�o ser em caso de m�ltiplos dom�nios MPLS).
*  Usar, como sugest�o, o n�mero zero para estes valores nos LERs, com exce��o do LER de ingresso, pois deve-se ter o cuidado
*  de manter a chave prim�ria composta �nica.  Neste caso, usar valores diferentes para os int_labels nos LER de ingresso.
*  Por exemplo, a seguinte LIB causar� problemas, pois as duas primeiras linhas t�m a chave n�o �nica (os nodos 1 e 3 s�o LERs de ingresso
*  e egresso, respectivamente):
*
*  Node;iIface;iLabel;oIface;oLabel
*  1	0	0	1	10
*  1	0	0	2	20
*  2	1	10	3	50
*  3	2	20	0	0
*  3	3	50	0	0
*
*  Esta LIB pode ser alterada conforme segue, e funcionar� corretamente.
*
*  1	0	1	1	10
*  1	0	2	2	20
*  2	1	10	3	50
*  3	2	20	0	0
*  3	3	50	0	0
*
*  Notar que os valores das in_labels foram configurados de forma �nica para o LER de ingresso.
*  Da mesma forma, para cada nodo, os valores de oIface e oLabel combinados t�m de ser �nicos, pois compor�o as chaves de busca
*  dos nodos seguintes.
*  Os valores das interfaces, que representam os links do modelo simulado, devem, por recomenda��o,
*  representar apenas os links realmente existentes.
*
*  Qualquer linha, no arquivo LIB.TXT, que n�o comece por um n�mero ser� ignorada.
*/
void buildLIBTableFromFile(char *libFile) {
	FILE *fp;
	char str[80]; //linha com 80 caracteres
	int node, iIface, iLabel, oIface, oLabel, LSPid;
	char status[]="up";

	buildLIBTable();  //cria a estrutura inicial da LIB e da LSP ID Table
	
	//puts("Lendo arquivo lib.txt...");
	if((fp=fopen(libFile, "r")) == NULL) {
		printf("\nError - buildLIBTable - file %s not found\n", tarvosParam.libFile);
		exit(1);
    }

	while(!feof(fp)) {
		fgets(str, 80, fp); //linha com 80 caracteres
		if(isdigit(str[0])) {
            if(sscanf(str, "%d %d %d %d %d %d", &node, &iIface, &iLabel, &oIface, &oLabel, &LSPid) != 6) {
				printf("\nbuildLIBTable - Error reading line\n");
				exit(1);
			}
			insertInLIB(node, iIface, iLabel, oIface, oLabel, LSPid, status, 0, 0, 0); //insere item na LIB, com zero para os timeouts (e marcando como working LSP; bak = 0)
			//if (searchInLSPTable(LSPid)==NULL)
			//	insertInLSPTable(LSPid); /*insere tamb�m dados pertinentes na LSP Table, se n�o foram ainda inseridos
			//							 * � importante verificar antes de inserir nesta tabela, pois s� pode haver nela
			//							 * uma �nica linha por LSPid.*/
		}
	}
	fclose(fp);
	//printf("Tamanho da estrutura LIB completa:  %d\n", getLIBSize());
}

/* CRIA A LIB TABLE (ESTRUTURA INICIAL)
*
*  A tabela criada estar� vazia (apenas contendo o Head Node), deixando-a pronta para receber entradas.
*  A LIB Table � uma lista din�mica duplamente encadeada, circular, com um Head Node.
*/
void buildLIBTable() {
	if (lib.head==NULL) { //evita duplicidade na cria��o da LIB
		lib.head = (LIBEntry*)malloc(sizeof *(lib.head)); //cria HEAD NODE
		if (lib.head==NULL) {
			printf("\nError - buildLIBTable - insufficient memory to allocate for LIB");
			exit(1);
		}
		lib.head->previous=lib.head;
		lib.head->next=lib.head; //perfaz a caracter�stica circular da lista
		lib.size = 0; //lista est� vazia
		buildLSPTable();  //cria tamb�m a estrutura inicial da LSP ID Table
	}
}

/* INSERE NOVO ITEM NA LIB
*
*  (nota:  a LIB � uma lista din�mica duplamente encadeada circular com Head Node)
*  Passar todo o conte�do de uma linha da LIB como par�metros
*/
void insertInLIB(int node, int iIface, int iLabel, int oIface, int oLabel, int LSPid, char *status, int bak, double timeout, double timeoutStamp) {
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return;
	
	lib.head->previous->next=(LIBEntry*)malloc(sizeof *(lib.head->previous->next)); //cria mais um n� ao final da lista; este n� � lib.head->previous->next (lista duplamente encadeada circular)
	if (lib.head->previous->next==NULL) {
		printf("\nError - insertInLIB - insufficient memory to allocate for LIB");
		exit(1);
	}
	lib.head->previous->next->previous=lib.head->previous; //atualiza ponteiro previous do novo n� da lista
	lib.head->previous->next->next=lib.head; //atualiza ponteiro next do novo n� da lista
	lib.head->previous=lib.head->previous->next; //atualiza ponteiro previous do Head Node
	lib.head->previous->previous->next=lib.head->previous; //atualiza ponteiro next do pen�ltimo n�
	lib.head->previous->node = node;
	lib.head->previous->iIface = iIface;
	lib.head->previous->iLabel = iLabel;
	lib.head->previous->oIface = oIface;
	lib.head->previous->oLabel = oLabel;
	lib.head->previous->LSPid = LSPid;
	strcpy(lib.head->previous->status, status);
	lib.head->previous->bak = bak;
	lib.head->previous->timeout = timeout;
	lib.head->previous->timeoutStamp = timeoutStamp;
	lib.size++;

	//if (searchInLSPTable(LSPid)==NULL)
	//			insertInLSPTable(LSPid); /*insere tamb�m dados pertinentes na LSP Table, se n�o foram ainda inseridos
	//									 * � importante verificar antes de inserir nesta tabela, pois s� pode haver nela
	//									 * uma �nica linha por LSPid.*/
	//printf("Item inserido:  %d\n", getLIBSize()); //confirma inser��o para efeito de debugging
}

/* IMPRIME O CONTE�DO DA LIB EM ARQUIVO
*
* imprime conte�do da Label Information Base no arquivo recebido como par�metro
*/
void dumpLIB(char *outfile) {
	struct LIBEntry *p;
	FILE *fp;
	
	fp=fopen(outfile, "w");
	if (lib.head==NULL) { //se LIB n�o existir ainda, nada fa�a
		fprintf(fp, "LIB does not exist.");
		return;
	}
	
	fprintf(fp, "Label Information Base - Contents - Tarvos Simulator\n\n");
	fprintf(fp, "Itens na LIB:  %d\n\n", getLIBSize());
	fprintf(fp, "Node iIface iLabel oIface  oLabel LSPid IsBackup               Status        Timeout   timeoutStamp\n");
	p=lib.head->next; //aponta para a primeira c�lula da LIB
	while(p!=lib.head) { //lista circular; o �ltimo n� atingido deve ser o Head Node
		fprintf(fp, "%3d   %3d     %4d    %3d    %4d   %3d    %3s   %20s   %12.4f   %12.4f\n", p->node, p->iIface, p->iLabel, p->oIface, p->oLabel, p->LSPid, 
			(p->bak==0)? ("no"):("yes"), p->status, p->timeout, p->timeoutStamp);
		p=p->next;
	}
	fclose(fp);
}

/* RETORNA O TAMANHO DA LIB EM LINHAS (OU ITENS)
*/
int getLIBSize() {
	return(lib.size);
}

/* BUSCA NA LIB USANDO CHAVE NODE-iIFACE-iLABEL
*
*  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado
*  chave para busca:  nodo atual, incoming interface (n�mero do link), incoming label
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*  CUIDADO:  Esta fun��o desconsidera o status da LSP indicada pela LIB.  N�o se recomenda usar esta busca para fazer comuta��o por r�tulo
*/
struct LIBEntry *searchInLIB(int node, int iIface, int iLabel) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->iIface=iIface;
	lib.head->iLabel=iLabel;
	
	p=lib.head->previous; //aponta para a �ltima c�lula da LIB
	while(p->node!=node || p->iIface!=iIface || p->iLabel!=iLabel) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-iIFACE-iLABEL-STATUS
*
*  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado
*  chave para busca:  nodo atual, incoming interface (n�mero do link), incoming label, status
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*  Esta � a busca indicada para fazer o chaveamento por r�tulo.
*/
struct LIBEntry *searchInLIBStatus(int node, int iIface, int iLabel, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->iIface=iIface;
	lib.head->iLabel=iLabel;
	strcpy(lib.head->status, status);
	
	p=lib.head->previous; //aponta para a �ltima c�lula da LIB
	while(p->node!=node || p->iIface!=iIface || p->iLabel!=iLabel || strcmp(p->status, status)!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS PARA WORKING LSPs
*
*  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado
*  chave para busca:  nodo atual, LSPid e string do status desejado.  Busca somente working LSPs,
*  que comecem ou n�o no nodo atual (o valor de iIface, por conseguinte, � irrelevante).
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*/
struct LIBEntry *searchInLIBnodLSPstat(int node, int LSPid, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->bak=0;
	
	p=lib.head->previous; //aponta para a �ltima c�lula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->bak!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS PARA BACKUP LSPs
*
*  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado.
*  chave para busca:  nodo atual, LSPid e string do status desejado.  Busca somente backup LSPs que tenham Merge Point
*  inicial no nodo atual (ou seja, onde o iIface � igual a zero, indica��o de que o "t�nel" LSP come�a neste nodo).
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*/
struct LIBEntry *searchInLIBnodLSPstatBak(int node, int LSPid, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->bak=1; //indica uma backup LSP
	lib.head->iIface=0; //iIface=0 indica que este MP � o in�cio da backup LSP
	
	p=lib.head->previous; //aponta para a �ltima c�lula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->iIface!=0 || p->bak!=1) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS-oIFACE PARA WORKING LSPs
*
*  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado
*  chave para busca:  nodo atual, LSPid, oIface e string do status desejado.  Busca somente working LSPs.
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*/
struct LIBEntry *searchInLIBnodLSPstatoIface(int node, int LSPid, char *status, int oIface) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->oIface=oIface;
	lib.head->bak=0; //procura por working LSPs
	
	p=lib.head->previous; //aponta para a �ltima c�lula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->oIface!=oIface || p->bak!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* CRIA A LSP (LABEL SWITCHED PATH) TABLE (ESTRUTURA INICIAL)
*
*  A Tabela LSP conter� uma chave, a LSPid, �nica em todo o dom�nio MPLS, e os par�metros pertinentes � LSP.
*  A LSP � composta por v�rias "subLSPs".  Cada "subLSP" conecta um par de nodos, e � unicamente identificada
*  pela chave nodo-iIface-iLabel, e est�o relacionadas na LIB.  Todo o caminho composto por todas as subLSPs que,
*  conectadas, ligam dois LERs, � o caminho LSP referenciado nesta tabela LSP Table.
*
*  A tabela aqui criada estar� vazia, contendo apenas o Head Node.  A tabela � uma lista din�mica duplamente
*  encadeada, circular.
*/
static void buildLSPTable() {
	lspTable.head = (LSPTableEntry*)malloc(sizeof*(lspTable.head)); //cria HEAD NODE
	if (lspTable.head==NULL) {
		printf("\nError - buildLSPTable - insufficient memory to allocate for LIB");
		exit(1);
	}
	lspTable.head->previous=lspTable.head;
	lspTable.head->next=lspTable.head; //perfaz a caracter�stica circular da lista
	lspTable.size = 0; //lista est� vazia
}

/* RETORNA O TAMANHO DA LSP TABLE EM LINHAS (OU ITENS)
*/
int getLSPTableSize() {
	return(lspTable.size);
}

/* INSERE NOVO ITEM NA LSP TABLE
*
*  (nota:  a LSP Table � uma lista din�mica duplamente encadeada circular com Head Node)
*  Passar todo o conte�do de uma linha da LSP Table como par�metros
*/
static void insertInLSPTable(int LSPid, int source, int dst, double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio) {
	if (lspTable.head==NULL) //se LSP n�o existir ainda, nada fa�a
		return;
	
	lspTable.head->previous->next=(LSPTableEntry*)malloc(sizeof *(lspTable.head->previous->next)); //cria mais um n� ao final da lista; este n� � lib.head->previous->next (lista duplamente encadeada circular)
	if (lspTable.head->previous->next==NULL) {
		printf("\nError - insertInLSPTable - insufficient memory to allocate for LSP Table");
		exit(1);
	}
	lspTable.head->previous->next->previous=lspTable.head->previous; //atualiza ponteiro previous do novo n� da lista
	lspTable.head->previous->next->next=lspTable.head; //atualiza ponteiro next do novo n� da lista
	lspTable.head->previous=lspTable.head->previous->next; //atualiza ponteiro previous do Head Node
	lspTable.head->previous->previous->next=lspTable.head->previous; //atualiza ponteiro next do pen�ltimo n�
	lspTable.head->previous->LSPid = LSPid;
	lspTable.head->previous->src = source;
	lspTable.head->previous->dst = dst;
	lspTable.head->previous->arrivalTime = simtime();
	lspTable.head->previous->cbs = cbs;
	lspTable.head->previous->cBucket = 0; //Bucket come�a vazio?
	lspTable.head->previous->cir = cir;
	lspTable.head->previous->maxPktSize = maxPktSize;
	lspTable.head->previous->minPolUnit = minPolUnit;
	lspTable.head->previous->pir = pir;
	lspTable.head->previous->setPrio = setPrio;
	lspTable.head->previous->holdPrio = holdPrio;
	lspTable.head->previous->tunnelDone = 0;
	lspTable.size++;
}

/* BUSCA NA LSP Table.  Retorna ponteiro para a c�lula com os par�metros pedidos ou NULL para n�o encontrado
*  chave para busca:  LSPid
*
*  A busca � feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista at� o in�cio.
*/
struct LSPTableEntry *searchInLSPTable(int LSPid) {
	struct LSPTableEntry *p;
	
	if (lspTable.head==NULL) //se LSP n�o existir ainda, nada fa�a
		return NULL;
	
	//insere a chave de busca no Head Node
	lspTable.head->LSPid=LSPid;
	
	p=lspTable.head->previous; //aponta para a �ltima c�lula da LSP Table
	while(p->LSPid!=LSPid) {
		p=p->previous;
	} //fim do percurso
	if (p==lspTable.head) //se for igual, a busca n�o encontrou nenhum item v�lido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* IMPRIME O CONTE�DO DA LSP TABLE EM ARQUIVO
*
* imprime conte�do da Label Switched Path Table no arquivo recebido como par�metro
*/
void dumpLSPTable(char *outfile) {
	struct LSPTableEntry *p;
	FILE *fp;
	
	fp=fopen(outfile, "w");
	
	if (lspTable.head==NULL) { //se LIB n�o existir ainda, nada fa�a
		fprintf(fp, "LSP Table does not exist.");
		return;
	}

	fprintf(fp, "Label Switched Path (LSP) Table - Contents - Tarvos Simulator\n\n");
	fprintf(fp, "Itens in LSP Table:  %d\n", getLSPTableSize());
	fprintf(fp, "Current simtime:  %f\n\n", simtime());
	fprintf(fp, "LSPid Src  Dst        CBS             CIR            PIR           cBucket  MinPolUnit  MaxPktSize   setPrio  HoldPrio  ArrivalTime  tunnelDone\n");
	p=lspTable.head->next; //aponta para a primeira c�lula da LSP Table
	while(p!=lspTable.head) { //lista circular; o �ltimo n� atingido deve ser o Head Node
		fprintf(fp, "%3d  %3d  %3d\t%10.1f\t%10.1f\t%10.1f\t%10.1f\t%5d\t  %5d\t\t%d\t%d\t %10.4f\t%s\n",
			p->LSPid, p->src, p->dst, p->cbs, p->cir, p->pir, p->cBucket, p->minPolUnit, p->maxPktSize, p->setPrio, p->holdPrio, p->arrivalTime, (p->tunnelDone==0)? ("no"):("yes"));
		p=p->next;
	}
	fclose(fp);
}

/* CONTROLA E DEVOLVE UM N�MERO �NICO, PARA TODO UM DOM�NIO MPLS, DE UM T�NEL LSP
*
*  Esta fun��o cria e devolve um LSPid, que � um n�mero inteiro e necessariamente �nico para todo um dom�nio MPLS, usado para caracterizar
*  um t�nel LSP.
*/
static int getNewLSPid() {
	static int LSPid=0; //defina o LSPid inicial
	
	LSPid++; //cria novo LSPid
	return LSPid;
}

/* ESTABELECE (CRIA) UMA LSP ENTRE DOIS NODOS, COM OU SEM PREEMP��O
*
*  Dada uma rota expl�cita, esta fun��o criar� uma LSP entre o primeiro e o �ltimo nodo da rota (entre os LERs de ingresso
*  e egresso, pois), se houver recursos dispon�veis por todo o caminho.
*  Os par�metros a passar s�o o tamanho do pacote de controle, o nodo fonte (LER de ingresso), o nodo destino (LER de egresso),
*  o objeto rota expl�cita, o cir (Committed Information Rate), o cbs (Committed Bucket Size), o pir (Peak Information Rate),
*  o tamanho m�nimo a ser considerado para policing e o tamanho m�ximo de um pacote para ser considerado conforme.
*  Tamb�m, a Setup Priority e Holding Priority para uma LSP.  Segundo a RFC 3209, a Setup Priority n�o pode ser mais alta que a Holding Priority para
*  uma mesma LSP.  Para estes dois par�metros, o valor varia de 0 a 7, 0 sendo a maior prioridade.
*  Por fim, o campo preempt dever� conter 1 se preemp��o for desejada, e 0 se preemp��o n�o for desejada.
*  A fun��o reservar� um n�mero LSPid que ser� �nico por todo o dom�nio MPLS e n�o mais poder� ser usado.
*
*  Em caso de preemp��o:
*  Para a reserva de recursos, poder� haver a preemp��o de LSPs de menor prioridade a fim de garantir os recursos exigidos.  A preemp��o segue o seguinte
*  algoritmo:
*    1.  Se houver recursos suficientes no link, n�o h� necessidade de preemp��o, e estes ser�o pr�-reservados por uma mensagem PATH e confirmados por uma RESV.
*    2.  Se n�o houver recursos suficientes, varrer todas as LSPs estabelecidas no link em quest�o (loop).  Para aquelas com Holding Priority menor que a Setup Priority
*		 da LSP sendo estabelecida, somar seus recursos reservados num acumulador.  Se o acumulador acusar recursos suficientes para estabelecimento da LSP,
*		 interromper o loop e deixar a mensagem PATH seguir adiante.  Isto significa que a LSP pode ser estabelecida, mas s� o ser� efetivamente com o recebimento de
*		 uma mensagem RESV.  Neste caso aqui, n�o haver� pr�-reserva, pois isto obrigaria uma "pr�-preemp��o" das LSPs estabelecidas, algo complexo de implementar.
*		 Se, ap�s varrer todas as LSPs estabelecidas para o link, n�o houver recursos suficientes que possam ser obtidos da preemp��o de LSPs de menor prioridade,
*		 ent�o n�o deixar a PATH seguir adiante.  (Pode ser emitida uma PATH_ERR aqui.)
*
*  A preemp��u usa duas mensagens espec�ficas:  PATH_LABEL_REQUEST_PREEMPT e RESV_LABEL_MAPPING_PREEMPT.  No caso 1 acima, a mensagem PATH comporta-se como uma
*  PATH_LABEL_REQUEST comum, fazendo a pr�-reserva de recursos.  No caso 2, a mensagem ativar� a varredura de LSPs pr�-existentes a fim de verificar a possibilidade
*  de liberar recursos atrav�s de preemp��o de LSPs de menor prioridade.  Caso positivo, a mensagem seguir� adiante, mas nenhuma pr�-reserva ser� feita.
*  No processamento da mensagem RESV, checar-se-� se recursos foram previamente reservados.  Caso positivo, a RESV confirma esta reserva, faz o mapeamento de r�tulo
*  e a mensagem segue adiante.  Se n�o foram (esta informa��o consta da lista de mensagens de controle do nodo), o processamento da RESV checar� se h� recursos
*  imediatamente dispon�veis.  Havendo, age conforme acima.  N�o havendo, far� uma varredura nas LSPs pr�-existentes, construindo uma lista auxiliar com as LSPs
*  de menor Holding Priority que a Setup Prioriry da LSP corrente, ordenada por menor prioridade.  Esta varredura acaba quando os recursos encontrados forem suficientes
*  para a LSP corrente, ou quando n�o houver mais nenhuma LSPs pr�-existente no link/nodo em quest�o.  Havendo recursos suficientes, o processamento colocar� o estado de
*  cada LSP, em ordem de menor prioridade, em "preempted", e devolver� seus recursos ao link.  Far� isso para cada LSP da lista auxiliar, at� que a quantidade de recursos
*  devolvidos seja suficiente.  Ent�o, far� a nova reserva para a LSP atual e seguir� com a mensagem RESV adiante.
*  Se n�o houver recursos suficientes para preemp��o, a mensagem n�o segue adiante.
*/
struct Packet *setLSP(int source, int dst, int er[], double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio, int preempt) {
	int LSPid;
	struct Packet *pkt;

	//checa se algum par�metro do Policer est� inv�lido; se houver, sai do programa com erro
	if (cir<0 || cbs<0 || pir<0 || minPolUnit<0 || maxPktSize<0) {
		printf("\nError -  setLSP - parameter cir||cbs||pir||minPolUnit||maxPktSize < 0");
		exit(1);
	}
	if (minPolUnit>maxPktSize) {
		printf("\nError - setLSP - parameter minPolUnit > maxPktSize");
		exit(1);
	}
	//valida valores das prioridades Setup Priority e Holding Priority
	if (setPrio<0 || setPrio>7 || holdPrio<0 || holdPrio>7) {
		printf("\nError - setLSP - parameter setPrio or holdPrio out of range [0;7]");
		exit(1);
	}
	//verifica se Setup Priority � maior que Holding Priority (ou seja, se tem um n�mero mais baixo) (RFC 3209 n�o recomenda que isto aconte�a)
	if (setPrio<holdPrio) {
		printf("\nError - setLSP - Setup Priority higher than Holding Priority");
		exit(1);
	}
	buildLIBTable(); //monta a LIB se ainda n�o houver sido montada (esta fun��o checa se a LIB j� existe a fim de prevenir duplicidade)
	LSPid=getNewLSPid(); //reserva novo LSPid para uso; mesmo que a LSP n�o consiga ser completamente montada, este n�mero n�o mais poder� ser usado na simula��o
	insertInLSPTable(LSPid, source, dst, cir, cbs, pir, minPolUnit, maxPktSize, setPrio, holdPrio); //insere os par�metros desta LSP na Tabela LSP
	if (preempt==0)
		pkt=createPathLabelControlMsg(source, dst, er, LSPid);  //inicia constru��o da LSP sem preemp��o
	else
		pkt=createPathPreemptControlMsg(source, dst, er, LSPid); //inicia constru��o da LSP com preemp��o
	return pkt;
}

/* ESTABELECE (CRIA) UMA LSP BACKUP ENTRE DOIS NODOS
*
*  Dada uma rota expl�cita e um LSPid de uma LSP j� estabelecida, esta fun��o criar� uma backup LSP entre o primeiro e o �ltimo nodo da rota
*  (entre os Merge Points de ingresso e egresso, pois), se houver recursos dispon�veis por todo o caminho.
*  Os par�metros a passar s�o o nodo fonte (Merge Point (MP) de ingresso), o nodo destino (MP de egresso) (que n�o s�o necessariamente os mesmos
*  LER de ingresso e LER de egresso da working LSP), o objeto rota expl�cita e o LSPid da working LSP.  Todos os demais par�metros de
*  Constraint Routing da LSP ser�o copiados da working LSP.
*  Se uma working LSP n�o for encontrada no MP de ingresso, ent�o a fun��o n�o estabelecer� nenhum backup.
*  As reservas de recursos ser�o feitas ao longo do caminho indicado, exceto se a working LSP participar do mesmo caminho em algum ponto (ou seja,
*  se o link (representado neste simulador pelo oIface) de sa�da da backup LSP e working LSP for o mesmo).
*  Neste caso, a backup LSP e a working LSP compartilhar�o a reserva j� feita.
*  A fun��o verifica se a working LSP j� existe (informado pelo LSPid).  Se n�o existir, a fun��o retorna NULL (a fun��o que chama deve testar isso,
*  se for de interesse).
*/
struct Packet *setBackupLSP(int LSPid, int sourceMP, int dstMP, int er[]) {
	struct Packet *pkt;
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL || p->tunnelDone==0) //verifica se working LSP j� existe E foi completada totalmente, do LER de ingresso ao LER de egresso
		return NULL; //working LSP ainda n�o existe ou n�o est� completa; n�o cria backup, retorna com NULL

	//aqui deve entrar gera��o de mensagem PATH_DETOUR
	pkt=createPathDetourControlMsg(sourceMP, dstMP, er, LSPid);
	return pkt;
}

/* CRIA UMA MENSAGEM DE CONTROLE
*
*  Cria um pacote contendo a mensagem de controle.
*  Esta fun��o deve receber via um ponteiro char o tipo de mensagem de controle a ser criada.
*  A fun��o que chama deve cuidar de enviar a mensagem adiante.
*/
static struct Packet *createControlMsg(int length, int source, int dst, char *msgType) {
	struct Packet *pkt;
	static int msgID = 0; /*inicializa msgID; o m�dulo TARVOS foi concebido de modo que esta msgID n�o se repita ao longo
						  da simula��o e do dom�nio MPLS.*/
	
	msgID++; //incrementa para uma nova mensagem de controle; o primeiro n�mero v�lido � 1
	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte est� vinculada, isto permite que v�rias fontes gerem para o mesmo nodo (fontes com tipos de gera��o diferentes)
	pkt->dst = dst; //Nodo ao qual o sorvedouro est� vinculado
	pkt->outgoingLink=0; //necess�rio para o correto funcionamento do MPLS e de v�rias fun��es do TARVOS; indica que o pacote est� sendo gerado no nodo
	strcpy(pkt->lblHdr.msgType, msgType);
	pkt->lblHdr.msgID=msgID;
	pkt->lblHdr.msgIDack=0;
	pkt->lblHdr.LSPid=0;
	pkt->lblHdr.priority=tarvosParam.ctrlMsgPrio; //coloca a prioridade das mensagens de controle no pacote
	return pkt;
	//schedulep(ev, 0, pkt->id, pkt);  //colocar na fun��o espec�fica de cria��o de mensagem de controle
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_LABEL_REQUEST
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel entre o nodo source e nodo dst.  Os recursos s�o pr�-reservados tamb�m por esta mensagem.
*/
struct Packet *createPathLabelControlMsg(int source, int dst, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_LABEL_REQUEST"); //cria uma mensagem tipo PATH_LABEL REQUEST
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_LABEL_MAPPING
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servir� para tratar e escalonar
*  o evento de recep��o da mensagem pelo nodo origem.
*  A rota expl�cita indicada, tipicamente, � a rota inversa percorrida pela mensagem PATH relacionada � mensagem RESV.
*  A fun��o de processamento da mensagem PATH deve gerar as informa��es pertinentes (inclusive a rota expl�cita inversa) e pass�-las para a presente
*  fun��o.  Cada nodo que receber uma mensagem RESV_LABEL_MAPPING deve buscar a mensagem PATH_LABEL_REQUEST correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB e LSP Table, de modo a construir a LSP.  Esta mensagem RESV cont�m o mapeamento de r�tulo adequado para a constru��o
*  das tabelas.
*  A princ�pio, a mensagem RESV n�o precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estour�-lo, causar� a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV n�o requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de r�tulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados.  O LSP Tunnel s� 
*  estar� completamente estabelecido e v�lido quando a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso isto n�o aconte�a, os recursos e paths
*  estabelecidos sofrer�o timeout, pois n�o haver� REFRESH.
*/
struct Packet *createResvMapControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_LABEL_MAPPING"); //cria uma mensagem tipo RESV_LABEL_MAPPING
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV n�o requer ACK, portanto n�o � necess�rio coloc�-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=label; //coloca o label criado pelo LSR na mensagem; este r�tulo dever� ser usado em pacotes enviados para este nodo-LSPid
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* RECUPERA R�TULO PARA UM DETERMINADO WORKING LSP EM UM LSR (OU NODO)
*
*  Esta fun��o recebe o n�mero do nodo (que � o LSR) e o n�mero do LSP (LSPid), ambos inteiros, e devolve o r�tulo (iLabel) mapeado para este LSP
*  se o LSP estiver em modo "up" (para working LSP).  A consulta � feita na LIB.  S� pode haver uma (working) LSP �nica em todo o dom�nio MPLS
*  neste simulador, mas o n�mero do r�tulo pode ser repetido em LSPs diferentes e inclusive em interfaces diferentes.
*  A fun��o devolve -1 para o caso de n�o ter achado uma LSP v�lida no nodo em quest�o.
*/
int getWorkingLSPLabel(int n_node, int LSPid) {
	struct LIBEntry *entry;

	entry=searchInLIBnodLSPstat(n_node, LSPid, "up");
	return (entry==NULL)? (-1):(entry->iLabel);
}

/* RESERVA RECURSOS EM UM LINK A PARTIR DOS REQUERIMENTOS DE UMA LSP
*
*  Esta fun��o tentar� reservar recursos solicitados para o estabelecimento de uma LSP no link indicado pelo pacote.
*  Os recursos s�o indicados, na estrutura do link, pelos par�metros CBS, CIR e PIR.  A fun��o comparar� os n�meros
*  pedidos pela LSP com os ainda dispon�veis no link; se os dispon�veis forem iguais ou maiores (todos eles), ent�o a fun��o diminuir� os
*  n�meros dispon�veis na estrutura do link, o que significa a aceita��o da reserva.  Se algum n�mero dispon�vel for menor que o solicitado
*  pela LSP, ent�o nada ser� modificado, o que significa a n�o aceita��o da reserva.
*  A fun��o retorna 1 se a reserva foi aceita e feita, e retorna 0 (ZERO) se a reserva n�o foi aceita (ou se a LSP indicada n�o foi achada na
*  Tabela de LSPs).
*/
int reserveResouces(int LSPid, int link) {
	struct LSPTableEntry *lsp;
	
	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; teste os recursos dispon�veis
		//testa agora se cada recurso solicitado para a LSP est� dispon�vel; se apenas um n�o estiver, retorne sem reserva
		if (tarvosModel.lnk[link].availCbs >= lsp->cbs && tarvosModel.lnk[link].availCir >= lsp->cir && tarvosModel.lnk[link].availPir >= lsp->pir) {
			//todos os recursos est�o dispon�veis; fa�a a reserva
			tarvosModel.lnk[link].availCbs -= lsp->cbs;
			tarvosModel.lnk[link].availCir -= lsp->cir;
			tarvosModel.lnk[link].availPir -= lsp->pir;
			return 1; //recursos foram efetivamente reservados
		}
	}
	return 0; //nenhuma reserva foi feita
}

/* RETORNA RECURSOS RESERVADOS POR UMA LSP PARA UM LINK
*
*  Esta fun��o devolver� os recursos reservados ou pr�-reservados para o estabelecimento de uma LSP para o link apropriado.
*  Os recursos s�o indicados, na estrutura do link, pelos par�metros CBS, CIR e PIR.
*  O retorno dos recursos devem-se � derrubada ou desativa��o de uma LSP, � falha em estabelecer uma nova LSP, � expira��o de uma mensagem
*  PATH_LABEL_REQUEST (que tamb�m faz pr�-reserva de recursos).
*/
static void returnResources(int LSPid, int link) {
	struct LSPTableEntry *lsp;

	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; devolva os recursos
		tarvosModel.lnk[link].availCbs += lsp->cbs;
		tarvosModel.lnk[link].availCir += lsp->cir;
		tarvosModel.lnk[link].availPir += lsp->pir;
	}
	/*Seria interessante colocar aqui uma rotina para verificar a coer�ncia dos valores availCbs, Cir e Pir.  Pode acontecer, por algum erro,
	que estes valores, ap�s retorno, superem a largura de banda caracter�stica do link, ficando pois irreais.  Mais seria portanto necess�rio
	comparar os valores dispon�veis com valores m�ximos te�ricos.*/
}


/* RETORNA PAR�METRO CBS (COMMITTED BUCKET SIZE) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
double getLSPcbs(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cbs;
}

/* RETORNA PAR�METRO CIR (COMMITTED INFORMATION RATE) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
double getLSPcir(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cir;
}

/* RETORNA PAR�METRO PIR (PEAK INFORMATION RATE) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
double getLSPpir(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->pir;
}

/* RETORNA PAR�METRO cBUCKET (COMMITTED BUCKET OU TAMANHO ATUAL DO BUCKET) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
double getLSPcBucket(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cBucket;
}

/* RETORNA PAR�METRO MAXPKTSIZE (MAXIMUM PACKET SIZE) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
int getLSPmaxPktSize(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->maxPktSize;
}

/* RETORNA PAR�METRO MINPOLUNIT (MININUM POLICED UNIT) DA LSP TABLE
*
*  Passar o n�mero da LSP como par�metro.  Se a LSPid n�o for encontrada, -1 ser� retornado.
*/
int getLSPminPolUnit(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->minPolUnit;
}

/* CHECA TIMEOUT DAS LSPs E MENSAGENS DE CONTROLE
*
*  Esta fun��o checa todas as LSPs de cada nodo, e tamb�m a fila de mensagens de controle de cada nodo, a fim de verificar se seus tempos de timeout
*  superam o tempo atual.  Caso positivo, ent�o o timeout estar� comprovado e a fun��o tomar� as a��es apropriadas em cada caso.
*
*  Para uma LSP, se o timeout for controlado pelas mensages RESV, o estado desta dever� ser colocado em "timed-out" e os recursos reservados no link de sa�da para a LSP dever�o ser devolvidos ou restaurados.
*  Ou, a rotina de rapid recovery deve ser iniciada.
*
*  Para o intervalo de detec��o de falha de comunica��o (nodo n�o alcan��vel atrav�s de mensagem HELLO), as LSPs entre os nodos envolvidos como origem e destino
*  devem ser colocadas em estado "HELLO failure" e a rotina de rapid recovery ativada.
*
*  Para mensagens de controle, o timeout existe para mensagens PATH_LABEL_REQUEST, PATH_REFRESH e HELLO.  As mensagens RESV, via de regra, n�o necessitando
*  de ACK, n�o carecem de timeout.
*
*  O timeout de uma PATH_LABEL_REQUEST deve elimin�-la da fila e devolver os recursos pr�-reservados no link de sa�da para o link.  Como a mensagem
*  RESV_LABEL_MAPPING correspondente ainda n�o foi recebida, ent�o n�o h� uma entrada na LIB para colocar como "down".
*
*  O timeout de uma PATH_REFRESH deve eliminar a mensagem da fila.  A LSP n�o deve ser derrubada, pois isto ser� feito explicitamente no timeout da LSP.
*  Verificar se o processo de rapid recovery deve ser ativado aqui.
*
*  O timeout de uma mensagem HELLO deve elimin�-la da fila, somente.
*/
void timeoutWatchdog() {
	double now;

	now=simtime();
	LSPtimeoutCheck(now); //Processa as LSPs para timeout, usando a Tabela LIB
	nodeCtrlMsgTimeoutCheck(now); //verifica timeout de mensagens de controle
	helloFailureCheck(now); //verifica timeout espec�fico indicado pelas mensagens HELLO (ou falhas de recebimento destas)
}

/* VERIFICA TIMEOUT DO INTERVALO DE VERIFICA��O DE FALHA DE RECEBIMENTO DE HELLO DE NODO VIZINHO OU ADJACENTE
*
*  No RSVP-TE, as mensagens HELLO podem ser usadas para determinar se nodos vizinhos est�o alcan��veis (RFC 3209).  Para isto, o nodo envia uma mensagem
*  HELLO para o nodo vizinho e este deve responder com uma HELLO_ACK dentro do intervalo m�ximo definido por tarvosModel.node[node].helloTimeLimit[link].
*  Este intervalo � definido, por default, como o intervalo de gera��o de HELLOs * 3,5.  O intervalo de gera��o default � 5ms, portanto o intervalo
*  m�ximo de recebimento � 17,5ms.  Se o intervalo for ultrapassado, ent�o o nodo � considerado inalcan��vel ou com falha.
*  Na RFC 3209, h� um esquema de n�meros instanciados para cada nodo de forma a determinar se o nodo foi ressetado; os n�meros s�o enviados dentro da
*  HELLO e HELLO_ACK e comparados com n�meros anteriores armazenados.  Nesta implementa��o do simulador, isto n�o � usado; apenas o recebimento de uma
*  HELLO_ACK dentro do intervalo, correspondente (msgIDack) � HELLO enviada, � verificada.
*  Ao receber uma HELLO_ACK, o nodo verifica se existe uma HELLO com mesmo msgIDack na fila de mensagens de controle do nodo.  Se houver, ent�o o valor
*  do helloTimeLimit no tarvosModel.node � atualizado para mais um intervalo de detec��o de falha (3,5 * helloInterval, por default).  Caso uma mensagem
*  HELLO com mesmo msgIDack n�o seja encontrada, ent�o a HELLO_ACK � descartada e o helloTimeLimit n�o � atualizado.
*
*  Esta rotina precisa verificar o tempo helloTimeLimit de cada link saindo de um nodo, ou seja, links com src no nodo atual e todos os dst, isto para cada
*  nodo do sistema.  Se o helloTimeLimit for menor ou igual a simtime(), ent�o as LSPs que usam o link em quest�o devem ser colocadas em estado "dst unreachable".
*  Os recursos reservados para as LSPs devem ser retornados aos links.  Uma LSP backup faz sua pr�pria reserva de recursos, pois em princ�pio uma LSP backup
*  segue um caminho (path) diverso da LSP prim�ria ou working LSP (entre os Merge Points).
*  Para minimizar processamento, a busca � feita a partir da LIB, pois, se n�o houver uma LSP conectando nodos, n�o importa para a simula��o se eles est�o
*  alcan��veis ou n�o.
*  Obs.:  um HelloTimeLimit igual a zero significa que o tempo n�o precisa ser checado para este link em particular.
*/
static void helloFailureCheck(double now){
	struct LIBEntry *p;
	char mainTraceString[255];
	double tmp;
	
	p=lib.head->previous; //percorre no sentido inverso
	sprintf(mainTraceString, "HELLO failure CHECK:  simtime:  %f\n", now);
	mainTrace(mainTraceString);
	
	while (p!=lib.head) { //s� testa timeout para LSPs "up"
		tmp=(*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface];
		if (strcmp(p->status, "up")==0 && (*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface]>0 && (*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface]<=now) { /*testa timers para link de sa�da (oIface);
																															   se o helloTimeLimit for zero, ent�o a checagem
																															   � desativada para aquele link*/
			strcpy(p->status, "dst fail (HELLO)"); //os recursos reservados para a LSP devem ser retornados ao link como dispon�veis
			p->timeoutStamp = now; //coloca o rel�gio atual no marcador timeoutStamp

			sprintf(mainTraceString, "HELLO failure STAMP - dst fail (HELLO):  node:  %d  oIface:  %d  simtime:  %f\n", p->node, p->oIface, now);
			mainTrace(mainTraceString);

			//os recursos devem ser sempre retornados ao link, mesmo com backup LSP (pois a backup LSP n�o trafegar�, logicamente, pelo link falho)
			returnResources(p->LSPid, p->oIface); //retorna os recursos reservados ao link; o campo oIface da LIB indica o n�mero do "link" de sa�da
			/*RAPID RECOVERY deve vir aqui*/
			rapidRecovery(p->node, p->LSPid, p->iIface, p->iLabel); //aciona o Rapid Recovery; a fun��o se encarrega de checar se existe backup LSP
		}
		p=p->previous; //atualiza para a entrada anterior na tabela LIB
	}
}

/* VERIFICA TIMEOUT DAS LSPs
*  A tabela verificada �, na verdade, a LIB, que cont�m todas as entradas, por nodo, das LSPs.
*  Como conven��o, se o campo timeout for 0, ent�o a fun��o de timeout est� desativada (ou seja, as LSP nunca entram em timeout).
*/
static void LSPtimeoutCheck(double now) {
	struct LIBEntry *p;

	if (lib.head==NULL) //LIB n�o existe; nada fa�a
		return;
	
	p=lib.head->previous; //percorre no sentido inverso
	while (p!=lib.head) {
		if (strcmp(p->status, "up")==0 && p->timeout <= now && p->timeout > 0) { //s� testa timeout para LSPs "up"; deve-se testar para backup tamb�m?
			strcpy(p->status, "timed-out"); //calma; os recursos reservados para a LSP devem ser retornados ao link como dispon�veis
			p->timeoutStamp = simtime(); //coloca o rel�gio atual no marcador timeoutStamp
			//os recursos devem ser sempre retornados ao link, mesmo com backup LSP (pois a backup LSP n�o trafegar�, logicamente, pelo link falho)
			returnResources(p->LSPid, p->oIface); //retorna os recursos reservados ao link; o campo oIface da LIB indica o n�mero do "link" de sa�da
			/*RAPID RECOVERY deve vir aqui*/
			rapidRecovery(p->node, p->LSPid, p->iIface, p->iLabel); //aciona o Rapid Recovery; a fun��o se encarrega de checar se existe backup LSP
		}
		p=p->previous; //atualiza para a entrada anterior na tabela LIB
	}
}

/* VERIFICA TIMEOUT DAS MENSAGENS NAS FILAS DE MENSAGENS DE CONTROLE DOS NODOS
*/
static void nodeCtrlMsgTimeoutCheck(double now) {
	struct nodeMsgQueue *p;
	int i, msgID;

	for (i=1; i<=sizeof(tarvosModel.node)/sizeof(*(tarvosModel.node))-1; i++) {
		p=tarvosModel.node[i].nodeMsgQueue->next;
		while (p!=tarvosModel.node[i].nodeMsgQueue) { //o ponteiro node.nodeMsgQueue aponta para o n� HEAD da lista
			if (p->timeout <= now) { //se timeout registrado na mensagem for menor ou igual a NOW, ent�o elimine a mensagem da fila
				if (p->resourcesReserved==1) //retorna os recursos para o link de sa�da se houver pr�-reserva (mensagem PATH_LABEL_REQUEST e PATH_LABEL_REQUEST_PREEMPT)
					returnResources(p->LSPid, p->oIface);
				msgID=p->msgID; //armazena o msgID desta mensagem a ser eliminada da fila
				p=p->next; //p deve ser avan�ado agora, pois esta posi��o da mem�ria ser� eliminada
				removeFromNodeMsgQueueAck(i, msgID);
			} else
				p=p->next; //s� avance aqui se o if for falso, pois o ponteiro j� � avan�ado dentro do if
		}
	}
}

/* PERFAZ REFRESH DAS LSPs E RECURSOS RESERVADOS
*
*  Esta fun��o gera mensagens PATH_REFRESH para todos os nodos e todas as LSPs em cada nodo, desde que estas LSPs tenham *origem* no nodo e estejam
*  em modo "up".  Para testar a origem das LSPs, o campo iIface � testado para zero na lista LIB, pois o zero ali � indicativo que o nodo nesta tupla
*  � o nodo de origem do t�nel LSP.
*  Observar que a rotina de gera��o das mensagens faz a busca em todos os nodos, de modo sincronizado, mas a gera��o de mensagens PATH_REFRESH n�o �
*  � sincronizada gra�as a uma gera��o aleat�ria de um intervalo de tempo de escalonamento para cada mensagem gerada.  Esta condi��o foi relatada na
*  RFC 2205 como desejada (um nodo deve gerar um intervalo de tempo aleat�rio entre 0,5 e 1,5 * Refresh_Interval) para evitar problemas.
*  Nesta implementa��o, � usado um intervalo aleat�rio entre 0 e 1,5 * Refresh_Interval adicionado ao Refresh_Interval (resultando em um intervalo total
*  entre 1 e 1,5 Refresh_Interval), pois o escalonamento do evento de refreshLSP � feito a cada 1 * Refresh_Interval.  N�o enxergo problemas nesta solu��o
*  de implementa��o, mas isto pode ser estudado mais profundamente.  O intervalo aleat�rio � calculado dentro da fun��o de cria��o de mensagens PATH_REFRESH.
*
*  Quando rec�m-criada, uma LSP pode sofrer o primeiro refresh, no pior caso, no tempo m�ximo configurado para refresh
*  nos par�metros da simula��o.  Em outros casos, este primeiro refresh ocorrer� em tempo inferior ao m�ximo, e, a partir de ent�o, os pr�ximos refresh
*  ocorrer�o em sincronia com todos os nodos e LSPs da topologia (ressalvado o intervalo aleat�rio descrito acima).
*  O refresh global -- feito por esta rotina -- deve ter um evento relacionado, programado portanto logo no in�cio da simula��o pelo usu�rio e
*  re-escalonado continuamente com o intervalo interevento configurado nos par�metros da simula��o.
*/
void refreshLSP() {
	struct LIBEntry *p;
	struct Packet *pkt;
	struct LSPTableEntry *lsp;

	p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
	while (p!=lib.head) {
		if (p->iIface==0 && strcmp(p->status, "up")==0) { //s� faz refresh para LSPs "up", todos os nodos
			lsp = searchInLSPTable(p->LSPid); //busca a entrada na LSP Table para o t�nel LSP encontrado
			if (lsp == NULL) { //entrada na LSP Table n�o encontrada; incongru�ncia nos bancos de dados!
				printf("\nError - refreshLSP - related entries in LIB and LSP Table not found");
				exit (1);
			}
			/*cria mensagem de controle PATH_REFRESH e escalona.  O src ser� o nodo atual da LIB, o destino, coletado na LSP Table.
			Isto � necess�rio porque as working LSPs come�am e terminam nos nodos src e dst constantes na LSP Table.  Por�m, as backup LSPs
			come�am em nodos diferentes do src da working LSP, mas terminam no mesmo nodo dst.*/
			pkt = createPathRefreshControlMsg(p->node, lsp->dst, p->LSPid, p->iLabel);
		}
		p=p->previous;
	}
}

/* INICIA GERA��O DAS MENSAGENS HELLO PARA TODOS OS NODOS
*
*  Esta fun��o l� todo o array de links.  Para cada link, uma mensagem HELLO � gerada do nodo origem ao nodo destino conectado pelo link.  Assim, os
*  nodos mandam mensagens HELLO para todos os outros nodos conectados a si, desde que seja origem.
*  Nesta implementa��o, as mensagens HELLO ser�o geradas segundo o intervalo HELLO_Interval definido nos par�metros da simula��o e usa-se mecanismo para
*  evitar sincroniza��o na gera��o das mensagens, inspirado na RFC 2205, item 3.7 (para mensagens de controle PATH_REFRESH).
*  N�o foi achada, apesar disso, nenhuma restri��o ao sincronismo de mensagens HELLO na RFC 3209, mas verificou-se na pr�tica que racing conditions podem ocorrer.
*  Como as mensagens HELLO s�o geradas com destino ao n� vizinho (neighbor node), o TTL deve ser definido como 1, segundo a RFC 3209.
*  A falha em receber uma mensagem HELLO ou HELLO_ACK dentro do intervalo especificado em tarvosParam.helloTimeout indica que houve falha de comunica��o
*  entre os nodos relacionados.
*/
void generateHello() {
	struct Packet *pkt;
	int links, i;
	
	links=(sizeof tarvosModel.lnk / sizeof *(tarvosModel.lnk))-1;
	for (i=1; i<=links; i++) { //percorre todo o array de links, de modo que cada nodo origem gere uma mensagem HELLO para o nodo destino do link
		pkt = createHelloControlMsg(tarvosModel.lnk[i].src, tarvosModel.lnk[i].dst);
		pkt->er.explicitRoute = (int*)malloc(sizeof (*(pkt->er.explicitRoute))); //cria espa�o para um int
		if (pkt->er.explicitRoute == NULL) {
			printf("\nError - generateHello - insufficient memory to allocate for explicit route object");
			exit (1);
		}
		*pkt->er.explicitRoute = tarvosModel.lnk[i].dst; //coloca o nodo destino na rota expl�cita; isto ter� de ser removido com free antes da remo��o do pacote
		pkt->er.erNextIndex = 0; //assegura que �ndice aponta para in�cio da rota expl�cita
		pkt->lblHdr.label = 0; //assegura que n�o h� label v�lido
		pkt->outgoingLink = 0; //marca pacote como gerado no nodo origem
	}
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO HELLO
*
*  Cria um pacote contendo a mensagem de controle HELLO e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  O pacote de controle criado ser� escalonado para um tempo entre 0 e 0,01 * Hello_Interval a partir do simtime() atual.  Isto �
*  feito a fim de evitar a sincroniza��o do envio das mensagens HELLO pelos nodos, inspirado pela RFC 2205, item 3.7.
*  (para mensagens PATH_REFRESH).  Sem esse recurso, os pacotes HELLO s�o gerados, para todos os nodos, de forma sincronizada, ao mesmo tempo.
*  Observou-se, nos testes, que isso pode levar a racing conditions, onde especificamente o segundo link da lista de links apresenta um n�mero
*  de dequeues bem menor que os outros links da lista (para simula��es acima de 120 segundos).
*  Aparentemente, a ordem de enfileiramento dos eventos de alguma forma privilegia este segundo link, e seus pacotes acabam sendo enfileirados
*  bem menos frequentemente que outros links.  Apesar disso, a utiliza��o, tamanho m�ximo e m�dio de filas n�o parecem se alterar com a sincroniza��o.
*  Este recurso de dessincroniza��o tende a aumentar o n�mero de dequeues para simula��es curtas, mas reduz substanciamente o n�mero de dequeues
*  para simula��es mais longas, e homogeiniza este n�mero para todos os links.
*/
struct Packet *createHelloControlMsg(int source, int dst) {
	struct Packet *pkt;
	double randInterval; //intervalo de tempo aleat�rio entre 0 e 0,01*HELLO_Interval que ser� somado ao tempo atual para escalonamento do pacote
	
	pkt = createControlMsg(tarvosParam.helloMsgSize, source, dst, "HELLO"); //cria uma mensagem tipo HELLO
	pkt->er.recordThisRoute=1; //ative a grava��o da rota no pacote, para uso da mensagem HELLO_ACK de volta (se necess�rio)
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	//pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	randInterval = uniform(0, 0.01) * tarvosParam.helloInterval; //cria intervalo de tempo aleat�rio entre 0 e 0,01 * HELLO_Interval, para evitar sincronismo de gera��o de mensagens HELLO
	pkt->generationTime=simtime() + randInterval; //marca o tempo em que o pacote foi gerado
	pkt->ttl = 1; //limite o Time To Live a 1 hop, conforme indicado na RFC 3209, item 5.1
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), randInterval, pkt->id, pkt); //escalona evento de tratamento HELLO imediatamente
	//schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt); //escalona evento de tratamento HELLO imediatamente
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO HELLO_ACK
*
*  Cria um pacote contendo a mensagem de controle HELLO_ACK e escalona evento recuperado do objeto nodo.  Este evento tipicamente servir� para tratar e escalonar
*  o evento de recep��o da mensagem pelo nodo origem.
*  A rota seguida, tipicamente, � a rota inversa percorrida pela mensagem HELLO relacionada � mensagem HELLO_ACK.  A fun��o de processamento da mensagem HELLO deve
*  gerar as informa��es pertinentes e pass�-las para a presente fun��o.  Cada nodo que receber uma mensagem HELLO_ACK deve buscar a mensagem
*  HELLO correspondente na Fila de Mensagens de Controle do Nodo e retir�-la, e tamb�m atualizar o timeout da LSP para o vizinho (espec�fico para HELLO).
*  A LSP entrar� em timeout se uma mensagem HELLO ficar sem ACK por tempo superior ao seu timeout-HELLO.
*  Se a mensagem HELLO_ACK n�o encontrar uma HELLO correspondente, descart�-la simplesmente?
*  A mensagem HELLO_ACK n�o requer outro ACK e portanto n�o entra na fila de mensagens de controle do nodo.
*/
struct Packet *createHelloAckControlMsg(int source, int dst, int er[], int msgIDack) {
	struct Packet *pkt;


	pkt = createControlMsg(tarvosParam.helloMsgSize, source, dst, "HELLO_ACK"); //cria uma mensagem tipo HELLO_ACK
	attachExplicitRoute(pkt, er);
	//uma mensagem HELLO_ACK n�o requer ACK, portanto n�o � necess�rio coloc�-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	pkt->ttl = 1; //limite o Time To Live a 1 hop, conforme indicado na RFC 3209, item 5.1
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt); //escalona evento de tratamento imediatamente
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_REFRESH
*
*  Cria um pacote contendo a mensagem de controle para refresh da LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  O pacote de controle criado ser� escalonado para um tempo entre 0 e 0,5 * Refresh_Interval a partir do simtime() atual.  Isto �
*  feito a fim de evitar a sincroniza��o do envio das mensagens PATH_REFRESH pelos nodos, conforme indicado pela RFC 2205, item 3.7.
*  A RFC 2205 indica, por�m, um intervalo entre 0,5 e 1.5 * Refresh_Interval, mas, no simulador, n�o � poss�vel escalonar para
*  tempos menores que o pr�prio Refresh_Interval, devido � forma como foi implementado.
*
*  ATEN��O:  O pacote contendo a mensagem PATH_REFRESH seguir� comutado por r�tulo, portanto a fun��o que chama dever� passar o r�tulo correto
*  como par�metro.  Caso contr�rio, a mensagem perder-se-�.
*/
struct Packet *createPathRefreshControlMsg(int source, int dst, int LSPid, int iLabel) {
	struct Packet *pkt;
	double randInterval; //intervalo de tempo aleat�rio entre 0 e 0,5*Refresh_Interval que ser� somado ao tempo atual para escalonamento do pacote
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_REFRESH"); //cria uma mensagem tipo PATH_REFRESH
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label = iLabel; //coloca o label inicial no pacote, para que esta mensagem prossiga comutada por r�tulo
	pkt->er.explicitRoute = NULL; //certifica que n�o h� rota expl�cita no pacote
	pkt->er.recordThisRoute=1; //ative a grava��o da rota no pacote, para uso da mensagem RESV_REFRESH de volta
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	randInterval = uniform(0, 0.5) * tarvosParam.LSPrefreshInterval; //cria intervalo de tempo aleat�rio entre 0 e 0,5 * Refresh_Interval, para evitar sincronismo de gera��o de mensagens PATH_REFRESH (RFC 2205)
	pkt->generationTime=simtime() + randInterval; //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), randInterval, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_REFRESH
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servir� para tratar e escalonar
*  o evento de recep��o da mensagem pelo nodo origem.
*  A rota seguida, tipicamente, � a rota inversa percorrida pela mensagem PATH relacionada � mensagem RESV.  A fun��o de processamento da mensagem PATH deve
*  gerar as informa��es pertinentes e pass�-las para a presente fun��o.  Cada nodo que receber uma mensagem RESV_REFRESH deve buscar a mensagem
*  PATH_REFRESH correspondente na Fila de Mensagens de Controle do Nodo e recalcular o timeout da LSP, modificando a entrada na LIB.
*  O timeout � calculado com base no momento em que a mensagem RESV_REFRESH � recebida.
*  Se a mensagem RESV_REFRESH n�o encontrar uma PATH_REFRESH correspondente, ainda assim dever� fazer o refresh na LSP?
*  A princ�pio, a mensagem RESV n�o precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estour�-lo, causar� a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV n�o requer ACK.)
*/
struct Packet *createResvRefreshControlMsg(int source, int dst, int er[], int msgIDack, int LSPid) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_REFRESH"); //cria uma mensagem tipo RESV_REFRESH
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV n�o requer ACK, portanto n�o � necess�rio coloc�-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* INICIA A CHECAGEM DE TIMEOUTS E OS TIMERS DE REFRESH
*
*  Esta fun��o escalona os eventos iniciais de checagem de timeouts de mensagens de controle e LSPs e tamb�m a gera��o de mensagens de REFRESH
*  (PATH_REFRESH e RESV_REFRESH) e HELLO.  Os eventos escalonados est�o contidos na estrutura de par�metros da simula��o e devem ser tradados no
*  programa do usu�rio.
*  Par�metros a receber:  evento de controle de timeout; evento de controle de refresh; evento de gera��o de HELLOs
*/
void startTimers(int timeoutEv, int refreshEv, int helloEv) {
	if (timeoutEv > 0)
		schedulep(timeoutEv, tarvosParam.timeoutWatchdog, -1, NULL); //escalona timeoutWatchdog
	if (refreshEv > 0)
		schedulep(refreshEv, tarvosParam.LSPrefreshInterval, -1, NULL); //escalona refresh
	if (helloEv > 0)
		schedulep(helloEv, tarvosParam.helloInterval, -1, NULL); //inicia gera��o de HELLOs
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_DETOUR
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel Backup entre o nodo sourceMP e nodo dstMP (source Merge Point e destination Merge Point).
*  Os recursos ser�o compartilhados em caso da backup compartilhar o mesmo link que a working LSP, e ser�o pr�-reservados tamb�m por esta mensagem
*  caso n�o haja reserva pr�via para a working LSP.
*/
struct Packet *createPathDetourControlMsg(int sourceMP, int dstMP, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, sourceMP, dstMP, "PATH_DETOUR"); //cria uma mensagem tipo PATH_DETOUR
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_DETOUR_MAPPING
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servir� para tratar e escalonar
*  o evento de recep��o da mensagem pelo nodo origem.
*  A rota expl�cita indicada, tipicamente, � a rota inversa percorrida pela mensagem PATH relacionada � mensagem RESV.
*  A fun��o de processamento da mensagem PATH deve gerar as informa��es pertinentes (inclusive a rota expl�cita inversa) e pass�-las para a presente
*  fun��o.  Cada nodo que receber uma mensagem RESV_DETOUR_MAPPING deve buscar a mensagem PATH_DETOUR correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB, de modo a construir a backup LSP.  Esta mensagem RESV cont�m o mapeamento de r�tulo adequado para a constru��o
*  das tabelas.  Pontos de jun��o (Merge Points) devem ser previstos tamb�m pelo processamento da RESV_DETOUR.
*  A princ�pio, a mensagem RESV n�o precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estour�-lo, causar� a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV n�o requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de r�tulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados, quando necess�rio.
*  O LSP Tunnel s� estar� completamente estabelecido e v�lido quando a mensagem RESV_DETOUR_MAPPING atingir o MP de ingresso.  Caso isto n�o aconte�a,
*  os recursos e paths estabelecidos sofrer�o timeout, pois n�o haver� REFRESH.
*/
struct Packet *createResvDetourControlMsg(int sourceMP, int dstMP, int er[], int msgIDack, int LSPid, int oLabel) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, sourceMP, dstMP, "RESV_DETOUR_MAPPING"); //cria uma mensagem tipo RESV_DETOUR_MAPPING
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV n�o requer ACK, portanto n�o � necess�rio coloc�-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=oLabel; //coloca o label copiado da working LSP na mensagem; este r�tulo dever� ser usado em pacotes enviados para este nodo-LSPid (para mapping)
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* DEVOLVE O STATUS DO T�NEL LSP:  COMPLETADO OU N�O
*
*  Um t�nel LSP est� completo se a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso contr�rio, o t�nel ainda n�o est� completado,
*  o que significa que nem todos os caminhos entre os nodos est�o populados para o t�nel.
*  Esta fun��o retorna o valor do campo tunnelDone da estrutura LSP Table, para um dado LSPid (identificador globalmente �nico do t�nel LSP).
*  O campo tunnelDone ser� ZERO se o t�nel n�o est� completo; ser� 1 se estiver completo.
*  Retorna -1 caso o t�nel LSP n�o tenha sido encontrado na LSP Table.
*/
int getLSPtunnelStatus(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	return (p==NULL)? (-1):(p->tunnelDone);
}

/* ATUALIZA O STATUS DO T�NEL LSP PARA COMPLETADO
*
*  Marca o campo tunnelDone na LSP Table, para uma determinada LSPid, para o valor 1.
*  Retorna 1 se a opera��o foi bem sucedida, ou -1 para o caso da LSPid n�o ter sido encontrada na tabela.
*/
int setLSPtunnelDone(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL)
		return -1; //LSPid n�o encontrado; retorne -1
	else
		p->tunnelDone=1;
	return 1; //opera��o bem sucedida; retorne 1
}

/* ATUALIZA O STATUS DO T�NEL LSP PARA N�O-COMPLETADO
*
*  Marca o campo tunnelDone na LSP Table, para uma determinada LSPid, para o valor 0.
*  Retorna 0 se a opera��o foi bem sucedida, ou -1 para o caso da LSPid n�o ter sido encontrada na tabela.
*/
int setLSPtunnelNotDone(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL)
		return -1; //LSPid n�o encontrado; retorne -1
	else
		p->tunnelDone=0;
	return 0; //opera��o bem sucedida; retorne 0
}

/* RAPID RECOVERY
*
*  Implementa��o do recurso de Rapid Recovery do MPLS.  Esta fun��o, ativada por uma sinaliza��o de timeout vinda de um PATH_REFRESH ou
*  HELLO, buscar� uma backup LSP para a working LSP sinalizada.
*  O algoritmo �:  buscar, para o nodo atual e LSPid atual, uma entrada na LIB para uma backup LSP (de mesma LSPid) com iIface igual a zero, ou seja,
*  uma backup LSP que inicie no nodo atual (chamado de Merge Point).  Encontrando-a, a fun��o copiar� os campos iIface e iLabel, recebidos
*  como par�metro (e que s�o os campos iIface e iLabel da timeout LSP), na entrada para a backup LSP (na LIB) encontrada.  (A LSP que entrou em
*  timeout j� deve ter sido assim marcada antes de chamada esta fun��o.)  Desta forma, a LSP ser� desviada imediatamente para a backup LSP, sem
*  qualquer sinaliza��o necess�ria, e de modo transparente para as aplica��es.
*  Podem haver v�rias backup LSPs saindo do MP em quest�o.  Neste caso, a fun��o searchInLIBnodLSPstatBak retorna a primeira "up" encontrada.  Caso esta
*  falhe no futuro, a pr�xima backup LSP poder� ser encontrada pela mesma fun��o.
*
*  A fun��o retorna 1 para o caso do Rapid Recovery ter sido bem sucedido, ou seja, uma entrada para backup LSP ter sido encontrada e o desvio, feito.
*  Neste caso, os recursos reservados para os links ser�o mantidos.
*  Retorna 0 para o caso do Rapid Recovery n�o se ter completado (nenhuma backup LSP encontrada).  Neste caso, a fun��o que processa os timeouts deve
*  retornar os recursos reservados para o link envolvido.
*/
static int rapidRecovery(int node, int LSPid, int iIface, int iLabel) {
	struct LIBEntry *p;

	p=searchInLIBnodLSPstatBak(node, LSPid, "up"); //busca uma (a primeira achada) backup LSP em status "up", que tenha como MP inicial o nodo atual (a fun��o chamada garante isso)
	if (p==NULL)
		return 0; //n�o achou backup LSP; retorne com ZERO para indicar a falha; a fun��o que chama deve tratar isso, retornando recursos reservados
	//achou backup LSP; copie os par�metros recebidos para a entrada da backup LSP, completando pois o detour
	p->iIface=iIface;
	p->iLabel=iLabel;
	return 1; //desvio completado com sucesso
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_ERR
*
*  Cria um pacote contendo a mensagem de controle de erro RESV_ERR para uma determinada LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  A mensagem RESV_ERR segue no sentido downstream, e, nesta implementa��o, segue comutada por r�tulo (a LSP deve pre-existir, portanto).
*/
struct Packet *createResvErrControlMsg(int source, int dst, int LSPid, int iLabel, char *errorCode, char *errorValue) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_ERR"); //cria uma mensagem tipo RESV_ERR
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label = iLabel; //coloca o label inicial no pacote, para que esta mensagem prossiga comutada por r�tulo
	strcpy(pkt->lblHdr.errorCode, errorCode);
	strcpy(pkt->lblHdr.errorValue, errorValue);
	pkt->er.explicitRoute = NULL; //certifica que n�o h� rota expl�cita no pacote
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_ERR
*
*  Cria um pacote contendo a mensagem de controle de erro PATH_ERR para uma determinada LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  A mensagem PATH_ERR segue no sentido upstream.  A rota inversa ser� computada nodo a nodo, atrav�s da leitura da LIB (a LSP deve pre-existir, portanto).
*/
struct Packet *createPathErrControlMsg(int source, int dst, int LSPid, char *errorCode, char *errorValue) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_ERR"); //cria uma mensagem tipo PATH_ERR
	pkt->lblHdr.LSPid=LSPid;
	strcpy(pkt->lblHdr.errorCode, errorCode);
	strcpy(pkt->lblHdr.errorValue, errorValue);
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_LABEL_REQUEST_PREEMPT
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem s�o obtidos do nodo por fun��es get espec�ficas.
*  Esta fun��o n�o cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel, com preemp��o, entre o nodo source e nodo dst.  Os recursos s�o pr�-reservados
*  por esta mensagem somente se estiverem dispon�veis imediatamente.  Se n�o estiverem, a mensagem varrer� as LSPs estabelecidas no link/nodo
*  em quest�o para verificar se as de menor prioridade, com preemp��o, disponibilizar�o os recursos.  N�o haver� pr�-reserva neste caso.
*  S� a mensagem RESV_LABEL_MAPPING_PREEMPT far� a reserva.
*  A mensagem s� segue adiante se houver possibilidade de recursos serem reservados por preemp��o ou imediatamente.
*/
struct Packet *createPathPreemptControlMsg(int source, int dst, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_LABEL_REQUEST_PREEMPT"); //cria uma mensagem tipo PATH_LABEL_REQUEST_PREEMPT
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem n�o deve ser inserida ainda na fila de mensagens de controle do nodo; � conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recep��o e transmiss�o apropriados.  A inser��o na fila de mensagens de controle deve ser feita
	quando a mensagem � *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_LABEL_MAPPING_PREEMPT
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servir� para tratar e escalonar
*  o evento de recep��o da mensagem pelo nodo origem.
*  A rota expl�cita indicada, tipicamente, � a rota inversa percorrida pela mensagem PATH relacionada � mensagem RESV.
*  A fun��o de processamento da mensagem PATH deve gerar as informa��es pertinentes (inclusive a rota expl�cita inversa) e pass�-las para a presente
*  fun��o.  Cada nodo que receber uma mensagem RESV_LABEL_MAPPING_PREEMPT deve buscar a mensagem PATH_LABEL_REQUEST_PREEMPT correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB e LSP Table, de modo a construir a LSP.  Esta mensagem RESV cont�m o mapeamento de r�tulo adequado para a constru��o
*  das tabelas.
*  A princ�pio, a mensagem RESV n�o precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estour�-lo, causar� a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV n�o requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de r�tulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados.  O LSP Tunnel s� 
*  estar� completamente estabelecido e v�lido quando a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso isto n�o aconte�a, os recursos e paths
*  estabelecidos sofrer�o timeout, pois n�o haver� REFRESH.
*/
struct Packet *createResvPreemptControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_LABEL_MAPPING_PREEMPT"); //cria uma mensagem tipo RESV_LABEL_MAPPING_PREEMPT
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV n�o requer ACK, portanto n�o � necess�rio coloc�-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=label; //coloca o label criado pelo LSR na mensagem; este r�tulo dever� ser usado em pacotes enviados para este nodo-LSPid
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* FUN��ES DE MANUTEN��O DA LISTA AUXILIAR DE LSPs DO NODO-oIface (LspList)
*  Esta fun��o inicializa a LspList.
*
*  A lista � do tipo duplamente encadeada com um Head Node, circular.
*/
static struct LspList *initLspList() {
	struct LspList *lspList;

	lspList=(LspList*)malloc(sizeof *lspList);
	if (lspList==NULL) {
		printf("\nError - initLspList - insufficient memory to allocate for lspList");
		exit(1);
	}
	lspList->head=(LspListItem*)malloc(sizeof *(lspList->head));
	if (lspList->head==NULL) {
		printf("\nError - initLspList - insufficient memory to allocate for lspList->head");
		exit(1);
	}

	//completa caracter�stica circular da lista
	lspList->head->next=lspList->head;
	lspList->head->previous=lspList->head;
	lspList->size=0;
	return lspList;
}

/* BUSCA NA LSPLIST
*
*  O item buscado � colocado no Head Node e a lista � percorrida do in�cio ao fim.
*  A fun��o retorna a posi��o do item buscado, ou a posi��o imediatamente anterior (pr�pria para inser��o), em ordem
*  decrescente.  Esta posi��o pode ser o pr�prio Head Node.  Como a lista � circular, a inser��o funciona sem problemas
*  mesmo com o Head Node como retorno.
*/
static struct LspListItem *searchInLspList(struct LspList *lspList, int holdPrio) {
	struct LspListItem *lspListItem;

	lspListItem=lspList->head->next;
	lspList->head->holdPrio=holdPrio; //coloca HoldPrio buscada no Head Node
	while (lspListItem->holdPrio > holdPrio) {
		lspListItem=lspListItem->next; //pr�ximo item na lista
	}
	return lspListItem;
}

/* INSERE NA LSPLIST
*
*  A inser��o � feita em ordem da menor Holding Priority para a maior, ou seja, do maior n�mero para o menor (pois 0 � a maior prioridade).
*/
static void insertInLspList(struct LspList *lspList, int LSPid, int setPrio, int holdPrio, struct LIBEntry *libEntry) {
	struct LspListItem *lspListItem, *newItem;

	lspListItem=searchInLspList(lspList, holdPrio); //posi��o imediatamente anterior ao ponto de inser��o, em ordem num�rica decrescente
	newItem=(LspListItem*)malloc(sizeof *newItem);
	newItem->LSPid=LSPid;
	newItem->setPrio=setPrio;
	newItem->holdPrio=holdPrio;
	newItem->libEntry=libEntry;
	newItem->previous=lspListItem->previous;
	newItem->next=lspListItem;
	newItem->previous->next=newItem;
	newItem->next->previous=newItem;
	lspList->size++;
}

/* FAZ PREEMP��O DE RECURSOS EM UM LINK A PARTIR DOS REQUERIMENTOS DE UMA LSP
*
*  Esta fun��o tentar� reservar recursos solicitados para o estabelecimento de uma LSP no link indicado pelo pacote, usando de preemp��o, se necess�rio.
*  Os recursos s�o indicados, na estrutura do link, pelos par�metros CBS, CIR e PIR.  A fun��o comparar� os n�meros
*  pedidos pela LSP com os ainda dispon�veis no link; se os dispon�veis forem iguais ou maiores (todos eles), ent�o a fun��o diminuir� os
*  n�meros dispon�veis na estrutura do link, o que significa a aceita��o da reserva.  Se algum n�mero dispon�vel for menor que o solicitado
*  pela LSP, ent�o a LIB ser� varrida para o nodo-oIface em quest�o e uma lista auxiliar, chamada LSPTable, constru�da para LSPs com Holding Priority
*  menor que a Setup Priority da LSP corrente.  Se os recursos acumulados das LSPs de menor prioridade forem suficientes para a LSP corrente, ent�o as
*  LSPs de menor prioridade ser�o colocadas em estado "preempted" e seus recursos, devolvidos para o link oIface (at� o ponto em que os recursos acumulados
*  forem minimamente suficientes, n�o em excesso).  Ent�o, a reserva ser� feita para a LSP corrente.
*  A fun��o retorna 1 se a reserva foi aceita e feita, com ou sem preemp��o, e retorna 0 (ZERO) se a reserva n�o foi aceita (ou se a LSP indicada n�o foi achada na
*  Tabela de LSPs).
*
*  As prioridades Holding Priority e Setup Priority variam de 0 a 7, 0 a maior, 7 a menor.
*/
int preemptResouces(int LSPid, int node, int link) {
	struct LSPTableEntry *lsp, *lspTmp;
	struct LspList *lspList;
	struct LspListItem *lspListItem, *aux;
	struct LIBEntry *p;
	double cbs, cir, pir;
	int preempt=0; //flag que indica se preemp��o ser� poss�vel
	int resv; //retorno da fun��o reserveResource

	cbs=pir=cir=0;	
	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; teste os recursos dispon�veis
		//testa agora se cada recurso solicitado para a LSP est� dispon�vel; se apenas um n�o estiver, tentar preemp��o
		if (tarvosModel.lnk[link].availCbs >= lsp->cbs && tarvosModel.lnk[link].availCir >= lsp->cir && tarvosModel.lnk[link].availPir >= lsp->pir) {
			//todos os recursos est�o dispon�veis; fa�a a reserva
			tarvosModel.lnk[link].availCbs -= lsp->cbs;
			tarvosModel.lnk[link].availCir -= lsp->cir;
			tarvosModel.lnk[link].availPir -= lsp->pir;
			return 1; //recursos foram efetivamente reservados
		}
		//algum recurso n�o est� imediatamente dispon�vel; tentar preemp��o
		lspList=initLspList();
		p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
		while (p!=lib.head) {
			lspTmp=searchInLSPTable(p->LSPid);
			if (p->oIface==link && p->node==node && strcmp(p->status, "up")==0 && lspTmp->holdPrio > lsp->setPrio && p->bak==0) { //s� varre working LSPs "up" com HoldPrio "menor" que SetPrio
				cbs+=lspTmp->cbs; //acumule recursos
				cir+=lspTmp->cir;
				pir+=lspTmp->pir;
				insertInLspList(lspList, lspTmp->LSPid, lspTmp->setPrio, lspTmp->holdPrio, p); //insere a LSP existente na lista auxiliar LSPList
				//testa se recursos j� acumulados s�o suficientes para reserva; se s�o, pare a varredura
				if (cbs>=lsp->cbs && cir>=lsp->cir && pir>=lsp->pir) {
					preempt=1; //marca flag de preemp��o poss�vel
					break; //recursos acumulados j� suficientes para preemp��o; interrompa a varredura
				}
			}
			p=p->previous;
		}
		//fim da varredura; se preempt=1, preemp��o ser� poss�vel; se =0, n�o h� recursos dispon�veis mesmo com preemp��o.  Limpe a lista auxiliar e retorne com zero da fun��o
		//come�ar varredura da lista auxiliar LSPList, j� ordenada por Holding Priority, da menor para a maior (do maior n�mero para o menor, de 0 a 7)
		lspListItem=lspList->head->next;
		while(lspListItem!=lspList->head) {
			if (preempt==1) { //preemp��o � poss�vel; ent�o, coloque todas as LSPs da lista auxiliar em "preempted" e devolva seus recursos
				returnResources(lspListItem->libEntry->LSPid, link); //devolva recursos
				strcpy(lspListItem->libEntry->status, "preempted"); //marca entrada na LIB para a LSP capturada como "preempted"
				lspListItem->libEntry->timeoutStamp=simtime(); //marca o timeoutStamp (mesmo que n�o tenha sido timeout) para este momento
			} //preempted
			aux=lspListItem;
			lspListItem=lspListItem->next;
			free(aux); //retira item da LSPList da mem�ria
			lspList->size--;
		}
		if (lspList->size>0) {
			printf("\nError - preemptResources - auxiliary list LSPList not empty before cleanup");
			exit(1);
		}
		free(lspList->head); //retira Head Node da mem�ria
		free(lspList); //remove ponteiro para a lista, concluindo a limpeza da lista auxiliar
		if (preempt==1) {
			resv=reserveResouces(LSPid, link);
			if (resv==0) {
				printf("\nError - preemptResources - resources unavailable for reservation even after preemption (inconsistency)");
				exit(1);
			}
			return 1; //recursos reservados com sucesso
		}
		//reserva n�o foi feita; fun��o sair� com ZERO
	}
	return 0; //nenhuma reserva foi feita
}

/* TESTA SE H� RECURSOS DISPON�VEIS NUM DETERMINADO NODO-oIface, SE HOUVER PREEMP��O
*
*  Esta fun��o varre a LIB para um determinado nodo, oIface (link), para working LSPs, com Holding Priority menor que a Setup Priority da LSP corrente,
*  acumulando os recursos de cada uma achada.  Se estes recursos, acumulados, forem suficientes para os requerimentos da LSP corrente, a fun��o retorna
*  1 para SUCESSO; se n�o forem suficientes, retorna 0 para FALHA.
*/
int testResources(int LSPid, int node, int link) {
	struct LSPTableEntry *lsp, *lspTmp;
	struct LIBEntry *p;
	double cbs, cir, pir;
	
	cir=pir=cbs=0;
	lsp=searchInLSPTable(LSPid); //LSPid corrente (que est� em montagem)
	p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
	while (p!=lib.head) {
		lspTmp=searchInLSPTable(p->LSPid);
		if (p->oIface==link && p->node==node && strcmp(p->status, "up")==0 && lspTmp->holdPrio > lsp->setPrio && p->bak==0) { //s� varre working LSPs "up" com HoldPrio "menor" que SetPrio
			cbs+=lspTmp->cbs; //acumule recursos
			cir+=lspTmp->cir;
			pir+=lspTmp->pir;
			//testa se recursos j� acumulados s�o suficientes para reserva; se s�o, pare a varredura
			if (cbs>=lsp->cbs && cir>=lsp->cir && pir>=lsp->pir)
				return 1; //recursos acumulados j� suficientes para preemp��o; interrompa a varredura e retorne com SUCESSO
		}
		p=p->previous;
	}
	return 0; //recursos insuficientes mesmo com preemp��o; retorne com FALHA
}
