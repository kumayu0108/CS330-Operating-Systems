#include<clone_threads.h>
#include<entry.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<mmap.h>

/*
  system call handler for clone, create thread like 
  execution contexts. Returns pid of the new context to the caller. 
  The new context starts execution from the 'th_func' and 
  use 'user_stack' for its stack
*/
long do_clone(void *th_func, void *user_stack, void *user_arg) 
{
  


  struct exec_context *new_ctx = get_new_ctx();
  struct exec_context *ctx = get_current_ctx();

  u32 pid = new_ctx->pid;
  
  if(!ctx->ctx_threads){  // This is the first thread
          ctx->ctx_threads = os_alloc(sizeof(struct ctx_thread_info));
          bzero((char *)ctx->ctx_threads, sizeof(struct ctx_thread_info));
          ctx->ctx_threads->pid = ctx->pid;
  }
     
 /* XXX Do not change anything above. Your implementation goes here*/
  
  
  // allocate page for os stack in kernel part of process's VAS
  // The following two lines should be there. The order can be 
  // decided depending on your logic.
  setup_child_context(new_ctx);
  new_ctx->type = EXEC_CTX_USER_TH;    // Make sure the context type is thread

  new_ctx->state = WAITING;            // For the time being. Remove it as per your need.
  // printk("############# %d %d \n", ctx->pid, ctx->ppid);
  new_ctx->ppid = ctx->pid;
  new_ctx->used_mem = ctx->used_mem;
  new_ctx->pgd = ctx->pgd;
  for(int i = 0; i < MAX_MM_SEGS; i++){ new_ctx->mms[i] = ctx->mms[i]; }
  new_ctx->vm_area = ctx->vm_area;
  
  new_ctx->regs.entry_cs = ctx->regs.entry_cs;
  new_ctx->regs.entry_rflags = ctx->regs.entry_rflags;
  new_ctx->regs.entry_ss = ctx->regs.entry_ss;

  new_ctx->regs.entry_rip = (u64)th_func;
  new_ctx->regs.entry_rsp = (u64)user_stack - 0x8;
  new_ctx->regs.rbp = (u64)user_stack;
  new_ctx->regs.rdi = (u64)user_arg;

	new_ctx->pending_signal_bitmap = ctx->pending_signal_bitmap;
  for(int i = 0; i < MAX_SIGNALS; i++){ new_ctx->sighandlers[i] = ctx->sighandlers[i]; }
  new_ctx->ticks_to_sleep = ctx->ticks_to_sleep;
  new_ctx->alarm_config_time = ctx->alarm_config_time;
  new_ctx->ticks_to_alarm = ctx->ticks_to_alarm;
  
  for(int i = 0; i < MAX_OPEN_FILES; i++){ new_ctx->files[i] = ctx->files[i]; }
  new_ctx->ctx_threads = NULL;

  for(int i = 0; i < MAX_THREADS; i++){
    if(ctx->ctx_threads->threads[i].status != TH_USED){
      ctx->ctx_threads->threads[i].status = TH_USED;
      ctx->ctx_threads->threads[i].pid = new_ctx->pid;
      ctx->ctx_threads->threads[i].parent_ctx = ctx;

      for(int j = 0; j < MAX_PRIVATE_AREAS; j++){
        ctx->ctx_threads->threads[i].private_mappings[j].owner = NULL;
        ctx->ctx_threads->threads[i].private_mappings[j].length = 0;
        ctx->ctx_threads->threads[i].private_mappings[j].start_addr = 0;
        ctx->ctx_threads->threads[i].private_mappings[j].flags = 0;
      }
      break;
    }
  }

  if(pid < 0)return -1;
  return pid;
}

/*This is the page fault handler for thread private memory area (allocated using 
 * gmalloc from user space). This should fix the fault as per the rules. If the the 
 * access is legal, the fault handler should fix it and return 1. Otherwise it should
 * invoke segfault_exit and return -1*/

int handle_thread_private_fault(struct exec_context *current, u64 addr, int error_code)
{
  // os_pfn_allo
   /* your implementation goes here*/
  // printk("________in handle private fault  ERROR : %d__________\n", error_code);
  asm volatile(
    "invlpg (%0)" 
    :
    :"r" (addr) 
    : "memory"
  );

  if(current->type != EXEC_CTX_USER_TH){
    // printk("$$$$$ accessed by parent $$$$$\n"); 
    // segfault_exit(current->pid, current->regs.entry_rip, addr);
    u64 first9 = ((addr & PGD_MASK) >> PGD_SHIFT);
    u64 second9 = ((addr & PUD_MASK)>> PUD_SHIFT);
    u64 third9 = ((addr & PMD_MASK) >> PMD_SHIFT);
    u64 fourth9 = ((addr & PTE_MASK) >> PTE_SHIFT);
    u64 pgd = current->pgd;
    u64 base = osmap(pgd);
    u64 *baseP = base;
    u64 pgd_t = baseP + first9;
    if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
      *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
      bzero( (char *)(*(u64 *)pgd_t), PAGE_SIZE );
      *(u64 *)pgd_t = (*((u64 *)pgd_t)) | 7; 
    }
    u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
    u64 base1 = pgd_t_shift << PAGE_SHIFT;
    u64 *baseP1 = base1;
    u64 pud_t = baseP1 + second9;
    if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
      *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
      bzero( (char *)(*(u64 *)pud_t), PAGE_SIZE );
      *(u64 *)pud_t = (*((u64 *)pud_t)) | 7; 
    }
    u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
    u64 base2 = pud_t_shift << PAGE_SHIFT;
    u64 *baseP2 = base2;
    u64 pmd_t = baseP2 + third9;
    if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
      *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
      bzero( (char *)(*(u64 *)pmd_t), PAGE_SIZE );
      *(u64 *)pmd_t = (*((u64 *)pmd_t)) | 7; 
    }
    u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
    u64 base3 = pmd_t_shift << PAGE_SHIFT;
    u64 *baseP3 = base3;
    u64 pte_t = baseP3 + fourth9;
    if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){
      *(u64 *)pte_t = osmap(os_pfn_alloc(USER_REG));
      bzero( (char *)(*(u64 *)pte_t), PAGE_SIZE );
    }
    *(u64 *)pte_t = (*((u64 *)pte_t)) | 7; 

    // asm volatile(
    //   "invlpg (%0)" 
    //   :
    //   :"r" (addr) 
    //   : "memory"
    // );

    return 1;
  }

  u64 ppid = current->ppid;
  struct exec_context *bla = get_ctx_by_pid(ppid);
  struct thread *curr_thread;
  for(int i = 0; i < MAX_THREADS; i++){
    if(bla->ctx_threads->threads[i].status != TH_USED)continue;
    if(bla->ctx_threads->threads[i].pid == current->pid){
      curr_thread = &bla->ctx_threads->threads[i];
    }
  }
  int found = 0;
  for(int i = 0; i < MAX_THREADS; i++){
    if(bla->ctx_threads->threads[i].status != TH_USED)continue;
    for(int j = 0; j < MAX_PRIVATE_AREAS; j++){
      struct thread *owner = bla->ctx_threads->threads[i].private_mappings[j].owner;
      u64 start_addr = bla->ctx_threads->threads[i].private_mappings[j].start_addr;
      u32 length = bla->ctx_threads->threads[i].private_mappings[j].length;
      u32 flags = (((bla->ctx_threads->threads[i].private_mappings[j].flags) >> 4) << 4);

      if(owner != NULL && addr >= start_addr && addr < start_addr + length){
        // printk("$$$$$ found! $$$$$\n");
        u64 first9 = ((addr & PGD_MASK) >> PGD_SHIFT);
        u64 second9 = ((addr & PUD_MASK)>> PUD_SHIFT);
        u64 third9 = ((addr & PMD_MASK) >> PMD_SHIFT);
        u64 fourth9 = ((addr & PTE_MASK) >> PTE_SHIFT);
        // u64 last12 = (addr & 0xfff);
        if(owner == curr_thread){
          // printk("$$$$$ owner $$$$$\n");
          // if(error_code == 0x4 || error_code == 0x6){
            // printk("$$$$$ unmapped page %x $$$$$\n", addr);
            u64 pgd = current->pgd;
            u64 base = osmap(pgd);
            u64 *baseP = base;
            u64 pgd_t = baseP + first9;
            if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
              *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pgd_t), PAGE_SIZE );
              *(u64 *)pgd_t = (*((u64 *)pgd_t)) | 7; 
            }
            u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
            u64 base1 = pgd_t_shift << PAGE_SHIFT;
            u64 *baseP1 = base1;
            u64 pud_t = baseP1 + second9;
            if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
              *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pud_t), PAGE_SIZE );
              *(u64 *)pud_t = (*((u64 *)pud_t)) | 7; 
            }
            u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
            u64 base2 = pud_t_shift << PAGE_SHIFT;
            u64 *baseP2 = base2;
            u64 pmd_t = baseP2 + third9;
            if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
              *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pmd_t), PAGE_SIZE );
              *(u64 *)pmd_t = (*((u64 *)pmd_t)) | 7; 
            }
            u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
            u64 base3 = pmd_t_shift << PAGE_SHIFT;
            u64 *baseP3 = base3;
            u64 pte_t = baseP3 + fourth9;
            if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){
              *(u64 *)pte_t = osmap(os_pfn_alloc(USER_REG));
              bzero( (char *)(*(u64 *)pte_t), PAGE_SIZE );
            }
            *(u64 *)pte_t = (*((u64 *)pte_t)) | 7; 
            
            // asm volatile(
            //   "invlpg (%0)" 
            //   :
            //   :"r" (addr) 
            //   : "memory"
            // );
          // }

          return 1;
        }
        else if(flags == TP_SIBLINGS_NOACCESS){
          segfault_exit(current->pid, current->regs.entry_rip, addr);
          return -1;
        }
        else {
          // printk("$$$$$ sibling $$$$$\n");
          if(error_code == 0x4){
            // printk("$$$$$ accessing read $$$$$\n"); 
            u64 pgd = current->pgd;
            u64 base = osmap(pgd);
            u64 *baseP = base;
            u64 pgd_t = baseP + first9;
            if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
              *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pgd_t), PAGE_SIZE );
              *(u64 *)pgd_t = (*((u64 *)pgd_t)) | 7; 
            }
            u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
            u64 base1 = pgd_t_shift << PAGE_SHIFT;
            u64 *baseP1 = base1;
            u64 pud_t = baseP1 + second9;
            if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
              *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pud_t), PAGE_SIZE );
              *(u64 *)pud_t = (*((u64 *)pud_t)) | 7; 
            }
            u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
            u64 base2 = pud_t_shift << PAGE_SHIFT;
            u64 *baseP2 = base2;
            u64 pmd_t = baseP2 + third9;
            if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
              *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pmd_t), PAGE_SIZE );
              *(u64 *)pmd_t = (*((u64 *)pmd_t)) | 7; 
            }
            u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
            u64 base3 = pmd_t_shift << PAGE_SHIFT;
            u64 *baseP3 = base3;
            u64 pte_t = baseP3 + fourth9;
            if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){
              *(u64 *)pte_t = osmap(os_pfn_alloc(USER_REG));
              bzero( (char *)(*(u64 *)pte_t), PAGE_SIZE );
            }

            // if(flags == TP_SIBLINGS_NOACCESS){ 
            //   *(u64 *)pte_t = ((*((u64 *)pte_t)) << PAGE_SHIFT) | 1; 
            // }
            if(flags == TP_SIBLINGS_RDONLY){ 
              *(u64 *)pte_t = (*((u64 *)pte_t)) | 5;
            }
            else {
              *(u64 *)pte_t = (*((u64 *)pte_t)) | 7;
            }
            
            return 1;
          }
          else if(flags == TP_SIBLINGS_RDONLY){
            segfault_exit(current->pid, current->regs.entry_rip, addr);
            return -1;
          }
          else if(error_code == 0x6){
            // printk("$$$$$ accessing write $$$$$\n"); 
            u64 pgd = current->pgd;
            u64 base = osmap(pgd);
            u64 *baseP = base;
            u64 pgd_t = baseP + first9;
            if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
              *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pgd_t), PAGE_SIZE );
              *(u64 *)pgd_t = (*((u64 *)pgd_t)) | 7; 
            }
            u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
            u64 base1 = pgd_t_shift << PAGE_SHIFT;
            u64 *baseP1 = base1;
            u64 pud_t = baseP1 + second9;
            if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
              *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pud_t), PAGE_SIZE );
              *(u64 *)pud_t = (*((u64 *)pud_t)) | 7; 
            }
            u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
            u64 base2 = pud_t_shift << PAGE_SHIFT;
            u64 *baseP2 = base2;
            u64 pmd_t = baseP2 + third9;
            if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
              *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
              bzero( (char *)(*(u64 *)pmd_t), PAGE_SIZE );
              *(u64 *)pmd_t = (*((u64 *)pmd_t)) | 7; 
            }
            u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
            u64 base3 = pmd_t_shift << PAGE_SHIFT;
            u64 *baseP3 = base3;
            u64 pte_t = baseP3 + fourth9;
            if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){
              *(u64 *)pte_t = osmap(os_pfn_alloc(USER_REG));
              bzero( (char *)(*(u64 *)pte_t), PAGE_SIZE );
            }

            // if(flags == TP_SIBLINGS_NOACCESS){ 
            //   *(u64 *)pte_t = ((*((u64 *)pte_t)) << PAGE_SHIFT) | 1; 
            // }
            // if(flags == TP_SIBLINGS_RDONLY){ 
            //   *(u64 *)pte_t = (*((u64 *)pte_t)) | 5;
            // }
            // else {
              *(u64 *)pte_t = (*((u64 *)pte_t)) | 7;
              return 1;
            // }
          }
          else if(error_code == 0x7){
            // printk("$$$$$ accessing write on read only %x $$$$$\n", addr); 
            if(flags == TP_SIBLINGS_RDWR){
              // printk(".........wtf..........\n");
              u64 pgd = current->pgd;
              u64 base = osmap(pgd);
              u64 *baseP = base;
              u64 pgd_t = baseP + first9;
              u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
              u64 base1 = pgd_t_shift << PAGE_SHIFT;
              u64 *baseP1 = base1;
              u64 pud_t = baseP1 + second9;
              u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
              u64 base2 = pud_t_shift << PAGE_SHIFT;
              u64 *baseP2 = base2;
              u64 pmd_t = baseP2 + third9;
              u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
              u64 base3 = pmd_t_shift << PAGE_SHIFT;
              u64 *baseP3 = base3;
              u64 pte_t = baseP3 + fourth9;
              *(u64 *)pte_t = (*((u64 *)pte_t)) | 7;
              // asm volatile(
              //   "invlpg (%0)" 
              //   :
              //   :"r" (addr) 
              //   : "memory"
              // );
              return 1; 
            }
          }
        }
        found = 1;
        break;
      }
    }
    if(found)break;
  }

  // printk("$$$$$ segfaulted normally $$$$$\n"); 
  segfault_exit(current->pid, current->regs.entry_rip, addr);
  return -1;
}

/*This is a handler called from scheduler. The 'current' refers to the outgoing context and the 'next' 
 * is the incoming context. Both of them can be either the parent process or one of the threads, but only
 * one of them can be the process (as we are having a system with a single user process). This handler
 * should apply the mapping rules passed in the gmalloc calls. */

int handle_private_ctxswitch(struct exec_context *current, struct exec_context *next)
{
  
   /* your implementation goes here*/
  //  printk("__________ in private ctx switch _____________\n");
   struct thread *currThd = NULL, *nextThd = NULL;
   struct exec_context *par = NULL;
   int isNextPar = 0, isCurrPar = 0;

   if(isProcess(next)){
    //  printk("--------------------- next is parent ----------------\n");
     par = next;
     isNextPar = 1;
   }
   else if(isProcess(current)){
    //  printk("--------------------- current is parent ----------------\n");
     par = current;
     isCurrPar = 1;
   }
   else {
     par = get_ctx_by_pid(current->ppid);
   }
   if(par == NULL)return -1;

   for(int i = 0; i < MAX_THREADS; i++){
     if(par->ctx_threads->threads[i].status != TH_USED)continue;
    //  if(par->ctx_threads->threads[i].pid == current->pid){currThd = &par->ctx_threads->threads[i];}
     if(par->ctx_threads->threads[i].pid == next->pid){nextThd = &par->ctx_threads->threads[i];}
   }
  //  if(nextThd != NULL){
    //  printk("--------------------- next thread pid is %d ---------------\n", nextThd->pid);
    for(int j = 0; j < MAX_THREADS; j++){
      currThd = &par->ctx_threads->threads[j];
      if(nextThd == currThd || currThd->status != TH_USED)continue;
     for(int i = 0; i < MAX_PRIVATE_AREAS; i++){
       if(currThd->private_mappings[i].owner == currThd){
        //  printk("-------------------- current addresses' permission bit set to 1 -----------------\n");
         u64 addr = currThd->private_mappings[i].start_addr;
         u32 len = currThd->private_mappings[i].length;
         u32 flags = (((currThd->private_mappings[i].flags) >> 4) << 4);
        //  printk("\n$$$$$$$$ LAST ADDR %x $$$$$$$$$\n", currThd->private_mappings[i].start_addr + len);
         while(addr < currThd->private_mappings[i].start_addr + len){
          //  printk("$$$$$$$$ %x $$$$$$$$$\n", addr);
            u64 first9 = ((addr & PGD_MASK) >> PGD_SHIFT);
            u64 second9 = ((addr & PUD_MASK)>> PUD_SHIFT);
            u64 third9 = ((addr & PMD_MASK) >> PMD_SHIFT);
            u64 fourth9 = ((addr & PTE_MASK) >> PTE_SHIFT);

            u64 pgd = current->pgd;
            u64 base = osmap(pgd);
            u64 *baseP = base;
            u64 pgd_t = baseP + first9;
            if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
            u64 base1 = pgd_t_shift << PAGE_SHIFT;
            u64 *baseP1 = base1;
            u64 pud_t = baseP1 + second9;
            if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
            u64 base2 = pud_t_shift << PAGE_SHIFT;
            u64 *baseP2 = base2;
            u64 pmd_t = baseP2 + third9;
            if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
            u64 base3 = pmd_t_shift << PAGE_SHIFT;
            u64 *baseP3 = base3;
            u64 pte_t = baseP3 + fourth9;
            
            if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){addr = (addr) + PAGE_SIZE; continue;}
            // if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 != 1){
            //   *(u64 *)pte_t = os_pfn_alloc(USER_REG);
            //   *(u64 *)pte_t = ((*((u64 *)pte_t)) << PAGE_SHIFT) | 1;
            // }
            if(isNextPar){
              *(u64 *)pte_t = (((*((u64 *)pte_t)) >> 3) << 3) | 7;
            }
            else if(flags == TP_SIBLINGS_NOACCESS){
              // printk(".....no access.....\n");
              *(u64 *)pte_t = (((*((u64 *)pte_t)) >> 3) << 3) | 1;
            }
            else if(flags == TP_SIBLINGS_RDONLY){
              // printk(".....RD only.....\n");
              *(u64 *)pte_t = (((*((u64 *)pte_t)) >> 3) << 3) | 5;
            }
            else if(flags == TP_SIBLINGS_RDWR){
              // printk(".....RDWR.....\n");
              *(u64 *)pte_t = (((*((u64 *)pte_t)) >> 3) << 3) | 7;
            }
            // printk("$$ %x $$\n", flags);
            // printk("$$$$$$$$ %x $$$$$$$$\n", *(u64 *)pte_t);

            asm volatile(
              "invlpg (%0)" 
              :
              :"r" (addr) 
              : "memory"
            );

           addr = (addr) + PAGE_SIZE;
         }
       }
     }
    }
  //  }
   if(nextThd != NULL){
     for(int i = 0; i < MAX_PRIVATE_AREAS; i++){
       if(nextThd->private_mappings[i].owner == nextThd){
        //  printk("-------------------- next addresses' permission bit set to 7 -----------------\n");
         u64 addr = nextThd->private_mappings[i].start_addr;
         u32 len = nextThd->private_mappings[i].length;
        //  printk("\n$$$$$$$$ LAST ADDR %x $$$$$$$$$\n", nextThd->private_mappings[i].start_addr + len);
         while(addr < nextThd->private_mappings[i].start_addr + len){
          //  printk("$$$$$$$$ %x $$$$$$$$$\n", addr);
            u64 first9 = ((addr & PGD_MASK) >> PGD_SHIFT);
            u64 second9 = ((addr & PUD_MASK)>> PUD_SHIFT);
            u64 third9 = ((addr & PMD_MASK) >> PMD_SHIFT);
            u64 fourth9 = ((addr & PTE_MASK) >> PTE_SHIFT);

            u64 pgd = next->pgd;
            u64 base = osmap(pgd);
            u64 *baseP = base;
            u64 pgd_t = baseP + first9;
            if(*(u64 *)pgd_t == NULL || (*(u64 *)pgd_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pgd_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pgd_t_shift = ((*(u64 *)pgd_t) >> PAGE_SHIFT);
            u64 base1 = pgd_t_shift << PAGE_SHIFT;
            u64 *baseP1 = base1;
            u64 pud_t = baseP1 + second9;
            if(*(u64 *)pud_t == NULL || (*(u64 *)pud_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pud_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pud_t_shift = ((*(u64 *)pud_t) >> PAGE_SHIFT);
            u64 base2 = pud_t_shift << PAGE_SHIFT;
            u64 *baseP2 = base2;
            u64 pmd_t = baseP2 + third9;
            if(*(u64 *)pmd_t == NULL || (*(u64 *)pmd_t) % 2 == 0){
              addr = (addr) + PAGE_SIZE; continue;
              // *(u64 *)pmd_t = osmap(os_pfn_alloc(OS_PT_REG));
            }
            u64 pmd_t_shift = ((*(u64 *)pmd_t) >> PAGE_SHIFT);
            u64 base3 = pmd_t_shift << PAGE_SHIFT;
            u64 *baseP3 = base3;
            u64 pte_t = baseP3 + fourth9;

            if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 == 0){addr = (addr) + PAGE_SIZE; continue;}
            // if(*(u64 *)pte_t == NULL || (*(u64 *)pte_t) % 2 != 1){
            //   *(u64 *)pte_t = os_pfn_alloc(USER_REG);
            //   *(u64 *)pte_t = ((*((u64 *)pte_t)) << PAGE_SHIFT) | 1;
            // }
            *(u64 *)pte_t = (((*((u64 *)pte_t)) >> 3) << 3) | 7;
            // printk(".....next.....\n");
            asm volatile(
              "invlpg (%0)" 
              :
              :"r" (addr) 
              : "memory"
            );
          // printk("$$$$$$$$ %x $$$$$$$$\n", *(u64 *)pte_t);
           addr = (addr) + PAGE_SIZE;
         }
       }
     }
   }
   return 0;	

}

