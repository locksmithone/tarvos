/*
* TARVOS Computer Networks Simulator
* Arquivo tarvos_rsvp-te
*
* Rotinas para construção e manutenção da Label Information Base (LIB), de uso do MPLS
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
#include "simm_globals.h" //alguma função local usa funções do kernel do simulador, como schedulep

//Prototypes das funções locais static
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

//Variáveis locais (static)
static struct LIB lib ={0}; //defina uma varíavel tipo estrutura LIB para uso exclusivo deste módulo e inicialize tudo com NULL ou zeros
static struct LSPTable lspTable ={0}; //defina uma varíavel tipo estrutura LSPTable para uso exclusivo deste módulo

/* LEITOR DO ARQUIVO LIB.TXT, PARA CRIAÇÃO DA TABELA DE ROTEAMENTO (LIB) MPLS
*
*  Esta função deve ser chamada pelo programa principal (main) no início da simulação,
*  antes da geração de qualquer evento, a fim de que o forwarding opere corretamente.
*
*  A LIB é única para a simulação, contendo a informação de todos os nodos.
*  Deve ser fornecida pelo usuário no arquivo LIB.TXT, contendo números inteiros.
*  O seguinte formato deve ser obedecido, para cada linha:
*
*  Node Incoming_Interface Incoming_Label Outgoing_Interface Outgoing_Label
*
*  Node:  nodo atual.  Tecnicamente, cada nodo (roteador) deveria ter sua própria tabela de roteamento,
*		  mas aqui todas elas são concentradas em uma só.  Portanto, é necessário ter um índice para os
*		  nodos.
*  Incoming_Interface:  Interface de entrada do nodo.  Aqui, as interfaces são os próprios números dos links,
*		               que são únicos
*  Incoming_Label:  Rótulo de entrada
*  Outgoing_Interface:  Interface de Saída.  Aqui, as interfaces são os próprios números dos links,
*					    que são únicos
*  Outgoing_Label:  Rótulo de saída, que substituirá o rótulo de entrada (label swap)
*  Status:  estado da entrada da LIB; up (funcional) ou down (não-funcional)
*  LSPid:  índice para a tabela de LSPid, que conterá parâmetros relativos ao LSP.  Um LSPid é único por todo o domínio MPLS
*						e é composto de vários "subLSPs" entre cada par de nodos.  O "subLSP" é unicamente identificado pela chave
*						nodo-iIface-iLabel.  Este LSPid terá de ser criado e coordenado pelo LER de ingresso, tipicamente como um
*						número inteiro único durante toda a simulação.
*
*  Todos os números devem ser inteiros.
*  
*  A chave primária é composta de node, in_interface, iLabel.  Deve ser ÚNICA.
*  
*  Os números para in_interface, iLabel, out_interface e oLabel para os LERs de ingresso e egresso
*  podem ser, a princípio, qualquer valor, já que não serão processados (a não ser em caso de múltiplos domínios MPLS).
*  Usar, como sugestão, o número zero para estes valores nos LERs, com exceção do LER de ingresso, pois deve-se ter o cuidado
*  de manter a chave primária composta única.  Neste caso, usar valores diferentes para os int_labels nos LER de ingresso.
*  Por exemplo, a seguinte LIB causará problemas, pois as duas primeiras linhas têm a chave não única (os nodos 1 e 3 são LERs de ingresso
*  e egresso, respectivamente):
*
*  Node;iIface;iLabel;oIface;oLabel
*  1	0	0	1	10
*  1	0	0	2	20
*  2	1	10	3	50
*  3	2	20	0	0
*  3	3	50	0	0
*
*  Esta LIB pode ser alterada conforme segue, e funcionará corretamente.
*
*  1	0	1	1	10
*  1	0	2	2	20
*  2	1	10	3	50
*  3	2	20	0	0
*  3	3	50	0	0
*
*  Notar que os valores das in_labels foram configurados de forma única para o LER de ingresso.
*  Da mesma forma, para cada nodo, os valores de oIface e oLabel combinados têm de ser únicos, pois comporão as chaves de busca
*  dos nodos seguintes.
*  Os valores das interfaces, que representam os links do modelo simulado, devem, por recomendação,
*  representar apenas os links realmente existentes.
*
*  Qualquer linha, no arquivo LIB.TXT, que não comece por um número será ignorada.
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
			//	insertInLSPTable(LSPid); /*insere também dados pertinentes na LSP Table, se não foram ainda inseridos
			//							 * É importante verificar antes de inserir nesta tabela, pois só pode haver nela
			//							 * uma única linha por LSPid.*/
		}
	}
	fclose(fp);
	//printf("Tamanho da estrutura LIB completa:  %d\n", getLIBSize());
}

/* CRIA A LIB TABLE (ESTRUTURA INICIAL)
*
*  A tabela criada estará vazia (apenas contendo o Head Node), deixando-a pronta para receber entradas.
*  A LIB Table é uma lista dinâmica duplamente encadeada, circular, com um Head Node.
*/
void buildLIBTable() {
	if (lib.head==NULL) { //evita duplicidade na criação da LIB
		lib.head = (LIBEntry*)malloc(sizeof *(lib.head)); //cria HEAD NODE
		if (lib.head==NULL) {
			printf("\nError - buildLIBTable - insufficient memory to allocate for LIB");
			exit(1);
		}
		lib.head->previous=lib.head;
		lib.head->next=lib.head; //perfaz a característica circular da lista
		lib.size = 0; //lista está vazia
		buildLSPTable();  //cria também a estrutura inicial da LSP ID Table
	}
}

/* INSERE NOVO ITEM NA LIB
*
*  (nota:  a LIB é uma lista dinâmica duplamente encadeada circular com Head Node)
*  Passar todo o conteúdo de uma linha da LIB como parâmetros
*/
void insertInLIB(int node, int iIface, int iLabel, int oIface, int oLabel, int LSPid, char *status, int bak, double timeout, double timeoutStamp) {
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return;
	
	lib.head->previous->next=(LIBEntry*)malloc(sizeof *(lib.head->previous->next)); //cria mais um nó ao final da lista; este nó é lib.head->previous->next (lista duplamente encadeada circular)
	if (lib.head->previous->next==NULL) {
		printf("\nError - insertInLIB - insufficient memory to allocate for LIB");
		exit(1);
	}
	lib.head->previous->next->previous=lib.head->previous; //atualiza ponteiro previous do novo nó da lista
	lib.head->previous->next->next=lib.head; //atualiza ponteiro next do novo nó da lista
	lib.head->previous=lib.head->previous->next; //atualiza ponteiro previous do Head Node
	lib.head->previous->previous->next=lib.head->previous; //atualiza ponteiro next do penúltimo nó
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
	//			insertInLSPTable(LSPid); /*insere também dados pertinentes na LSP Table, se não foram ainda inseridos
	//									 * É importante verificar antes de inserir nesta tabela, pois só pode haver nela
	//									 * uma única linha por LSPid.*/
	//printf("Item inserido:  %d\n", getLIBSize()); //confirma inserção para efeito de debugging
}

/* IMPRIME O CONTEÚDO DA LIB EM ARQUIVO
*
* imprime conteúdo da Label Information Base no arquivo recebido como parâmetro
*/
void dumpLIB(char *outfile) {
	struct LIBEntry *p;
	FILE *fp;
	
	fp=fopen(outfile, "w");
	if (lib.head==NULL) { //se LIB não existir ainda, nada faça
		fprintf(fp, "LIB does not exist.");
		return;
	}
	
	fprintf(fp, "Label Information Base - Contents - Tarvos Simulator\n\n");
	fprintf(fp, "Itens na LIB:  %d\n\n", getLIBSize());
	fprintf(fp, "Node iIface iLabel oIface  oLabel LSPid IsBackup               Status        Timeout   timeoutStamp\n");
	p=lib.head->next; //aponta para a primeira célula da LIB
	while(p!=lib.head) { //lista circular; o último nó atingido deve ser o Head Node
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
*  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado
*  chave para busca:  nodo atual, incoming interface (número do link), incoming label
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*  CUIDADO:  Esta função desconsidera o status da LSP indicada pela LIB.  Não se recomenda usar esta busca para fazer comutação por rótulo
*/
struct LIBEntry *searchInLIB(int node, int iIface, int iLabel) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->iIface=iIface;
	lib.head->iLabel=iLabel;
	
	p=lib.head->previous; //aponta para a última célula da LIB
	while(p->node!=node || p->iIface!=iIface || p->iLabel!=iLabel) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-iIFACE-iLABEL-STATUS
*
*  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado
*  chave para busca:  nodo atual, incoming interface (número do link), incoming label, status
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*  Esta é a busca indicada para fazer o chaveamento por rótulo.
*/
struct LIBEntry *searchInLIBStatus(int node, int iIface, int iLabel, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->iIface=iIface;
	lib.head->iLabel=iLabel;
	strcpy(lib.head->status, status);
	
	p=lib.head->previous; //aponta para a última célula da LIB
	while(p->node!=node || p->iIface!=iIface || p->iLabel!=iLabel || strcmp(p->status, status)!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS PARA WORKING LSPs
*
*  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado
*  chave para busca:  nodo atual, LSPid e string do status desejado.  Busca somente working LSPs,
*  que comecem ou não no nodo atual (o valor de iIface, por conseguinte, é irrelevante).
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*/
struct LIBEntry *searchInLIBnodLSPstat(int node, int LSPid, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->bak=0;
	
	p=lib.head->previous; //aponta para a última célula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->bak!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS PARA BACKUP LSPs
*
*  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado.
*  chave para busca:  nodo atual, LSPid e string do status desejado.  Busca somente backup LSPs que tenham Merge Point
*  inicial no nodo atual (ou seja, onde o iIface é igual a zero, indicação de que o "túnel" LSP começa neste nodo).
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*/
struct LIBEntry *searchInLIBnodLSPstatBak(int node, int LSPid, char *status) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->bak=1; //indica uma backup LSP
	lib.head->iIface=0; //iIface=0 indica que este MP é o início da backup LSP
	
	p=lib.head->previous; //aponta para a última célula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->iIface!=0 || p->bak!=1) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* BUSCA NA LIB USANDO CHAVE NODO-LSPID-STATUS-oIFACE PARA WORKING LSPs
*
*  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado
*  chave para busca:  nodo atual, LSPid, oIface e string do status desejado.  Busca somente working LSPs.
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*/
struct LIBEntry *searchInLIBnodLSPstatoIface(int node, int LSPid, char *status, int oIface) {
	struct LIBEntry *p;
	
	if (lib.head==NULL) //se LIB não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lib.head->node=node;
	lib.head->LSPid=LSPid;
	strcpy(lib.head->status, status);
	lib.head->oIface=oIface;
	lib.head->bak=0; //procura por working LSPs
	
	p=lib.head->previous; //aponta para a última célula da LIB
	while(p->node!=node || p->LSPid!=LSPid || (strcmp(p->status, status)!=0) || p->oIface!=oIface || p->bak!=0) {
		p=p->previous;
	} //fim do percurso
	if (p==lib.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* CRIA A LSP (LABEL SWITCHED PATH) TABLE (ESTRUTURA INICIAL)
*
*  A Tabela LSP conterá uma chave, a LSPid, única em todo o domínio MPLS, e os parâmetros pertinentes à LSP.
*  A LSP é composta por várias "subLSPs".  Cada "subLSP" conecta um par de nodos, e é unicamente identificada
*  pela chave nodo-iIface-iLabel, e estão relacionadas na LIB.  Todo o caminho composto por todas as subLSPs que,
*  conectadas, ligam dois LERs, é o caminho LSP referenciado nesta tabela LSP Table.
*
*  A tabela aqui criada estará vazia, contendo apenas o Head Node.  A tabela é uma lista dinâmica duplamente
*  encadeada, circular.
*/
static void buildLSPTable() {
	lspTable.head = (LSPTableEntry*)malloc(sizeof*(lspTable.head)); //cria HEAD NODE
	if (lspTable.head==NULL) {
		printf("\nError - buildLSPTable - insufficient memory to allocate for LIB");
		exit(1);
	}
	lspTable.head->previous=lspTable.head;
	lspTable.head->next=lspTable.head; //perfaz a característica circular da lista
	lspTable.size = 0; //lista está vazia
}

/* RETORNA O TAMANHO DA LSP TABLE EM LINHAS (OU ITENS)
*/
int getLSPTableSize() {
	return(lspTable.size);
}

/* INSERE NOVO ITEM NA LSP TABLE
*
*  (nota:  a LSP Table é uma lista dinâmica duplamente encadeada circular com Head Node)
*  Passar todo o conteúdo de uma linha da LSP Table como parâmetros
*/
static void insertInLSPTable(int LSPid, int source, int dst, double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio) {
	if (lspTable.head==NULL) //se LSP não existir ainda, nada faça
		return;
	
	lspTable.head->previous->next=(LSPTableEntry*)malloc(sizeof *(lspTable.head->previous->next)); //cria mais um nó ao final da lista; este nó é lib.head->previous->next (lista duplamente encadeada circular)
	if (lspTable.head->previous->next==NULL) {
		printf("\nError - insertInLSPTable - insufficient memory to allocate for LSP Table");
		exit(1);
	}
	lspTable.head->previous->next->previous=lspTable.head->previous; //atualiza ponteiro previous do novo nó da lista
	lspTable.head->previous->next->next=lspTable.head; //atualiza ponteiro next do novo nó da lista
	lspTable.head->previous=lspTable.head->previous->next; //atualiza ponteiro previous do Head Node
	lspTable.head->previous->previous->next=lspTable.head->previous; //atualiza ponteiro next do penúltimo nó
	lspTable.head->previous->LSPid = LSPid;
	lspTable.head->previous->src = source;
	lspTable.head->previous->dst = dst;
	lspTable.head->previous->arrivalTime = simtime();
	lspTable.head->previous->cbs = cbs;
	lspTable.head->previous->cBucket = 0; //Bucket começa vazio?
	lspTable.head->previous->cir = cir;
	lspTable.head->previous->maxPktSize = maxPktSize;
	lspTable.head->previous->minPolUnit = minPolUnit;
	lspTable.head->previous->pir = pir;
	lspTable.head->previous->setPrio = setPrio;
	lspTable.head->previous->holdPrio = holdPrio;
	lspTable.head->previous->tunnelDone = 0;
	lspTable.size++;
}

/* BUSCA NA LSP Table.  Retorna ponteiro para a célula com os parâmetros pedidos ou NULL para não encontrado
*  chave para busca:  LSPid
*
*  A busca é feita inserindo a chave buscada no Head Node e fazendo o percurso reverso, do final da
*  lista até o início.
*/
struct LSPTableEntry *searchInLSPTable(int LSPid) {
	struct LSPTableEntry *p;
	
	if (lspTable.head==NULL) //se LSP não existir ainda, nada faça
		return NULL;
	
	//insere a chave de busca no Head Node
	lspTable.head->LSPid=LSPid;
	
	p=lspTable.head->previous; //aponta para a última célula da LSP Table
	while(p->LSPid!=LSPid) {
		p=p->previous;
	} //fim do percurso
	if (p==lspTable.head) //se for igual, a busca não encontrou nenhum item válido
		return NULL; //retorne NULL, significando nada encontrado
	else
		return p; //retorne o ponteiro para o item encontrado
}

/* IMPRIME O CONTEÚDO DA LSP TABLE EM ARQUIVO
*
* imprime conteúdo da Label Switched Path Table no arquivo recebido como parâmetro
*/
void dumpLSPTable(char *outfile) {
	struct LSPTableEntry *p;
	FILE *fp;
	
	fp=fopen(outfile, "w");
	
	if (lspTable.head==NULL) { //se LIB não existir ainda, nada faça
		fprintf(fp, "LSP Table does not exist.");
		return;
	}

	fprintf(fp, "Label Switched Path (LSP) Table - Contents - Tarvos Simulator\n\n");
	fprintf(fp, "Itens in LSP Table:  %d\n", getLSPTableSize());
	fprintf(fp, "Current simtime:  %f\n\n", simtime());
	fprintf(fp, "LSPid Src  Dst        CBS             CIR            PIR           cBucket  MinPolUnit  MaxPktSize   setPrio  HoldPrio  ArrivalTime  tunnelDone\n");
	p=lspTable.head->next; //aponta para a primeira célula da LSP Table
	while(p!=lspTable.head) { //lista circular; o último nó atingido deve ser o Head Node
		fprintf(fp, "%3d  %3d  %3d\t%10.1f\t%10.1f\t%10.1f\t%10.1f\t%5d\t  %5d\t\t%d\t%d\t %10.4f\t%s\n",
			p->LSPid, p->src, p->dst, p->cbs, p->cir, p->pir, p->cBucket, p->minPolUnit, p->maxPktSize, p->setPrio, p->holdPrio, p->arrivalTime, (p->tunnelDone==0)? ("no"):("yes"));
		p=p->next;
	}
	fclose(fp);
}

/* CONTROLA E DEVOLVE UM NÚMERO ÚNICO, PARA TODO UM DOMÍNIO MPLS, DE UM TÚNEL LSP
*
*  Esta função cria e devolve um LSPid, que é um número inteiro e necessariamente único para todo um domínio MPLS, usado para caracterizar
*  um túnel LSP.
*/
static int getNewLSPid() {
	static int LSPid=0; //defina o LSPid inicial
	
	LSPid++; //cria novo LSPid
	return LSPid;
}

/* ESTABELECE (CRIA) UMA LSP ENTRE DOIS NODOS, COM OU SEM PREEMPÇÃO
*
*  Dada uma rota explícita, esta função criará uma LSP entre o primeiro e o último nodo da rota (entre os LERs de ingresso
*  e egresso, pois), se houver recursos disponíveis por todo o caminho.
*  Os parâmetros a passar são o tamanho do pacote de controle, o nodo fonte (LER de ingresso), o nodo destino (LER de egresso),
*  o objeto rota explícita, o cir (Committed Information Rate), o cbs (Committed Bucket Size), o pir (Peak Information Rate),
*  o tamanho mínimo a ser considerado para policing e o tamanho máximo de um pacote para ser considerado conforme.
*  Também, a Setup Priority e Holding Priority para uma LSP.  Segundo a RFC 3209, a Setup Priority não pode ser mais alta que a Holding Priority para
*  uma mesma LSP.  Para estes dois parâmetros, o valor varia de 0 a 7, 0 sendo a maior prioridade.
*  Por fim, o campo preempt deverá conter 1 se preempção for desejada, e 0 se preempção não for desejada.
*  A função reservará um número LSPid que será único por todo o domínio MPLS e não mais poderá ser usado.
*
*  Em caso de preempção:
*  Para a reserva de recursos, poderá haver a preempção de LSPs de menor prioridade a fim de garantir os recursos exigidos.  A preempção segue o seguinte
*  algoritmo:
*    1.  Se houver recursos suficientes no link, não há necessidade de preempção, e estes serão pré-reservados por uma mensagem PATH e confirmados por uma RESV.
*    2.  Se não houver recursos suficientes, varrer todas as LSPs estabelecidas no link em questão (loop).  Para aquelas com Holding Priority menor que a Setup Priority
*		 da LSP sendo estabelecida, somar seus recursos reservados num acumulador.  Se o acumulador acusar recursos suficientes para estabelecimento da LSP,
*		 interromper o loop e deixar a mensagem PATH seguir adiante.  Isto significa que a LSP pode ser estabelecida, mas só o será efetivamente com o recebimento de
*		 uma mensagem RESV.  Neste caso aqui, não haverá pré-reserva, pois isto obrigaria uma "pré-preempção" das LSPs estabelecidas, algo complexo de implementar.
*		 Se, após varrer todas as LSPs estabelecidas para o link, não houver recursos suficientes que possam ser obtidos da preempção de LSPs de menor prioridade,
*		 então não deixar a PATH seguir adiante.  (Pode ser emitida uma PATH_ERR aqui.)
*
*  A preempçãu usa duas mensagens específicas:  PATH_LABEL_REQUEST_PREEMPT e RESV_LABEL_MAPPING_PREEMPT.  No caso 1 acima, a mensagem PATH comporta-se como uma
*  PATH_LABEL_REQUEST comum, fazendo a pré-reserva de recursos.  No caso 2, a mensagem ativará a varredura de LSPs pré-existentes a fim de verificar a possibilidade
*  de liberar recursos através de preempção de LSPs de menor prioridade.  Caso positivo, a mensagem seguirá adiante, mas nenhuma pré-reserva será feita.
*  No processamento da mensagem RESV, checar-se-á se recursos foram previamente reservados.  Caso positivo, a RESV confirma esta reserva, faz o mapeamento de rótulo
*  e a mensagem segue adiante.  Se não foram (esta informação consta da lista de mensagens de controle do nodo), o processamento da RESV checará se há recursos
*  imediatamente disponíveis.  Havendo, age conforme acima.  Não havendo, fará uma varredura nas LSPs pré-existentes, construindo uma lista auxiliar com as LSPs
*  de menor Holding Priority que a Setup Prioriry da LSP corrente, ordenada por menor prioridade.  Esta varredura acaba quando os recursos encontrados forem suficientes
*  para a LSP corrente, ou quando não houver mais nenhuma LSPs pré-existente no link/nodo em questão.  Havendo recursos suficientes, o processamento colocará o estado de
*  cada LSP, em ordem de menor prioridade, em "preempted", e devolverá seus recursos ao link.  Fará isso para cada LSP da lista auxiliar, até que a quantidade de recursos
*  devolvidos seja suficiente.  Então, fará a nova reserva para a LSP atual e seguirá com a mensagem RESV adiante.
*  Se não houver recursos suficientes para preempção, a mensagem não segue adiante.
*/
struct Packet *setLSP(int source, int dst, int er[], double cir, double cbs, double pir, int minPolUnit, int maxPktSize, int setPrio, int holdPrio, int preempt) {
	int LSPid;
	struct Packet *pkt;

	//checa se algum parâmetro do Policer está inválido; se houver, sai do programa com erro
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
	//verifica se Setup Priority é maior que Holding Priority (ou seja, se tem um número mais baixo) (RFC 3209 não recomenda que isto aconteça)
	if (setPrio<holdPrio) {
		printf("\nError - setLSP - Setup Priority higher than Holding Priority");
		exit(1);
	}
	buildLIBTable(); //monta a LIB se ainda não houver sido montada (esta função checa se a LIB já existe a fim de prevenir duplicidade)
	LSPid=getNewLSPid(); //reserva novo LSPid para uso; mesmo que a LSP não consiga ser completamente montada, este número não mais poderá ser usado na simulação
	insertInLSPTable(LSPid, source, dst, cir, cbs, pir, minPolUnit, maxPktSize, setPrio, holdPrio); //insere os parâmetros desta LSP na Tabela LSP
	if (preempt==0)
		pkt=createPathLabelControlMsg(source, dst, er, LSPid);  //inicia construção da LSP sem preempção
	else
		pkt=createPathPreemptControlMsg(source, dst, er, LSPid); //inicia construção da LSP com preempção
	return pkt;
}

/* ESTABELECE (CRIA) UMA LSP BACKUP ENTRE DOIS NODOS
*
*  Dada uma rota explícita e um LSPid de uma LSP já estabelecida, esta função criará uma backup LSP entre o primeiro e o último nodo da rota
*  (entre os Merge Points de ingresso e egresso, pois), se houver recursos disponíveis por todo o caminho.
*  Os parâmetros a passar são o nodo fonte (Merge Point (MP) de ingresso), o nodo destino (MP de egresso) (que não são necessariamente os mesmos
*  LER de ingresso e LER de egresso da working LSP), o objeto rota explícita e o LSPid da working LSP.  Todos os demais parâmetros de
*  Constraint Routing da LSP serão copiados da working LSP.
*  Se uma working LSP não for encontrada no MP de ingresso, então a função não estabelecerá nenhum backup.
*  As reservas de recursos serão feitas ao longo do caminho indicado, exceto se a working LSP participar do mesmo caminho em algum ponto (ou seja,
*  se o link (representado neste simulador pelo oIface) de saída da backup LSP e working LSP for o mesmo).
*  Neste caso, a backup LSP e a working LSP compartilharão a reserva já feita.
*  A função verifica se a working LSP já existe (informado pelo LSPid).  Se não existir, a função retorna NULL (a função que chama deve testar isso,
*  se for de interesse).
*/
struct Packet *setBackupLSP(int LSPid, int sourceMP, int dstMP, int er[]) {
	struct Packet *pkt;
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL || p->tunnelDone==0) //verifica se working LSP já existe E foi completada totalmente, do LER de ingresso ao LER de egresso
		return NULL; //working LSP ainda não existe ou não está completa; não cria backup, retorna com NULL

	//aqui deve entrar geração de mensagem PATH_DETOUR
	pkt=createPathDetourControlMsg(sourceMP, dstMP, er, LSPid);
	return pkt;
}

/* CRIA UMA MENSAGEM DE CONTROLE
*
*  Cria um pacote contendo a mensagem de controle.
*  Esta função deve receber via um ponteiro char o tipo de mensagem de controle a ser criada.
*  A função que chama deve cuidar de enviar a mensagem adiante.
*/
static struct Packet *createControlMsg(int length, int source, int dst, char *msgType) {
	struct Packet *pkt;
	static int msgID = 0; /*inicializa msgID; o módulo TARVOS foi concebido de modo que esta msgID não se repita ao longo
						  da simulação e do domínio MPLS.*/
	
	msgID++; //incrementa para uma nova mensagem de controle; o primeiro número válido é 1
	pkt = createPacket();
	pkt->currentNode = source;
	pkt->length = length;
	pkt->src = source; //Nodo ao qual a fonte está vinculada, isto permite que várias fontes gerem para o mesmo nodo (fontes com tipos de geração diferentes)
	pkt->dst = dst; //Nodo ao qual o sorvedouro está vinculado
	pkt->outgoingLink=0; //necessário para o correto funcionamento do MPLS e de várias funções do TARVOS; indica que o pacote está sendo gerado no nodo
	strcpy(pkt->lblHdr.msgType, msgType);
	pkt->lblHdr.msgID=msgID;
	pkt->lblHdr.msgIDack=0;
	pkt->lblHdr.LSPid=0;
	pkt->lblHdr.priority=tarvosParam.ctrlMsgPrio; //coloca a prioridade das mensagens de controle no pacote
	return pkt;
	//schedulep(ev, 0, pkt->id, pkt);  //colocar na função específica de criação de mensagem de controle
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_LABEL_REQUEST
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel entre o nodo source e nodo dst.  Os recursos são pré-reservados também por esta mensagem.
*/
struct Packet *createPathLabelControlMsg(int source, int dst, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_LABEL_REQUEST"); //cria uma mensagem tipo PATH_LABEL REQUEST
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_LABEL_MAPPING
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servirá para tratar e escalonar
*  o evento de recepção da mensagem pelo nodo origem.
*  A rota explícita indicada, tipicamente, é a rota inversa percorrida pela mensagem PATH relacionada à mensagem RESV.
*  A função de processamento da mensagem PATH deve gerar as informações pertinentes (inclusive a rota explícita inversa) e passá-las para a presente
*  função.  Cada nodo que receber uma mensagem RESV_LABEL_MAPPING deve buscar a mensagem PATH_LABEL_REQUEST correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB e LSP Table, de modo a construir a LSP.  Esta mensagem RESV contém o mapeamento de rótulo adequado para a construção
*  das tabelas.
*  A princípio, a mensagem RESV não precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estourá-lo, causará a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV não requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de rótulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados.  O LSP Tunnel só 
*  estará completamente estabelecido e válido quando a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso isto não aconteça, os recursos e paths
*  estabelecidos sofrerão timeout, pois não haverá REFRESH.
*/
struct Packet *createResvMapControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_LABEL_MAPPING"); //cria uma mensagem tipo RESV_LABEL_MAPPING
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV não requer ACK, portanto não é necessário colocá-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=label; //coloca o label criado pelo LSR na mensagem; este rótulo deverá ser usado em pacotes enviados para este nodo-LSPid
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* RECUPERA RÓTULO PARA UM DETERMINADO WORKING LSP EM UM LSR (OU NODO)
*
*  Esta função recebe o número do nodo (que é o LSR) e o número do LSP (LSPid), ambos inteiros, e devolve o rótulo (iLabel) mapeado para este LSP
*  se o LSP estiver em modo "up" (para working LSP).  A consulta é feita na LIB.  Só pode haver uma (working) LSP única em todo o domínio MPLS
*  neste simulador, mas o número do rótulo pode ser repetido em LSPs diferentes e inclusive em interfaces diferentes.
*  A função devolve -1 para o caso de não ter achado uma LSP válida no nodo em questão.
*/
int getWorkingLSPLabel(int n_node, int LSPid) {
	struct LIBEntry *entry;

	entry=searchInLIBnodLSPstat(n_node, LSPid, "up");
	return (entry==NULL)? (-1):(entry->iLabel);
}

/* RESERVA RECURSOS EM UM LINK A PARTIR DOS REQUERIMENTOS DE UMA LSP
*
*  Esta função tentará reservar recursos solicitados para o estabelecimento de uma LSP no link indicado pelo pacote.
*  Os recursos são indicados, na estrutura do link, pelos parâmetros CBS, CIR e PIR.  A função comparará os números
*  pedidos pela LSP com os ainda disponíveis no link; se os disponíveis forem iguais ou maiores (todos eles), então a função diminuirá os
*  números disponíveis na estrutura do link, o que significa a aceitação da reserva.  Se algum número disponível for menor que o solicitado
*  pela LSP, então nada será modificado, o que significa a não aceitação da reserva.
*  A função retorna 1 se a reserva foi aceita e feita, e retorna 0 (ZERO) se a reserva não foi aceita (ou se a LSP indicada não foi achada na
*  Tabela de LSPs).
*/
int reserveResouces(int LSPid, int link) {
	struct LSPTableEntry *lsp;
	
	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; teste os recursos disponíveis
		//testa agora se cada recurso solicitado para a LSP está disponível; se apenas um não estiver, retorne sem reserva
		if (tarvosModel.lnk[link].availCbs >= lsp->cbs && tarvosModel.lnk[link].availCir >= lsp->cir && tarvosModel.lnk[link].availPir >= lsp->pir) {
			//todos os recursos estão disponíveis; faça a reserva
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
*  Esta função devolverá os recursos reservados ou pré-reservados para o estabelecimento de uma LSP para o link apropriado.
*  Os recursos são indicados, na estrutura do link, pelos parâmetros CBS, CIR e PIR.
*  O retorno dos recursos devem-se à derrubada ou desativação de uma LSP, à falha em estabelecer uma nova LSP, à expiração de uma mensagem
*  PATH_LABEL_REQUEST (que também faz pré-reserva de recursos).
*/
static void returnResources(int LSPid, int link) {
	struct LSPTableEntry *lsp;

	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; devolva os recursos
		tarvosModel.lnk[link].availCbs += lsp->cbs;
		tarvosModel.lnk[link].availCir += lsp->cir;
		tarvosModel.lnk[link].availPir += lsp->pir;
	}
	/*Seria interessante colocar aqui uma rotina para verificar a coerência dos valores availCbs, Cir e Pir.  Pode acontecer, por algum erro,
	que estes valores, após retorno, superem a largura de banda característica do link, ficando pois irreais.  Mais seria portanto necessário
	comparar os valores disponíveis com valores máximos teóricos.*/
}


/* RETORNA PARÂMETRO CBS (COMMITTED BUCKET SIZE) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
*/
double getLSPcbs(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cbs;
}

/* RETORNA PARÂMETRO CIR (COMMITTED INFORMATION RATE) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
*/
double getLSPcir(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cir;
}

/* RETORNA PARÂMETRO PIR (PEAK INFORMATION RATE) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
*/
double getLSPpir(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->pir;
}

/* RETORNA PARÂMETRO cBUCKET (COMMITTED BUCKET OU TAMANHO ATUAL DO BUCKET) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
*/
double getLSPcBucket(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->cBucket;
}

/* RETORNA PARÂMETRO MAXPKTSIZE (MAXIMUM PACKET SIZE) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
*/
int getLSPmaxPktSize(int LSPid) {
	struct LSPTableEntry *lsp;

	lsp=searchInLSPTable(LSPid);
	if (lsp==NULL)
		return -1;
	else
		return lsp->maxPktSize;
}

/* RETORNA PARÂMETRO MINPOLUNIT (MININUM POLICED UNIT) DA LSP TABLE
*
*  Passar o número da LSP como parâmetro.  Se a LSPid não for encontrada, -1 será retornado.
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
*  Esta função checa todas as LSPs de cada nodo, e também a fila de mensagens de controle de cada nodo, a fim de verificar se seus tempos de timeout
*  superam o tempo atual.  Caso positivo, então o timeout estará comprovado e a função tomará as ações apropriadas em cada caso.
*
*  Para uma LSP, se o timeout for controlado pelas mensages RESV, o estado desta deverá ser colocado em "timed-out" e os recursos reservados no link de saída para a LSP deverão ser devolvidos ou restaurados.
*  Ou, a rotina de rapid recovery deve ser iniciada.
*
*  Para o intervalo de detecção de falha de comunicação (nodo não alcançável através de mensagem HELLO), as LSPs entre os nodos envolvidos como origem e destino
*  devem ser colocadas em estado "HELLO failure" e a rotina de rapid recovery ativada.
*
*  Para mensagens de controle, o timeout existe para mensagens PATH_LABEL_REQUEST, PATH_REFRESH e HELLO.  As mensagens RESV, via de regra, não necessitando
*  de ACK, não carecem de timeout.
*
*  O timeout de uma PATH_LABEL_REQUEST deve eliminá-la da fila e devolver os recursos pré-reservados no link de saída para o link.  Como a mensagem
*  RESV_LABEL_MAPPING correspondente ainda não foi recebida, então não há uma entrada na LIB para colocar como "down".
*
*  O timeout de uma PATH_REFRESH deve eliminar a mensagem da fila.  A LSP não deve ser derrubada, pois isto será feito explicitamente no timeout da LSP.
*  Verificar se o processo de rapid recovery deve ser ativado aqui.
*
*  O timeout de uma mensagem HELLO deve eliminá-la da fila, somente.
*/
void timeoutWatchdog() {
	double now;

	now=simtime();
	LSPtimeoutCheck(now); //Processa as LSPs para timeout, usando a Tabela LIB
	nodeCtrlMsgTimeoutCheck(now); //verifica timeout de mensagens de controle
	helloFailureCheck(now); //verifica timeout específico indicado pelas mensagens HELLO (ou falhas de recebimento destas)
}

/* VERIFICA TIMEOUT DO INTERVALO DE VERIFICAÇÃO DE FALHA DE RECEBIMENTO DE HELLO DE NODO VIZINHO OU ADJACENTE
*
*  No RSVP-TE, as mensagens HELLO podem ser usadas para determinar se nodos vizinhos estão alcançáveis (RFC 3209).  Para isto, o nodo envia uma mensagem
*  HELLO para o nodo vizinho e este deve responder com uma HELLO_ACK dentro do intervalo máximo definido por tarvosModel.node[node].helloTimeLimit[link].
*  Este intervalo é definido, por default, como o intervalo de geração de HELLOs * 3,5.  O intervalo de geração default é 5ms, portanto o intervalo
*  máximo de recebimento é 17,5ms.  Se o intervalo for ultrapassado, então o nodo é considerado inalcançável ou com falha.
*  Na RFC 3209, há um esquema de números instanciados para cada nodo de forma a determinar se o nodo foi ressetado; os números são enviados dentro da
*  HELLO e HELLO_ACK e comparados com números anteriores armazenados.  Nesta implementação do simulador, isto não é usado; apenas o recebimento de uma
*  HELLO_ACK dentro do intervalo, correspondente (msgIDack) à HELLO enviada, é verificada.
*  Ao receber uma HELLO_ACK, o nodo verifica se existe uma HELLO com mesmo msgIDack na fila de mensagens de controle do nodo.  Se houver, então o valor
*  do helloTimeLimit no tarvosModel.node é atualizado para mais um intervalo de detecção de falha (3,5 * helloInterval, por default).  Caso uma mensagem
*  HELLO com mesmo msgIDack não seja encontrada, então a HELLO_ACK é descartada e o helloTimeLimit não é atualizado.
*
*  Esta rotina precisa verificar o tempo helloTimeLimit de cada link saindo de um nodo, ou seja, links com src no nodo atual e todos os dst, isto para cada
*  nodo do sistema.  Se o helloTimeLimit for menor ou igual a simtime(), então as LSPs que usam o link em questão devem ser colocadas em estado "dst unreachable".
*  Os recursos reservados para as LSPs devem ser retornados aos links.  Uma LSP backup faz sua própria reserva de recursos, pois em princípio uma LSP backup
*  segue um caminho (path) diverso da LSP primária ou working LSP (entre os Merge Points).
*  Para minimizar processamento, a busca é feita a partir da LIB, pois, se não houver uma LSP conectando nodos, não importa para a simulação se eles estão
*  alcançáveis ou não.
*  Obs.:  um HelloTimeLimit igual a zero significa que o tempo não precisa ser checado para este link em particular.
*/
static void helloFailureCheck(double now){
	struct LIBEntry *p;
	char mainTraceString[255];
	double tmp;
	
	p=lib.head->previous; //percorre no sentido inverso
	sprintf(mainTraceString, "HELLO failure CHECK:  simtime:  %f\n", now);
	mainTrace(mainTraceString);
	
	while (p!=lib.head) { //só testa timeout para LSPs "up"
		tmp=(*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface];
		if (strcmp(p->status, "up")==0 && (*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface]>0 && (*(tarvosModel.node[p->node].helloTimeLimit))[p->oIface]<=now) { /*testa timers para link de saída (oIface);
																															   se o helloTimeLimit for zero, então a checagem
																															   é desativada para aquele link*/
			strcpy(p->status, "dst fail (HELLO)"); //os recursos reservados para a LSP devem ser retornados ao link como disponíveis
			p->timeoutStamp = now; //coloca o relógio atual no marcador timeoutStamp

			sprintf(mainTraceString, "HELLO failure STAMP - dst fail (HELLO):  node:  %d  oIface:  %d  simtime:  %f\n", p->node, p->oIface, now);
			mainTrace(mainTraceString);

			//os recursos devem ser sempre retornados ao link, mesmo com backup LSP (pois a backup LSP não trafegará, logicamente, pelo link falho)
			returnResources(p->LSPid, p->oIface); //retorna os recursos reservados ao link; o campo oIface da LIB indica o número do "link" de saída
			/*RAPID RECOVERY deve vir aqui*/
			rapidRecovery(p->node, p->LSPid, p->iIface, p->iLabel); //aciona o Rapid Recovery; a função se encarrega de checar se existe backup LSP
		}
		p=p->previous; //atualiza para a entrada anterior na tabela LIB
	}
}

/* VERIFICA TIMEOUT DAS LSPs
*  A tabela verificada é, na verdade, a LIB, que contém todas as entradas, por nodo, das LSPs.
*  Como convenção, se o campo timeout for 0, então a função de timeout está desativada (ou seja, as LSP nunca entram em timeout).
*/
static void LSPtimeoutCheck(double now) {
	struct LIBEntry *p;

	if (lib.head==NULL) //LIB não existe; nada faça
		return;
	
	p=lib.head->previous; //percorre no sentido inverso
	while (p!=lib.head) {
		if (strcmp(p->status, "up")==0 && p->timeout <= now && p->timeout > 0) { //só testa timeout para LSPs "up"; deve-se testar para backup também?
			strcpy(p->status, "timed-out"); //calma; os recursos reservados para a LSP devem ser retornados ao link como disponíveis
			p->timeoutStamp = simtime(); //coloca o relógio atual no marcador timeoutStamp
			//os recursos devem ser sempre retornados ao link, mesmo com backup LSP (pois a backup LSP não trafegará, logicamente, pelo link falho)
			returnResources(p->LSPid, p->oIface); //retorna os recursos reservados ao link; o campo oIface da LIB indica o número do "link" de saída
			/*RAPID RECOVERY deve vir aqui*/
			rapidRecovery(p->node, p->LSPid, p->iIface, p->iLabel); //aciona o Rapid Recovery; a função se encarrega de checar se existe backup LSP
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
		while (p!=tarvosModel.node[i].nodeMsgQueue) { //o ponteiro node.nodeMsgQueue aponta para o nó HEAD da lista
			if (p->timeout <= now) { //se timeout registrado na mensagem for menor ou igual a NOW, então elimine a mensagem da fila
				if (p->resourcesReserved==1) //retorna os recursos para o link de saída se houver pré-reserva (mensagem PATH_LABEL_REQUEST e PATH_LABEL_REQUEST_PREEMPT)
					returnResources(p->LSPid, p->oIface);
				msgID=p->msgID; //armazena o msgID desta mensagem a ser eliminada da fila
				p=p->next; //p deve ser avançado agora, pois esta posição da memória será eliminada
				removeFromNodeMsgQueueAck(i, msgID);
			} else
				p=p->next; //só avance aqui se o if for falso, pois o ponteiro já é avançado dentro do if
		}
	}
}

/* PERFAZ REFRESH DAS LSPs E RECURSOS RESERVADOS
*
*  Esta função gera mensagens PATH_REFRESH para todos os nodos e todas as LSPs em cada nodo, desde que estas LSPs tenham *origem* no nodo e estejam
*  em modo "up".  Para testar a origem das LSPs, o campo iIface é testado para zero na lista LIB, pois o zero ali é indicativo que o nodo nesta tupla
*  é o nodo de origem do túnel LSP.
*  Observar que a rotina de geração das mensagens faz a busca em todos os nodos, de modo sincronizado, mas a geração de mensagens PATH_REFRESH não é
*  é sincronizada graças a uma geração aleatória de um intervalo de tempo de escalonamento para cada mensagem gerada.  Esta condição foi relatada na
*  RFC 2205 como desejada (um nodo deve gerar um intervalo de tempo aleatório entre 0,5 e 1,5 * Refresh_Interval) para evitar problemas.
*  Nesta implementação, é usado um intervalo aleatório entre 0 e 1,5 * Refresh_Interval adicionado ao Refresh_Interval (resultando em um intervalo total
*  entre 1 e 1,5 Refresh_Interval), pois o escalonamento do evento de refreshLSP é feito a cada 1 * Refresh_Interval.  Não enxergo problemas nesta solução
*  de implementação, mas isto pode ser estudado mais profundamente.  O intervalo aleatório é calculado dentro da função de criação de mensagens PATH_REFRESH.
*
*  Quando recém-criada, uma LSP pode sofrer o primeiro refresh, no pior caso, no tempo máximo configurado para refresh
*  nos parâmetros da simulação.  Em outros casos, este primeiro refresh ocorrerá em tempo inferior ao máximo, e, a partir de então, os próximos refresh
*  ocorrerão em sincronia com todos os nodos e LSPs da topologia (ressalvado o intervalo aleatório descrito acima).
*  O refresh global -- feito por esta rotina -- deve ter um evento relacionado, programado portanto logo no início da simulação pelo usuário e
*  re-escalonado continuamente com o intervalo interevento configurado nos parâmetros da simulação.
*/
void refreshLSP() {
	struct LIBEntry *p;
	struct Packet *pkt;
	struct LSPTableEntry *lsp;

	p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
	while (p!=lib.head) {
		if (p->iIface==0 && strcmp(p->status, "up")==0) { //só faz refresh para LSPs "up", todos os nodos
			lsp = searchInLSPTable(p->LSPid); //busca a entrada na LSP Table para o túnel LSP encontrado
			if (lsp == NULL) { //entrada na LSP Table não encontrada; incongruência nos bancos de dados!
				printf("\nError - refreshLSP - related entries in LIB and LSP Table not found");
				exit (1);
			}
			/*cria mensagem de controle PATH_REFRESH e escalona.  O src será o nodo atual da LIB, o destino, coletado na LSP Table.
			Isto é necessário porque as working LSPs começam e terminam nos nodos src e dst constantes na LSP Table.  Porém, as backup LSPs
			começam em nodos diferentes do src da working LSP, mas terminam no mesmo nodo dst.*/
			pkt = createPathRefreshControlMsg(p->node, lsp->dst, p->LSPid, p->iLabel);
		}
		p=p->previous;
	}
}

/* INICIA GERAÇÃO DAS MENSAGENS HELLO PARA TODOS OS NODOS
*
*  Esta função lê todo o array de links.  Para cada link, uma mensagem HELLO é gerada do nodo origem ao nodo destino conectado pelo link.  Assim, os
*  nodos mandam mensagens HELLO para todos os outros nodos conectados a si, desde que seja origem.
*  Nesta implementação, as mensagens HELLO serão geradas segundo o intervalo HELLO_Interval definido nos parâmetros da simulação e usa-se mecanismo para
*  evitar sincronização na geração das mensagens, inspirado na RFC 2205, item 3.7 (para mensagens de controle PATH_REFRESH).
*  Não foi achada, apesar disso, nenhuma restrição ao sincronismo de mensagens HELLO na RFC 3209, mas verificou-se na prática que racing conditions podem ocorrer.
*  Como as mensagens HELLO são geradas com destino ao nó vizinho (neighbor node), o TTL deve ser definido como 1, segundo a RFC 3209.
*  A falha em receber uma mensagem HELLO ou HELLO_ACK dentro do intervalo especificado em tarvosParam.helloTimeout indica que houve falha de comunicação
*  entre os nodos relacionados.
*/
void generateHello() {
	struct Packet *pkt;
	int links, i;
	
	links=(sizeof tarvosModel.lnk / sizeof *(tarvosModel.lnk))-1;
	for (i=1; i<=links; i++) { //percorre todo o array de links, de modo que cada nodo origem gere uma mensagem HELLO para o nodo destino do link
		pkt = createHelloControlMsg(tarvosModel.lnk[i].src, tarvosModel.lnk[i].dst);
		pkt->er.explicitRoute = (int*)malloc(sizeof (*(pkt->er.explicitRoute))); //cria espaço para um int
		if (pkt->er.explicitRoute == NULL) {
			printf("\nError - generateHello - insufficient memory to allocate for explicit route object");
			exit (1);
		}
		*pkt->er.explicitRoute = tarvosModel.lnk[i].dst; //coloca o nodo destino na rota explícita; isto terá de ser removido com free antes da remoção do pacote
		pkt->er.erNextIndex = 0; //assegura que índice aponta para início da rota explícita
		pkt->lblHdr.label = 0; //assegura que não há label válido
		pkt->outgoingLink = 0; //marca pacote como gerado no nodo origem
	}
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO HELLO
*
*  Cria um pacote contendo a mensagem de controle HELLO e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  O pacote de controle criado será escalonado para um tempo entre 0 e 0,01 * Hello_Interval a partir do simtime() atual.  Isto é
*  feito a fim de evitar a sincronização do envio das mensagens HELLO pelos nodos, inspirado pela RFC 2205, item 3.7.
*  (para mensagens PATH_REFRESH).  Sem esse recurso, os pacotes HELLO são gerados, para todos os nodos, de forma sincronizada, ao mesmo tempo.
*  Observou-se, nos testes, que isso pode levar a racing conditions, onde especificamente o segundo link da lista de links apresenta um número
*  de dequeues bem menor que os outros links da lista (para simulações acima de 120 segundos).
*  Aparentemente, a ordem de enfileiramento dos eventos de alguma forma privilegia este segundo link, e seus pacotes acabam sendo enfileirados
*  bem menos frequentemente que outros links.  Apesar disso, a utilização, tamanho máximo e médio de filas não parecem se alterar com a sincronização.
*  Este recurso de dessincronização tende a aumentar o número de dequeues para simulações curtas, mas reduz substanciamente o número de dequeues
*  para simulações mais longas, e homogeiniza este número para todos os links.
*/
struct Packet *createHelloControlMsg(int source, int dst) {
	struct Packet *pkt;
	double randInterval; //intervalo de tempo aleatório entre 0 e 0,01*HELLO_Interval que será somado ao tempo atual para escalonamento do pacote
	
	pkt = createControlMsg(tarvosParam.helloMsgSize, source, dst, "HELLO"); //cria uma mensagem tipo HELLO
	pkt->er.recordThisRoute=1; //ative a gravação da rota no pacote, para uso da mensagem HELLO_ACK de volta (se necessário)
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	//pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	randInterval = uniform(0, 0.01) * tarvosParam.helloInterval; //cria intervalo de tempo aleatório entre 0 e 0,01 * HELLO_Interval, para evitar sincronismo de geração de mensagens HELLO
	pkt->generationTime=simtime() + randInterval; //marca o tempo em que o pacote foi gerado
	pkt->ttl = 1; //limite o Time To Live a 1 hop, conforme indicado na RFC 3209, item 5.1
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), randInterval, pkt->id, pkt); //escalona evento de tratamento HELLO imediatamente
	//schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt); //escalona evento de tratamento HELLO imediatamente
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO HELLO_ACK
*
*  Cria um pacote contendo a mensagem de controle HELLO_ACK e escalona evento recuperado do objeto nodo.  Este evento tipicamente servirá para tratar e escalonar
*  o evento de recepção da mensagem pelo nodo origem.
*  A rota seguida, tipicamente, é a rota inversa percorrida pela mensagem HELLO relacionada à mensagem HELLO_ACK.  A função de processamento da mensagem HELLO deve
*  gerar as informações pertinentes e passá-las para a presente função.  Cada nodo que receber uma mensagem HELLO_ACK deve buscar a mensagem
*  HELLO correspondente na Fila de Mensagens de Controle do Nodo e retirá-la, e também atualizar o timeout da LSP para o vizinho (específico para HELLO).
*  A LSP entrará em timeout se uma mensagem HELLO ficar sem ACK por tempo superior ao seu timeout-HELLO.
*  Se a mensagem HELLO_ACK não encontrar uma HELLO correspondente, descartá-la simplesmente?
*  A mensagem HELLO_ACK não requer outro ACK e portanto não entra na fila de mensagens de controle do nodo.
*/
struct Packet *createHelloAckControlMsg(int source, int dst, int er[], int msgIDack) {
	struct Packet *pkt;


	pkt = createControlMsg(tarvosParam.helloMsgSize, source, dst, "HELLO_ACK"); //cria uma mensagem tipo HELLO_ACK
	attachExplicitRoute(pkt, er);
	//uma mensagem HELLO_ACK não requer ACK, portanto não é necessário colocá-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	pkt->ttl = 1; //limite o Time To Live a 1 hop, conforme indicado na RFC 3209, item 5.1
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt); //escalona evento de tratamento imediatamente
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_REFRESH
*
*  Cria um pacote contendo a mensagem de controle para refresh da LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  O pacote de controle criado será escalonado para um tempo entre 0 e 0,5 * Refresh_Interval a partir do simtime() atual.  Isto é
*  feito a fim de evitar a sincronização do envio das mensagens PATH_REFRESH pelos nodos, conforme indicado pela RFC 2205, item 3.7.
*  A RFC 2205 indica, porém, um intervalo entre 0,5 e 1.5 * Refresh_Interval, mas, no simulador, não é possível escalonar para
*  tempos menores que o próprio Refresh_Interval, devido à forma como foi implementado.
*
*  ATENÇÃO:  O pacote contendo a mensagem PATH_REFRESH seguirá comutado por rótulo, portanto a função que chama deverá passar o rótulo correto
*  como parâmetro.  Caso contrário, a mensagem perder-se-á.
*/
struct Packet *createPathRefreshControlMsg(int source, int dst, int LSPid, int iLabel) {
	struct Packet *pkt;
	double randInterval; //intervalo de tempo aleatório entre 0 e 0,5*Refresh_Interval que será somado ao tempo atual para escalonamento do pacote
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_REFRESH"); //cria uma mensagem tipo PATH_REFRESH
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label = iLabel; //coloca o label inicial no pacote, para que esta mensagem prossiga comutada por rótulo
	pkt->er.explicitRoute = NULL; //certifica que não há rota explícita no pacote
	pkt->er.recordThisRoute=1; //ative a gravação da rota no pacote, para uso da mensagem RESV_REFRESH de volta
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	randInterval = uniform(0, 0.5) * tarvosParam.LSPrefreshInterval; //cria intervalo de tempo aleatório entre 0 e 0,5 * Refresh_Interval, para evitar sincronismo de geração de mensagens PATH_REFRESH (RFC 2205)
	pkt->generationTime=simtime() + randInterval; //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), randInterval, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_REFRESH
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servirá para tratar e escalonar
*  o evento de recepção da mensagem pelo nodo origem.
*  A rota seguida, tipicamente, é a rota inversa percorrida pela mensagem PATH relacionada à mensagem RESV.  A função de processamento da mensagem PATH deve
*  gerar as informações pertinentes e passá-las para a presente função.  Cada nodo que receber uma mensagem RESV_REFRESH deve buscar a mensagem
*  PATH_REFRESH correspondente na Fila de Mensagens de Controle do Nodo e recalcular o timeout da LSP, modificando a entrada na LIB.
*  O timeout é calculado com base no momento em que a mensagem RESV_REFRESH é recebida.
*  Se a mensagem RESV_REFRESH não encontrar uma PATH_REFRESH correspondente, ainda assim deverá fazer o refresh na LSP?
*  A princípio, a mensagem RESV não precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estourá-lo, causará a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV não requer ACK.)
*/
struct Packet *createResvRefreshControlMsg(int source, int dst, int er[], int msgIDack, int LSPid) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_REFRESH"); //cria uma mensagem tipo RESV_REFRESH
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV não requer ACK, portanto não é necessário colocá-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* INICIA A CHECAGEM DE TIMEOUTS E OS TIMERS DE REFRESH
*
*  Esta função escalona os eventos iniciais de checagem de timeouts de mensagens de controle e LSPs e também a geração de mensagens de REFRESH
*  (PATH_REFRESH e RESV_REFRESH) e HELLO.  Os eventos escalonados estão contidos na estrutura de parâmetros da simulação e devem ser tradados no
*  programa do usuário.
*  Parâmetros a receber:  evento de controle de timeout; evento de controle de refresh; evento de geração de HELLOs
*/
void startTimers(int timeoutEv, int refreshEv, int helloEv) {
	if (timeoutEv > 0)
		schedulep(timeoutEv, tarvosParam.timeoutWatchdog, -1, NULL); //escalona timeoutWatchdog
	if (refreshEv > 0)
		schedulep(refreshEv, tarvosParam.LSPrefreshInterval, -1, NULL); //escalona refresh
	if (helloEv > 0)
		schedulep(helloEv, tarvosParam.helloInterval, -1, NULL); //inicia geração de HELLOs
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_DETOUR
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel Backup entre o nodo sourceMP e nodo dstMP (source Merge Point e destination Merge Point).
*  Os recursos serão compartilhados em caso da backup compartilhar o mesmo link que a working LSP, e serão pré-reservados também por esta mensagem
*  caso não haja reserva prévia para a working LSP.
*/
struct Packet *createPathDetourControlMsg(int sourceMP, int dstMP, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, sourceMP, dstMP, "PATH_DETOUR"); //cria uma mensagem tipo PATH_DETOUR
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_DETOUR_MAPPING
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servirá para tratar e escalonar
*  o evento de recepção da mensagem pelo nodo origem.
*  A rota explícita indicada, tipicamente, é a rota inversa percorrida pela mensagem PATH relacionada à mensagem RESV.
*  A função de processamento da mensagem PATH deve gerar as informações pertinentes (inclusive a rota explícita inversa) e passá-las para a presente
*  função.  Cada nodo que receber uma mensagem RESV_DETOUR_MAPPING deve buscar a mensagem PATH_DETOUR correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB, de modo a construir a backup LSP.  Esta mensagem RESV contém o mapeamento de rótulo adequado para a construção
*  das tabelas.  Pontos de junção (Merge Points) devem ser previstos também pelo processamento da RESV_DETOUR.
*  A princípio, a mensagem RESV não precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estourá-lo, causará a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV não requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de rótulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados, quando necessário.
*  O LSP Tunnel só estará completamente estabelecido e válido quando a mensagem RESV_DETOUR_MAPPING atingir o MP de ingresso.  Caso isto não aconteça,
*  os recursos e paths estabelecidos sofrerão timeout, pois não haverá REFRESH.
*/
struct Packet *createResvDetourControlMsg(int sourceMP, int dstMP, int er[], int msgIDack, int LSPid, int oLabel) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, sourceMP, dstMP, "RESV_DETOUR_MAPPING"); //cria uma mensagem tipo RESV_DETOUR_MAPPING
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV não requer ACK, portanto não é necessário colocá-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=oLabel; //coloca o label copiado da working LSP na mensagem; este rótulo deverá ser usado em pacotes enviados para este nodo-LSPid (para mapping)
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* DEVOLVE O STATUS DO TÚNEL LSP:  COMPLETADO OU NÃO
*
*  Um túnel LSP está completo se a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso contrário, o túnel ainda não está completado,
*  o que significa que nem todos os caminhos entre os nodos estão populados para o túnel.
*  Esta função retorna o valor do campo tunnelDone da estrutura LSP Table, para um dado LSPid (identificador globalmente único do túnel LSP).
*  O campo tunnelDone será ZERO se o túnel não está completo; será 1 se estiver completo.
*  Retorna -1 caso o túnel LSP não tenha sido encontrado na LSP Table.
*/
int getLSPtunnelStatus(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	return (p==NULL)? (-1):(p->tunnelDone);
}

/* ATUALIZA O STATUS DO TÚNEL LSP PARA COMPLETADO
*
*  Marca o campo tunnelDone na LSP Table, para uma determinada LSPid, para o valor 1.
*  Retorna 1 se a operação foi bem sucedida, ou -1 para o caso da LSPid não ter sido encontrada na tabela.
*/
int setLSPtunnelDone(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL)
		return -1; //LSPid não encontrado; retorne -1
	else
		p->tunnelDone=1;
	return 1; //operação bem sucedida; retorne 1
}

/* ATUALIZA O STATUS DO TÚNEL LSP PARA NÃO-COMPLETADO
*
*  Marca o campo tunnelDone na LSP Table, para uma determinada LSPid, para o valor 0.
*  Retorna 0 se a operação foi bem sucedida, ou -1 para o caso da LSPid não ter sido encontrada na tabela.
*/
int setLSPtunnelNotDone(int LSPid) {
	struct LSPTableEntry *p;

	p=searchInLSPTable(LSPid);
	if (p==NULL)
		return -1; //LSPid não encontrado; retorne -1
	else
		p->tunnelDone=0;
	return 0; //operação bem sucedida; retorne 0
}

/* RAPID RECOVERY
*
*  Implementação do recurso de Rapid Recovery do MPLS.  Esta função, ativada por uma sinalização de timeout vinda de um PATH_REFRESH ou
*  HELLO, buscará uma backup LSP para a working LSP sinalizada.
*  O algoritmo é:  buscar, para o nodo atual e LSPid atual, uma entrada na LIB para uma backup LSP (de mesma LSPid) com iIface igual a zero, ou seja,
*  uma backup LSP que inicie no nodo atual (chamado de Merge Point).  Encontrando-a, a função copiará os campos iIface e iLabel, recebidos
*  como parâmetro (e que são os campos iIface e iLabel da timeout LSP), na entrada para a backup LSP (na LIB) encontrada.  (A LSP que entrou em
*  timeout já deve ter sido assim marcada antes de chamada esta função.)  Desta forma, a LSP será desviada imediatamente para a backup LSP, sem
*  qualquer sinalização necessária, e de modo transparente para as aplicações.
*  Podem haver várias backup LSPs saindo do MP em questão.  Neste caso, a função searchInLIBnodLSPstatBak retorna a primeira "up" encontrada.  Caso esta
*  falhe no futuro, a próxima backup LSP poderá ser encontrada pela mesma função.
*
*  A função retorna 1 para o caso do Rapid Recovery ter sido bem sucedido, ou seja, uma entrada para backup LSP ter sido encontrada e o desvio, feito.
*  Neste caso, os recursos reservados para os links serão mantidos.
*  Retorna 0 para o caso do Rapid Recovery não se ter completado (nenhuma backup LSP encontrada).  Neste caso, a função que processa os timeouts deve
*  retornar os recursos reservados para o link envolvido.
*/
static int rapidRecovery(int node, int LSPid, int iIface, int iLabel) {
	struct LIBEntry *p;

	p=searchInLIBnodLSPstatBak(node, LSPid, "up"); //busca uma (a primeira achada) backup LSP em status "up", que tenha como MP inicial o nodo atual (a função chamada garante isso)
	if (p==NULL)
		return 0; //não achou backup LSP; retorne com ZERO para indicar a falha; a função que chama deve tratar isso, retornando recursos reservados
	//achou backup LSP; copie os parâmetros recebidos para a entrada da backup LSP, completando pois o detour
	p->iIface=iIface;
	p->iLabel=iLabel;
	return 1; //desvio completado com sucesso
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_ERR
*
*  Cria um pacote contendo a mensagem de controle de erro RESV_ERR para uma determinada LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  A mensagem RESV_ERR segue no sentido downstream, e, nesta implementação, segue comutada por rótulo (a LSP deve pre-existir, portanto).
*/
struct Packet *createResvErrControlMsg(int source, int dst, int LSPid, int iLabel, char *errorCode, char *errorValue) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_ERR"); //cria uma mensagem tipo RESV_ERR
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label = iLabel; //coloca o label inicial no pacote, para que esta mensagem prossiga comutada por rótulo
	strcpy(pkt->lblHdr.errorCode, errorCode);
	strcpy(pkt->lblHdr.errorValue, errorValue);
	pkt->er.explicitRoute = NULL; //certifica que não há rota explícita no pacote
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_ERR
*
*  Cria um pacote contendo a mensagem de controle de erro PATH_ERR para uma determinada LSP e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  A mensagem PATH_ERR segue no sentido upstream.  A rota inversa será computada nodo a nodo, através da leitura da LIB (a LSP deve pre-existir, portanto).
*/
struct Packet *createPathErrControlMsg(int source, int dst, int LSPid, char *errorCode, char *errorValue) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_ERR"); //cria uma mensagem tipo PATH_ERR
	pkt->lblHdr.LSPid=LSPid;
	strcpy(pkt->lblHdr.errorCode, errorCode);
	strcpy(pkt->lblHdr.errorValue, errorValue);
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO PATH_LABEL_REQUEST_PREEMPT
*
*  Cria um pacote contendo a mensagem de controle e escalona evento de tratamento contido na estrutura do nodo.
*  O evento de tratamento e o timeout da mensagem são obtidos do nodo por funções get específicas.
*  Esta função não cria uma entrada na Lista ou Fila de Mensagens de Controle do Nodo; a mensagem foi apenas gerada, e deve ainda
*  ser recebida pelo nodo de origem.
*  Esta mensagem objetiva estabelecer um LSP Tunnel, com preempção, entre o nodo source e nodo dst.  Os recursos são pré-reservados
*  por esta mensagem somente se estiverem disponíveis imediatamente.  Se não estiverem, a mensagem varrerá as LSPs estabelecidas no link/nodo
*  em questão para verificar se as de menor prioridade, com preempção, disponibilizarão os recursos.  Não haverá pré-reserva neste caso.
*  Só a mensagem RESV_LABEL_MAPPING_PREEMPT fará a reserva.
*  A mensagem só segue adiante se houver possibilidade de recursos serem reservados por preempção ou imediatamente.
*/
struct Packet *createPathPreemptControlMsg(int source, int dst, int er[], int LSPid) {
	struct Packet *pkt;
	
	pkt = createControlMsg(tarvosParam.pathMsgSize, source, dst, "PATH_LABEL_REQUEST_PREEMPT"); //cria uma mensagem tipo PATH_LABEL_REQUEST_PREEMPT
	pkt->lblHdr.LSPid=LSPid;
	attachExplicitRoute(pkt, er);
	/*a mensagem não deve ser inserida ainda na fila de mensagens de controle do nodo; é conveniente escalonar a mensagem para um evento que
	a trate e escalone-a para os eventos de recepção e transmissão apropriados.  A inserção na fila de mensagens de controle deve ser feita
	quando a mensagem é *efetivamente recebida* pelo nodo.*/
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* CRIA E ENVIA UMA MENSAGEM DE CONTROLE TIPO RESV_LABEL_MAPPING_PREEMPT
*
*  Cria um pacote contendo a mensagem de controle RESV e escalona evento recuperado do objeto nodo.  Este evento tipicamente servirá para tratar e escalonar
*  o evento de recepção da mensagem pelo nodo origem.
*  A rota explícita indicada, tipicamente, é a rota inversa percorrida pela mensagem PATH relacionada à mensagem RESV.
*  A função de processamento da mensagem PATH deve gerar as informações pertinentes (inclusive a rota explícita inversa) e passá-las para a presente
*  função.  Cada nodo que receber uma mensagem RESV_LABEL_MAPPING_PREEMPT deve buscar a mensagem PATH_LABEL_REQUEST_PREEMPT correspondente na Fila de Mensagens de Controle
*  do Nodo e inserir uma entrada na LIB e LSP Table, de modo a construir a LSP.  Esta mensagem RESV contém o mapeamento de rótulo adequado para a construção
*  das tabelas.
*  A princípio, a mensagem RESV não precisa de um timeout; a mensagem PATH correspondente tem sim seu timeout, e, ao estourá-lo, causará a mensagem RESV
*  que a responde ser simplesmente ignorada.  (RESV não requer ACK.)
*  Esta mensagem objetiva distribuir o mapeamento de rótulo para o LSP Tunnel pedido e efetivamente reservar os recursos solicitados.  O LSP Tunnel só 
*  estará completamente estabelecido e válido quando a mensagem RESV_LABEL_MAPPING atingir o LER de ingresso.  Caso isto não aconteça, os recursos e paths
*  estabelecidos sofrerão timeout, pois não haverá REFRESH.
*/
struct Packet *createResvPreemptControlMsg(int source, int dst, int er[], int msgIDack, int LSPid, int label) {
	struct Packet *pkt;

	pkt = createControlMsg(tarvosParam.resvMsgSize, source, dst, "RESV_LABEL_MAPPING_PREEMPT"); //cria uma mensagem tipo RESV_LABEL_MAPPING_PREEMPT
	attachExplicitRoute(pkt, er);
	//uma mensagem RESV não requer ACK, portanto não é necessário colocá-la na fila de mensagens de controle do nodo
	pkt->lblHdr.msgIDack=msgIDack;
	pkt->lblHdr.LSPid=LSPid;
	pkt->lblHdr.label=label; //coloca o label criado pelo LSR na mensagem; este rótulo deverá ser usado em pacotes enviados para este nodo-LSPid
	pkt->generationTime=simtime(); //marca o tempo em que o pacote foi gerado
	schedulep(getNodeCtrlMsgHandlEv(pkt->src), 0, pkt->id, pkt);
	return pkt;
}

/* FUNÇÕES DE MANUTENÇÃO DA LISTA AUXILIAR DE LSPs DO NODO-oIface (LspList)
*  Esta função inicializa a LspList.
*
*  A lista é do tipo duplamente encadeada com um Head Node, circular.
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

	//completa característica circular da lista
	lspList->head->next=lspList->head;
	lspList->head->previous=lspList->head;
	lspList->size=0;
	return lspList;
}

/* BUSCA NA LSPLIST
*
*  O item buscado é colocado no Head Node e a lista é percorrida do início ao fim.
*  A função retorna a posição do item buscado, ou a posição imediatamente anterior (própria para inserção), em ordem
*  decrescente.  Esta posição pode ser o próprio Head Node.  Como a lista é circular, a inserção funciona sem problemas
*  mesmo com o Head Node como retorno.
*/
static struct LspListItem *searchInLspList(struct LspList *lspList, int holdPrio) {
	struct LspListItem *lspListItem;

	lspListItem=lspList->head->next;
	lspList->head->holdPrio=holdPrio; //coloca HoldPrio buscada no Head Node
	while (lspListItem->holdPrio > holdPrio) {
		lspListItem=lspListItem->next; //próximo item na lista
	}
	return lspListItem;
}

/* INSERE NA LSPLIST
*
*  A inserção é feita em ordem da menor Holding Priority para a maior, ou seja, do maior número para o menor (pois 0 é a maior prioridade).
*/
static void insertInLspList(struct LspList *lspList, int LSPid, int setPrio, int holdPrio, struct LIBEntry *libEntry) {
	struct LspListItem *lspListItem, *newItem;

	lspListItem=searchInLspList(lspList, holdPrio); //posição imediatamente anterior ao ponto de inserção, em ordem numérica decrescente
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

/* FAZ PREEMPÇÃO DE RECURSOS EM UM LINK A PARTIR DOS REQUERIMENTOS DE UMA LSP
*
*  Esta função tentará reservar recursos solicitados para o estabelecimento de uma LSP no link indicado pelo pacote, usando de preempção, se necessário.
*  Os recursos são indicados, na estrutura do link, pelos parâmetros CBS, CIR e PIR.  A função comparará os números
*  pedidos pela LSP com os ainda disponíveis no link; se os disponíveis forem iguais ou maiores (todos eles), então a função diminuirá os
*  números disponíveis na estrutura do link, o que significa a aceitação da reserva.  Se algum número disponível for menor que o solicitado
*  pela LSP, então a LIB será varrida para o nodo-oIface em questão e uma lista auxiliar, chamada LSPTable, construída para LSPs com Holding Priority
*  menor que a Setup Priority da LSP corrente.  Se os recursos acumulados das LSPs de menor prioridade forem suficientes para a LSP corrente, então as
*  LSPs de menor prioridade serão colocadas em estado "preempted" e seus recursos, devolvidos para o link oIface (até o ponto em que os recursos acumulados
*  forem minimamente suficientes, não em excesso).  Então, a reserva será feita para a LSP corrente.
*  A função retorna 1 se a reserva foi aceita e feita, com ou sem preempção, e retorna 0 (ZERO) se a reserva não foi aceita (ou se a LSP indicada não foi achada na
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
	int preempt=0; //flag que indica se preempção será possível
	int resv; //retorno da função reserveResource

	cbs=pir=cir=0;	
	lsp = searchInLSPTable(LSPid);
	if (lsp != NULL) { //LSPid encontrado; teste os recursos disponíveis
		//testa agora se cada recurso solicitado para a LSP está disponível; se apenas um não estiver, tentar preempção
		if (tarvosModel.lnk[link].availCbs >= lsp->cbs && tarvosModel.lnk[link].availCir >= lsp->cir && tarvosModel.lnk[link].availPir >= lsp->pir) {
			//todos os recursos estão disponíveis; faça a reserva
			tarvosModel.lnk[link].availCbs -= lsp->cbs;
			tarvosModel.lnk[link].availCir -= lsp->cir;
			tarvosModel.lnk[link].availPir -= lsp->pir;
			return 1; //recursos foram efetivamente reservados
		}
		//algum recurso não está imediatamente disponível; tentar preempção
		lspList=initLspList();
		p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
		while (p!=lib.head) {
			lspTmp=searchInLSPTable(p->LSPid);
			if (p->oIface==link && p->node==node && strcmp(p->status, "up")==0 && lspTmp->holdPrio > lsp->setPrio && p->bak==0) { //só varre working LSPs "up" com HoldPrio "menor" que SetPrio
				cbs+=lspTmp->cbs; //acumule recursos
				cir+=lspTmp->cir;
				pir+=lspTmp->pir;
				insertInLspList(lspList, lspTmp->LSPid, lspTmp->setPrio, lspTmp->holdPrio, p); //insere a LSP existente na lista auxiliar LSPList
				//testa se recursos já acumulados são suficientes para reserva; se são, pare a varredura
				if (cbs>=lsp->cbs && cir>=lsp->cir && pir>=lsp->pir) {
					preempt=1; //marca flag de preempção possível
					break; //recursos acumulados já suficientes para preempção; interrompa a varredura
				}
			}
			p=p->previous;
		}
		//fim da varredura; se preempt=1, preempção será possível; se =0, não há recursos disponíveis mesmo com preempção.  Limpe a lista auxiliar e retorne com zero da função
		//começar varredura da lista auxiliar LSPList, já ordenada por Holding Priority, da menor para a maior (do maior número para o menor, de 0 a 7)
		lspListItem=lspList->head->next;
		while(lspListItem!=lspList->head) {
			if (preempt==1) { //preempção é possível; então, coloque todas as LSPs da lista auxiliar em "preempted" e devolva seus recursos
				returnResources(lspListItem->libEntry->LSPid, link); //devolva recursos
				strcpy(lspListItem->libEntry->status, "preempted"); //marca entrada na LIB para a LSP capturada como "preempted"
				lspListItem->libEntry->timeoutStamp=simtime(); //marca o timeoutStamp (mesmo que não tenha sido timeout) para este momento
			} //preempted
			aux=lspListItem;
			lspListItem=lspListItem->next;
			free(aux); //retira item da LSPList da memória
			lspList->size--;
		}
		if (lspList->size>0) {
			printf("\nError - preemptResources - auxiliary list LSPList not empty before cleanup");
			exit(1);
		}
		free(lspList->head); //retira Head Node da memória
		free(lspList); //remove ponteiro para a lista, concluindo a limpeza da lista auxiliar
		if (preempt==1) {
			resv=reserveResouces(LSPid, link);
			if (resv==0) {
				printf("\nError - preemptResources - resources unavailable for reservation even after preemption (inconsistency)");
				exit(1);
			}
			return 1; //recursos reservados com sucesso
		}
		//reserva não foi feita; função sairá com ZERO
	}
	return 0; //nenhuma reserva foi feita
}

/* TESTA SE HÁ RECURSOS DISPONÍVEIS NUM DETERMINADO NODO-oIface, SE HOUVER PREEMPÇÃO
*
*  Esta função varre a LIB para um determinado nodo, oIface (link), para working LSPs, com Holding Priority menor que a Setup Priority da LSP corrente,
*  acumulando os recursos de cada uma achada.  Se estes recursos, acumulados, forem suficientes para os requerimentos da LSP corrente, a função retorna
*  1 para SUCESSO; se não forem suficientes, retorna 0 para FALHA.
*/
int testResources(int LSPid, int node, int link) {
	struct LSPTableEntry *lsp, *lspTmp;
	struct LIBEntry *p;
	double cbs, cir, pir;
	
	cir=pir=cbs=0;
	lsp=searchInLSPTable(LSPid); //LSPid corrente (que está em montagem)
	p=lib.head->previous; //percorre no sentido inverso toda a tabela LIB
	while (p!=lib.head) {
		lspTmp=searchInLSPTable(p->LSPid);
		if (p->oIface==link && p->node==node && strcmp(p->status, "up")==0 && lspTmp->holdPrio > lsp->setPrio && p->bak==0) { //só varre working LSPs "up" com HoldPrio "menor" que SetPrio
			cbs+=lspTmp->cbs; //acumule recursos
			cir+=lspTmp->cir;
			pir+=lspTmp->pir;
			//testa se recursos já acumulados são suficientes para reserva; se são, pare a varredura
			if (cbs>=lsp->cbs && cir>=lsp->cir && pir>=lsp->pir)
				return 1; //recursos acumulados já suficientes para preempção; interrompa a varredura e retorne com SUCESSO
		}
		p=p->previous;
	}
	return 0; //recursos insuficientes mesmo com preempção; retorne com FALHA
}
