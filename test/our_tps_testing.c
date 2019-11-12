#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>

static sem_t sem1;
static sem_t sem2;


void *thread5(void arg){
  tps_create();
  sem_up(sem1);
  sem_down(sem2);
}

void *thread4(void arg){
  pthread_t tid;

  pthread_create(&tid, NULL, thread5, NULL);

  assert(tps_clone(tid) == -1); //this tests if you clone a tps that does not exit (should fail and return -1)
  tps_create();
  sem_down(sem1);

  assert(tps_clone(tid) == -1); // this tests if you clone and we already have a tps (should fail and return -1)
  sem_up(sem2);

  fprintf(stdout, "Done testing tps_clone function!\n");
}


void *thread3(void arg){
  char test_string[256] = {0};

  assert(tps_create() == -1); //this tests calling tps_create before calling tps_init
  assert(tps_destroy() == -1); //this tests calling tps_destroy before calling tps_init
  assert(tps_read(0, 256, test_string) == -1); //this tests calling init_read without calling tps_init
  assert(tps_write(0, 256, test_string) == -1); //this tests calling init_write write without calling tps_init

  fprintf(stdout, "Done testing functions being called before tps_init!\n");

}


void *thread2(void arg){

  assert(tps_init(1) == 0); //this tests a first call of tps_init
  assert(tps_init(0) == -1); //this tests a second call of tps_init (should fail and return -1)

  assert(tps_create() == 0); //this tests a first call of tps_create
  assert(tps_create() == -1); //this tests a second call of tps_create (should fail and return -1)

  assert(tps_destroy() == 0); // this tests a first call of tps_destroy
  assert(tps_destroy() == -1); // this tests a second call of tps_destroy (should fail and return -1)

  fprintf(stdout, "Done testing functions being called twice!\n");
}

void *thread1(void arg){
  char test_string[256] = {0};

  assert(tps_init(1) == 0);
  assert(tps_create() == 0);
  assert(tps_destroy() == 0);

  assert(tps_create() == 0);

  assert(tps_write(0, 256, test_string) == 0);
  test_string[0] = 1;
  assert(tps_write(1, 256, test_string) == 0);
  assert(tps_read(0, 256, test_string) == 0);
  assert(test_string[0] == 0);
  assert(test_string[1] == 1);

  fprintf(stderr, "Done testing functions being valid!\n");
}


void *thread_testing_called_twice(void arg){
  pthread_t tid;

  //testing functions being called twice
  pthread_create(&tid, NULL, thread2, NULL);
  pthread_join(tid, NULL);
}


void *thread_testing_before_calling_init(void arg){
  pthread_t tid;

  //testing functions being called before tps_init
  pthread_create(&tid, NULL, thread3, NULL);
  pthread_join(tid, NULL);
}


void *thread_testing_clone(void arg){
  pthread_t tid;

  sem1 = sem_create(0);
  sem2 = sem_create(0);

  //testing clone
  pthread_create(&tid, NULL, thread4, NULL);
  pthread_join(tid, NULL);

  sem_destroy(sem1);
  sem_destroy(sem2);
}



void *thread_testing_valid(void arg){
  pthread_t tid;

  //testing functions being used correctly
  pthread_create(&tid, NULL, thread1, NULL);
  pthread_join(tid, NULL);
}


int main(int argc, char argv**){

  thread_testing_called_twice();
  thread_testing_before_calling_init();
  thread_testing_clone();
  thread_testing_valid();

  return 0;
}
