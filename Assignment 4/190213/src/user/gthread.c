#include <gthread.h>
#include <ulib.h>

static struct process_thread_info tinfo __attribute__((section(".user_data"))) = {};
/*XXX 
      Do not modifiy anything above this line. The global variable tinfo maintains user
      level accounting of threads. Refer gthread.h for definition of related structs.
 */


static int custom_exit(){
	u64 ret;
	asm volatile(
	"mov %%rax, %0;"
	: "=r"(ret)
	:
	: "memory");

	gthread_exit((void *)ret);
}
/* Returns 0 on success and -1 on failure */
/* Here you can use helper system call "make_thread_ready" for your implementation */

int gthread_create(int *tid, void *(*fc)(void *), void *arg) {
        
	/* You need to fill in your implementation here*/
	if(tinfo.num_threads >= MAX_THREADS){return -1;}
	void *stackp = mmap(NULL, TH_STACK_SIZE, PROT_READ|PROT_WRITE, 0);
	if(!stackp || stackp == MAP_ERR){return -1;}
	long int thpid = clone(fc, ((u64)stackp) + TH_STACK_SIZE, arg);
	if(thpid <= 0){return -1;}
	tinfo.num_threads++;
	// getpid()
	
	// printf("***** %x ******\n", stackp);

	for(int i = 0; i < MAX_THREADS; i++){
		if(tinfo.threads[i].status != TH_STATUS_ALIVE && tinfo.threads[i].status != TH_STATUS_EXITED){
			tinfo.threads[i].status = TH_STATUS_ALIVE;
			tinfo.threads[i].pid = thpid;
			tinfo.threads[i].ret_addr = NULL;
			tinfo.threads[i].tid = i;
			*tid = i;
			tinfo.threads[i].stack_addr = stackp;
			// printf("______________%d tid used____________\n", i);
			for(int j = 0; j < MAX_GALLOC_AREAS; j++){
				tinfo.threads[i].priv_areas[j].owner = NULL;
				tinfo.threads[i].priv_areas[j].length = 0;
				tinfo.threads[i].priv_areas[j].start = 0;
				tinfo.threads[i].priv_areas[j].flags = 0;
			}
			break;
		}
	}

	u64 nwRsp = (((u64)stackp) + TH_STACK_SIZE - 0x8);
	// u64 bla;
	// asm volatile(
	// "mov %%rsp, %0;"
	// : "=r"(bla)
	// :
	// : "memory");
	*((u64 *)nwRsp) = (u64)custom_exit;

	int ret = make_thread_ready(thpid);
	
	// printf("#### %x ####\n", bla);
	// printf("### %x | %x | %x | %x | %x | %x ###\n", stackp, (((u64)stackp) + TH_STACK_SIZE), nwRsp, *((u64 *)nwRsp, (u64)custom_exit, gthread_exit));
	if(ret < 0){return -1;}
	return 0;
}

int gthread_exit(void *retval) {

	/* You need to fill in your implementation here*/
	u64 pid = getpid();
	for(int i = 0; i < MAX_THREADS; i++){
		if(tinfo.threads[i].status != TH_STATUS_ALIVE)continue;
		if(tinfo.threads[i].pid == pid){
			tinfo.threads[i].ret_addr = retval;
			tinfo.threads[i].status = TH_STATUS_EXITED;
			// printf("**************exit called for pid = %d, tid = %d and returned %x*****************\n", pid, tinfo.threads[i].tid, tinfo.threads[i].ret_addr);
			break;
		}
	}
	//call exit
	exit(0);
}

void* gthread_join(int tid) {
        
     /* Here you can use helper system call "wait_for_thread" for your implementation */
       
     /* You need to fill in your implementation here*/
	int i;
	for(i = 0; i < MAX_THREADS; i++){
		if(tinfo.threads[i].status != TH_STATUS_ALIVE && tinfo.threads[i].status != TH_STATUS_EXITED)continue;
		if(tinfo.threads[i].tid == tid){
			while(wait_for_thread(tinfo.threads[i].pid) == 0);
			// for(int j = 0; j < MAX_GALLOC_AREAS; j++){
			// 	if(tinfo.threads[i].priv_areas[j].owner == &tinfo.threads[i]){
			// 		munmap(tinfo.threads[i].priv_areas[j].start, tinfo.threads[i].priv_areas[j].length);
			// 	}
			// }
			munmap(tinfo.threads[i].stack_addr, TH_STACK_SIZE);
			tinfo.threads[i].status = TH_STATUS_USED;
			tinfo.num_threads--;
			return tinfo.threads[i].ret_addr;
		}
	}

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
	void *prStack;
	if(size > GALLOC_MAX)return NULL;

	u64 pid = getpid();  int thd, galAreaNum;
	for(thd = 0; thd < MAX_THREADS; thd++){
		if(tinfo.threads[thd].pid == pid){
			break;
		}
	}
	if(thd == MAX_THREADS)return NULL;

	for(galAreaNum = 0; galAreaNum < MAX_GALLOC_AREAS; galAreaNum++){
		if(tinfo.threads[thd].priv_areas[galAreaNum].owner != &tinfo.threads[thd]){
			break;
		}
	}
	if(galAreaNum == MAX_GALLOC_AREAS)return NULL;

	if(alloc_flag == GALLOC_OWNONLY){
		prStack = mmap(NULL, size, PROT_READ|PROT_WRITE|TP_SIBLINGS_NOACCESS, MAP_TH_PRIVATE);
	}
	else if(alloc_flag == GALLOC_OTRDONLY){
		prStack = mmap(NULL, size, PROT_READ|PROT_WRITE|TP_SIBLINGS_RDONLY, MAP_TH_PRIVATE);
	}
	else if(alloc_flag == GALLOC_OTRDWR){
		prStack = mmap(NULL, size, PROT_READ|PROT_WRITE|TP_SIBLINGS_RDWR, MAP_TH_PRIVATE);
	}
	else {return NULL;}
	// printf("___________________%x________________\n", prStack);
	tinfo.threads[thd].priv_areas[galAreaNum].owner = &tinfo.threads[thd];
	tinfo.threads[thd].priv_areas[galAreaNum].start = prStack;
	tinfo.threads[thd].priv_areas[galAreaNum].length = size;
	tinfo.threads[thd].priv_areas[galAreaNum].flags = alloc_flag;

	

	return prStack;
}
/*
   Only threads will invoke this. No need to check if the caller is a process.
*/
int gfree(void *ptr)
{
   
    /* You need to fill in your implementation here*/
	u64 pid = getpid();
	int thd, galAreaNum;
	for(thd = 0; thd < MAX_THREADS; thd++){
		if(tinfo.threads[thd].pid == pid){
			break;
		}
	}
	if(thd == MAX_THREADS)return -1;

	for(galAreaNum = 0; galAreaNum < MAX_GALLOC_AREAS; galAreaNum++){
		if(tinfo.threads[thd].priv_areas[galAreaNum].start == (u64)ptr){
			break;
		}
	}
	if(galAreaNum == MAX_GALLOC_AREAS)return -1;

	munmap(tinfo.threads[thd].priv_areas[galAreaNum].start, tinfo.threads[thd].priv_areas[galAreaNum].length);
	// printf("%%%%%% %d %%%%%%\n", rrr);
	tinfo.threads[thd].priv_areas[galAreaNum].owner = NULL;
	tinfo.threads[thd].priv_areas[galAreaNum].length = 0;
	tinfo.threads[thd].priv_areas[galAreaNum].start = 0;
	tinfo.threads[thd].priv_areas[galAreaNum].flags = 0;

	return 0;
}

