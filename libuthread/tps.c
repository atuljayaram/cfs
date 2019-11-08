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
  void* address;
  int ref_counter;
} page;

typedef struct tps {
  page* tps_page;
  pthread_t TID;
} tps;

queue_t tps_queue = NULL;

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


 static int find_tps(void* data, void* argv) {
   pthread_t tid = *((pthread_t*)argv);
   //return 1 if tps found
   if(((tps*)data)->tid == tid) {
     return 1;
   }
   else{
     return 0;
   }
 }


 static int find_address(void* data, void* argv)
 {
   void* addr = argv;
   //return 1 if address found
   if(((tps*)data)->tps_page->addr == addr) {
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



int tps_init(int segv)
{
  //return -1 if TPS API has already been initiliazied
  if(tps_queue != NULL){
    return -1;
  }

  tps_queue_create = queue_create();
  if(tps_queue_create == NULL){
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
  tps* current_tps = NULL;

  //if current thread already has a TPS (tps already exists), return -1
  if(current_tps != NULL){
    return -1;
  }

  //TODO
  //Create a TPS area an associate it to the current thread


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
  tps* current_tps = NULL;

  //if current thread doesn't have a TPS, return -1
  if(current_tps == NULL){
    return -1;
  }

  //TODO
  //Destroy the TPS area associated to the current thread

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
  tps* current_tps = NULL;


  //if current thread doesn't have a TPS, return -1
  if(current_tps == NULL){
    return -1;
  }


  //if the reading operation is out of bound, return -1
  if(offset + length > TPS_SIZE){
    return -1;
  }

  //if @buffer is NULL
  if(buffer == NULL){
    return -1;
  }

  //TODO
  //Read @length bytes of data from the current thread's TPS at byte offset
  //@offset into data buffer @buffer.


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

}
