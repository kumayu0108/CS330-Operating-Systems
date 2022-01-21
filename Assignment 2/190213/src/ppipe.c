#include<ppipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>


// Per process information for the ppipe.
struct ppipe_info_per_process {

    // TODO:: Add members as per your need...
    long long r, w;
    long long pid;
};

// Global information for the ppipe.
struct ppipe_info_global {

    char *ppipe_buff;       // Persistent pipe buffer: DO NOT MODIFY THIS.

    // TODO:: Add members as per your need...
    long long minPtr, maxWritten;
    int fd0, fd1;
};

// Persistent pipe structure.
// NOTE: DO NOT MODIFY THIS STRUCTURE.
struct ppipe_info {

    struct ppipe_info_per_process ppipe_per_proc [MAX_PPIPE_PROC];
    struct ppipe_info_global ppipe_global;

};


// Function to allocate space for the ppipe and initialize its members.
struct ppipe_info* alloc_ppipe_info() {

    // Allocate space for ppipe structure and ppipe buffer.
    struct ppipe_info *ppipe = (struct ppipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);

    // Assign ppipe buffer.
    ppipe->ppipe_global.ppipe_buff = buffer;

    /**
     *  TODO:: Initializing pipe fields
     *
     *  Initialize per process fields for this ppipe.
     *  Initialize global fields for this ppipe.
     *
     */ 

    // Return the ppipe.
    return ppipe;

}

// Function to free ppipe buffer and ppipe info object.
// NOTE: DO NOT MODIFY THIS FUNCTION.
void free_ppipe (struct file *filep) {

    os_page_free(OS_DS_REG, filep->ppipe->ppipe_global.ppipe_buff);
    os_page_free(OS_DS_REG, filep->ppipe);

} 

// Fork handler for ppipe.
int do_ppipe_fork (struct exec_context *child, struct file *filep) {
    
    /**
     *  TODO:: Implementation for fork handler
     *
     *  You may need to update some per process or global info for the ppipe.
     *  This handler will be called twice since ppipe has 2 file objects.
     *  Also consider the limit on no of processes a ppipe can have.
     *  Return 0 on success.
     *  Incase of any error return -EOTHERS.
     *
     */
    
    if(filep->mode == O_READ)child->files[filep->ppipe->ppipe_global.fd0] = filep;
    else if(filep->mode == O_WRITE)child->files[filep->ppipe->ppipe_global.fd1] = filep;

    int i, parent;
    for(parent = 0; parent < MAX_PPIPE_PROC; parent++){
        if(filep->ppipe->ppipe_per_proc[parent].pid == child->ppid){break;}
    }

    for(i = 0; i < MAX_PPIPE_PROC; i++){
        if((filep->ppipe->ppipe_per_proc[i].r == -1 && filep->ppipe->ppipe_per_proc[i].w == -1) || (filep->ppipe->ppipe_per_proc[i].pid == child->pid)){break;}
    }
    if(i == MAX_PPIPE_PROC)return -EOTHERS;


    if(filep->mode == O_READ)filep->ppipe->ppipe_per_proc[i].r = filep->ppipe->ppipe_per_proc[parent].r;
    else if(filep->mode == O_WRITE)filep->ppipe->ppipe_per_proc[i].w = filep->ppipe->ppipe_per_proc[parent].w;
    filep->ppipe->ppipe_per_proc[i].pid = child->pid;
    
    // Return successfully.
    return 0;

}


// Function to close the ppipe ends and free the ppipe when necessary.
long ppipe_close (struct file *filep) {

    /**
     *  TODO:: Implementation of Pipe Close
     *
     *  Close the read or write end of the ppipe depending upon the file
     *      object's mode.
     *  You may need to update some per process or global info for the ppipe.
     *  Use free_pipe() function to free ppipe buffer and ppipe object,
     *      whenever applicable.
     *  After successful close, it return 0.
     *  Incase of any error return -EOTHERS.
     *                                                                          
     */

    int ret_value;
    long long curr_pid = get_current_ctx()->pid;
    int processNum = 0;
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        if(filep->ppipe->ppipe_per_proc[i].pid == curr_pid){processNum = i; break;}
    }
    // if(filep->ref_count == 1){
        // printk("$$$$ %d $$$$\n", processNum);
        if(filep->mode == O_READ){filep->ppipe->ppipe_per_proc[processNum].r = -1;}
        else if(filep->mode == O_WRITE){filep->ppipe->ppipe_per_proc[processNum].w = -1;}

        if(filep->ppipe->ppipe_per_proc[processNum].w == -1 && filep->ppipe->ppipe_per_proc[processNum].r == -1){
            filep->ppipe->ppipe_per_proc[processNum].pid = -1;
        }

    // }
    // else printk("$$$$$ %d $$$$$\n", filep->ref_count);

    int numClosed = 0;
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        if(filep->ppipe->ppipe_per_proc[i].r == -1)numClosed++;
        if(filep->ppipe->ppipe_per_proc[i].w == -1)numClosed++;
    }
    if(numClosed == 2 * MAX_PPIPE_PROC){free_ppipe(filep);}

    // Close the file.
    ret_value = file_close (filep);         // DO NOT MODIFY THIS LINE.

    // And return.
    return ret_value;

}

// Function to perform flush operation on ppipe.
int do_flush_ppipe (struct file *filep) {

    /**
     *  TODO:: Implementation of Flush system call
     *
     *  Reclaim the region of the persistent pipe which has been read by 
     *      all the processes.
     *  Return no of reclaimed bytes.
     *  In case of any error return -EOTHERS.
     *
     */

    int reclaimed_bytes = 0;
    long long minRead = 99999999999;
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        if(filep->ppipe->ppipe_per_proc[i].r == -1)continue;
        if(filep->ppipe->ppipe_per_proc[i].r < minRead)minRead = filep->ppipe->ppipe_per_proc[i].r;
        // printk("####### %d -> %d #######\n", filep->ppipe->ppipe_per_proc[i].r, i);
    }
    // printk("........%d........%d", minRead, filep->ppipe->ppipe_global.minPtr);
    if(minRead == 99999999999){minRead = filep->ppipe->ppipe_global.minPtr;}
    reclaimed_bytes = minRead - filep->ppipe->ppipe_global.minPtr;
    filep->ppipe->ppipe_global.minPtr = minRead;

    // Return reclaimed bytes.
    return reclaimed_bytes;

}

// Read handler for the ppipe.
int ppipe_read (struct file *filep, char *buff, u32 count) {
    
    /**
     *  TODO:: Implementation of PPipe Read
     *
     *  Read the data from ppipe buffer and write to the provided buffer.
     *  If count is greater than the present data size in the ppipe then just read
     *      that much data.
     *  Validate file object's access right.
     *  On successful read, return no of bytes read.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If read end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    if(filep == NULL || filep->ref_count == 0)return -EINVAL;
    if(filep->mode != O_READ || filep->type != PPIPE)return -EACCES;

    int bytes_read = 0;
    long long curr_pid = get_current_ctx()->pid;//, maxWritten = 0;
    int processNum = 0;
    
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        if(filep->ppipe->ppipe_per_proc[i].pid == curr_pid){processNum = i; break;}
        // if(filep->ppipe->ppipe_per_proc[i].w > maxWritten){maxWritten = filep->ppipe->ppipe_per_proc[i].w;}
        // printk("####### %d ---> %d #######\n", filep->ppipe->ppipe_per_proc[i].w, i);
    }
    // if(maxWritten > 2000)printk("######## %d #######\n", maxWritten);

    while(bytes_read < count && (filep->ppipe->ppipe_per_proc[processNum].r) < filep->ppipe->ppipe_global.maxWritten){
        buff[bytes_read] = filep->ppipe->ppipe_global.ppipe_buff[(filep->ppipe->ppipe_per_proc[processNum].r) % MAX_PPIPE_SIZE];
        bytes_read++;
        filep->ppipe->ppipe_per_proc[processNum].r++;
    }

    /////////////////// check +1
    if(filep->ppipe->ppipe_per_proc[processNum].r > filep->ppipe->ppipe_per_proc[processNum].w){
        filep->ppipe->ppipe_per_proc[processNum].w = filep->ppipe->ppipe_per_proc[processNum].r;
    }
    // Return no of bytes read.
    return bytes_read;
	
}

// Write handler for ppipe.
int ppipe_write (struct file *filep, char *buff, u32 count) {

    /**
     *  TODO:: Implementation of PPipe Write
     *
     *  Write the data from the provided buffer to the ppipe buffer.
     *  If count is greater than available space in the ppipe then just write
     *      data that fits in that space.
     *  Validate file object's access right.
     *  On successful write, return no of written bytes.
     *  Incase of Error return valid error code.
     *      -EACCES: In case access is not valid.
     *      -EINVAL: If write end is already closed.
     *      -EOTHERS: For any other errors.
     *
     */
    if(filep == NULL || filep->ref_count == 0)return -EINVAL;
    if(filep->mode != O_WRITE || filep->type != PPIPE)return -EACCES;

    int bytes_written = 0;
    long long curr_pid = get_current_ctx()->pid;
    // long long maxWritten = 0;

    int processNum = 0;
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        // printk("|%d|", filep->ppipe->ppipe_per_proc[i].w);
        if(filep->ppipe->ppipe_per_proc[i].pid == curr_pid){processNum = i; break;}
        // if(filep->ppipe->ppipe_per_proc[i].w > filep->ppipe->ppipe_global.maxWritten){maxWritten = filep->ppipe->ppipe_per_proc[i].w;}
    }

    // printk("\n.......%d......%d\n", maxWritten, filep->ppipe->ppipe_global.minPtr);
    while(bytes_written < count && (filep->ppipe->ppipe_global.maxWritten) - (filep->ppipe->ppipe_global.minPtr) < MAX_PPIPE_SIZE){
        filep->ppipe->ppipe_global.ppipe_buff[(filep->ppipe->ppipe_global.maxWritten) % MAX_PPIPE_SIZE] = buff[bytes_written];
        bytes_written++;
        filep->ppipe->ppipe_global.maxWritten++;
    }
    // filep->ppipe->ppipe_per_proc[processNum].w = maxWritten;
    // if(maxWritten > 2000)printk("######## %d -> %d #######\n", filep->ppipe->ppipe_per_proc[processNum].w, processNum);
    // for(int i = 0; i < MAX_PPIPE_PROC; i++){
    //     printk("####### %d ---> %d #######\n", filep->ppipe->ppipe_per_proc[i].w, i);
    // }

    // Return no of bytes written.
    return bytes_written;

}

// Function to create persistent pipe.
int create_persistent_pipe (struct exec_context *current, int *fd) {

    /**
     *  TODO:: Implementation of PPipe Create
     *
     *  Find two free file descriptors.
     *  Create two file objects for both ends by invoking the alloc_file() function.
     *  Create ppipe_info object by invoking the alloc_ppipe_info() function and
     *      fill per process and global info fields.
     *  Fill the fields for those file objects like type, fops, etc.
     *  Fill the valid file descriptor in *fd param.
     *  On success, return 0.
     *  Incase of Error return valid Error code.
     *      -ENOMEM: If memory is not enough.
     *      -EOTHERS: Some other errors.
     *
     */
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

    struct ppipe_info* pp_inf = alloc_ppipe_info();
    if(pp_inf == NULL)return -ENOMEM;
    
    current->files[fd[0]]->type = PPIPE;
    current->files[fd[1]]->type = PPIPE;
    
    current->files[fd[0]]->ppipe = pp_inf;
    current->files[fd[1]]->ppipe = pp_inf;

    current->files[fd[0]]->mode = O_READ;
    current->files[fd[1]]->mode = O_WRITE;
    
    current->files[fd[0]]->pipe = NULL;
    current->files[fd[1]]->pipe = NULL;

    current->files[fd[0]]->inode = NULL;
    current->files[fd[1]]->inode = NULL;
    
    current->files[fd[0]]->msg_queue = NULL;
    current->files[fd[1]]->msg_queue = NULL;
    
    current->files[fd[0]]->fops->read = ppipe_read;
    current->files[fd[1]]->fops->write = ppipe_write;
    
    current->files[fd[0]]->fops->close = ppipe_close;
    current->files[fd[1]]->fops->close = ppipe_close;
    
    for(int i = 0; i < MAX_PPIPE_PROC; i++){
        pp_inf->ppipe_per_proc[i].r = -1;
        pp_inf->ppipe_per_proc[i].w = -1;
        pp_inf->ppipe_per_proc[i].pid = -1;
    }

    pp_inf->ppipe_per_proc[0].r = 0;
    pp_inf->ppipe_per_proc[0].w = 0;
    pp_inf->ppipe_per_proc[0].pid = current->pid;
    pp_inf->ppipe_global.fd0 = fd[0];
    pp_inf->ppipe_global.fd1 = fd[1];

    pp_inf->ppipe_global.minPtr = 0;
    pp_inf->ppipe_global.maxWritten = 0;
    // Simple return.
    return 0;

}

