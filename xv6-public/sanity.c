#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  
  int * val = malloc(sizeof(int));
       *val = 12;
  int* pa = malloc(sizeof(int)*1000);
  int* pb = malloc(sizeof(int)*1000);
  pa[10]=-1;
  pb[10]=-1;
  
  printf(2, "***** Sanity Check *****\n ***** Please be patiant *****\n");

    printf(1, "*****Father stats: *****\n" );
    procdump();
    printf(1, "**********************************\n");

  if(fork() == 0){ // child
     printf(1,"Fork has occured, childs stats:\n");
     procdump();
    printf(1, "Process Pid: %d: Value at 10th place before change is %d\n",getpid(),pa[10]);
     sleep(500);
      printf(1, "**************Change val child array in the 10th place********************\n");
     *val = 9999;
     pa[10]= 10;
    printf(1, "Process Pid: %d: Value at 10th place is %d\n",getpid(),pa[10]);
          procdump();

  exit();

  }
  else
  {
    sleep(500);
    wait();
    printf(1, "**************Checking that the parent array value in the 10th place remains -1********************\n");
    printf(1, "Process Pid: %d: Value at 10th place is %d\n",getpid(),pb[10]);
    procdump();
   sleep(500);
  }
    printf(1, "**************End Test********************\n");
  exit();
}
   
