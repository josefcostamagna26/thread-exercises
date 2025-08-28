# Transaction Simulation – Operating Systems Project 2021/22

This repository contains the implementation of the **Operating Systems course project (A.Y. 2021/22)**.  
The goal is to simulate a distributed system of *users* and *nodes* that exchange and process monetary transactions, storing them in a shared **ledger (libro mastro)**.

The project was developed in C using **processes, shared memory, semaphores, and message queues** to model concurrent interactions between components.

---

## 📌 Project Overview

The simulation models a system with:

- **Master process**  
  Manages the lifecycle of the simulation, spawns other processes, and periodically prints statistics.

- **User processes (`utente`)**  
  Each user has an initial budget and generates monetary transactions toward other users. Transactions are sent to random nodes for processing, with a fee (`reward`) paid to the node.

- **Node processes (`nodo`)**  
  Nodes receive, validate, and bundle transactions into blocks. Each block is added to the **ledger (libro mastro)**, which is a shared memory structure accessible to all processes.  
  Nodes can also forward transactions to *friend nodes* (normal version).

- **Ledger process (`libroMastro`)**  
  Stores validated transactions in blocks. Each block contains `SO_BLOCK_SIZE` transactions, and the ledger has a maximum size of `SO_REGISTRY_SIZE`.

The simulation terminates when:
- The simulation time (`SO_SIM_SEC`) expires, or  
- The ledger is full, or  
- All user processes have terminated.

At termination, the master prints a complete **report** including the reason for termination, budgets of all users and nodes, and ledger statistics.

---

## ⚙️ Configuration

Runtime parameters are read from `parameters.txt`.  
Key parameters include:

- `SO_USERS_NUM` → number of user processes  
- `SO_NODES_NUM` → number of node processes  
- `SO_BUDGET_INIT` → initial budget of each user  
- `SO_REWARD` → percentage of transaction paid as reward  
- `SO_MIN_TRANS_GEN_NSEC` / `SO_MAX_TRANS_GEN_NSEC` → interval for transaction generation  
- `SO_TP_SIZE` → transaction pool size of each node  
- `SO_MIN_TRANS_PROC_NSEC` / `SO_MAX_TRANS_PROC_NSEC` → processing time per block  
- `SO_SIM_SEC` → simulation duration  
- `SO_RETRY` → max consecutive failed attempts before a user terminates  
- `SO_NUM_FRIENDS` / `SO_HOPS` → (normal version) number of friends and max hops for forwarded transactions  

Compile-time parameters (defined in `libroMastro.h`):
- `SO_REGISTRY_SIZE` → maximum number of blocks in the ledger  
- `SO_BLOCK_SIZE` → number of transactions per block  

---

## 🛠️ Build & Run

This project uses a **Makefile** for compilation.

### Build
```bash
make
