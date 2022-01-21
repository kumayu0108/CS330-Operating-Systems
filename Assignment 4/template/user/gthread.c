#include <gthread.h>
#include <ulib.h>

static struct process_thread_info tinfo __attribute__((section(".user_data"))) = {};
/*XXX 
      Do not modifiy anything above this line. The global variable tinfo maintains user
      level accounting of threads. Refer gthread.h for definition of related structs.
 */


/* Returns 0 on success and -1 on failure */
/* Here you can use helper system call "make_thread_ready" for your implementation */
int gthread_create(int *tid, void *(*fc)(void *), void *arg) {
        
	/* You need to fill in your implementation here*/
	
	return -1;
}

int gthread_exit(void *retval) {

	/* You need to fill in your implementation here*/
	
	
	//call exit
	exit(0);
}

void* gthread_join(int tid) {
        
     /* Here you can use helper system call "wait_for_thread" for your implementation */
       
     /* You need to fill in your implementation here*/

	return NULL;
}


/*Only threads will invoke this. No need to check if its a process
 * The allocation size is always < GALLOC_MAX and flags can be one
 * of the alloc flags (GALLOC_*) defined in gthread.h. Need to 
 * invoke mmap using the proper protection flags (for prot param to mmap)
 * and MAP_TH_PRIVATE as the flag param of mmap. The mmap call will be 
 * handled by handle_thread_private_map in the OS.
 * */

void* gmalloc(u32 size, u8 alloc_flag)
{
   
	/* You need to fill in your implementation here*/

}
/*
   Only threads will invoke this. No need to check if the caller is a process.
*/
int gfree(void *ptr)
{
   
    /* You need to fill in your implementation here*/
     	return 0;
}
