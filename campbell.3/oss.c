#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/msg.h>

#define MAX_WORKERS 20
#define PERMS 0700

typedef struct {
int occupied;
pid_t pid;
unsigned int startSec;
unsigned int startNano;
unsigned int endSec;
unsigned int endNano;
unsigned int messagesSent;
} PCB;
PCB processTable[MAX_WORKERS];

typedef struct {
long mtype;
char strdata[100];
int intData;
}msgbuffer;

void signal_handler(int sig){
 exit(1);
}
typedef struct {
 unsigned int sec;
 unsigned int nano;
}Clock;

Clock *clock;
int shm_id;
void cleanup(int sig){
printf("\n OSS cleaning...\n");
exit(1);
}
int getFreeSlot(){
 for(int k=0; k<MAX_WORKERS;k++){
  if(processTable[k].occupied == 0) return k;
 }
 return -1;
}

int activeWorkers(){
 int count = 0;
 for(int k=0; k<MAX_WORKERS; k++)
  if(processTable[k].occupied) count++;
 return count;
}

void launchWorker(double t){
 int slot = getFreeSlot();
 if(slot == -1) return;

 pid_t pid = fork();
 if(pid == 0) {
 
  int workerSec = (int)t;
  int workerNano = (int)((t - workerSec)*1e9);
  char secStr[16], nanoStr[16];
  sprintf(secStr, "%d", workerSec);
  sprintf(nanoStr, "%d", workerNano);
  execl("./worker","worker",secStr, nanoStr, NULL);
  perror("execl failed");
  exit(1); 
 }
else{
  processTable[slot].occupied = 1;
  processTable[slot].pid = pid;
  processTable[slot].startSec = clock->sec;
  processTable[slot].startNano = clock->nano;
  processTable[slot].messagesSent = 0;

  unsigned int termSec = clock->sec + (int)t;
  unsigned int termNano = clock->nano + (int)((t - (int)t)*1e9);
  if(termNano >= 1000000000){
   termSec++;
   termNano -= 1000000000;
  }
  processTable[slot].endSec = termSec;
  processTable[slot].endNano = termNano;

  printf("launched worker PID %d in slot %d\n", pid, slot);

 }
}
int main(int argc, char *argv[]){

 signal(SIGALRM, signal_handler);
 signal(SIGINT, cleanup);
 alarm(60);

 int opt;
 int n = 1;
 int s = 1;
 double t = 1.0;
 double i = 1.0;

 while ((opt = getopt(argc,argv, "hn:s:t:i:")) != -1){
  switch(opt) {
    case 'h':
	 printf("Usage: oss [-h] [-n proc] -s simul] [-t limit] [-i interval]\n");
    	 exit(0);

    case 'n':
	n = atoi(optarg);
	break;

    case 's':
        s = atoi(optarg);
        break;

    case 't':
        t = atof(optarg);
        break;

    case 'i':
        i = atof(optarg);
        break;

    default:
    	fprintf(stderr,"wrong input\n");
    	exit(1);	
  }
 }

 printf("OSS starting, PID: %d PPID: %d\n", getpid(), getppid());
 printf("Called with:\n");
 printf("-n: %d\n", n);
 printf("-s: %d\n", s);
 printf("-t: %.1f\n", t);
 printf("-i: %.1f\n", i);

 msgbuffer buf0, buf1;
 int msqid;
 key_t msgkey;
 system("touch msgq.txt");

 if((msgkey = ftok("msgq.txt", 1)) == -1){
  perror("ftok");
  exit(1);
 }

 if((msqid = msgget(msgkey, PERMS | IPC_CREAT)) == -1){
  perror("msgget in parent");
  exit(1);
 }

 key_t key = ftok("oss.c",1);
 if (key == -1){
  printf("failed\n");
  exit(1);
  }

 int shm_id = shmget(key,sizeof(Clock), IPC_CREAT | 0700);
 if(shm_id == -1) {
  printf("parent: error\n");
  exit(1); 
 }
 clock = (Clock *)shmat(shm_id,NULL,0);
 if(clock == (void*)-1){
  printf("parent: error\n");
  exit(1);
 }

 clock->sec = 0;
 clock->nano = 0;
 unsigned int lastPrintedSec = 0;

 for(int k=0;k<MAX_WORKERS;k++) processTable[k].occupied = 0;


 double lastLaunchTime = 0.0;
 unsigned int incrementNano = 500000000;
 int launchedWorkers = 0;

 while(1){
  clock->nano += incrementNano;
  if(clock->nano >= 1000000000){
   clock->sec++;
   clock->nano -= 100000000;
  }
  if(launchedWorkers < s && activeWorkers() < n){
   }
  }

  int status;
  pid_t w = waitpid(-1, &status, WNOHANG);
  if(w > 0){
   for(int k=0;k<MAX_WORKERS;k++){
    if(processTable[k].occupied && processTable[k].pid == w){
     processTable[k].occupied = 0;
     printf("Worker PID %d terminated, cleared slot %d\n", w,k);
   }   
  }
  
   printf("OSS Clock: %u s %u ns\n", clock->sec, clock->nano);
   
 }

 if(clock->nano % 50000000 < incrementNano){
  lastPrintedSec = clock->sec;
  printf("OSS PID: %d SimClock: %u s %u ns\n", getpid(), clock->sec, clock->nano);
  printf("Process Table:\n Entry Occupied PID StartS STartN EndS EndN\n");
  for(int k=0; k<MAX_WORKERS; k++){
   PCB *p = &processTable[k];
   printf("%2d %d %5d %5u %5u %5u %5u %5u\n", k, p->occupied, p->pid, p->startSec, p->startNano, p->endSec, p->endNano, p->messagesSent);

  }
 }
 if(launchedWorkers == s && activeWorkers()==0) break;
 }

 shmdt(clock);
 shmctl(shm_id,IPC_RMID, NULL);

 printf("oss done.\n");

 return(0);
   
}

