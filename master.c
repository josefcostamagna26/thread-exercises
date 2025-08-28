#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>

#include "libroMastro.h"
#include "nodo.h"
#include "utente.h"
#include "master.h"

int key_lista_nodi, key_lista_utenti, key_num_utenti, key_itp_nodi, key_num_friends;
int key_libro_mastro, key_ind_lib, key_libro_pid;             
pid_t *lista_utenti, *lista_nodi;
struct libro_mastro *libro_mastro;
int msgIdMaster;
int i, j, k, proc_esec;

int SO_USERS_NUM;
int SO_NODES_NUM;
int SO_MAX_NUM_NEW_NODI;
int SO_BUDGET_INIT;
int SO_REWARD;
long SO_MIN_TRANS_GEN_NSEC;
long SO_MAX_TRANS_GEN_NSEC;
long SO_MIN_TRANS_PROC_NSEC;
long SO_MAX_TRANS_PROC_NSEC;
int SO_TP_SIZE;
int SO_SIM_SEC;
int SO_RETRY;
int SO_NUM_FRIENDS;
int SO_MAX_NUM_NEW_FRIENDS;
int SO_HOPS;
int SO_MIN_TRANS_FRIEND_GEN_SEC;
int SO_MAX_TRANS_FRIEND_GEN_SEC;

int main(void)
{
    char param[1024];
    char val[1024];
    int value;
    int *num_utenti;
    
    FILE *fileParam = fopen("parameters.txt", "r");   /* lettura parametri da file */
    while (!feof(fileParam))     
    {
        fscanf(fileParam, "%s", param); 
        fscanf(fileParam, "%s", val);  
        sscanf(val, "%d", &value); 
        if (strcmp(param, "SO_USERS_NUM") == 0)
        {
            SO_USERS_NUM = value;
        }
        else if (strcmp(param, "SO_NODES_NUM") == 0)
        {
            SO_NODES_NUM = value;
        }
        else if(strcmp(param, "SO_MAX_NUM_NEW_NODI") == 0){
            SO_MAX_NUM_NEW_NODI = value + SO_NODES_NUM;
        }
        else if (strcmp(param, "SO_BUDGET_INIT") == 0)
        {
            SO_BUDGET_INIT = value;
        }
        else if (strcmp(param, "SO_REWARD") == 0)
        {
            SO_REWARD = value;
        }
        else if (strcmp(param, "SO_MIN_TRANS_GEN_NSEC") == 0)
        {
            SO_MIN_TRANS_GEN_NSEC = value;
        }
        else if (strcmp(param, "SO_MAX_TRANS_GEN_NSEC") == 0)
        {
            SO_MAX_TRANS_GEN_NSEC = value;
        }
        else if (strcmp(param, "SO_MIN_TRANS_PROC_NSEC") == 0)
        {
            SO_MIN_TRANS_PROC_NSEC = value;
        }
        else if (strcmp(param, "SO_MAX_TRANS_PROC_NSEC") == 0)
        {
            SO_MAX_TRANS_PROC_NSEC = value;
        }
        else if (strcmp(param, "SO_TP_SIZE") == 0)
        {
            SO_TP_SIZE = value;
        }
        else if (strcmp(param, "SO_SIM_SEC") == 0)
        {
            SO_SIM_SEC = value;
        }
        else if (strcmp(param, "SO_RETRY") == 0)
        {
            SO_RETRY = value;
        }
        else if(strcmp(param, "SO_NUM_FRIENDS") == 0){
            SO_NUM_FRIENDS = value;
        }
        else if(strcmp(param, "SO_MAX_NUM_NEW_FRIENDS") == 0){
            SO_MAX_NUM_NEW_FRIENDS = value + SO_NUM_FRIENDS;
        }
        else if(strcmp(param, "SO_HOPS") == 0){
            SO_HOPS = value;
        }
        else if(strcmp(param, "SO_MIN_TRANS_FRIEND_GEN_SEC") == 0){
            SO_MIN_TRANS_FRIEND_GEN_SEC = value;
        }
        else if(strcmp(param, "SO_MAX_TRANS_FRIEND_GEN_SEC") == 0){
            SO_MAX_TRANS_FRIEND_GEN_SEC = value;
        }
    }
    fclose(fileParam);

    key_lista_nodi = shmget(IPC_PRIVATE, sizeof(pid_t) * SO_MAX_NUM_NEW_NODI, IPC_CREAT | 0666);                        /* shm lista dei pid nodi*/
    key_itp_nodi = shmget(IPC_PRIVATE, sizeof(pid_t) * SO_MAX_NUM_NEW_NODI, IPC_CREAT | 0666);                          /* shm per itp dei nodi*/
    key_lista_utenti = shmget(IPC_PRIVATE, sizeof(pid_t) * SO_USERS_NUM, IPC_CREAT | 0666);                             /* shm lista dei pid utenti*/
    key_num_utenti = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);                                                /* shm numero di utenti*/
    key_libro_mastro = shmget(IPC_PRIVATE, sizeof(struct transazione_blocco *) * SO_REGISTRY_SIZE, IPC_CREAT | 0666);   /* shm del libro mastro*/
    key_ind_lib = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);                                                   /* shm indice del libro mastro*/
    key_libro_pid = shmget(IPC_PRIVATE, sizeof(pid_t), IPC_CREAT | 0666);                                               /* shm pid del libro mastro*/
    key_num_friends = shmget(IPC_PRIVATE, sizeof(int) * SO_MAX_NUM_NEW_FRIENDS, IPC_CREAT | 0666);                      /* shm per num friends in nodi */

    num_utenti = (int *)shmat(key_num_utenti, NULL, SHM_RDONLY);
    msgIdMaster = msgget(getpid(), IPC_CREAT | 0664);

    lm_handler();
    nodo_handler();
    utente_handler();

    signal(SIGALRM, sigIntHandler);    
    alarm(SO_SIM_SEC);                   /* timer di simulazione */

    while(1){
        if(num_utenti[0] == SO_USERS_NUM){
            shmdt(num_utenti);
            stampa_master();
            break;
        }
    }
    msgctl(msgIdMaster, IPC_RMID, 0);
    shmctl(key_lista_utenti, IPC_RMID, 0);
    shmctl(key_num_utenti, IPC_RMID, 0);
    shmctl(key_lista_nodi, IPC_RMID, 0);
    shmctl(key_itp_nodi, IPC_RMID, 0);
    shmctl(key_libro_mastro, IPC_RMID, 0);
    shmctl(key_ind_lib, IPC_RMID, 0);
    shmctl(key_libro_pid, IPC_RMID, 0);
    shmctl(key_num_friends, IPC_RMID, 0);
}

void stampa_master(){
    struct msg_friend msg_f;
    pid_t *lista_utenti_app = (pid_t*)malloc(sizeof(pid_t) * SO_USERS_NUM);
    int *utenti_budget = (pid_t*)malloc(sizeof(pid_t) * SO_USERS_NUM);
    pid_t *lista_nodi_app = (pid_t*)malloc(sizeof(pid_t) * SO_MAX_NUM_NEW_NODI);
    int *nodi_budget = (pid_t*)malloc(sizeof(pid_t) * SO_MAX_NUM_NEW_NODI);
    int *num_utenti = (int *)shmat(key_num_utenti, NULL, SHM_RDONLY);
    int *ind_lib = (int *)shmat(key_ind_lib, NULL, SHM_RDONLY);
    int *num_friends = (int *)shmat(key_num_friends, NULL, 0);
    int posMin = 0, aus = 0, lm_id = 0;
    
    lista_utenti = (pid_t *)shmat(key_lista_utenti, NULL, SHM_RDONLY); 
    lista_nodi = (pid_t *)shmat(key_lista_nodi, NULL, SHM_RDONLY);
    libro_mastro = (struct libro_mastro *)shmat(key_libro_mastro, NULL, SHM_RDONLY);
    proc_esec = 1;

    while (proc_esec)            /* stampa output ogni secondo */
    {
        system("clear"); 
        if(num_utenti[0] == 0){
            sigIntHandler(2);
            break;
        }else if(ind_lib[0] == SO_REGISTRY_SIZE){
            sigIntHandler(1);
            break;
        }

        if(msgrcv(msgIdMaster, &msg_f, sizeof(msg_f), 4, IPC_NOWAIT) != -1){    /* gestione creazione nuovo nodo */
            if(SO_NODES_NUM != SO_MAX_NUM_NEW_NODI){ 
                struct msg_t msg;
                pid_t pid_nodo_nuovo, pid_nodo_friend;
                int msgIdNodo, indice_rand;
                int *lista_nodi_check = (int *)malloc(sizeof(int) * SO_MAX_NUM_NEW_NODI);
                SO_NODES_NUM++;
                nodo_lifecycle();
                do{
                    pid_nodo_nuovo = lista_nodi[SO_NODES_NUM - 1];
                }while(pid_nodo_nuovo == 0);
                msgIdNodo = msgget(pid_nodo_nuovo, 0664);
                msg.mtype = 1;
                msg.body = msg_f.body;
                msgsnd(msgIdNodo, &msg, sizeof(msg), 0);

                for(i = 0; i < SO_NUM_FRIENDS; i++){
                    do{
                        indice_rand = rand() % SO_NODES_NUM - 1;
                        if(lista_nodi_check[indice_rand] == 0 && num_friends[indice_rand] != SO_MAX_NUM_NEW_FRIENDS){
                            struct msg_nodo_add msg_n;
                            pid_nodo_friend = lista_nodi[indice_rand];
                            lista_nodi_check[indice_rand] = 1;
                            msg_n.mtype = 5;
                            msg_n.nuovo_nodo = pid_nodo_nuovo;
                            msgIdNodo = msgget(pid_nodo_friend, 0664);
                            msgsnd(msgIdNodo, &msg_n, sizeof(msg_n), 0);
                            break;
                        }
                    }while(1);
                }
                free(lista_nodi_check);
            }
        }

        printf("Processi utente a nodo attivi: %d\n", num_utenti[0]);

        lm_id = ind_lib[0];
        aus = 0;

        for ( k = 0; k < SO_USERS_NUM; k++){           /* calcolo budget utenti dal libro mastro */
            lista_utenti_app[k] = lista_utenti[k];
            utenti_budget[k] = SO_BUDGET_INIT;
            for ( i = 0; i < SO_REGISTRY_SIZE; i++)
            {
                if (i != lm_id)
                {
                    for ( j = 0; j < SO_BLOCK_SIZE - 1; j++)
                    {
                        if (libro_mastro->tb[i].vtl[j].receiver == lista_utenti_app[k])
                        {
                            utenti_budget[k] += libro_mastro->tb[i].vtl[j].quantita;
                        }
                        else if (libro_mastro->tb[i].vtl[j].sender == lista_utenti_app[k])
                        {
                            utenti_budget[k] -= libro_mastro->tb[i].vtl[j].quantita; 
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }

        for ( i = 0; i <= SO_USERS_NUM - 2; i++){      /* ordinamento decrescente utenti per budget */
            posMin = i;
            for ( j = (i + 1); j <= SO_USERS_NUM - 1; j++)
            {
                if (utenti_budget[posMin] < utenti_budget[j])
                    posMin = j;
            }
            if (posMin != i)
            {
                aus = utenti_budget[i];
                utenti_budget[i] = utenti_budget[posMin];
                utenti_budget[posMin] = aus;

                aus = lista_utenti_app[i];
                lista_utenti_app[i] = lista_utenti_app[posMin];
                lista_utenti_app[posMin] = aus;
            }
        }

        for ( i = 0; i < SO_NODES_NUM; i++){           /* calcolo budget dei nodi dal libro mastro */
            lista_nodi_app[i] = lista_nodi[i];
            nodi_budget[i] = 0;
            for ( j = 0; j < SO_REGISTRY_SIZE; j++)
            {
                if (j != lm_id)
                {
                    if (libro_mastro->tb[j].vtl[SO_BLOCK_SIZE - 1].receiver == lista_nodi_app[i])
                    {
                        nodi_budget[i] += libro_mastro->tb[j].vtl[SO_BLOCK_SIZE - 1].quantita;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        for ( i = 0; i <= SO_NODES_NUM - 2; i++){      /* ordinamento decrescente nodi per budget */
            posMin = i;
            for ( j = (i + 1); j <= SO_NODES_NUM - 1; j++)
            {
                if (nodi_budget[posMin] < nodi_budget[j])
                    posMin = j;
            }
            if (posMin != i)
            {
                aus = nodi_budget[i];
                nodi_budget[i] = nodi_budget[posMin];
                nodi_budget[posMin] = aus;

                aus = lista_nodi_app[i];
                lista_nodi_app[i] = lista_nodi_app[posMin];
                lista_nodi_app[posMin] = aus;
            }
        }

        if(SO_USERS_NUM <= 8){                         /* stampa dei budget di utenti e nodi */
            for ( i = 0; i < SO_USERS_NUM; i++)
                printf("Utente %d: budget -> %d\n", lista_utenti_app[i], utenti_budget[i]);
        }else{
            for( i = 0; i < 4; i++)
                printf("Utente %d: budget -> %d\n", lista_utenti_app[i], utenti_budget[i]);
            for( i = SO_USERS_NUM - 4; i < SO_USERS_NUM; i++)
                printf("Utente %d: budget -> %d\n", lista_utenti_app[i], utenti_budget[i]);
        }
        if(SO_NODES_NUM <= 8){
            for ( i = 0; i < SO_NODES_NUM; i++)
                printf("Nodo %d: budget -> %d\n", lista_nodi_app[i], nodi_budget[i]);
        }else{
            for( i = 0; i < 4; i++)
                printf("Nodo %d: budget -> %d\n", lista_nodi_app[i], nodi_budget[i]);
            for( i = SO_NODES_NUM - 4; i < SO_NODES_NUM; i++)
                printf("Nodo %d: budget -> %d\n", lista_nodi_app[i], nodi_budget[i]);
        }
        printf("\n\n");
        sleep(1);
    }
    free(lista_utenti_app);
    free(utenti_budget);
    free(lista_nodi_app);
    free(nodi_budget);
    shmdt(num_utenti);
    shmdt(ind_lib);
    shmdt(lista_utenti);
    shmdt(lista_nodi);
    shmdt(libro_mastro);
    shmdt(num_friends);
}

void sigIntHandler(int reason)         /* gestore interruzione della simulazione */
{
    int *ind_lib = (int *)shmat(key_ind_lib, NULL, 0);
    int *itp_nodi = (int *)shmat(key_itp_nodi, NULL, 0);
    int *num_utenti = (int *)shmat(key_num_utenti, NULL, 0);
    int *key_lib = (int *)shmat(key_libro_pid, NULL, 0);
    int status;
    int budget = 0, msgQueueId = 0;
    struct libro_mastro *libro_mastro = (struct libro_mastro *)shmat(key_libro_mastro, NULL, 0);
    pid_t *lista_utenti = (pid_t *)shmat(key_lista_utenti, NULL, 0);
    pid_t *lista_nodi = (pid_t *)shmat(key_lista_nodi, NULL, 0);
    alarm(0);
    proc_esec = 0;
    
    printf("\n*** FINE DELLA SIMULAZIONE ***\n\n");
    printf("Motivo del termine: ");
    switch (reason)
    {
        case 1:
            printf("Il libro mastro ha raggiunto la capienza massima.\n");
            break;
        case 2:
            printf("Tutti i processi utente sono terminati.\n");
            break;
        default:
            printf("La simulazione ha raggiunto la durata massima limite.\n");
            break;
    }
    
    printf("\nNumero di blocchi contenuti nel libro mastro: %d\n", ind_lib[0]);

    msgQueueId = msgget(key_lib[0], 0664);
    msgctl(msgQueueId, IPC_RMID, 0);
    kill(key_lib[0], SIGTERM);
    waitpid(key_lib[0], &status, 0);
    shmdt(key_lib);

    printf("\nStampa bilancio dei processi utente:\n");
    
    for ( k = 0; k < SO_USERS_NUM; k++)       /* calcolo resoconto utenti */
    {
        msgQueueId = msgget(lista_utenti[k], 0664);  
        msgctl(msgQueueId, IPC_RMID, 0);
        kill(lista_utenti[k], SIGTERM);
        waitpid(lista_utenti[k], &status, 0);
        budget = SO_BUDGET_INIT;
        for ( i = 0; i < SO_REGISTRY_SIZE; i++)
        {
            if (i != ind_lib[0])
            {
                for ( j = 0; j < SO_BLOCK_SIZE - 1; j++)
                {
                    if (libro_mastro->tb[i].vtl[j].receiver == lista_utenti[k])
                    {
                        budget += libro_mastro->tb[i].vtl[j].quantita;
                    }
                    else if (libro_mastro->tb[i].vtl[j].sender == lista_utenti[k])
                    {
                        budget -= libro_mastro->tb[i].vtl[j].quantita;
                    }
                }
            }
            else
            {
                break;
            }
        }
        printf("Utente %d --> Budget: %d\n", lista_utenti[k], budget);
    }
    shmdt(lista_utenti);

    printf("\nStampa bilancio dei processi nodo:\n");
    for ( k = 0; k < SO_NODES_NUM; k++)       /* calcolo resoconto nodi */
    {
        msgQueueId = msgget(lista_nodi[k], 0664);
        msgctl(msgQueueId, IPC_RMID, 0);         
        kill(lista_nodi[k], SIGTERM);
        waitpid(lista_nodi[k], &status, 0);
        budget = 0;
        for ( i = 0; i < SO_REGISTRY_SIZE; i++)
        {
            if (i != ind_lib[0])
            {
                if (libro_mastro->tb[i].vtl[SO_BLOCK_SIZE - 1].receiver == lista_nodi[k])
                {
                    budget += libro_mastro->tb[i].vtl[SO_BLOCK_SIZE - 1].quantita;
                }
            }
            else
            {
                break;
            }
        }
        printf("Nodo %d --> Budget: %d, Transazioni rimaste in transaction pool: %d\n", lista_nodi[k], budget, itp_nodi[k]);
    }
    shmdt(lista_nodi);
    shmdt(itp_nodi);
    shmdt(ind_lib);
    shmdt(libro_mastro);

    printf("\nNumero dei processi utente terminati prematuramente: %d\n", SO_USERS_NUM - num_utenti[0]);
    shmdt(num_utenti);
}