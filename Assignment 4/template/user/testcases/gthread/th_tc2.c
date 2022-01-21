#include<ulib.h>
#include<gthread.h>
/*Thread functions must be declared as static*/
static void *thfunc1(void *arg){
            int *ptr = (int *) arg;
            for(int ctr=0; ctr < *ptr; ++ctr){
	       printf("[pid %d]Arg is %d\n", getpid(), *ptr);
            } 
            sleep(*ptr);
            *ptr += 100;
             return ptr;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  int s_count[8];
  int tid[8];
  int ctr;
  void *retval;
  //tc1
  // create one thread. and join should be success (pthread_exit)
  for(ctr=0; ctr<1; ++ctr){
        s_count[ctr] = ctr + 1;
        if(gthread_create(&tid[ctr], thfunc1, &s_count[ctr]) < 0){
             printf("gthread_create failed\n");
             exit(-1);
        }
	printf("Created thread: %d\n", tid[ctr]);
   }        
  for(ctr=0; ctr<1; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread %d returned is %d\n", tid[ctr], *((int *)retval));
   }
     
   return 0;
}
