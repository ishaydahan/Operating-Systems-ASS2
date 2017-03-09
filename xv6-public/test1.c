#include "types.h"
#include "stat.h"
#include "user.h"

static int time=0;

int soldiers;
int *global_states;
int *global_tid;
int *global_status;
int mutex1;
int mutex2;

int counter =0;

int me(int sol){
	return global_states[sol];
}

int left(int sol){
	return global_states[sol-1];
}

int right(int sol){
	return global_states[sol+1];
}

int func(int sol){	

	if (me(sol)==1){
		if (left(sol)==1 && right(sol)==1) return 1;
		if (left(sol)==1 && right(sol)==5) return 1;
		if (left(sol)==1 && right(sol)==6) return 1;
		if (left(sol)==3 && right(sol)==1) return 1;
		if (left(sol)==3 && right(sol)==5) return 1;
		if (left(sol)==3 && right(sol)==6) return 1;
		if (left(sol)==5 && right(sol)==1) return 1;
		if (left(sol)==5 && right(sol)==5) return 1;
		if (left(sol)==5 && right(sol)==6) return 1;
		if (left(sol)==6 && right(sol)==1) return 1;
		if (left(sol)==6 && right(sol)==5) return 1;
		if (left(sol)==6 && right(sol)==6) return 1;
		if (left(sol)==7 && right(sol)==5) return 1;
		if (left(sol)==7 && right(sol)==6) return 1;

		if (left(sol)==1 && right(sol)==3) return 3;
		if (left(sol)==3 && right(sol)==3) return 3;
		if (left(sol)==5 && right(sol)==3) return 3;
		if (left(sol)==6 && right(sol)==3) return 3;

		if (left(sol)==1 && right(sol)==4) return 4;
		if (left(sol)==1 && right(sol)==7) return 4;
		if (left(sol)==3 && right(sol)==4) return 4;
		if (left(sol)==3 && right(sol)==7) return 4;
		if (left(sol)==5 && right(sol)==4) return 4;
		if (left(sol)==5 && right(sol)==7) return 4;

		if (left(sol)==6 && right(sol)==4) return 6;
		if (left(sol)==6 && right(sol)==7) return 6;
		if (left(sol)==7 && right(sol)==3) return 6;

		if (left(sol)==7 && right(sol)==7) return 7;

	}
	if (me(sol)==2){
		if (left(sol)==2 && right(sol)==0) return 2;
		if (left(sol)==2 && right(sol)==2) return 2;
		if (left(sol)==2 && right(sol)==4) return 2;
		if (left(sol)==2 && right(sol)==5) return 2;
		if (left(sol)==2 && right(sol)==6) return 2;
		if (left(sol)==5 && right(sol)==2) return 2;
		if (left(sol)==5 && right(sol)==4) return 2;
		if (left(sol)==5 && right(sol)==5) return 2;
		if (left(sol)==5 && right(sol)==6) return 2;
		if (left(sol)==5 && right(sol)==7) return 2;
		if (left(sol)==6 && right(sol)==2) return 2;
		if (left(sol)==6 && right(sol)==4) return 2;
		if (left(sol)==6 && right(sol)==5) return 2;
		if (left(sol)==6 && right(sol)==6) return 2;
		if (left(sol)==6 && right(sol)==7) return 2;

		if (left(sol)==3 && right(sol)==2) return 3;
		if (left(sol)==3 && right(sol)==4) return 3;
		if (left(sol)==3 && right(sol)==5) return 3;
		if (left(sol)==7 && right(sol)==2) return 3;
		if (left(sol)==7 && right(sol)==4) return 3;
		if (left(sol)==7 && right(sol)==5) return 3;

		if (left(sol)==4 && right(sol)==2) return 4;
		if (left(sol)==4 && right(sol)==4) return 4;
		if (left(sol)==4 && right(sol)==5) return 4;
		if (left(sol)==4 && right(sol)==6) return 4;

		if (left(sol)==3 && right(sol)==6) return 6;
		if (left(sol)==4 && right(sol)==7) return 6;
		if (left(sol)==7 && right(sol)==6) return 6;

		if (left(sol)==3 && right(sol)==0) return 7;
		if (left(sol)==7 && right(sol)==0) return 7;
		if (left(sol)==7 && right(sol)==7) return 7;
	}
	if (me(sol)==3){
		if (left(sol)==1 && right(sol)==1) return 1;
		if (left(sol)==1 && right(sol)==2) return 1;
		if (left(sol)==1 && right(sol)==5) return 1;
		if (left(sol)==5 && right(sol)==1) return 1;
		if (left(sol)==7 && right(sol)==2) return 1;
		if (left(sol)==7 && right(sol)==5) return 1;

		if (left(sol)==6 && right(sol)==1) return 5;
	}
	if (me(sol)==4){
		if (left(sol)==1 && right(sol)==2) return 2;
		if (left(sol)==1 && right(sol)==7) return 2;
		if (left(sol)==2 && right(sol)==2) return 2;
		if (left(sol)==2 && right(sol)==5) return 2;
		if (left(sol)==5 && right(sol)==2) return 2;
		if (left(sol)==5 && right(sol)==7) return 2;

		if (left(sol)==2 && right(sol)==6) return 5;
	}
	if (me(sol)==5){
		if (left(sol)==1 && right(sol)==1) return 5;
		if (left(sol)==2 && right(sol)==2) return 5;
		if (left(sol)==2 && right(sol)==4) return 5;
		if (left(sol)==3 && right(sol)==1) return 5;

		if (left(sol)==1 && right(sol)==3) return 6;
		if (left(sol)==3 && right(sol)==3) return 6;
		if (left(sol)==4 && right(sol)==2) return 6;
		if (left(sol)==4 && right(sol)==4) return 6;

		if (left(sol)==1 && right(sol)==4) return 7;
		if (left(sol)==3 && right(sol)==2) return 7;
		if (left(sol)==3 && right(sol)==4) return 7;		
	}
	if (me(sol)==6){
		if (left(sol)==7 && right(sol)==3) return 1;

		if (left(sol)==4 && right(sol)==7) return 2;

		if (left(sol)==1 && right(sol)==3) return 3;

		if (left(sol)==4 && right(sol)==2) return 4;

		if (left(sol)==1 && right(sol)==1) return 6;
		if (left(sol)==2 && right(sol)==2) return 6;
		if (left(sol)==2 && right(sol)==7) return 6;
		if (left(sol)==7 && right(sol)==1) return 6;

		if (left(sol)==1 && right(sol)==6) return 7;
		if (left(sol)==6 && right(sol)==2) return 7;
		if (left(sol)==6 && right(sol)==7) return 7;	
		if (left(sol)==7 && right(sol)==6) return 7;					
	}
	if (me(sol)==7){
		if (left(sol)==0 && right(sol)==1) return 7;
		if (left(sol)==0 && right(sol)==2) return 7;
		if (left(sol)==0 && right(sol)==3) return 7;
		if (left(sol)==0 && right(sol)==6) return 7;
		if (left(sol)==1 && right(sol)==0) return 7;
		if (left(sol)==1 && right(sol)==2) return 7;
		if (left(sol)==1 && right(sol)==7) return 7;
		if (left(sol)==2 && right(sol)==0) return 7;
		if (left(sol)==2 && right(sol)==1) return 7;
		if (left(sol)==2 && right(sol)==7) return 7;
		if (left(sol)==4 && right(sol)==0) return 7;
		if (left(sol)==4 && right(sol)==3) return 7;
		if (left(sol)==4 && right(sol)==7) return 7;
		if (left(sol)==6 && right(sol)==0) return 7;
		if (left(sol)==6 && right(sol)==6) return 7;
		if (left(sol)==6 && right(sol)==7) return 7;
		if (left(sol)==7 && right(sol)==1) return 7;
		if (left(sol)==7 && right(sol)==2) return 7;
		if (left(sol)==7 && right(sol)==3) return 7;
		if (left(sol)==7 && right(sol)==6) return 7;

		if (left(sol)==0 && right(sol)==7) return 8;		
		if (left(sol)==7 && right(sol)==0) return 8;
		if (left(sol)==7 && right(sol)==7) return 8;
	}

	printf(1,"bad %d %d %d %d\n", sol, me(sol) ,left(sol), right(sol));
	exit();

}

void* foo(void){
	int i;
	int tid = kthread_id();
	int sol=0;
	int ans;
	for (i=0; i<soldiers; i++){
		if (global_tid[i]==tid) sol=i+1;
	}

	for(;;){
		if ((kthread_mutex_lock(mutex1))<0)
			printf(1,"fail lock mutex\n");
		ans = func(sol);
		counter++;
		if ((kthread_mutex_unlock(mutex1))<0)
			printf(1,"fail unlock mutex\n");
		if ((kthread_mutex_lock(mutex2))<0)
			printf(1,"fail lock mutex\n");
		global_states[sol] = ans;
		counter++;
		if ((kthread_mutex_unlock(mutex2))<0)
			printf(1,"fail unlock mutex\n");
		if (global_states[sol]==8){
			time=0;
			kthread_exit();
		} 
	}	
	return 0;
}

int
main(int argc, char *argv[])
{
	int i;

	soldiers = atoi(argv[1]);
	if (soldiers==0) return 1;
	
	int status[soldiers];
	global_status = status;
	int tid[soldiers];
	global_tid = tid;
	int states[soldiers+2];
	global_states = states;

	if ((mutex1 = kthread_mutex_alloc())<0)
		printf(1,"fail alloc mutex\n");
	if ((mutex2 = kthread_mutex_alloc())<0)
		printf(1,"fail alloc mutex\n");
	if ((kthread_mutex_lock(mutex1))<0)
		printf(1,"fail lock mutex\n");
	if ((kthread_mutex_lock(mutex2))<0)
		printf(1,"fail lock mutex\n");

	uint * stack;
	for (i=0; i<soldiers; i++){
		stack = malloc(4000);
		memset(stack, 0, sizeof(*stack));
		if ((tid[i] = kthread_create(foo, stack, 4000))<0)
			printf(1,"fail thread create\n");
	}

	states[0]=0;
	states[1]=7;
	for(i=2; i<soldiers+1; i++){
		states[i]=2;
	}
	states[soldiers+1]=0;
	printf(1,"TIME = %d\n", time++);
	//print
	printf(1,"%d ", states[0]); //commander
	for(i=1; i<soldiers+1; i++){
		printf(1,"%d ", states[i]);
	}
	printf(1,"%d\n", states[i]); //commander

	//main loop
	for (;;){
		printf(1,"TIME = %d\n", time++);
		//wait for everyone to recive ans
		if ((kthread_mutex_unlock(mutex1))<0)
			printf(1,"fail unlock mutex\n");
		while(counter<soldiers) sleep(1);
		counter=0;
		if ((kthread_mutex_lock(mutex1))<0)
			printf(1,"fail lock mutex\n");
		//wait for everyone to write ans
		if ((kthread_mutex_unlock(mutex2))<0)
			printf(1,"fail unlock mutex\n");
		while(counter<soldiers) sleep(1);
		counter=0;
		if ((kthread_mutex_lock(mutex2))<0)
			printf(1,"fail lock mutex\n");
		//print
		printf(1,"%d ", states[0]); //commander
		for(i=1; i<soldiers+1; i++){
			printf(1,"%d ", states[i]);
		}
		printf(1,"%d\n", states[i]); //commander
		//finish loop
		if (time == 0) break;
	}
	
	for (i=0; i<soldiers; i++)
		kthread_join(tid[i]);
	if ((kthread_mutex_dealloc(mutex1))<0)
		printf(1,"fail dealloc mutex\n");
	if ((kthread_mutex_dealloc(mutex2))<0)
		printf(1,"fail dealloc mutex\n");
  	exit();
}