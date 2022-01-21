#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>

// #include "user/ulib.h"


// Per process info for the pipe.
struct pipe_info_per_process {

    // TODO:: Add members as per your need...
    int isOpenR, isOpenW;
    long long pid;
};

// Global information for the pipe.
struct pipe_info_global {

    char *pipe_buff;    // Pipe buffer: DO NOT MODIFY THIS.
    // TODO:: Add members as per your need...
    // char buf[MAX_PIPE_SIZE];
    long long readP, writeP;
    // u64 ref0, ref1;
    long long fd0, fd1;
};

// Pipe information structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct pipe_info {

    struct pipe_info_per_process pipe_per_proc [MAX_PIPE_PROC];
    struct pipe_info_global pipe_global;

};


// Function to allocate space for the pipe and initialize its members.
struct pipe_info* alloc_pipe_info () {
	
    // Allocate space for pipe structure and pipe buffer.
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign pipe buffer.
    pipe->pipe_global.pipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *  
     *  Initialize per process fields for this pipe.
     *  Initialize global fields for this pipe.
     *
     */
    
    // char arr[MAX_PIPE_SIZE];
    // memset(arr, 0, sizeof(arr));
    // strncat(pipe->pipe_global.pipe_buff, arr, MAX_PIPE_SIZE);

    // Return the pipe.
    return pipe;

}

// Function to free pipe buffer and pipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_pipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->pipe->pipe_global.pipe_buff);
    os_page_free(OS_DS_REG, filep->pipe);

}

// Fork handler for the pipe.
int do_pipe_fork (struct exec_context *child, struct file *filep) {

    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the pipe.
     *  This handler will be called twice since pipe has 2 file objects.
     *  Also consider the limit on no of processes a pipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    if(filep->mode == O_READ)child->files[filep->pipe->pipe_global.fd0] = filep;
    else if(filep->mode == O_WRITE)child->files[filep->pipe->pipe_global.fd1] = filep;
    
    int i, parent;
    for(parent = 0; parent < MAX_PIPE_PROC; parent++){
        if(filep->pipe->pipe_per_proc[parent].pid == child->ppid){break;}
    }

    for(i = 0; i < MAX_PIPE_PROC; i++){
        if((filep->pipe->pipe_per_proc[i].isOpenW == 0 && filep->pipe->pipe_per_proc[i].isOpenR == 0) || (filep->pipe->pipe_per_proc[i].pid == child->pid)){break;}
    }
    if(i == MAX_PIPE_PROC){return -EOTHERS;}

    if(filep->mode == O_READ)filep->pipe->pipe_per_proc[i].isOpenR = 1;
    else if(filep->mode == O_WRITE)filep->pipe->pipe_per_proc[i].isOpenW = 1;
    filep->pipe->pipe_per_proc[i].pid = child->pid;

    // Return successfully.
    return 0;

}

// Function to close the pipe ends and free the pipe when necessary.
long pipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the pipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the pipe.
     *  Use free_pipe() function to free pipe buffer and pipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *
     */
    long long curr_pid = get_current_ctx()->pid;
    int processNum = 0;
    for(int i = 0; i < MAX_PIPE_PROC; i++){
        if(filep->pipe->pipe_per_proc[i].pid == curr_pid){processNum = i; break;}
    }

    // if(filep->ref_count == 1){
        if(filep->mode == O_READ){filep->pipe->pipe_per_proc[processNum].isOpenR = 0;}
        else if(filep->mode == O_WRITE){filep->pipe->pipe_per_proc[processNum].isOpenW = 0;}

        if(filep->pipe->pipe_per_proc[processNum].isOpenR == 0 && filep->pipe->pipe_per_proc[processNum].isOpenW == 0){
            filep->pipe->pipe_per_proc[processNum].pid = -1;
        }
    // }

    int numClosed = 0;
    for(int i = 0; i < MAX_PIPE_PROC; i++){
        if(filep->pipe->pipe_per_proc[i].isOpenR == 0)numClosed++;
        if(filep->pipe->pipe_per_proc[i].isOpenW == 0)numClosed++;
    }
    if(numClosed == 2 * MAX_PIPE_PROC){free_pipe(filep);}
    
    int ret_value;


    // Close the file and return.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Check whether passed buffer is valid memory location for read or write.
int is_valid_mem_range (unsigned long buff, u32 count, int access_bit) {

    /**
     *  TODO:: Implementation for buffer memory range checking
     *
     *  Check whether passed memory range is suitable for read or write.
     *  If access_bit == 1, then it is asking to check read permission.
     *  If access_bit == 2, then it is asking to check write permission.
     *  If range is valid then return 1.
     *  Incase range is not valid or have some permission issue return -EBADMEM.
     *
     */

    int ret_value = -EBADMEM;
    struct exec_context* curr = get_current_ctx();

    for(int i = 0; i < MAX_MM_SEGS; i++){

        unsigned long end = (curr->mms[i].end == STACK_START ? curr->mms[i].end : curr->mms[i].next_free);
        int af = curr->mms[i].access_flags;
        // printk("\n%d......%d......%d....%x....%x....%x\n", i, af, access_bit, curr->mms[i].start, buff, end);
        int rb = af%2, wb = (af>>1)%2;
        if(curr->mms[i].start <= buff && buff + count <= end && ((access_bit == 1 && rb == 1) || (access_bit == 2 && wb == 1))){
            return 1;
        }
    }

    struct vm_area* vArea = curr->vm_area;
    while(vArea != NULL){
        int af = vArea->access_flags;
        int rb = af%2, wb = (af>>1)%2;
        if(vArea->vm_start <= buff && buff + count <= vArea->vm_end && ((access_bit == 1 && rb == 1) || (access_bit == 2 && wb == 1))){
            return 1;
        }
        vArea = vArea->vm_next;
    }

    // Return the finding.
    //printk("\n.............\n");

    return ret_value;

}

// Function to read given no of bytes from the pipe.
int pipe_read (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Read
     *
     *  Read the data from pipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the pipe then just read
     *       that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If read end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */
    if(is_valid_mem_range((unsigned long)buff, count, 2) != 1)return -EACCES;
    if(filep == NULL || filep->ref_count == 0)return -EINVAL;
    if(filep->mode != O_READ || filep->type != PIPE)return -EACCES;
    int bytes_read = 0;

    while(bytes_read < count && (filep->pipe->pipe_global.readP) < (filep->pipe->pipe_global.writeP)){
        buff[bytes_read] = filep->pipe->pipe_global.pipe_buff[(filep->pipe->pipe_global.readP) % MAX_PIPE_SIZE];
        bytes_read++;
        filep->pipe->pipe_global.readP++;
    }
    // Return no of bytes read.
    return bytes_read;

}

// Function to write given no of bytes to the pipe.
int pipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of Pipe Write
     *
     *  Write the data from the provided buffer to the pipe buffer.
     *  If count is greater than available space in the pipe then just write data
     *       that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *       -EACCES: In case access is not valid.
     *       -EINVAL: If write end is already closed.
     *       -EOTHERS: For any other errors.
     *
     */
    // printk("\n...{%x}...\n", buff);
    if(is_valid_mem_range((unsigned long)buff, count, 1) != 1)return -EACCES;
    if(filep == NULL || filep->ref_count == 0)return -EINVAL;
    if(filep->mode != O_WRITE || filep->type != PIPE)return -EACCES;

    int bytes_written = 0;
    while(bytes_written < count && (filep->pipe->pipe_global.writeP) - (filep->pipe->pipe_global.readP) < MAX_PIPE_SIZE){
        filep->pipe->pipe_global.pipe_buff[(filep->pipe->pipe_global.writeP) % MAX_PIPE_SIZE] = buff[bytes_written];
        bytes_written++;
        filep->pipe->pipe_global.writeP++;
    }
    // Return no of bytes written.
    return bytes_written;

}

// Function to create pipe.
int create_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of Pipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function. 
     *  Create pipe_info object by invoking the alloc_pipe_info() function and
     *       fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *       -ENOMEM: If memory is not enough.
     *       -EOTHERS: Some other errors.
     *
     */
    // printk("ayush\n");
    // REF_COUNT will tell if file descritor is used or not. REF_COUNT = 0 means unused.
    // fd_dup(current, 0);
    // struct file* file1 = alloc_file();
    
    int flag = 0;
    for(int i = 0; i < MAX_OPEN_FILES; i++){
        if(current->files[i] == NULL && flag == 0){fd[0] = i; flag++;}
        else if(current->files[i] == NULL && flag == 1){fd[1] = i; flag++;}
    }
    // printk("%d\n", fd[0]);   // printk("%d\n", fd[1]);

    current->files[fd[0]] = alloc_file();
    if(current->files[fd[0]] == NULL)return -ENOMEM;
    current->files[fd[1]] = alloc_file();
    if(current->files[fd[1]] == NULL)return -ENOMEM;

    struct pipe_info* p_inf = alloc_pipe_info();
    if(p_inf == NULL)return -ENOMEM;
    
    current->files[fd[0]]->type = PIPE;
    current->files[fd[1]]->type = PIPE;
    
    current->files[fd[0]]->pipe = p_inf;
    current->files[fd[1]]->pipe = p_inf;

    current->files[fd[0]]->mode = O_READ;
    current->files[fd[1]]->mode = O_WRITE;
    
    current->files[fd[0]]->ppipe = NULL;
    current->files[fd[1]]->ppipe = NULL;

    current->files[fd[0]]->inode = NULL;
    current->files[fd[1]]->inode = NULL;
    
    current->files[fd[0]]->msg_queue = NULL;
    current->files[fd[1]]->msg_queue = NULL;
    
    current->files[fd[0]]->fops->read = pipe_read;
    current->files[fd[1]]->fops->write = pipe_write;
    
    current->files[fd[0]]->fops->close = pipe_close;
    current->files[fd[1]]->fops->close = pipe_close;
    
    p_inf->pipe_global.readP = 0;
    p_inf->pipe_global.writeP = 0;

    for(int i = 0; i < MAX_PIPE_PROC; i++){
        p_inf->pipe_per_proc[i].isOpenR = 0;
        p_inf->pipe_per_proc[i].isOpenW = 0;
        p_inf->pipe_per_proc[i].pid = -1;
    }
    p_inf->pipe_per_proc[0].isOpenR = 1;
    p_inf->pipe_per_proc[0].isOpenW = 1;
    p_inf->pipe_per_proc[0].pid = current->pid;

    // p_inf->pipe_global.ref0 = 1;
    // p_inf->pipe_global.ref1 = 1;

    p_inf->pipe_global.fd0 = fd[0];
    p_inf->pipe_global.fd1 = fd[1];
    
    // Simple return.
    return 0;

}

