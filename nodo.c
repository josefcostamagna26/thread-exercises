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
#include <sys/signal.h>

#include "libroMastro.h"
#include "nodo.h"
#include "master.h"

#define SO_SENDER_REWARD -1

sem_t Sem_Nodi;
extern int SO_NODES_NUM;
extern int SO_TP_SIZE;
extern long SO_MAX_TRANS_PROC_NSEC;
extern long SO_MIN_TRANS_PROC_NSEC;
extern int SO_NUM_FRIENDS;
extern int SO_MAX_NUM_NEW_FRIENDS;
extern int SO_HOPS;
extern int SO_MIN_TRANS_FRIEND_GEN_SEC;
extern int SO_MAX_TRANS_FRIEND_GEN_SEC;
extern int key_lista_nodi;
extern int key_libro_pid;
extern int key_itp_nodi;
extern int key_num_friends;
int nodo_exec;
int itp;         /* indice transaction pool */
int itp_pos;
int *itp_nodi;
struct transazione_utente *transaction_pool;
pid_t *lista_nodi_friends;
pid_t nodo_friend;
int c, d;

void nodo_handler(){
    sem_init(&Sem_Nodi, 1, 1);
    for ( c = 0; c < SO_NODES_NUM; c++){
        nodo_lifecycle();
    }
}

void nodo_lifecycle(){
    if (fork() == 0){
        struct msg_t msg;
        struct msg_friend msg_friend;
        struct msg_user_notify msg_n;
        struct msg_nodo_add msg_nodo_add;
        struct timespec ts;
        struct timespec elaborazione, remaining = {0, 0};
        struct transazione_utente *trans_elab = (struct transazione_utente *)malloc(SO_BLOCK_SIZE * sizeof(struct transazione_utente));
        pid_t *lista_nodi = (pid_t *)shmat(key_lista_nodi, NULL, 0);
        int *key_lib = (int *)shmat(key_libro_pid, NULL, SHM_RDONLY); 
        int *num_friends = (int *)shmat(key_num_friends, NULL, 0);
        long ttwNodo;    /*time to wait nodo*/
        int msgIdLibroMastro = 0, msgIdNotify = 0, msgNodoFriend = 0;
        int msgIdTrans = msgget(getpid(), 0664 | IPC_CREAT);
        int msgIdMaster = msgget(getppid(), 0664);
        itp_nodi = (int *)shmat(key_itp_nodi, NULL, 0);
        transaction_pool = (struct transazione_utente *)malloc(SO_TP_SIZE * sizeof(struct transazione_utente));
        lista_nodi_friends = (pid_t *)malloc(sizeof(pid_t) * SO_MAX_NUM_NEW_FRIENDS);
        itp_pos = 0, itp = 0;

        sem_wait(&Sem_Nodi);
        for ( c = 0; c < SO_NODES_NUM; c++)
        {
            if (lista_nodi[c] == 0)
            {
                lista_nodi[c] = getpid();
                itp_pos = c;
                num_friends[c] = SO_NUM_FRIENDS;
                break;
            }
        }
        sem_post(&Sem_Nodi);

        for(c = 0; c < SO_NUM_FRIENDS; c++){
            do{
                do{
                    nodo_friend = lista_nodi[rand() % (SO_NODES_NUM)];
                } while (nodo_friend == 0 || nodo_friend == getpid());
                for(d = 0; d < SO_NUM_FRIENDS; d++){
                    if(lista_nodi_friends[d] == nodo_friend){
                        nodo_friend = 0;
                        break;
                    }else if(lista_nodi_friends[d] == 0){
                        lista_nodi_friends[d] = nodo_friend;
                        break;
                    }
                }
            }while(nodo_friend == 0);
        }
        shmdt(lista_nodi);
        
        while (1)
        {
            if(key_lib[0] != 0){
                msgIdLibroMastro = msgget(key_lib[0], 0664);
                break;
            }
        };
        shmdt(key_lib);
        signal(SIGTERM, sigTermNodoHandler);
        nodo_exec = 1;
        signal(SIGALRM, inviaTransAmico);
        alarm((rand() % (SO_MAX_TRANS_FRIEND_GEN_SEC - SO_MIN_TRANS_FRIEND_GEN_SEC + 1)) + SO_MIN_TRANS_FRIEND_GEN_SEC);

        while (nodo_exec){ 
            if (msgrcv(msgIdTrans, &msg, sizeof(msg), 1, IPC_NOWAIT) != -1){      /* ricerca transazioni in entrata */
                if (itp != SO_TP_SIZE){
                    transaction_pool[itp].timeStamp = msg.body.timeStamp;
                    transaction_pool[itp].sender = msg.body.sender;
                    transaction_pool[itp].receiver = msg.body.receiver;
                    transaction_pool[itp].quantita = msg.body.quantita;
                    transaction_pool[itp++].reward = msg.body.reward;
                    itp_nodi[itp_pos]++;
                }
                else{
                    msg_n.mtype = 2;
                    msgIdNotify = msgget(msg.body.sender, 0664);
                    msg_n.quantita = msg.body.quantita;
                    msg_n.reward = msg.body.reward;
                    msgsnd(msgIdNotify, &msg_n, sizeof(msg_n), 0);
                }
            }

            if(msgrcv(msgIdTrans, &msg_nodo_add, sizeof(msg_nodo_add), 5, IPC_NOWAIT) != -1){   /* ricerca nodi da aggiungere agli amici dal master */
                for(c = 0; c < SO_MAX_NUM_NEW_FRIENDS; c++){
                    if(lista_nodi_friends[c] != 0){
                        lista_nodi_friends[c] = msg_nodo_add.nuovo_nodo;
                        num_friends[itp_pos]++;
                        break;
                    }
                }
            }

            if(msgrcv(msgIdTrans, &msg_friend, sizeof(msg_friend), 4, IPC_NOWAIT) != -1){    /* ricerca transazioni da nodi amici */
                if (itp != SO_TP_SIZE){
                    transaction_pool[itp].timeStamp = msg_friend.body.timeStamp;
                    transaction_pool[itp].sender = msg_friend.body.sender;
                    transaction_pool[itp].receiver = msg_friend.body.receiver;
                    transaction_pool[itp].quantita = msg_friend.body.quantita;
                    transaction_pool[itp++].reward = msg_friend.body.reward;
                    itp_nodi[itp_pos]++;
                }else if(msg_friend.hops < SO_HOPS){
                    msg_friend.hops = msg_friend.hops + 1;
                    do{
                        nodo_friend = lista_nodi_friends[rand() % (SO_NUM_FRIENDS)];
                    }while(nodo_friend == getpid() || nodo_friend == 0);
                    msgNodoFriend = msgget(nodo_friend, 0664);
                    msgsnd(msgNodoFriend, &msg_friend, sizeof(msg_friend), 0);
                }else{
                    msgsnd(msgIdMaster, &msg_friend, sizeof(msg_friend), 0);
                }
            }

            if (itp >= SO_BLOCK_SIZE - 1)
            {   
                struct transazione_utente t_reward;
                struct msg_lm msg_lib;
                int cont_q = 0;
                msg_lib.mtype = 3;
                for (c = 0; c <= SO_BLOCK_SIZE - 2; c++)
                {
                    trans_elab[c] = transaction_pool[c];
                    cont_q += trans_elab[c].reward;
                    itp--;
                }
                if (itp != 0)
                {
                    int pointer = 0;
                    for ( d = c + 1; d < SO_TP_SIZE; d++)
                    {
                        if (transaction_pool[d].sender != 0)
                        {
                            transaction_pool[d] = transaction_pool[pointer];
                            pointer++;
                        }
                        else
                            break;
                    }
                }

                clock_gettime(CLOCK_REALTIME, &ts);
                t_reward.timeStamp = ts.tv_sec;
                t_reward.sender = SO_SENDER_REWARD;
                t_reward.receiver = getpid();
                t_reward.quantita = cont_q;
                t_reward.reward = 0;
                trans_elab[SO_BLOCK_SIZE - 1] = t_reward;

                srand(time(NULL) ^ (getpid()));
                ttwNodo = (rand() % (SO_MAX_TRANS_PROC_NSEC - SO_MIN_TRANS_PROC_NSEC + 1)) + SO_MIN_TRANS_PROC_NSEC;
                elaborazione.tv_nsec = ttwNodo % 1000000000;
                elaborazione.tv_sec = (ttwNodo / 1000000000);
                nanosleep(&elaborazione, &remaining);

                sem_wait(&Sem_Nodi);
                for ( c = 0; c < SO_BLOCK_SIZE; c++)
                {
                    msg_lib.dati.timeStamp = trans_elab[c].timeStamp;
                    msg_lib.dati.sender = trans_elab[c].sender;
                    msg_lib.dati.receiver = trans_elab[c].receiver;
                    msg_lib.dati.quantita = trans_elab[c].quantita;
                    itp_nodi[itp_pos]--;
                    msgsnd(msgIdLibroMastro, &msg_lib, sizeof(msg_lib), 0);  /* invio blocco di transazioni al libro mastro */
                }
                sem_post(&Sem_Nodi);
                itp_nodi[itp_pos]++;
            }
        }
        shmdt(itp_nodi);
        shmdt(num_friends);
        free(transaction_pool);
        free(trans_elab);
        exit(0);
    }
}

void sigTermNodoHandler(){
    nodo_exec = 0;
}

void inviaTransAmico(){        /* gestione invio transazione verso un nodo amico */
    if(itp > 0){  
        struct msg_friend msg_f;
        int msgNodoFriend;
        msg_f.mtype = 4;
        msg_f.hops = 1;
        msg_f.body.timeStamp = transaction_pool[itp-1].timeStamp; 
        msg_f.body.sender = transaction_pool[itp-1].sender;
        msg_f.body.receiver = transaction_pool[itp-1].receiver;
        msg_f.body.quantita = transaction_pool[itp-1].quantita;
        msg_f.body.reward = transaction_pool[itp-1].reward;
        itp--;
        itp_nodi[itp_pos]--;

        nodo_friend = lista_nodi_friends[rand() % (SO_NUM_FRIENDS)]; 
        msgNodoFriend = msgget(nodo_friend, 0664);
        msgsnd(msgNodoFriend, &msg_f, sizeof(msg_f), 0);
    }
    signal(SIGALRM, inviaTransAmico);
    alarm((rand() % (SO_MAX_TRANS_FRIEND_GEN_SEC - SO_MIN_TRANS_FRIEND_GEN_SEC + 1)) + SO_MIN_TRANS_FRIEND_GEN_SEC);
}