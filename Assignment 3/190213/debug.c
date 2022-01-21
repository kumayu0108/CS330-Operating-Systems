#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/*
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info));
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}
/*
 * frees a debug_info struct
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}



/*
 * allocates a page to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers));
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/*
 * allocate a node for breakpoint list
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));

	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	child_ctx->dbg = NULL;
	child_ctx->state = WAITING;
}

struct funcStack {
	u64 addr;
	u8 flag;
	struct funcStack *next;
};

struct funcStack *alloc_funcStack()
{
	struct funcStack *info = (struct funcStack *)os_alloc(sizeof(struct funcStack));
	if(info)
		bzero((char *)info, sizeof(struct funcStack));
	return info;
}

void free_funcStack(struct funcStack *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct funcStack));
}

struct funcStack *fnHead;

/******************************************************************************/


/* This is the int 0x3 handler
 * Hit from the childs context
 */

long int3_handler(struct exec_context *ctx)
{	
	//Your code
	// printk("\n************** INT3 *****************\n");
	if(ctx == NULL || ctx->dbg != NULL) return -1;
	
	u32 ppid = ctx->ppid;
	struct exec_context* parent_ctx = get_ctx_by_pid(ppid);
	if(parent_ctx == NULL) return -1;

	u64 current_addr = (ctx->regs.entry_rip)-1;
	struct debug_info* dbg_info = parent_ctx->dbg;
	if(current_addr == (u64)dbg_info->end_handler){
		*((u8*)current_addr) = PUSHRBP_OPCODE;
		u8 b_code = *((u8*)(ctx->regs.entry_rip));
		*((u8*)(ctx->regs.entry_rip)) = INT3_OPCODE;
		
		dbg_info->opCode = b_code;
		ctx->regs.entry_rip -= 1;

		u64* bt = dbg_info->backtrace; 
		dbg_info->bt_overflow=0;
		dbg_info->num_bt=0;

		if(MAX_BACKTRACE >= 1){
			bt[0] = *((u64*)ctx->regs.entry_rsp);
			u64* head = (u64*)ctx->regs.rbp;
			int i=1;
			u64 curr_addr;
			do{
				curr_addr = *(head+1);
				if(i == MAX_BACKTRACE){
					if(curr_addr != END_ADDR) dbg_info->bt_overflow = 1;
					else dbg_info->num_bt = i;
					break;
				}
				if(END_ADDR != curr_addr){
					bt[i] = (curr_addr == (u64)parent_ctx->dbg->end_handler ? *(head + 2) : curr_addr); 
					head = (u64*)(*head);
					i++;
				}
			} while(END_ADDR != curr_addr);
			if(dbg_info->bt_overflow == 0) dbg_info->num_bt = i;
		}
		else {
			dbg_info->bt_overflow=1;
		}

		struct funcStack *curr = fnHead;
		struct funcStack *bla = NULL;
		struct funcStack *prev = NULL, *toRem = NULL;
		while(curr != NULL){if(curr->flag){bla = prev; toRem = curr;} prev = curr; curr = curr->next;}
		bla->next = NULL;
		struct breakpoint_info* tmp = parent_ctx->dbg->head;
		while(tmp != NULL){
			if((tmp->addr) == (toRem->addr)){
				tmp->stackNum--;
				break;
			}
			tmp = tmp->next;
		}
		while(toRem != NULL){
			struct funcStack *abc = toRem;
			toRem = toRem->next;
			free_funcStack(abc);
		}

		dbg_info->RSP = ctx->regs.entry_rsp;

		parent_ctx->state = READY;
		ctx->state = WAITING;
		schedule(parent_ctx);
		return 0;
	}
	else if(current_addr - (u64)1 == (u64)dbg_info->end_handler){
		u8 b_code = dbg_info->opCode;
		u8* target = (u8*)current_addr;
		*target = b_code;
		target -= 1;

		*target = INT3_OPCODE;
		ctx->regs.entry_rip -= 1;

		parent_ctx->state = WAITING;
		ctx->state = READY;
		schedule(ctx);
		return 0;
	}


	struct breakpoint_info* head=NULL;
	head = parent_ctx->dbg->head;
	int found = 0;
	while(head != NULL){
		if((head->addr) == current_addr){
			found = 1;
			break;
		}
		head = head->next;
	}
	
	if(found){
		*((u8*)current_addr) = PUSHRBP_OPCODE;
		u8 b_code = *((u8*)(ctx->regs.entry_rip));
		*((u8*)(ctx->regs.entry_rip)) = INT3_OPCODE;

		parent_ctx->dbg->opCode = b_code;

		ctx->regs.entry_rip -= 1;
		parent_ctx->regs.rax = (u64)current_addr;

		u64* bt = dbg_info->backtrace; 
		dbg_info->bt_overflow=0;
		dbg_info->num_bt=0;

		if(MAX_BACKTRACE >= 2){
			bt[0] = ctx->regs.entry_rip;
			bt[1] = *((u64*)ctx->regs.entry_rsp);
			u64* head = (u64*)ctx->regs.rbp;
			int i=2;
			u64 curr_addr;
			do{
				curr_addr = *(head + 1);
				if(i == MAX_BACKTRACE){
					if(curr_addr != END_ADDR) dbg_info->bt_overflow = 1;
					else dbg_info->num_bt = i;
					break;
				}
				if(END_ADDR != curr_addr){
					bt[i] = (curr_addr == (u64)parent_ctx->dbg->end_handler ? *(head + 2) : curr_addr); 
					head = (u64*)(*head);
					i++;
				}
			} while(END_ADDR != curr_addr);
			if(dbg_info->bt_overflow == 0) dbg_info->num_bt = i;

		}
		else {
			dbg_info->bt_overflow=1;
		}

		head->stackNum++;
		struct funcStack *curr = fnHead;
		while(curr->next != NULL){curr = curr->next;}
		curr->next = alloc_funcStack();
		curr = curr->next;
		curr->addr = current_addr;
		curr->flag = head->end_breakpoint_enable;
		dbg_info->RSP = ctx->regs.entry_rsp;

		if(head->end_breakpoint_enable){
			ctx->regs.entry_rsp = ctx->regs.entry_rsp - sizeof((u64)(parent_ctx->dbg->end_handler));
			*((u64*)(ctx->regs.entry_rsp)) = (u64)(parent_ctx->dbg->end_handler);
		}
		
		parent_ctx->state = READY;
		ctx->state = WAITING;
		schedule(parent_ctx);
		return 0;

	}
	else {
		u8 b_code = parent_ctx->dbg->opCode;
		u8* target = (u8*)current_addr;
		*target = b_code;
		target -= 1;	

		struct breakpoint_info* bp_ptr=dbg_info->head;
		while(bp_ptr != NULL){
			if(bp_ptr->addr == (u64)target){
				*target = INT3_OPCODE;
			}
			bp_ptr = bp_ptr->next;
		}
		ctx->regs.entry_rip -= 1;

		parent_ctx->state = WAITING;
		ctx->state = READY;
		schedule(ctx);
	}	
	return 0;
}

/*
 * Exit handler.
 * Deallocate the debug_info struct if its a debugger.
 * Wake up the debugger if its a child
 */
void debugger_on_exit(struct exec_context *ctx)
{
	// Your code
	if(ctx->dbg == NULL){
		u32 ppid = ctx->ppid;
		struct exec_context* debugger = get_ctx_by_pid(ppid);
		debugger->dbg->childExit = 1;
		debugger->regs.rax = CHILD_EXIT;
		debugger->state = READY;

	}else{
		struct breakpoint_info* bp_ptr = ctx->dbg->head;
		struct breakpoint_info* to_rem;

		while(bp_ptr!=NULL){
			to_rem = bp_ptr;
			bp_ptr = bp_ptr->next;
			free_breakpoint_info(to_rem);	
		}
		free_debug_info(ctx->dbg);
	}
	return;
}


/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx, void *addr)
{
	// Your code
	struct debug_info *info = alloc_debug_info();
	fnHead = alloc_funcStack();
	fnHead->flag = 2;

	if(!info){
		return -1;
	}

	info->breakpoint_count = 0;
	info->head = NULL;
	info->end_handler = addr;
	info->BP_NUM = 1;
	info->childExit = 0;
	info->flag = 0;
	info->opCode = 0;
	ctx->dbg = info;

	u8* target = (u8*) addr;
	*target = INT3_OPCODE;
	
	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr, int flag)
{

	// Your code

	if(ctx->dbg->breakpoint_count == 0){
		struct breakpoint_info *info = alloc_breakpoint_info();
		info->addr = (u64)addr;
		info->end_breakpoint_enable = flag;
		info->next = NULL;
		info->num = ctx->dbg->BP_NUM;
		info->stackNum = 0;
		info->op_code = 0;
		// info->status = 1;
		ctx->dbg->head = info;
	}
	else {
		struct breakpoint_info *curr = ctx->dbg->head;
		while(curr->next != NULL){
			curr = curr->next;
		}
		struct breakpoint_info *tmp = ctx->dbg->head;
		while(tmp != NULL){
			if(tmp->addr == (u64)addr){break;}
			tmp = tmp->next;
		}
		if(tmp != NULL){
			struct funcStack *abc = fnHead;
			while(abc != NULL){
				if(abc->addr == (u64)addr){
					if(abc->flag == 1 && flag == 0)return -1;
				}
				abc = abc->next;
			}
			tmp->end_breakpoint_enable = flag;
			return 0;
		}
		if(ctx->dbg->breakpoint_count >= MAX_BREAKPOINTS){return -1;}


		struct breakpoint_info *info = alloc_breakpoint_info();
		info->addr = (u64)addr;
		info->end_breakpoint_enable = flag;
		info->next = NULL;
		info->num = ctx->dbg->BP_NUM;
		info->stackNum = 0;
		// info->status = 1;
		curr->next = info;
	}
	ctx->dbg->breakpoint_count++;
	ctx->dbg->BP_NUM++;

	u8* target = (u8*) addr;
	*target = INT3_OPCODE;

	return 0;
}

/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	//Your code
	struct breakpoint_info *curr = ctx->dbg->head;
	struct breakpoint_info *prev = NULL;
	while(curr != NULL){
		if(curr->addr == (u64)addr)break;
		prev = curr;
		curr = curr->next;
	}
	if(curr == NULL)return -1;

	struct funcStack *tmp = fnHead;
	while(tmp != NULL){
		if(tmp->addr == (u64)addr){
			if(tmp->flag){return -1;}
		}
		tmp = tmp->next;
	}

	ctx->dbg->breakpoint_count--;
	if(prev == NULL){
		ctx->dbg->head = curr->next;
		free_breakpoint_info(curr);
	}
	else {
		prev->next = curr->next;
		free_breakpoint_info(curr);
	}
	*((u8*)addr) = PUSHRBP_OPCODE;
	return 0;
}


/*
 * called from debuggers context
 */

int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *ubp)
{
	
	// Your code
	if(ctx == NULL || ctx->dbg == NULL) return -1;
	struct debug_info* dbg_info = ctx->dbg;
	struct breakpoint_info* bp_ptr = dbg_info->head;
	int count = 0;
	while(bp_ptr != NULL){
		ubp[count].addr = bp_ptr->addr;
		ubp[count].end_breakpoint_enable = bp_ptr->end_breakpoint_enable;
		ubp[count].num = bp_ptr->num;
		count++;
		bp_ptr = bp_ptr->next;
	}
	return count;
}


/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	// Your code
	if(ctx == NULL || ctx->dbg == NULL) return -1;
	struct exec_context* child_ctx = NULL;
	for(int i = 1; i <= MAX_PROCESSES; i++){
		child_ctx = get_ctx_by_pid(i);
		if(child_ctx != NULL){
			if(child_ctx->ppid == ctx->pid){
				regs->r15 = child_ctx->regs.r15;
				regs->r14 = child_ctx->regs.r14;
				regs->r13 = child_ctx->regs.r13;
				regs->r12 = child_ctx->regs.r12;
				regs->r11 = child_ctx->regs.r11;
				regs->r10 = child_ctx->regs.r10;
				regs->r9 = child_ctx->regs.r9;
				regs->r8 = child_ctx->regs.r8;
				regs->rbp = child_ctx->regs.rbp;
				regs->rdi = child_ctx->regs.rdi;
				regs->rsi = child_ctx->regs.rsi;
				regs->rdx = child_ctx->regs.rdx;
				regs->rcx = child_ctx->regs.rcx;
				regs->rbx = child_ctx->regs.rbx;
				regs->rax = child_ctx->regs.rax;
				regs->entry_rip = child_ctx->regs.entry_rip;
				regs->entry_cs = child_ctx->regs.entry_cs;
				regs->entry_rflags = child_ctx->regs.entry_rflags;
				regs->entry_rsp = ctx->dbg->RSP;
				regs->entry_ss = child_ctx->regs.entry_ss;
				return 0;
			}
		}
	}
	return -1;
}

/*
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{

	// Your code
	if(ctx==NULL || ctx->dbg == NULL) return -1;
	struct exec_context* child_ctx = NULL;
	
	for(int i=1; i <= MAX_PROCESSES; i++){
		child_ctx = get_ctx_by_pid(i);
		if(child_ctx != NULL){
			if(child_ctx->ppid == ctx->pid){
				break;
			}
		}
	}
	if(child_ctx == NULL || ctx->dbg->bt_overflow == 1) return -1;

	u64* bt_arr = (u64*)bt_buf; 
	
	for(int i = 0; i < (ctx->dbg->num_bt); i++){
		bt_arr[i] = ctx->dbg->backtrace[i];
	}

	return ctx->dbg->num_bt;
}

/*
 * When the debugger calls wait
 * it must move to WAITING state
 * and its child must move to READY state
 */

s64 do_wait_and_continue(struct exec_context *ctx)
{
	// Your code
	if(ctx==NULL || ctx->dbg == NULL) return -1;
	if(ctx->dbg->childExit){return CHILD_EXIT;}

	struct exec_context* child_ctx = NULL;
	for(int i=1; i <= MAX_PROCESSES; i++){
		child_ctx = get_ctx_by_pid(i);
		if(child_ctx != NULL){
			if(child_ctx->ppid == ctx->pid){
				struct breakpoint_info *tmp = ctx->dbg->head;
				while(tmp != NULL){
					if(tmp->addr == (u64)(child_ctx->regs.entry_rip)){break;}
					tmp = tmp->next;
				}
				
				ctx->state = WAITING;
				child_ctx->state = READY;
				schedule(child_ctx);
				if(tmp != NULL){return tmp->addr;}
			}
		}
	}
	return CHILD_EXIT;
}







