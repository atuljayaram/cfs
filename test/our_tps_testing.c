#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>


void *thread_testing_before_calling_init(void arg){
  char test_string[256] = {0};

  assert(tps_create() == -1); //this tests calling tps_create before calling tps_init
  assert(tps_destroy() == -1); //this tests calling tps_destroy before calling tps_init
  assert(tps_read(0, 256, test_string) == -1); //this tests calling init_read without calling tps_init
  assert(tps_write(0, 256, test_string) == -1); //this tests calling init_write write without calling tps_init

  fprintf(stdout, "Done testing functions being called before tps_init!\n");

}


void *thread_testing_called_twice(void arg){

  assert(tps_init(1) == 0); //this tests a first call of tps_init
  assert(tps_init(0) == -1); //this tests a second call of tps_init (should fail and return -1)

  assert(tps_create() == 0); //this tests a first call of tps_create
  assert(tps_create() == -1); //this tests a second call of tps_create (should fail and return -1)

  assert(tps_destroy() == 0); // this tests a first call of tps_destroy
  assert(tps_destroy() == -1); // this tests a second call of tps_destroy (should fail and return -1)

  fprintf(stdout, "Done testing functions being called twice!\n");
}

void *thread_testing_valid(void arg){
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

void run_test_cases(){
  pthread_t tid;

  //testing functions being called twice
  pthread_create(&tid, NULL, thread_testing_called_twice, NULL);
  pthread_join(tid, NULL);

  //testing functions being called before tps_init
  pthread_create(&tid, NULL, thread_testing_before_calling_init, NULL);
  pthread_join(tid, NULL);

  //testing functions being used correctly
  pthread_create(&tid, NULL, thread_testing_valid, NULL);
  pthread_join(tid, NULL);
}


int main(int argc, char argv**){
  run_test_cases(); //begin
  return 0;
}
