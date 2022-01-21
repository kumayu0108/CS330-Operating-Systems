#include<ulib.h>
#include<gthread.h>
/* XXX  Do not declare any global/static vars for this assignment*/

/*Thread functions must be declared as static*/
static void *thfunc1(void *arg){
            int *ptr = (int *) arg;
            int count = 0;
            printf("In thread: ");
            while(count < 256){
                    while(*ptr == 0)
                       sleep(1);
                    printf("%d ", *ptr);
                    *ptr = (*ptr) * 2;
                    count++;
                    ptr++;
            }
            printf("\n");
            exit(0);
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  int ctr;
  void *stackp;
  int thpid;
  int tharg[256] = {0};
  // clonetc1: create a simple thread using mmap and return 
  
  stackp = mmap(NULL, 8192, PROT_READ|PROT_WRITE, 0);
  if(!stackp || stackp == MAP_ERR){
        printf("Can not allocated stack\n");
        exit(0);
  }
  thpid = clone(&thfunc1, ((u64)stackp) + 8192, (void *)&tharg[0]);   // Returns the PID of the thread
  if(thpid <= 0){
       printf("Error creating thread!\n");
       exit(0);
  }
  printf("Created thread %d\n", thpid);
  make_thread_ready(thpid);
  for(ctr=0; ctr<256; ++ctr){
       tharg[ctr] = ctr + 1;
  }
  sleep(50);    // lets sleep for the thread to finish
  printf("In parent: ");
  for(ctr=0; ctr<256; ++ctr){
         printf("%d ", tharg[ctr]);
  }
  printf("\n");
  return 0;
}
