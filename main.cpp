#include <iostream>
#include <queue>
#include <vector>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
using namespace std;

typedef struct thread_args_t
{
  pthread_cond_t *cond_ptr;
  pthread_mutex_t *mutex_ptr;
  pthread_barrier_t *barrier_ptr;
  bool ready;
  std::queue<int> *queue_ptr;
  std::vector<int> *vector_ptr;

} thread_args_t;

void save_pid_to_file()
{
  pid_t pid = getpid();
  FILE *fp = fopen("/home/box/main.pid", "w");
  if(fp != NULL)
  {
    fprintf(fp, "%d", (int) pid);
    fclose(fp);
  }
}

void *producer_thread(void *arg)
{
  thread_args_t *cp = (thread_args_t *) arg;

  for(int i = 0; i < 10; i++)
  {
    usleep(1000000);
    pthread_mutex_lock(cp->mutex_ptr);
    cp->queue_ptr->push(i);
    cout << "push " << i << endl;
    cp->ready = true;
    pthread_mutex_unlock(cp->mutex_ptr);
    pthread_cond_signal(cp->cond_ptr);
  }
  cout << "consumer thread at barrier" << endl;
  pthread_barrier_wait(cp->barrier_ptr);
  return NULL;
}

void *consumer_thread(void *arg)
{
  thread_args_t *cp = (thread_args_t *) arg;

  for(;;)
  {
    pthread_mutex_lock(cp->mutex_ptr);

    while(!cp->ready)
      pthread_cond_wait(cp->cond_ptr, cp->mutex_ptr);
    int val = cp->queue_ptr->front();
    cp->queue_ptr->pop();
    cp->vector_ptr->push_back(val);
    cp->ready = false;
    cout << "pop " << val << endl;
    pthread_mutex_unlock(cp->mutex_ptr);
    if(val == 9)
      break;
  }
  cout << "producer thread at barrier" << endl;
  pthread_barrier_wait(cp->barrier_ptr);
  return NULL;
}

void *barrier_thread(void *arg)
{
  thread_args_t *cp = (thread_args_t *) arg;
  cout << "barrier thread at barrier" << endl;
  pthread_barrier_wait(cp->barrier_ptr);

  cout << "vec = ";
  for(auto it : *(cp->vector_ptr))
  {
    int i = it;
    cout << i << " ";
  }
  cout << endl;

  return NULL;
}

int main(int argc, char *argv[])
{
  save_pid_to_file();
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_barrier_t barrier;
  pthread_barrier_init(&barrier, NULL, 3);

  std::queue<int> que;
  std::vector<int> vec;

  thread_args_t thread_par;
  thread_par.barrier_ptr = &barrier;
  thread_par.cond_ptr = &cond;
  thread_par.mutex_ptr = &mutex;
  thread_par.ready = false;
  thread_par.queue_ptr = &que;
  thread_par.vector_ptr = &vec;

  pthread_t prod_id;
  pthread_t cons_id;
  pthread_t barr_id;
  pthread_create(&prod_id, NULL, producer_thread, &thread_par);
  pthread_create(&cons_id, NULL, consumer_thread, &thread_par);
  pthread_create(&barr_id, NULL, barrier_thread, &thread_par);

  pthread_join(prod_id, NULL);
  pthread_join(cons_id, NULL);

  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  pthread_barrier_destroy(&barrier);

  return 0;
}
