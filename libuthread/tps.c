#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

typedef struct page {
  void* address; //address in memory where we have our TPS
  int ref_counter; //num of TPS structs that points to page
} page;

typedef struct tps {
  page* tps_page; //page the TPS is pointing to
  pthread_t TID; //this is the TID of the thread using TPS
} tps;

queue_t tps_queue = NULL;

int thread_init = 0;


 //helper function to find tps (called in segv_handler)
 static int find_tps(void* data, void* argv){
   pthread_t tid = *((pthread_t*)argv);
   //return 1 if tps found
   if(((tps*)data)->TID == tid) {
     return 1;
   }
   else{
     return 0;
   }
 }

 //helper function to find address (called in segv_handler)
 static int find_address(void* data, void* argv)
 {
   void* address = argv;
   //return 1 if address found
   if(((tps*)data)->tps_page->address == address) {
     return 1;
   }
   else{
     return 0;
   }
 }


 //function provided in prompt
 static void segv_handler(int sig, siginfo_t *si, void *context)
 {
   tps* current_tps = NULL;
   /*
    * Get the address corresponding to the beginning of the page where the
    * fault occurred
    */
   void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

   //Iterate through all the TPS areas and find if p_fault matches one of them
   queue_iterate(tps_queue, find_address, p_fault, (void**)&current_tps);
   //if there is a match
   if (current_tps != NULL)
   /* Printf the following error message */
       fprintf(stderr, "TPS protection error!\n");

   /* In any case, restore the default signal handlers */
   signal(SIGSEGV, SIG_DFL);
   signal(SIGBUS, SIG_DFL);
   /* And transmit the signal again in order to cause the program to crash */
   raise(sig);
 }



 /*
  * tps_init - Initialize TPS
  * @segv - Activate segfault handler
  *
  * Initialize TPS API. This function should only be called once by the client
  * application. If @segv is different than 0, the TPS API should install a
  * page fault handler that is able to recognize TPS protection errors and
  * display the message "TPS protection error!\n" on stderr.
  *
  * Return: -1 if TPS API has already been initialized, or in case of failure
  * during the initialization. 0 if the TPS API was successfully initialized.
  */

int tps_init(int segv)
{

  thread_init = 1;

  //return -1 if TPS API has already been initiliazied
  if(tps_queue != NULL){
    return -1;
  }

  tps_queue = queue_create();
  if(tps_queue == NULL){
    return -1;
  }


  //if segv is different than 0
  if(segv != 0){
    struct sigaction sa;
    //the TPS API should install a
    // page fault handler that is able to recognize TPS
    // protection errors and
    // display the message "TPS protection error!\n" on stderr.
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler; //use function segv_handler from prompt
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
  }
  return 0;
}






/*
 * tps_create - Create TPS
 *
 * Create a TPS area and associate it to the current thread.
 *
 * Return: -1 if current thread already has a TPS, or in case of failure during
 * the creation (e.g. memory allocation). 0 if the TPS area was successfully
 * created.
 */
int tps_create(void)
{
  if(thread_init == 0){
    return -1;
  }
  pthread_t current_tid = pthread_self(); //identify client threaeds by getting their Thread ID with pthread_self()
  struct tps* current_tps = NULL;

  enter_critical_section(); //enter critical section (duh)
  queue_iterate(tps_queue, find_tps, (void*)&current_tid, (void**)&current_tps); // find tps block
  exit_critical_section(); //exit the critical section (double duh)

  //if current thread already has a TPS (tps already exists), return -1
  if(current_tps != NULL){
    return -1;
  }


  struct tps * node = (struct tps *)malloc(sizeof(struct tps));

  if(!node){
    return -1;
  }

  if(node){
    //this is the tps block
    current_tps = malloc(sizeof(tps));

    if(!current_tps){
      return -1;
    }

    //this is our created page
    //The page of memory associated to a TPS should be allocated using the C library function mmap().
    current_tps->tps_page = malloc(sizeof(page));
    if(!current_tps){
      return -1;
    }
    current_tps->tps_page->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if(current_tps->tps_page->address == MAP_FAILED){
      return -1;
    }

    current_tps->TID = current_tid; //update tid
    current_tps->tps_page->ref_counter = 1; //reference counter to 1

    enter_critical_section(); //enter critical section (duh)
    queue_enqueue(tps_queue, (void*)current_tps); //add to queue
    exit_critical_section(); //exit the critical section (double duh)
  }

  return 0;
}



/*
 * tps_destroy - Destroy TPS
 *
 * Destroy the TPS area associated to the current thread.
 *
 * Return: -1 if current thread doesn't have a TPS. 0 if the TPS area was
 * successfully destroyed.
 */
int tps_destroy(void)
{
  if(thread_init == 0){
    return -1;
  }

  int destroy_success;

  pthread_t current_tid = pthread_self(); //identify client threads by getting their Thread ID with pthread_self()
  struct tps* current_tps = NULL;

  enter_critical_section(); //enter critical section (duh)
  queue_iterate(tps_queue, find_tps, (void*)&current_tid, (void**)&current_tps); // find tps block
  exit_critical_section(); //exit the critical section (double duh)

  //if current thread doesn't have a TPS, return -1
  if(current_tps == NULL){
    return -1;
  }


  enter_critical_section();
  queue_delete(tps_queue, (void*)current_tps);
  exit_critical_section();

  if(current_tps->tps_page->ref_counter > 1){
    (current_tps->tps_page->ref_counter)--;
  }
  else if(current_tps->tps_page->ref_counter == 1){
    enter_critical_section(); //enter critical section (duh)
    destroy_success = munmap(current_tps->tps_page->address,TPS_SIZE); //here. we use munmap to destroy the TPS
    exit_critical_section(); //exit the critical section (double duh)

    free(current_tps->tps_page);
  }
  free(current_tps);

  //if error with destroying TPS
  if(destroy_success == -1){
    return -1; //return -1
  }

  return 0;
}




/*
 * tps_read - Read from TPS
 * @offset: Offset where to read from in the TPS
 * @length: Length of the data to read
 * @buffer: Data buffer receiving the read data
 *
 * Read @length bytes of data from the current thread's TPS at byte offset
 * @offset into data buffer @buffer.
 *
 * Return: -1 if current thread doesn't have a TPS, or if the reading operation
 * is out of bound, or if @buffer is NULL, or in case of internal failure. 0 if
 * the TPS was successfully read from.
 */
int tps_read(size_t offset, size_t length, char *buffer)
{

  if(thread_init == 0){
    return -1;
  }

  int set_read_on,set_read_off; // Flags for success of turning read protections on or off + memcpy

  if (length > TPS_SIZE) // Can't read more than what is in page
  {
    return -1;
  }


  pthread_t current_tid = pthread_self(); //identify client threaeds by getting their Thread ID with pthread_self()
  tps* current_tps = NULL;

  enter_critical_section(); //enter critical section (duh)
  queue_iterate(tps_queue, find_tps, (void*)&current_tid, (void**)&current_tps); // find tps block
  exit_critical_section(); //exit the critical section (double duh)


  //if current thread doesn't have a TPS, return -1
  if(current_tps == NULL){
    return -1;
  }

  set_read_on = mprotect(current_tps->tps_page->address,length,PROT_READ); // Allow region of memory to be read from
  memcpy(buffer,current_tps->tps_page->address+offset,length); // Copy contents of memory from page to provided buffer
  set_read_off = mprotect(current_tps->tps_page->address,length,PROT_NONE); // Remove read permissions from page memory

  if(set_read_on == -1 || set_read_off == -1){
    // If either permission sets fail...
    return -1;
  }

  return 0;
}





/*
 * tps_write - Write to TPS
 * @offset: Offset where to write to in the TPS
 * @length: Length of the data to write
 * @buffer: Data buffer holding the data to be written
 *
 * Write @length bytes located in data buffer @buffer into the current thread's
 * TPS at byte offset @offset.
 *
 * If the current thread's TPS shares a memory page with another thread's TPS,
 * this should trigger a copy-on-write operation before the actual write occurs.
 *
 * Return: -1 if current thread doesn't have a TPS, or if the writing operation
 * is out of bound, or if @buffer is NULL, or in case of failure. 0 if the TPS
 * was successfully written to.
 */
int tps_write(size_t offset, size_t length, char *buffer)
{
  if(thread_init == 0){
    return -1;
  }

  int set_write_on,set_write_off,turn_read_on,turn_read_off; // Flags for success of turning read/write protections on or off + memcpy

  if (length > TPS_SIZE) // Can't read more than what is in page
  {
    return -1;
  }

  pthread_t current_tid = pthread_self(); //identify client threaeds by getting their Thread ID with pthread_self()
  tps* current_tps = NULL;

  page * other_page;

  enter_critical_section(); //enter critical section (duh)
  queue_iterate(tps_queue, find_tps, (void*)&current_tid, (void**)&current_tps); // find tps block
  exit_critical_section(); //exit the critical section (double duh)


  //if current thread doesn't have a TPS, return -1
  if(current_tps == NULL)
    return -1;

  if(current_tps->tps_page->ref_counter > 1)
  {
        current_tps->tps_page->ref_counter--;
        other_page = current_tps->tps_page->address;
        struct page * p = (struct page *)malloc(sizeof(struct page));

        if (!p)
            return -1;

        current_tps->tps_page = p;
        current_tps->tps_page->address = (char *) mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANON, -1, 0);
        current_tps->tps_page->ref_counter += 1;

        turn_read_on = mprotect(other_page,length,PROT_READ);
        set_write_on = mprotect(current_tps->tps_page->address,length,PROT_WRITE);
        memcpy(current_tps->tps_page->address,other_page,TPS_SIZE);
        turn_read_off = mprotect(other_page,length,PROT_NONE);
        set_write_off = mprotect(current_tps->tps_page->address,length,PROT_NONE);

        if ( turn_read_on==-1 || set_write_on==-1 || turn_read_off==-1 || set_write_off==-1)
            return -1;
  }
  set_write_on = mprotect(current_tps->tps_page->address,length,PROT_WRITE); // Allow region of memory to be written to
  memcpy(current_tps->tps_page->address+offset,buffer,length); // Write to memory page from buffer
  set_write_off = mprotect(current_tps->tps_page->address,length,PROT_NONE); // Remove write permissions from page memory

  if(set_write_on == -1 || set_write_off == -1) // If either permission sets fail...
    return -1;

  return 0;
}




/*
 * tps_clone - Clone TPS
 * @tid: TID of the thread to clone
 *
 * Clone thread @tid's TPS. In the first phase, the cloned TPS's content should
 * copied directly. In the last phase, the new TPS should not copy the cloned
 * TPS's content but should refer to the same memory page.
 *
 * Return: -1 if thread @tid doesn't have a TPS, or if current thread already
 * has a TPS, or in case of failure. 0 is TPS was successfully cloned.
 */
int tps_clone(pthread_t tid)
{
  if(thread_init == 0){
    return -1;
  }
  struct tps* current_tps = NULL;
  struct tps* clone_me_tps = NULL; //clone this one
  pthread_t current_tid = pthread_self();  //identify client threaeds by getting their Thread ID with pthread_self()

  enter_critical_section(); //enter critical section (duh)

  queue_iterate(tps_queue, find_tps, (void*)&tid, (void**)&clone_me_tps);
  queue_iterate(tps_queue, find_tps, (void*)&current_tid, (void**)&current_tps);

  //thread doesn't have a TPS
  //current thread already has a TPS
  if(clone_me_tps == NULL || current_tps != NULL){
    exit_critical_section(); //exit critical section (duh)
    return -1;
  }

  struct tps * newNode = (struct tps*)malloc(sizeof(struct tps));

  //not failure
  if(newNode){
    current_tps = malloc(sizeof(tps));

    current_tps->tps_page = clone_me_tps->tps_page;
    (current_tps->tps_page->ref_counter)++; //update reference counter

    current_tps->TID = current_tid;

    queue_enqueue(tps_queue, (void*)current_tps);

    exit_critical_section(); //exit critical section (duh)
  }
  //failure!
  else if(!newNode){
    exit_critical_section(); //exit critical section (duh)
    return -1;
  }
  return 0;
}
