#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/msg.h>
#include <signal.h>

#include "libroMastro.h"
#include "utente.h"
#include "master.h"

sem_t Sem_utenti;
extern int SO_BUDGET_INIT;
extern int SO_USERS_NUM;
extern int SO_NODES_NUM;
extern int SO_REWARD;
extern long SO_MAX_TRANS_GEN_NSEC;
extern long SO_MIN_TRANS_GEN_NSEC;
extern int SO_RETRY;
extern int key_lista_utenti;
extern int key_lista_nodi;
extern int key_num_utenti;
extern int key_libro_mastro;
extern int key_ind_lib;
int user_budget, user_spent_budget, user_exec;
int a,b;

void utente_handler(){
    sem_init(&Sem_utenti, 0, 1);
    for ( b = 0; b < SO_USERS_NUM; b++){
        if (fork() == 0){
            struct msg_user_notify msg_n;
            struct timespec elaborazione, remaining = {0, 0};
            long ttwUtente;      /*time to wait utente*/
            int user_retry = 0, lm_id = 0;
            int msgIdNotify = msgget(getpid(), IPC_CREAT | 0664);
            pid_t *lista_utenti = (pid_t *)shmat(key_lista_utenti, 0, 0);
            pid_t *lista_nodi = (pid_t *)shmat(key_lista_nodi, NULL, 0);
            struct libro_mastro *libro_mastro = (struct libro_mastro *)shmat(key_libro_mastro, NULL, 0);
            int *num_utenti = (int *)shmat(key_num_utenti, NULL, 0);
            int *ind_lib = (int *)shmat(key_ind_lib, NULL, SHM_RDONLY);

            sem_wait(&Sem_utenti);
            for ( a = 0; a < SO_USERS_NUM; a++){
                if (lista_utenti[a] == 0){
                    lista_utenti[a] = getpid();
                    break;
                }
            }
            num_utenti[0]++;
            sem_post(&Sem_utenti);
            signal(SIGUSR1, sigTransUser);
            signal(SIGTERM, sigTermHandler);
            user_exec = 1;

            sleep(1);
            while (user_exec){
                lm_id = ind_lib[0];
                user_budget = SO_BUDGET_INIT;
                for ( a = 0; a < SO_REGISTRY_SIZE; a++){        /* calcolo denaro in entrata */
                    if (a != lm_id){
                        for ( b = 0; b < SO_BLOCK_SIZE - 1; b++){
                            if (libro_mastro->tb[a].vtl[b].receiver == getpid()){
                                user_budget += libro_mastro->tb[a].vtl[b].quantita;
                            }
                        }
                    }
                    else{
                        break;
                    }
                }
                
                if (msgrcv(msgIdNotify, &msg_n, sizeof(msg_n), 2, IPC_NOWAIT) != -1){  /* ricerca transazioni scartate */
                    user_spent_budget -= (msg_n.quantita + msg_n.reward);
                }
                user_budget -= user_spent_budget;

                if (user_budget >= 2 && num_utenti[0] > 1){
                    generaTransazione(lista_utenti, lista_nodi, user_budget);
                    user_retry = 0;
                    
                    ttwUtente = (rand() % (SO_MAX_TRANS_GEN_NSEC - SO_MIN_TRANS_GEN_NSEC + 1)) + SO_MIN_TRANS_GEN_NSEC;
                    elaborazione.tv_nsec = ttwUtente % 1000000000;
                    elaborazione.tv_sec = (ttwUtente / 1000000000);
                    nanosleep(&elaborazione, &remaining);
                }
                else{
                    if (user_budget < 2 && num_utenti[0] > 1){ 
                        if (user_retry < SO_RETRY){    
                            user_retry++;
                            ttwUtente = (rand() % (SO_MAX_TRANS_GEN_NSEC - SO_MIN_TRANS_GEN_NSEC + 1)) + SO_MIN_TRANS_GEN_NSEC;
                            elaborazione.tv_nsec = ttwUtente % 1000000000;
                            elaborazione.tv_sec = (ttwUtente / 1000000000);
                            nanosleep(&elaborazione, &remaining);
                        }
                        else{       
                            sem_wait(&Sem_utenti);
                            num_utenti[0]--;
                            user_exec = 0;
                        }
                    }
                    else if (num_utenti[0] <= 1){      
                        num_utenti[0]--;
                        user_exec = 0;
                    }
                }
            }
            shmdt(num_utenti);
            shmdt(lista_utenti);
            shmdt(ind_lib);
            shmdt(lista_nodi);
            shmdt(libro_mastro);
            sem_destroy(&Sem_utenti);
            exit(0);
        }
    }
}

void sigTransUser(){                      /* generazione transazione in corrispondenza di un segnale da terminale */
    struct timespec elaborazione, remaining = {0, 0};
    pid_t *lista_utenti = (pid_t *)shmat(key_lista_utenti, 0, 0);
    pid_t *lista_nodi = (pid_t *)shmat(key_lista_nodi, NULL, 0);
    long ttwUtente;

    generaTransazione(lista_utenti, lista_nodi, user_budget);
    printf("Il processo utente %d ha effettuato una transazione per conto di un segnale ricevuto da terminale.\n\n", getpid());
    ttwUtente = (rand() % (SO_MAX_TRANS_GEN_NSEC - SO_MIN_TRANS_GEN_NSEC + 1)) + SO_MIN_TRANS_GEN_NSEC;
    elaborazione.tv_nsec = ttwUtente % 1000000000;
    elaborazione.tv_sec = (ttwUtente / 1000000000);
    nanosleep(&elaborazione, &remaining);
    shmdt(lista_utenti);
    shmdt(lista_nodi);
}

void generaTransazione(pid_t *lista_utenti, pid_t *lista_nodi, int user_budget){  /* generazione nuova transazione */
    struct msg_t msg_t;
    struct timespec tf;
    pid_t pid_nodo_msg;
    int msgIdTrans = 0;
    srand(time(NULL) ^ (getpid()));

    do{
        msg_t.body.receiver = lista_utenti[rand() % (SO_USERS_NUM)];
    } while (msg_t.body.receiver == getpid() || msg_t.body.receiver == 0);

    do{
        pid_nodo_msg = lista_nodi[rand() % (SO_NODES_NUM)]; 
    } while (pid_nodo_msg == 0);

    msgIdTrans = msgget(pid_nodo_msg, 0664);
    msg_t.mtype = 1;

    clock_gettime(CLOCK_REALTIME, &tf);
    msg_t.body.timeStamp = tf.tv_sec;
    msg_t.body.sender = getpid();
    msg_t.body.quantita = (rand() % (user_budget - 2 + 1)) + 2;    

    msg_t.body.reward = (int)msg_t.body.quantita * (SO_REWARD * 0.01);
    if (msg_t.body.reward < 1) 
        msg_t.body.reward = 1;
    user_spent_budget += msg_t.body.quantita;
    msg_t.body.quantita -= msg_t.body.reward;
    msgsnd(msgIdTrans, &msg_t, sizeof(msg_t), 0); 
}

void sigTermHandler(){
    user_exec = 0;
}