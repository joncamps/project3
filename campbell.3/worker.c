#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct {
 unsigned int sec;
 unsigned int nano;
}Clock;

int main(int argc, char *argv[]){
 if (argc != 3){
  fprintf(stderr, "usage: worker [seconds] [nanoseconds]\n");
  exit(1);
 }
 int intervalSec = atoi(argv[1]);
 int intervalNano = atoi(argv[2]);
 
 
 printf("Worker starting, PID: %d PPID: %d\n", getpid(),getppid());
  
 key_t key = ftok("oss.c",1);
 if (key < 0){
  printf("error\n");
  exit(1);
 }

 int shm_id = shmget(key, sizeof(Clock),0700);
 if(shm_id < 0 ){
  printf("error\n");
  exit(1);
 }
 Clock *clock = (Clock *) shmat(shm_id, 0,0);
 if(clock ==(void *) -1){
 printf("error\n");
 exit(1);
 }
 unsigned int startSec = clock->sec;
 unsigned int startNano = clock->nano;

 unsigned int termSec = startSec + intervalSec;
 unsigned int termNano = startNano + intervalNano;

 if(termNano >= 1000000000){
  termSec ++;
  termNano -= 1000000000;
 }
 printf("Worker sees clock: %u sec %u nanosec\n", clock->sec,clock->nano);
 printf("WORKER PID:%d PPID:%d\n", getpid(), getppid());
 printf("SysClockS: %u SysClockNano: %u TermTimeS: %u TermTimeNano: %u\n", startSec, startNano, termSec, termNano);
 printf("-Just starting\n");

 unsigned int lastprintedSec = startSec;

 while(1) {
  unsigned int currentSec = clock->sec;
  unsigned int currentNano = clock->nano;

  if (currentSec > termSec || (currentSec == termSec && currentNano >= termNano))  {
   printf("WORKER PID: %d PPID: %d\n", getpid(), getppid());
   printf("sysClockS: %u SysClockNano: %u TermTimeS: %u TermTimeNano: %u\n", currentSec, currentNano, termSec, termNano);
   printf("-Terminating\n");
   break;
  }

  if ( currentSec > lastprintedSec) {
   printf("sysClockS: %u SysClockNano: %u TermTimeS: %u TermTimeNano: %u\n", currentSec, currentNano, termSec, termNano);
   printf(" %u seconds have passed since starting\n", currentSec - startSec);
   lastprintedSec = currentSec;
  }

	  
 }
 shmdt(clock);

 return 0;
}
