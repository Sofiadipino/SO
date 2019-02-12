//CONSEGNA 1
 
#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<pthread.h>
#include<sys/sysinfo.h>
#include<dirent.h>
#define N 200

int controlloterminazione[N];
typedef struct ElemLista{
	char nome[N];
	int tipo;
	char path[N];  
} TipoElemLista;

typedef struct NodoLista{
	TipoElemLista info;
	struct NodoLista *next;
} TipoNodoLista;

typedef TipoNodoLista *TipoLista;
//////////////////////////////////////////////////////////////// INSERIMENTO ORDINATO IN LISTA
void InserimentoOrdinatoInLista (TipoLista *lis, char *elem, int tipo, char *path){
	TipoLista paux, prec, corr;
	char pathaux[N];
	if((paux=(TipoLista)malloc(sizeof(TipoNodoLista)))==NULL){
		printf("ERRORE ALLOCAZIONE MEMORIA");
		exit(1);
	}
	strcpy(paux->info.nome, elem);
	paux->info.tipo= tipo;
	strcpy(pathaux, path);
	strcat(pathaux, "/");
	strcat(pathaux, paux->info.nome);
	strcpy(paux->info.path, pathaux);
	corr=*lis;
	prec=NULL;
	while(corr!=NULL && strcmp(corr->info.nome, elem)<0){
		prec=corr;
		corr=corr->next;
	}
	paux->next=corr;
	if(prec!=NULL)
		prec->next=paux;
	else
		*lis=paux;
	}
///////////////////////////////////////////////////////////// VISITA LISTA //////////////////////////////////////
	void Visita_Lista (TipoLista lis){
	while(lis!=NULL){
		if(lis->info.tipo==1)
			printf("Tipo: Directory. Nome: %s Path: %s\n\n", lis->info.nome, lis->info.path);
		else
			printf("Tipo: File. Nome: %s Path: %s\n\n", lis->info.nome, lis->info.path);
		lis=lis->next;
		}
	}//////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// SCANSIONA LISTA /////////////////////////////
void ScansionaLista (TipoLista *lis){
	int tipo;
	DIR *dr;
	struct dirent *de;
	while((*lis)!=NULL){
		if((*lis)->info.tipo==1){
			dr=opendir((*lis)->info.path);
			if(dr!=NULL){
				while((de=readdir(dr))!=NULL){
					if(de->d_type==DT_DIR) {
						tipo=1;
						InserimentoOrdinatoInLista(&(*lis), de->d_name, tipo, (*lis)->info.path);
						ScansionaLista(&(*lis));
					}
					else
						tipo=0;
						InserimentoOrdinatoInLista(&(*lis), de->d_name, tipo, (*lis)->info.path);  //COPIARE IN LISTA. incrementare lista
					}//while
				}//if
			}//if
		*lis=(*lis)->next;
		}//while
}//funz

void *Funzione_Thread(void *arg);
TipoLista lis=NULL;
pthread_mutex_t MutexLista;
pthread_mutex_t Mutex2Lista;
TipoLista TrovaDir(TipoLista lista);
///////////////////////////////////// ELIMAZIONE DIRECTORY /////////////7
void EliminaDirectory(TipoLista *lis, TipoElemLista elem){
	TipoLista paux;
	if(*lis!=NULL)
		if(strcmp((*lis)->info.path, elem.path)==0){
			paux=*lis;
		*lis=(*lis)->next;
		free(paux);
	}
	else
		EliminaDirectory(&(*lis)->next, elem);
}

////////////////////////////////////////// MAIN /////////////////////////////////////////
int main(){
	int dir, tipo;
	pthread_mutex_init(&MutexLista,NULL);
	pthread_mutex_init(&Mutex2Lista,NULL);

	struct dirent *de;
	char path[N];
	pthread_t th[N]; //tanti thread quanti cores
	int ncores, i,res;
	for(i=0;i<N;i++){
		controlloterminazione[i]=0;
	}

	DIR *dr;
	printf("Inserisci path assoluto: ");
	scanf("%s", path);
	dr=opendir(path);

	if(dr!=NULL){
		while((de=readdir(dr))!=NULL){
			if(de->d_type==DT_DIR)
				tipo=1;
			else
				tipo=0;
			if(strncmp(de->d_name, ".", 1)!=0){
				pthread_mutex_lock(&MutexLista);
				InserimentoOrdinatoInLista(&lis, de->d_name, tipo, path);  //COPIARE IN LISTA. incrementare lista
				pthread_mutex_unlock(&MutexLista);
			}//if
		}
	}

	closedir(dr);
	ncores=get_nprocs_conf(); //IMPORTANTE

	printf("\nQuesto PC ha %d cores\n", ncores);
//
	for(i=0; i<ncores; i++){
		res=pthread_create(&th[i], NULL, Funzione_Thread, (void *)(intptr_t)i); //HO SOST NULL CON i

		if(res!=0){
			printf("Errore creazione thread.\n");
			exit(EXIT_FAILURE);
		}
		pthread_mutex_unlock(&MutexLista);
	}//fine for

	for(i=0; i<ncores; i++){
		res=pthread_join(th[i],NULL);
		if(res!=0){
			printf("Errore join\n");
			exit(EXIT_FAILURE);
		}//if
	}
	Visita_Lista(lis);
	pthread_mutex_destroy(&MutexLista);
	pthread_mutex_destroy(&Mutex2Lista);
} //fine main

//////////////////////////////////////////////////////////// FUNZIONE THREAD ///////////////////////////////////
void *Funzione_Thread(void *arg){
	int numcores=(intptr_t)arg;
	int tipo;
	int finito=0;
	char pathaux[N];
	TipoLista esistedirectory;

	while(!finito){
		pthread_mutex_lock(&MutexLista);
		esistedirectory= TrovaDir(lis);
		pthread_mutex_unlock(&MutexLista);

		if(esistedirectory!=NULL){
			esistedirectory->next=NULL;
			pthread_mutex_lock(&MutexLista);
			EliminaDirectory(&lis, esistedirectory->info);
			pthread_mutex_unlock(&MutexLista);
			strcpy(pathaux, esistedirectory->info.path);

			pthread_mutex_lock(&Mutex2Lista);
			controlloterminazione[numcores]=0;
			pthread_mutex_unlock(&Mutex2Lista);

			DIR *dr;
			struct dirent *de;
			dr=opendir(pathaux);
			pthread_mutex_lock(&MutexLista);

			if(dr!=NULL){
				while((de=readdir(dr))!=NULL){
					if(de->d_type==DT_DIR) {
						tipo=1;
						if(strncmp(de->d_name, ".", 1)!=0){
							InserimentoOrdinatoInLista(&lis,de->d_name, tipo,pathaux);
						}//if
					}//if
					else{
						tipo=0;
						if(strncmp(de->d_name, ".", 1)!=0){
							InserimentoOrdinatoInLista(&lis,de->d_name, tipo,pathaux);
						}//if
					}//else
				}//while
			}//if
			pthread_mutex_unlock(&MutexLista);
			}
	finito=1;
	}//while
}//funz thread

//////////////////////////////////// TROVA DIRECTORY //////////////////////////////
TipoLista TrovaDir(TipoLista lista){ //se c'è ancora una directory torna il puntatore
	while(lis!=NULL){
		if(lista->info.tipo==1){
		return lista;
		}else
			lista=lista->next;
		}
	return NULL;
}
