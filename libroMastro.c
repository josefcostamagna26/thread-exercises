#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/signal.h>

#include "libroMastro.h"
#include "master.h"

extern int key_libro_mastro;
extern int key_ind_lib;
extern int key_libro_pid;
int libro_exec;
int f,g;

void lm_handler(){
    if (fork() == 0){
        struct msg_lm msg_lm;
        struct libro_mastro *libro_mastro = (struct libro_mastro *)shmat(key_libro_mastro, NULL, 0);
        struct transazione_blocco *tb = (struct transazione_blocco *)malloc(sizeof(struct transazione_blocco));
        int *ind_lib =  (int *)shmat(key_ind_lib, NULL, 0);
        int msgidMastro = msgget(getpid(), 0664 | IPC_CREAT);
        
        int *pid_libro = (int *)shmat(key_libro_pid, 0, 0);
        pid_libro[0] = getpid();
        shmdt(pid_libro);
        signal(SIGTERM, sigTermLibroHandler);
        libro_exec = 1;
        
        while (libro_exec){
            if (ind_lib[0] != SO_REGISTRY_SIZE){             /* gestore per l'aggiornamento del libro mastro */
                tb[0].id = ind_lib[0];
                for ( f = 0; f < SO_BLOCK_SIZE; f++){
                    if (msgrcv(msgidMastro, &msg_lm, sizeof(msg_lm), 3, 0) != -1){
                        tb[0].vtl[f].timeStamp = msg_lm.dati.timeStamp;
                        tb[0].vtl[f].sender = msg_lm.dati.sender;
                        tb[0].vtl[f].receiver = msg_lm.dati.receiver;
                        tb[0].vtl[f].quantita = msg_lm.dati.quantita;
                    }
                }
                libro_mastro->tb[ind_lib[0]] = tb[0];
                ind_lib[0]++;
            }
            else{
                break;
            }
        }
        shmdt(libro_mastro);
        shmdt(ind_lib);
        free(tb);
        exit(0);
    }
}

void sigTermLibroHandler(){
    libro_exec = 0;
}