# ECS 150 Project 3
This is our report of the ECS 150 Project 3 about implementing a User Level Thread Library

## Phase 1 Semaphore API
We made a struct:

```
struct semaphore {
	int count;
	struct queue* wthreads;
};
```
that contains a counter and "wthreads", which is the threads waiting to be executed/blocked threads.

## Phase 2 TPS API
This is where the bulk of our work was. We first made two structs:

```
typedef struct page {
  void* address; //address in memory where we have our TPS
  int ref_counter; //num of TPS structs that points to page
} page;

typedef struct tps {
  page* tps_page; //page the TPS is pointing to
  pthread_t TID; //this is the TID of the thread using TPS
} tps;

```
In our "page" struct, we have the:
1. address, which is our address in memory where we have our TPS
2. ref_counter, which is the number of TPS structs that points to page

In our "tps" struct, we have the:
1. tps_page, which is the page the TPS is pointing to
2. TID, which is the TID of the thread using TPS

Something notable about our implementation of our tps.c is our calls to enter_critical_section() and exit_critical_section(). Most times, when we enter the critical section, we immediately exit out of it. We did this to ensure DOUBLE protection and make sure we're not doing anything we're not supposed to while we're in or out of the critical section.

### tps_init
In our tps_init, we check if the TPS API has already been initialized and return -1 if we do. If segv is anything other than 0, we install a page fault handler that is able to recognize TPS protection errors. We did this buy using the provided code and functions in the prompt. We made two extra functions to help out with this, find_tps which finds the tps, and find_address which finds the address (self explanatory). We call these two functions when we use queue_iterate to iterate through all the TPS areas and find if p_fault matches one of them.

### tps_create
In tps_create, we of course check if the current thread already has a TPS or in case of failure and return -1. Note how we enter the critical section, find the TPS block, then immediately exit out of the critical section. We then create a TPS area and associate it to the current thread by using:

```
mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
```

### tps_destroy
In our tps_destroy, we of course check if the current thread doesn't have a TPS and return -1. We enter the critical section, find tps block, then immediately exit out of the critical section. We actually use the opposite of mmap to destroy the TPS area associated to the current thread, munmap! We then check if munmap did it's job and return -1 if it fails.

### tps_read
In our tps_read, we initialize integer variables "set_read_on" and "set_read_off." We use these to flag for success of turning read protections on or off + memcpy. We of course check the error where length is greater than the TPS_SIZE, or in other words, when we try to read more than what is in the page, and return -1. Note how we enter the critical section, iterate, then immediately exit the critical section. Check if the current thread doesn't have a TPS and return -1 if it does. We use our "set_read_on" which allows region of memeory to be read from. Then we use memcpy to copy the contents of memoery from page to the provided buffer. We then use "set_read_off" to remove the read permissions from page memory. Finally, if either permissions fail, return -1.

### tps_write
In our tps_write


### tps_clone
We had a bit of trouble implementing this one, but we finally got it. Notice how we DO NOT immediately exit out of the critical section when we enter it. We felt like that it could confuse our implementation a little so we decided not to in this function. In our tps_clone, we set up two variables:
```
struct tps* current_tps = NULL;
struct tps* clone_me_tps = NULL;
```

Self explanatory. We check if the thread doesn't have a TPS or if the current thread already has a TPS, and return -1 if either of them are true (note how we exit the critical section when there's errors).

We allocate current_tps and start the cloning process, making sure to update everything when we clone (tps_page, reference counter, etc). If there's a failure when we clone, then we return -1. Make sure to exit out of the critical section.

## Testing
We were able to pass the professor's provided test case (yay)! We wrote a test case file called "our_tps_testing.c" which comprehensively tests our tps.c file. Here, we check:
1. Functions are being called twice
2. Functions are called before we initailize tps
3. Functions running correctly
4. Our tps_clone
5. etc.
