#include "a3.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#define NUM_READY_QUEUES 3 // each queue is a list of PCBs
#define NUM_SEMAPHORES 5
#define MESSAGE_SIZE 40

#define RUN_ORDER_SIZE 6
#define INIT_PROCESS_PID -1

static int runningIndex = 0;
static int processCounter = 0;
static int numReadyProcesses = 0;	

static bool isExecuting = true; // keep prompting user input while simulation is running

static LIST *readyQueues[NUM_READY_QUEUES];
static LIST *sendQueue;
static LIST *receiveQueue;

static PCB *currentProcess; 
static PCB *initProcess;

static SEMAPHORE *semaphoreArray[NUM_SEMAPHORES];
static PRIORITY runOrder[RUN_ORDER_SIZE] = { high, high, high, normal, normal, low };

/**************************************DOCUMENTATION***************************************/

/*
	For this assignment I implemented an inbox style message queue.
	Semaphore implmentation is followed from the CMPT 300 lecture notes. 
*/

/*********************************INITIALIZATION FUNCTIONS*********************************/

void createQueues() {
	for (int i = 0; i < NUM_READY_QUEUES; i++) {
		readyQueues[i] = ListCreate();
	}

	sendQueue = ListCreate();
	receiveQueue = ListCreate();
}

void freeQueues() {
	for (int i = 0; i < NUM_READY_QUEUES; i++) {
		for (int j = 0; j < ListCount(readyQueues[i]); j++) {
			
			// free all processes for each ready queue
			ListFree(readyQueues[i], freePCB);
			ListNext(readyQueues[i]);
		}
	}
	
	ListFree(sendQueue, freePCB);
	ListFree(receiveQueue, freePCB);
}

PCB *createPCB() {
	PCB *newProcess = malloc(sizeof(PCB));
	if (newProcess == NULL) {
		return NULL;
	}

	// allocate up to 40 characters + 1 null terminating character
	newProcess->sentMessage = malloc((MESSAGE_SIZE + 1) * sizeof(char));
	newProcess->proc_message = malloc((MESSAGE_SIZE + 1) * sizeof(char));

	if (newProcess->sentMessage == NULL || newProcess->proc_message == NULL) {
		free(newProcess);
		return NULL;
	}

	return newProcess;
}

void freePCB(PCB *thisProcess) {
	if (thisProcess != NULL) {
		free(thisProcess);
	}
}

void init() {
	initProcess = createPCB();
	initProcess->pid = INIT_PROCESS_PID;
	currentProcess = initProcess;
}

SEMAPHORE *createSemaphore() {
	SEMAPHORE *newSem = malloc(sizeof(SEMAPHORE));
	if (newSem == NULL) {
		return NULL;
	}

	newSem->processes = ListCreate();
	if (newSem->processes == NULL) {
		free(newSem);
		return NULL;
	}
	
	return newSem;
}

void freeSemaphores() {
	for (int i = 0; i < NUM_SEMAPHORES; i++) {
		if (semaphoreArray[i] != NULL) {
			free(semaphoreArray[i]->processes);
			free(semaphoreArray[i]);
		}
	}
	
}

/*************************************HELPER FUNCTIONS*************************************/

PCB *searchByPid(LIST *list, int pid) {
	for (int i = 0; i < ListCount(list); i++) {
		if (((PCB *)ListCurr(list))->pid == pid) {
			return ((PCB *)ListCurr(list));
		}
		ListNext(list);
	}

	// return back to the front of the list
	ListFirst(list);
	return NULL;
}

void runNextProcess() {
	// all readyQueues of each priority has processes available
	if (ListCount(readyQueues[high]) > 0 && ListCount(readyQueues[normal]) > 0 && ListCount(readyQueues[low]) > 0) {
		((PCB *)ListFirst(readyQueues[runOrder[runningIndex]]))->state = running;
		currentProcess = ListFirst(readyQueues[runOrder[runningIndex]]);
		runningIndex++;
	} 
	// readyQueue of high priority has processes available
	else if (ListCount(readyQueues[high]) > 0) {
		((PCB *)ListFirst(readyQueues[high]))->state = running;
		currentProcess = ListFirst(readyQueues[high]);
	}
	// readyQueue of normal priority has processes available
	else if (ListCount(readyQueues[normal]) > 0) {
		((PCB *)ListFirst(readyQueues[normal]))->state = running;
		currentProcess = ListFirst(readyQueues[normal]);
	} 
	// readyQueue of low priority has processes available
	else if (ListCount(readyQueues[low]) > 0) {
		((PCB *)ListFirst(readyQueues[low]))->state = running;
		currentProcess = ListFirst(readyQueues[low]);
	}
	else {
		currentProcess = initProcess;
		initProcess->state = running;
	}

	if (runningIndex >= RUN_ORDER_SIZE) {
		runningIndex = 0;
	}

	printf("Process with PID %d is now running.\n", currentProcess->pid);
}

void printInfo(PCB *process) {
	printf("Process ID: %d\n", process->pid);

	printf("Last message sent: %s\n", process->sentMessage);

	printf("Last message received: %s\n", process->proc_message);

	printf("Priority: ");
	if (process->priority == high) {
		printf("High\n");
	} 
	else if(process->priority == normal) {
		printf("Medium\n");
	} 
	else if(process->priority == low) {
		printf("Low\n");
	} 
	else {
		printf("N/A\n");
	}

	printf("State: ");
	if (process->state == running) {
		printf("Running\n");
	} 
	else if(process->state == ready) {
		printf("Ready\n");
	} 
	else if(process->state == blocked) {
		printf("Blocked\n");
	} 
}

int getIntArg() {
	char i[10];
	char *endptr = NULL;
	int answer;
	bool isCorrect = false;

	// keep prompting user for valid user input
	while (!isCorrect) {
		scanf("%s", i);
		answer = strtol(i, &endptr, 10);

		if (i != endptr && i && !*endptr) {
			isCorrect = true;
		} 
		else {
			printf("Invalid command. Please enter an integer.\n");

			// clears input buffer
			fflush(stdin);
		}
	}

	return answer;
}

/************************************SIMULATION FUNCTIONS**********************************/

void Create(int priority) {
	PCB *process = createPCB();

	if (process != NULL) {
		process->sentMessage = "";
		process->proc_message = "";
		process->targetPid = -1;
		process->priority = priority;
		process->pid = processCounter;
		processCounter++;

		if (numReadyProcesses == 0) {
			process->state = running;
			currentProcess = process;
		} 
		else {
			process->state = ready;
		}

		numReadyProcesses++;

		if (ListAppend(readyQueues[process->priority], process) == 0) {
			ListFirst(readyQueues[process->priority]);
			printf("Create successful.\n");
			printf("PID of new process: %d\n", process->pid);

			if (process->state == running) {
				printf("No currently running processes. This process is now running.\n");
			}

			return;
		}
	}
	else {
		printf("Create unsuccessful.\n");
		printf("New processes cannot be created.\n");
	}
}

void Fork() {
	if (currentProcess == initProcess) {
		printf("Fork unsuccessful.\n");
		printf("Attempted to fork init process.\n");
		return;
	}

	PCB *process = createPCB();

	if (process != NULL) {
		strcpy(process->sentMessage, currentProcess->sentMessage);
		strcpy(process->proc_message, currentProcess->proc_message);

		process->pid = processCounter;
		process->targetPid = currentProcess->targetPid;
		process->priority = currentProcess->priority;
		process->state = ready;

		processCounter++;
		numReadyProcesses++;

		if (ListAppend(readyQueues[process->priority], process) == 0) {
			ListFirst(readyQueues[process->priority]);
			printf("Fork successful.\n");
			printf("PID of new process: %d\n", process->pid);
			return;
		}
	} 
	else {
		printf("Fork unsuccessful.\n");
		printf("New processes cannot be created.\n");
	}	
}


void Kill(int pid) {
	// attempting to kill init process with pid -1
	if (pid == INIT_PROCESS_PID) {
		if (currentProcess == initProcess) {
			printf("Killing initial process with PID -1.\n");
			freePCB(initProcess);
			isExecuting = false;
			return;
		} 
		else {
			printf("Kill unsuccessful.\n");
			printf("Cannot kill initial process while there are other processes available.\n");
			return;
		}
	}

	if (currentProcess->pid == pid) {
		printf("Killing current process.\n");
		freePCB(ListRemove(readyQueues[currentProcess->priority]));

		numReadyProcesses--;

		runNextProcess();
		return;
	}

	bool processFound = false;

	if ((PCB *)searchByPid(readyQueues[high], pid) != NULL) {
		freePCB(ListRemove(readyQueues[high]));
		numReadyProcesses--;
		processFound = true;
	} 
	else if ((PCB *)searchByPid(readyQueues[normal], pid) != NULL) {
		freePCB(ListRemove(readyQueues[normal]));
		numReadyProcesses--;
		processFound = true;
	} 
	else if ((PCB *)searchByPid(readyQueues[low], pid) != NULL) {
		freePCB(ListRemove(readyQueues[low]));
		numReadyProcesses--;
		processFound = true;
	}
	else if ((PCB *)searchByPid(sendQueue, pid) != NULL) {
		freePCB(ListRemove(sendQueue));
		numReadyProcesses--;
		processFound = true;
	}
	else if ((PCB *)searchByPid(receiveQueue, pid) != NULL) {
		freePCB(ListRemove(receiveQueue));
		numReadyProcesses--;
		processFound = true;
	}
	else {
		for (int i = 0; i < NUM_SEMAPHORES; i++) {
			if (semaphoreArray[i] != NULL) {
				if ((PCB *) searchByPid(semaphoreArray[i]->processes, pid) != NULL) {
					freePCB(ListRemove(semaphoreArray[i]->processes));
					numReadyProcesses--;
					processFound = true;
				}
			}
			
		}
	}

	if (processFound == true) {
		printf("Kill successful.\n");
		printf("Process with PID %d successfully removed.\n", pid);
	}
	else {
		printf("Kill unsuccessful.\n");
		printf("Process with PID %d could not be found.\n", pid);
	}
}

void Exit() {
	if (currentProcess == initProcess) {
		printf("Exiting initial process with PID -1.\n");
		freePCB(initProcess);
		isExecuting = false;
		return;
	}
	
	printf("Terminated process with PID %d.\n", currentProcess->pid);
	freePCB(ListRemove(readyQueues[currentProcess->priority]));
	numReadyProcesses--;

	runNextProcess();
}

void Quantum() {
	printf("Time quantum of process with PID %d has expired.\n", currentProcess->pid);
	currentProcess->state = ready;

	ListAppend(readyQueues[currentProcess->priority], ListRemove(readyQueues[currentProcess->priority]));
	ListFirst(readyQueues[currentProcess->priority]);

	runNextProcess();
}

void Send(int pid, char *msg) {
	if (numReadyProcesses + ListCount(receiveQueue) <= 1) {
		printf("Send failure. No processes to send a message to.\n");
		return;
	}

	currentProcess->targetPid = pid;
	currentProcess->sentMessage = msg;

	if (currentProcess != initProcess) {
		printf("Process with PID %d will now be blocked.\n", currentProcess->pid);
		currentProcess->state = blocked;

		ListAppend(sendQueue, ListRemove(readyQueues[currentProcess->priority]));
		numReadyProcesses--;

		runNextProcess();
	}
	else {
		printf("Initial process has received message and target PID.\n");
	}
}

void Receive() {
	if (ListCount(sendQueue) == 0) {
		printf("No messages to receive.\n");

		if (currentProcess != initProcess) {
			printf("Process with PID %d will now be blocked.\n", currentProcess->pid);
			currentProcess->state = blocked;

			ListAppend(receiveQueue, ListRemove(readyQueues[currentProcess->priority]));
			numReadyProcesses--;

			runNextProcess();
		}
	}
	else {
		for (int i = 0; i < ListCount(sendQueue); i++) {
			PCB *process = ListCurr(sendQueue);

			if (process->targetPid == currentProcess->pid) {
				printf("Message of '%s' received from process with PID %d.\n", process->sentMessage, process->pid);
				currentProcess->proc_message = process->sentMessage;
			}

			ListNext(sendQueue);
		}

		ListFirst(sendQueue);
	}
}

void Reply(int pid, char *msg) {
	if (numReadyProcesses + ListCount(sendQueue) <= 1) {
		printf("Reply failure. No processes to reply to.\n");
		return;
	}

	PCB *sourceProcess = currentProcess;
	sourceProcess->pid = pid;
	sourceProcess->sentMessage = msg;
	
	PCB *process = ListFirst(sendQueue);
	process->proc_message = msg;
	printf("Message of '%s' from process with PID %d has been delivered.\n", sourceProcess->sentMessage, sourceProcess->pid);
	
	process->state = ready;
	ListAppend(readyQueues[process->priority], ListRemove(sendQueue));
	numReadyProcesses++;

	printf("Process with PID %d has been unblocked.\n", process->pid);
}

void NewSemaphore(int semaphoreID, int initalValue) {
	if (semaphoreArray[semaphoreID] != NULL) {
		printf("Creation of new Semaphore failed.\n");
		printf("Semaphore with ID %d has already been created.\n", semaphoreID);
		return;
	}

	if (semaphoreID < 0 || semaphoreID > 4) {
		printf("Creation of new Semaphore failed.\n");
		printf("Semaphore ID is out of bounds. Please enter a value between 0 and 4.\n");
		return;
	}

	semaphoreArray[semaphoreID] = createSemaphore();

	if (semaphoreArray[semaphoreID] != NULL) {
		semaphoreArray[semaphoreID]->value = initalValue;
		printf("Semaphore %d has been successfully initialized with an initial value of %d.\n", semaphoreID, initalValue);
		return;
	}
	else {
		printf("Creation of new Semaphore failed.\n");
		return;
	}
}

void SemP(int semaphoreID) {
	if (semaphoreArray[semaphoreID] == NULL) {
		printf("Semaphore %d has not been created. Cannot do a P operation.\n", semaphoreID);
		return;
	}

	SEMAPHORE *currentSemaphore = semaphoreArray[semaphoreID];

	// if the currently executing process is the init process, no need to block
	if (currentProcess != initProcess) {
		// if value <= 0, block the currently executing process and decrement value.
		if (currentSemaphore->value <= 0 && currentProcess != initProcess) {
			printf("Current running process was blocked.\n");
			currentProcess->state = blocked;

			ListAppend(currentSemaphore->processes, ListRemove(readyQueues[currentProcess->priority]));
			ListFirst(currentSemaphore->processes);

			runNextProcess();
		}
	}

	currentSemaphore->value--;

	printf("Semaphore with ID %d successfully decremented.\n", semaphoreID);
	printf("Current value: %d\n", semaphoreArray[semaphoreID]->value);
}

void SemV(int semaphoreID) {
	if(semaphoreArray[semaphoreID] == NULL) {
		printf("Semaphore %d has not been created. Cannot do a V operation.\n", semaphoreID);
		return;
	}

	SEMAPHORE *currentSemaphore = semaphoreArray[semaphoreID];

	currentSemaphore->value++;

	printf("Semaphore with ID %d successfully incremented.\n", semaphoreID);
	printf("Current value: %d\n", semaphoreArray[semaphoreID]->value);

	// if value > 0 then one process blocked from SemP becomes unblocked
	if (currentSemaphore->value > 0 && ListCount(currentSemaphore->processes) > 0 ) {
		PCB *process = ListFirst(currentSemaphore->processes);
		process->state = ready;

		ListAppend(readyQueues[process->priority], ListRemove(currentSemaphore->processes));
		ListFirst(readyQueues[process->priority]);

		printf("Process with PID of %d was unblocked.\n", process->pid);
	}
}

void ProcInfo(int pid) {
	// init process info
	if (pid == INIT_PROCESS_PID) {
		printInfo(initProcess);
		return;
	}

	PCB *processFound = NULL; 

	if ((PCB *)searchByPid(readyQueues[high], pid) != NULL) { 
		processFound = (PCB *)searchByPid(readyQueues[high], pid);
	} 
	else if ((PCB *)searchByPid(readyQueues[normal], pid) != NULL) { 
		processFound = (PCB *)searchByPid(readyQueues[normal], pid);
	} 
	else if ((PCB *)searchByPid(readyQueues[low], pid) != NULL) { 
		processFound = (PCB *)searchByPid(readyQueues[low], pid);
	} 
	else if ((PCB *)searchByPid(sendQueue, pid) != NULL) { 
		processFound = (PCB *)searchByPid(sendQueue, pid);
	}
	else if ((PCB *)searchByPid(receiveQueue, pid) != NULL) {
		processFound = (PCB *)searchByPid(receiveQueue, pid);
	}
	else {
		for (int i = 0; i < NUM_SEMAPHORES; i++) {
			if (semaphoreArray[i] != NULL) {
				if ((PCB *)searchByPid(semaphoreArray[i]->processes, pid) != NULL) {
					processFound = (PCB *)searchByPid(semaphoreArray[i]->processes, pid);
					break;
				}
			}
		}
	}

	if (processFound != NULL) {
		printInfo(processFound);
	} else {
		printf("Process with PID %d was not found.\n", pid);
	}
}

void TotalInfo() {
	PCB *currentProcess;

	printf("----High Priority Ready Queue----\n");
	for (int i = 0; i < ListCount(readyQueues[high]); i++) {
		currentProcess = ((PCB *)ListCurr(readyQueues[high]));

		printInfo(currentProcess);
		ListNext(readyQueues[high]);
	}
	ListFirst(readyQueues[high]);


	printf("\n----Normal Priority Ready Queue----\n");
	for (int i = 0; i < ListCount(readyQueues[normal]); i++) {
		currentProcess = ((PCB *)ListCurr(readyQueues[normal]));

		printInfo(currentProcess);
		ListNext(readyQueues[normal]);
	}
	ListFirst(readyQueues[normal]);


	printf("\n----Low Priority Ready Queue----\n");
	for (int i = 0; i < ListCount(readyQueues[low]); i++) {
		currentProcess = ((PCB *)ListCurr(readyQueues[low]));

		printInfo(currentProcess);
		ListNext(readyQueues[low]);
	}
	ListFirst(readyQueues[low]);


	for (int i = 0; i < NUM_SEMAPHORES; i++) {
		if (semaphoreArray[i] != NULL) {
			printf("\n-----Semaphore %d Blocked Queue-----\n", i);

			for (int j = 0; j < ListCount(semaphoreArray[i]->processes); j++) {
				currentProcess = ((PCB *)ListCurr(semaphoreArray[i]->processes));

				printInfo(currentProcess);
				ListNext(semaphoreArray[i]->processes);
			}

			ListFirst(semaphoreArray[i]->processes);
		}
	}


	printf("\n-----Waiting on Send Queue-----\n");
	for (int i = 0; i < ListCount(sendQueue); i++) {
		currentProcess = ((PCB *)ListCurr(sendQueue));

		printInfo(currentProcess);
		ListNext(sendQueue);
	}
	ListFirst(sendQueue);


	printf("\n-----Waiting on Receive Queue-----\n");
	for (int i = 0; i < ListCount(receiveQueue); i++) {
		currentProcess = ((PCB *)ListCurr(receiveQueue));

		printInfo(currentProcess);
		ListNext(receiveQueue);
	}
	ListFirst(receiveQueue);
}

/****************************************MAIN DRIVER***************************************/

int main() { 
	init();
	createQueues();

	char userInput[2];
	int arg;
	int arg2;
	
	while(isExecuting) {
		printf("\nEnter a command from the following possibilities: \n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\n", 
			"C: Create", 
			"F: Fork", 
			"K: Kill", 
			"E: Exit", 
			"Q: Quantum", 
			"S: Send", 
			"R: Receive",
			"Y: Reply", 
			"N: New Semaphore", 
			"P: Semaphore P", 
			"V: Semaphore V", 
			"I: ProcInfo",
			"T: TotalInfo");

		scanf("%s", userInput);
		// if the second character in the arg isn't the null terminating string
		while(userInput[1] != '\0') {
			printf("Invalid command. Please enter only one value.\n");

			// clears input buffer
			fflush(stdin);

			scanf("%s", userInput);
		}

		*userInput = tolower(*userInput);
		switch(userInput[0]) {
			case 'c':
				printf("Enter the priority for the process from the following possibilities: 0 = High, 1 = Normal, 2 = Low\n");
				arg = getIntArg();

				Create(arg);
				break;
			case 'f':
				Fork();
				break;
			case 'k':
				printf("Enter target process ID: \n");
				arg = getIntArg();

				Kill(arg);
				break;
			case 'e':
				Exit();
				break;
			case 'q':
				Quantum();
				break;
			case 's':
				printf("Enter target process ID: \n");
				arg = getIntArg();

				printf("Enter message you wish to send. Must be 40 characters or shorter.\n");
				char sendMessage[MESSAGE_SIZE + 1];
				scanf("%s", sendMessage);
				Send(arg, sendMessage);
				break;
			case 'r':
				Receive();
				break;
			case 'y':
				printf("Enter target process ID: \n");
				arg = getIntArg();

				printf("Enter message you wish to send. Must be 40 characters or shorter.\n");
				char receiveMessage[MESSAGE_SIZE + 1];
				scanf("%s", receiveMessage);
				Reply(arg, receiveMessage);
				break;
			case 'n':
				printf("Enter the ID between 0 and 4 for the semaphore you wish to create: \n");
				arg = getIntArg();

				printf("Enter the initial value for the semaphore you wish to create: \n");
				arg2 = getIntArg();

				NewSemaphore(arg, arg2);
				break;
			case 'p':
				printf("Enter target semaphore ID between 0 and 4: \n");
				arg = getIntArg();

				SemP(arg);
				break;
			case 'v':
				printf("Enter target semaphore ID between 0 and 4: \n");
				arg = getIntArg();

				SemV(arg);
				break;
			case 'i':
				printf("Enter target process ID: \n");
				arg = getIntArg();
				
				ProcInfo(arg);
				break;
			case 't':
				TotalInfo(arg);
				break;
			default:
				printf("Not a valid command.\n");
				break;
		}
	}
	printf("Initial process has been exited or killed. Program shall now terminate.\n");
	
	freeQueues();
	freeSemaphores();
}