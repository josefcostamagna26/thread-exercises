#define SO_REGISTRY_SIZE 1000
#define SO_BLOCK_SIZE 10

void lm_handler();

void sigTermLibroHandler();

struct transazione_utente{    /* struct transazione */
        time_t timeStamp;
        pid_t sender;
        pid_t receiver;
        int quantita;
        int reward;
};

struct transazione_libro{    /* struct per transazione verso libro mastro */
        time_t timeStamp;
        pid_t sender;
        pid_t receiver;
        int quantita;
};

struct transazione_blocco{    /* struct blocco di transazioni del libro mastro */
        int id;
        struct transazione_libro vtl[SO_BLOCK_SIZE];
};

struct libro_mastro{             /* struct per shm del libro mastro */
        struct transazione_blocco tb[SO_REGISTRY_SIZE];
};

struct msg_body{        /* struct dati di un messaggio msg_t e msg_friend */
        time_t timeStamp;
        pid_t sender;
        pid_t receiver;
        int quantita;
        int reward;
};

struct msg_t{         /* struct per transazioni da utente a nodo */
        long mtype;
        struct msg_body body;
};

struct msg_user_notify{         /* struct per transazioni da nodo a utente */
        long mtype;
        int quantita;
        int reward;
};

struct msg_friend{       /* struct per transazioni verso nodi amici */
        long mtype;
        int hops;
        struct msg_body body;
};

struct msg_nodo_add{       /* struct per richiedere di aggiungere un nodo come amico ad altri nodi */
        long mtype;
        pid_t nuovo_nodo;
};

struct libroDati{        /* struct dati di una transazione da nodo a libro mastro */
        time_t timeStamp;
        pid_t sender;
        pid_t receiver;
        int quantita;
};

struct msg_lm{          /* struct per invio transazioni da nodo a libro mastro */
        long mtype;
        struct libroDati dati;
};