#include "list.h"

typedef enum {high = 0, normal = 1, low = 2} PRIORITY;
typedef enum {ready = 0, running = 1, blocked = 2} STATE;

typedef struct PCB { // associated with every process
	int pid;
	int targetPid;
	char *sentMessage;
	char *proc_message; // message that will be printed out next time process is scheduled
	PRIORITY priority;
	STATE state;
} PCB;

typedef struct SEMAPHORE {
	int value;
	LIST *processes; // processes blocked on this semaphore
} SEMAPHORE;

/*********************************INITIALIZATION FUNCTIONS*********************************/

void createQueues();

void freeQueues();

PCB *createPCB();

void freePCB(PCB *process);

void init();

SEMAPHORE *createSemaphore();

void freeSemaphores();

/*************************************HELPER FUNCTIONS*************************************/

PCB *searchByPid(LIST *list, int pid);

void runNextProcess(); 

void printInfo(PCB *p);

int getIntArg();

/************************************SIMULATION FUNCTIONS**********************************/

void Create(int priority);

void Fork();

void Kill(int pid);

void Exit();

void Quantum();

void Send(int pid, char *msg);

void Receive();

void Reply(int pid, char *msg);

void NewSemaphore(int semaphoreID, int initialValue);

void SemP(int semaphoreID);

void SemV(int semaphoreID);

void ProcInfo(int pid);

void TotalInfo();